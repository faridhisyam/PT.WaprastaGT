// Written 2024 by sivar2311
// Based on Espressif USBMSC.h
// Licence of USBMSC.h:

// Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "sdkconfig.h"

#if CONFIG_TINYUSB_MSC_ENABLED

class FatUSBMSC
{
public:
    FatUSBMSC();
    ~FatUSBMSC();
    bool begin(const char* partition_label = nullptr);
    void end();
    void readOnly(bool read_only);
    void vendorID(const char * vid = "ESP32");
    void productID(const char * pid = "USB_MSC");
    void productRevision(const char * ver = "1.0");
private:
	uint8_t _lun;
};

#endif /* CONFIG_TINYUSB_MSC_ENABLED */
