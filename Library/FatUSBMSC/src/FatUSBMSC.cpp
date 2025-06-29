// Written 2024 by sivar2311
// Based on Espressif USBMSC.cpp
// Licence of USBMSC.cpp:

// Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "FatUSBMSC.h"

#if CONFIG_TINYUSB_MSC_ENABLED

#include "esp32-hal-tinyusb.h"
#include "esp_partition.h"

extern "C" uint16_t tusb_msc_load_descriptor(uint8_t * dst, uint8_t * itf)
{
    uint8_t str_index = tinyusb_add_string_descriptor("TinyUSB MSC");
    uint8_t ep_num = tinyusb_get_free_duplex_endpoint();
    TU_VERIFY (ep_num != 0);
    uint8_t descriptor[TUD_MSC_DESC_LEN] = {
        // Interface number, string index, EP Out & EP In address, EP size
        TUD_MSC_DESCRIPTOR(*itf, str_index, ep_num, (uint8_t)(0x80 | ep_num), 64)
    };
    *itf+=1;
    memcpy(dst, descriptor, TUD_MSC_DESC_LEN);
    return TUD_MSC_DESC_LEN;
}

typedef struct {
    uint8_t vendor_id[8];
    uint8_t product_id[16];
    uint8_t product_rev[4];
    uint16_t block_size;
    uint32_t block_count;
    const esp_partition_t* partition;
    bool read_only;
} msc_lun_t;

static const uint8_t MSC_MAX_LUN = 3;
static uint8_t MSC_ACTIVE_LUN = 0;
static msc_lun_t msc_luns[MSC_MAX_LUN];

static void cplstr(void *dst, const void * src, size_t max_len){
    if(!src || !dst || !max_len){
        return;
    }
    size_t l = strlen((const char *)src);
    if(l > max_len){
        l = max_len;
    }
    memcpy(dst, src, l);
}

// Invoked when received GET_MAX_LUN request, required for multiple LUNs implementation
uint8_t tud_msc_get_maxlun_cb(void)
{
    log_v("%u", MSC_ACTIVE_LUN);
    return MSC_ACTIVE_LUN;
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    log_v("[%u]", lun);
    cplstr(vendor_id  , msc_luns[lun].vendor_id, 8);
    cplstr(product_id , msc_luns[lun].product_id, 16);
    cplstr(product_rev, msc_luns[lun].product_rev, 4);
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    log_v("[%u]: %u", lun, msc_luns[lun].partition ? 1 : 0);
    return msc_luns[lun].partition ? true : false; // RAM disk is always ready
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
    log_v("[%u]", lun);

    if(!msc_luns[lun].partition){
        *block_count = 0;
        *block_size  = 0;
        return;
    }

    *block_count = msc_luns[lun].block_count;
    *block_size  = msc_luns[lun].block_size;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    log_v("[%u] power: %u, start: %u, eject: %u", lun, power_condition, start, load_eject);
    return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    log_v("[%u], lba: %u, offset: %u, bufsize: %u", lun, lba, offset, bufsize);

    if(!msc_luns[lun].partition){
        return -1;
    }

    size_t partition_offset = (lba+1) * msc_luns[lun].block_size + offset;
    esp_partition_read(msc_luns[lun].partition, partition_offset, buffer, bufsize);

    return bufsize;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
    log_v("[%u], lba: %u, offset: %u, bufsize: %u", lun, lba, offset, bufsize);

    if(!msc_luns[lun].partition){
      return -1;
    }

    if (!msc_luns[lun].read_only) {
      size_t partition_offset = (lba+1) * msc_luns[lun].block_size + offset;
      esp_partition_erase_range(msc_luns[lun].partition, partition_offset, bufsize);
      esp_partition_write(msc_luns[lun].partition, partition_offset, buffer, bufsize);
    }

    return bufsize;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
    // read10 & write10 has their own callback and MUST not be handled here
    log_v("[%u] cmd: %u, bufsize: %u", lun, scsi_cmd[0], bufsize);

    void const* response = NULL;
    uint16_t resplen = 0;

    // most scsi handled is input
    bool in_xfer = true;
    
    // if(!msc_luns[lun].media_present){
    if(!msc_luns[lun].partition){
        return -1;
    }

    switch (scsi_cmd[0]) {
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            // Host is about to read/write etc ... better not to disconnect disk
            resplen = 0;
            break;

        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

            // negative means error -> tinyusb could stall and/or response with failed status
            resplen = -1;
            break;
    }

    // return resplen must not larger than bufsize
    if (resplen > bufsize) resplen = bufsize;

    if (response && (resplen > 0)) {
        if (in_xfer) {
            memcpy(buffer, response, resplen);
        } else {
            // SCSI output
        }
    }

    return resplen;
}

FatUSBMSC::FatUSBMSC(){
    if(MSC_ACTIVE_LUN < MSC_MAX_LUN){
        _lun = MSC_ACTIVE_LUN;
        MSC_ACTIVE_LUN++;
        vendorID();
        productID();
        productRevision();
        msc_luns[_lun].block_size = 0;
        msc_luns[_lun].block_count = 0;
        msc_luns[_lun].partition = NULL;
        msc_luns[_lun].read_only = false;
    }
    if(_lun == 0){
        tinyusb_enable_interface(USB_INTERFACE_MSC, TUD_MSC_DESC_LEN, tusb_msc_load_descriptor);
    }
}

FatUSBMSC::~FatUSBMSC(){
  end();
}

// Provide FAT partition as USB drive
// - partition_label (optional) if not specified, the first FAT partition will be used
bool FatUSBMSC::begin(const char* partition_label) {
    const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, partition_label);
    if (!partition) return false;

    const size_t block_size = CONFIG_WL_SECTOR_SIZE;
    const size_t block_count = partition->size / block_size;

    msc_luns[_lun].partition = partition;
    msc_luns[_lun].block_size = block_size;
    msc_luns[_lun].block_count = block_count;

    return true;
}

void FatUSBMSC::end(){
    msc_luns[_lun].vendor_id[0] = 0;
    msc_luns[_lun].product_id[0] = 0;
    msc_luns[_lun].product_rev[0] = 0;
    msc_luns[_lun].block_size = 0;
    msc_luns[_lun].block_count = 0;
    msc_luns[_lun].partition = NULL;
    msc_luns[_lun].read_only = false;
}

// Set read_only
// - read_only = true: writing to the partition is disabled
// - read_only = false: writing to the partition is enabled
void FatUSBMSC::readOnly(bool read_only) {
    msc_luns[_lun].read_only = read_only;
}

// Set vendorID 
// - vid: maximum 8 characters (default: "ESP32")
void FatUSBMSC::vendorID(const char * vid){
    cplstr(msc_luns[_lun].vendor_id, vid, 8);
}

// set productID
// - pid: maximum 16 characters (default: "USB_MSC")
void FatUSBMSC::productID(const char * pid){
    cplstr(msc_luns[_lun].product_id, pid, 16);
}

// Override default productRevision
// - rev: maximum 4 characters (default: "1.0")
void FatUSBMSC::productRevision(const char * rev){
    cplstr(msc_luns[_lun].product_rev, rev, 4);
}

#endif /* CONFIG_TINYUSB_MSC_ENABLED */
