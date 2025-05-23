#include <Arduino.h>
#include <lvgl.h>
#include "touch.h"
#include <SPI.h>
#include <SD.h>
#include <Arduino_GFX_Library.h>

// --- Konfigurasi Backlight ---
#define TFT_BL 2
#define BL_CHANNEL 0
#define BL_FREQ 30000
#define BL_RES 8

// --- UART ---
#define RXD2 18
#define TXD2 17

// --- Shared state antar task ---
static volatile uint32_t lastTouchMs = 0;
static volatile uint8_t curBrightness = 255;

uint32_t last = 0;
const uint32_t DUR = 500;

// Display bus
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
  GFX_NOT_DEFINED, GFX_NOT_DEFINED, GFX_NOT_DEFINED,
  40, 41, 39, 42, 45, 48, 47, 21, 14, 5, 6, 7, 15, 16, 4, 8, 3, 46, 9, 1);
Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
  bus, 800, 0, 8, 4, 8, 480, 0, 8, 4, 8, 1, 16000000, true);

// LVGL buffer/driver
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf_draw;
static lv_disp_drv_t disp_drv;

// Screens & widgets
static lv_obj_t *scr_main, *scr_second, *scr_ok, *scr_fail;
static lv_obj_t *body, *bodySecond, *bodyThird;
static lv_obj_t *btnHome, *btnInformation, *btnSetting, *kb, *btn_save;
static lv_obj_t *ta_mode, *ta_ssid, *ta_pwd, *ta_device;

static lv_obj_t *lbl_name, *lbl_time, *lbl_date, *lbl_status, *lbl_door, *lbl_duration;
static lv_obj_t *lbl_mode, *lbl_device, *lbl_ssid, *lbl_statuswifi, *lbl_ip, *lbl_waktuaktif;
lv_obj_t *b1;

char bufMode[32] = "-";
char bufDevice[32] = "-";
char bufssid[16] = "-";
char bufStatuswifi[16] = "-";
char bufip[16] = "-";
char bufwaktuaktif[32] = "-";

// Data buffers
char bufName[32] = "-";
char bufTime[16] = "-";
char bufDate[16] = "-";
char bufStat[16] = "-";
char bufDoor[16] = "-";
char bufDur[16] = "-";

char buf[128];
// Queue untuk passing line dari Serial2 → taskParser
static QueueHandle_t serialQueue;

// Forward-declare task function
void taskLVGL(void *pv);
void taskSerial(void *pv);
void taskBacklight(void *pv);

// --- Prototypes fungsi UI / inisialisasi LVGL ---
void lvgl_sdcard_fs_init();
void create_main_screen();
void create_status_screens();
void my_disp_flush(lv_disp_drv_t *, const lv_area_t *, lv_color_t *);
void my_touchpad_read(lv_indev_drv_t *, lv_indev_data_t *);

//===========================================================================
// Flush callback
void my_disp_flush(lv_disp_drv_t *d, const lv_area_t *a, lv_color_t *c) {
  uint32_t w = a->x2 - a->x1 + 1;
  uint32_t h = a->y2 - a->y1 + 1;
#if LV_COLOR_16_SWAP
  gfx->draw16bitBeRGBBitmap(a->x1, a->y1, (uint16_t *)c, w, h);
#else
  gfx->draw16bitRGBBitmap(a->x1, a->y1, (uint16_t *)c, w, h);
#endif
  lv_disp_flush_ready(d);
}

// Touch callback
void my_touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  if (!touch_has_signal()) {
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  if (touch_touched()) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touch_last_x;
    data->point.y = touch_last_y;

    // *** Reset timer idle ***
    lastTouchMs = millis();
    // Jika sebelumnya redup, kembalikan terang penuh
    if (curBrightness != 255) {
      curBrightness = 255;
      ledcWrite(BL_CHANNEL, curBrightness);
    }
  } else data->state = LV_INDEV_STATE_REL;
}

// Callback untuk resize tombol
static void btn_zoom_event_cb(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target(e);
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_PRESSED) {
    // Saat ditekan, perbesar 10px di setiap sisi
    lv_obj_set_size(btn,
                    lv_obj_get_width(btn) - 5,
                    lv_obj_get_height(btn) - 5);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    // Saat dilepas atau terlepas fokus, kembalikan ukuran awal
    lv_obj_set_size(btn, 40, 40);
  }
}

static void btnInformation_cb(lv_event_t *e) {
  // Sembunyikan cont_a, tampilkan cont_b
  lv_obj_add_flag(body, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(bodyThird, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(bodySecond, LV_OBJ_FLAG_HIDDEN);
}

static void btnHome_cb(lv_event_t *e) {
  // Sembunyikan cont_a, tampilkan cont_b
  lv_obj_add_flag(bodySecond, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(bodyThird, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(body, LV_OBJ_FLAG_HIDDEN);
}

static void btnSetting_cb(lv_event_t *e) {
  // Sembunyikan cont_a, tampilkan cont_b
  lv_obj_add_flag(body, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(bodySecond, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(bodyThird, LV_OBJ_FLAG_HIDDEN);
}

// Callback tombol Save
static void btn_save_event_cb(lv_event_t *e) {
  // Kirim SSID & password
  const char *ssid = lv_textarea_get_text(ta_ssid);
  const char *pwd = lv_textarea_get_text(ta_pwd);
  const char *device = lv_textarea_get_text(ta_device);
  const char *mode = lv_textarea_get_text(ta_mode);

  snprintf(buf, sizeof(buf), "IKI;%s;%s;%s;%s", device, mode, ssid, pwd);
  Serial2.printf(buf);
  Serial.println(buf);

  // Serial2.print("SEND");
  // Serial2.print(";");
  // Serial2.print(device);
  // Serial2.print(";");
  // Serial2.print(ssid);
  // Serial2.print(";");
  // Serial2.println(pwd);

  // Serial.print("SEND");
  // Serial.print(";");
  // Serial.print(device);
  // Serial.print(";");
  // Serial.print(ssid);
  // Serial.print(";");
  // Serial.println(pwd);
  // Reset dan sembunyikan keyboard
  lv_textarea_set_text(ta_ssid, "");
  lv_textarea_set_text(ta_pwd, "");
  lv_textarea_set_text(ta_device, "");
  lv_textarea_set_text(ta_mode, "");
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_keyboard_set_textarea(kb, NULL);
}

static void ta_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_FOCUSED) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_keyboard_set_textarea(kb, target);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
  }
}

static void kb_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    // Tekan Enter → sembunyikan
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(kb, NULL);
  }
}


// Simple OK/FAIL
void create_status_screens() {
  scr_ok = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_ok, lv_color_hex(0x228B22), 0);
  auto o = lv_label_create(scr_ok);
  lv_label_set_text(o, "RFID Terdeteksi !!!");
  lv_obj_set_style_text_color(o, lv_color_white(), 0);
  lv_obj_set_style_text_font(o, &lv_font_montserrat_46, 0);
  lv_obj_center(o);

  scr_fail = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_fail, lv_color_hex(0xff0000), 0);
  auto f = lv_label_create(scr_fail);
  lv_label_set_text(f, "RFID Tidak Terdaftar !!!");
  lv_obj_set_style_text_color(f, lv_color_white(), 0);
  lv_obj_set_style_text_font(f, &lv_font_montserrat_46, 0);
  lv_obj_center(f);
}

// Build full-screen main
void create_main_screen() {
  //Background=======================================
  scr_main = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_main, lv_color_hex(0x000000), 0);

  static lv_coord_t cols[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t rows[] = { 55, 348, 55, LV_GRID_TEMPLATE_LAST };

  lv_obj_set_layout(scr_main, LV_LAYOUT_GRID);
  lv_obj_set_grid_dsc_array(scr_main, cols, rows);

  // Container TOP===================================
  lv_obj_t *bodytop = lv_obj_create(scr_main);
  // lv_obj_set_style_bg_color(bodytop, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(bodytop, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(bodytop, 0, 0);
  // lv_obj_set_style_border_color(bodytop, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_pad_top(bodytop, 0, 0);
  lv_obj_set_style_pad_bottom(bodytop, 0, 0);
  lv_obj_set_style_pad_left(bodytop, 0, 0);
  lv_obj_set_style_pad_right(bodytop, 0, 0);
  // lv_obj_set_style_pad_left(bodytop, 1, 1);
  static lv_coord_t bodytopbcols[] = { 112, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t bodytopbrows[] = { 55, LV_GRID_TEMPLATE_LAST };
  lv_obj_set_layout(bodytop, LV_LAYOUT_GRID);
  lv_obj_set_grid_dsc_array(bodytop, bodytopbcols, bodytopbrows);
  lv_obj_set_grid_cell(bodytop,
                       LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_style_bg_img_src(bodytop, "S:/WaprastaYazaki.bin", 0);
  // lv_img_set_offset_x(bodytop, 0);
  // lv_img_set_offset_y(bodytop, 20);
  // lv_obj_set_style_bg_img_opa(bodytop, LV_OPA_COVER, 0);
  // lv_obj_set_style_bg_img_tiled(bodytop, false, 0);
  // lv_obj_set_style_bg_img_recolor_opa(bodytop, LV_OPA_TRANSP, 0);

  // matikan scroll
  lv_obj_clear_flag(bodytop, LV_OBJ_FLAG_SCROLLABLE);
  // dan kalau perlu:
  lv_obj_set_scrollbar_mode(bodytop, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t *hdr = lv_label_create(bodytop);
  lv_label_set_text(hdr, "RFID Door Lock Access");
  lv_obj_set_style_text_font(hdr, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(hdr, lv_color_hex(0x000000), 0);
  lv_obj_set_grid_cell(hdr,
                       LV_GRID_ALIGN_CENTER, 1, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);

  // Container Tengah 1===================================
  body = lv_obj_create(scr_main);
  lv_obj_set_style_bg_color(body, lv_color_hex(0x7ed856), 0);
  // lv_obj_set_style_border_width(body, 4, 0);
  // lv_obj_set_style_border_color(body, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_pad_top(body, 0, 0);
  lv_obj_set_style_pad_bottom(body, 0, 0);
  lv_obj_set_style_pad_left(body, 1, 1);
  lv_obj_set_style_pad_right(body, 1, 1);
  static lv_coord_t bcols[] = { LV_GRID_FR(1), 190, LV_GRID_TEMPLATE_LAST };
  static lv_coord_t brows[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  lv_obj_set_layout(body, LV_LAYOUT_GRID);
  lv_obj_set_grid_dsc_array(body, bcols, brows);
  lv_obj_set_grid_cell(body,
                       LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_clear_flag(body, LV_OBJ_FLAG_HIDDEN);

  // Sub Container Tengah 1 LEFT===================================
  static lv_coord_t lcols[] = { 85, 3, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t lrows[] = { 0, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), 0, LV_GRID_TEMPLATE_LAST };
  lv_obj_t *left = lv_obj_create(body);
  lv_obj_set_style_bg_color(left, lv_color_hex(0x7ed856), 0);
  lv_obj_set_style_border_width(left, 0, 0);
  lv_obj_set_style_pad_all(left, 0, 0);
  lv_obj_set_layout(left, LV_LAYOUT_GRID);
  lv_obj_set_grid_dsc_array(left, lcols, lrows);
  lv_obj_set_grid_cell(left,
                       LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_STRETCH, 0, 1);

  const char *keys[] = { "Nama",
                         "Waktu",
                         "Tanggal",
                         "Akses" };
  const char *keystitikdua[] = { ":",
                                 ":",
                                 ":",
                                 ":" };
  lv_obj_t *labels[] = { lbl_name, lbl_time, lbl_date, lbl_status };
  char *bufs[] = { bufName, bufTime, bufDate, bufStat };
  for (int i = 0; i < 4; i++) {
    // key
    lv_obj_t *k = lv_label_create(left);
    lv_label_set_text(k, keys[i]);
    lv_obj_set_style_text_color(k, lv_color_black(), 0);
    lv_obj_set_style_text_font(k, &lv_font_montserrat_22, 0);
    lv_obj_set_grid_cell(k,
                         LV_GRID_ALIGN_START, 0, 1,
                         LV_GRID_ALIGN_CENTER, i + 1, 1);

    lv_obj_t *titikdua = lv_label_create(left);
    lv_label_set_text(titikdua, keystitikdua[i]);
    lv_obj_set_style_text_color(titikdua, lv_color_black(), 0);
    lv_obj_set_style_text_font(titikdua, &lv_font_montserrat_22, 0);
    lv_obj_set_grid_cell(titikdua,
                         LV_GRID_ALIGN_START, 1, 1,
                         LV_GRID_ALIGN_CENTER, i + 1, 1);
    // box & value
    lv_obj_t *box = lv_obj_create(left);
    // lv_obj_set_style_border_width(box, 2, 0);
    // lv_obj_set_style_border_color(box, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_color(box, lv_color_black(), 0);
    // lv_obj_set_style_bg_color(box, lv_color_hex(0x333333), LV_STATE_PRESSED);
    lv_obj_set_grid_cell(box,
                         LV_GRID_ALIGN_STRETCH, 2, 1,
                         LV_GRID_ALIGN_STRETCH, i + 1, 1);
    labels[i] = lv_label_create(box);
    lv_label_set_text(labels[i], bufs[i]);
    lv_obj_set_style_text_color(labels[i], lv_color_white(), 0);
    lv_obj_set_style_text_font(labels[i], &lv_font_montserrat_22, 0);
    lv_obj_center(labels[i]);
  }
  lbl_name = labels[0];
  lbl_time = labels[1];
  lbl_date = labels[2];
  lbl_status = labels[3];

  // Sub Container Tengah 1 RIGHT===================================
  lv_obj_t *right = lv_obj_create(body);
  lv_obj_set_style_bg_color(right, lv_color_hex(0x7ed856), 0);
  lv_obj_set_style_border_width(right, 0, 0);
  lv_obj_set_style_pad_all(right, 0, 0);
  static lv_coord_t rcols[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t rrows[] = { 0, 25, 166, 25, 72, LV_GRID_FR(1), 0, LV_GRID_TEMPLATE_LAST };
  lv_obj_set_layout(right, LV_LAYOUT_GRID);
  lv_obj_set_grid_dsc_array(right, rcols, rrows);
  lv_obj_set_grid_cell(right,
                       LV_GRID_ALIGN_STRETCH, 1, 1,
                       LV_GRID_ALIGN_STRETCH, 0, 1);

  // Status Pintu box
  lv_obj_t *judulstatuspintu = lv_label_create(right);
  lv_label_set_text(judulstatuspintu, "Status Pintu");
  lv_obj_set_style_text_color(judulstatuspintu, lv_color_black(), 0);
  lv_obj_set_style_text_font(judulstatuspintu, &lv_font_montserrat_22, 0);
  lv_obj_set_grid_cell(judulstatuspintu,
                       LV_GRID_ALIGN_CENTER, 0, 1,
                       LV_GRID_ALIGN_START, 1, 1);

  b1 = lv_obj_create(right);
  lv_obj_set_style_border_width(b1, 0, 0);
  // lv_obj_set_style_border_color(b1, lv_color_hex(0x7ed856), 0);
  lv_obj_set_style_bg_color(b1, lv_color_hex(0x7ed856), 0);
  lv_obj_set_style_bg_opa(b1, LV_OPA_COVER, 0);

  lv_obj_set_grid_cell(b1,
                       LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_style_bg_img_src(b1, "S:/LOCKED.bin", 0);

  // Durasi Terbuka box
  lv_obj_t *durasi = lv_label_create(right);
  lv_label_set_text(durasi, "Durasi Terbuka");
  lv_obj_set_style_text_color(durasi, lv_color_black(), 0);
  lv_obj_set_style_text_font(durasi, &lv_font_montserrat_22, 0);
  lv_obj_set_grid_cell(durasi,
                       LV_GRID_ALIGN_CENTER, 0, 1,
                       LV_GRID_ALIGN_START, 3, 1);

  lv_obj_t *b2 = lv_obj_create(right);
  // lv_obj_set_style_border_width(b2, 2, 0);
  // lv_obj_set_style_border_color(b2, lv_color_hex(0xffffff), 0);
  // lv_obj_set_style_bg_color(b2, lv_color_hex(0x333333), LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(b2, lv_color_black(), 0);
  lv_obj_set_grid_cell(b2,
                       LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_STRETCH, 4, 1);

  lbl_duration = lv_label_create(b2);
  lv_label_set_text(lbl_duration, bufDur);
  lv_obj_set_style_text_color(lbl_duration, lv_color_white(), 0);
  lv_obj_set_style_text_font(lbl_duration, &lv_font_montserrat_22, 0);
  lv_obj_center(lbl_duration);


  // Container Tengah 2===================================
  bodySecond = lv_obj_create(scr_main);
  lv_obj_set_style_bg_color(bodySecond, lv_color_hex(0x7ed856), 0);
  // lv_obj_set_style_border_width(bodySecond, 4, 0);
  // lv_obj_set_style_border_color(bodySecond, lv_color_hex(0xffffff), 0);
  // lv_obj_set_style_pad_all(bodySecond, 0, 0);
  lv_obj_set_style_pad_top(bodySecond, 0, 0);
  lv_obj_set_style_pad_bottom(bodySecond, 1, 1);
  lv_obj_set_style_pad_left(bodySecond, 1, 1);
  lv_obj_set_style_pad_right(bodySecond, 1, 1);

  static lv_coord_t bcolsbodySecond[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t browsbodySecond[] = { 0, 25, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  lv_obj_set_layout(bodySecond, LV_LAYOUT_GRID);
  lv_obj_set_grid_dsc_array(bodySecond, bcolsbodySecond, browsbodySecond);
  lv_obj_set_grid_cell(bodySecond,
                       LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_add_flag(bodySecond, LV_OBJ_FLAG_HIDDEN);

  static lv_coord_t lcolsbodySecond[] = { LV_GRID_FR(1), 300, 10, 300, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t lrowsbodySecond[] = { 0, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), 0, LV_GRID_TEMPLATE_LAST };

  lv_obj_t *information = lv_obj_create(bodySecond);
  lv_obj_set_style_bg_color(information, lv_color_hex(0x7ed856), 0);
  // lv_obj_set_style_border_width(information, 1, 0);
  // lv_obj_set_style_border_color(information, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_pad_all(information, 0, 0);
  lv_obj_set_layout(information, LV_LAYOUT_GRID);
  lv_obj_set_grid_dsc_array(information, lcolsbodySecond, lrowsbodySecond);
  lv_obj_set_grid_cell(information,
                       LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_STRETCH, 2, 1);

  lv_obj_t *hdrbodySecond = lv_label_create(bodySecond);
  lv_label_set_text(hdrbodySecond, "Informasi");
  lv_obj_set_style_text_font(hdrbodySecond, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(hdrbodySecond, lv_color_hex(0x000000), 0);
  lv_obj_set_grid_cell(hdrbodySecond,
                       LV_GRID_ALIGN_CENTER, 0, 1,
                       LV_GRID_ALIGN_CENTER, 1, 1);

  const char *keysbodySecond[] = { "Mode",
                                   "Device",
                                   "SSID WiFi",
                                   "Status WiFi",
                                   "IP Addrees",
                                   "Waktu Aktif" };
  const char *keystitikduabodySecond[] = { ":",
                                           ":",
                                           ":",
                                           ":",
                                           ":",
                                           ":" };
  lv_obj_t *labelsbodySecond[] = { lbl_mode, lbl_device, lbl_ssid, lbl_statuswifi, lbl_ip, lbl_waktuaktif };
  char *bufsbodySecond[] = { bufMode, bufDevice, bufssid, bufStatuswifi, bufip, bufwaktuaktif };
  for (int i = 0; i < 6; i++) {
    // key
    lv_obj_t *k = lv_label_create(information);
    lv_label_set_text(k, keysbodySecond[i]);
    lv_obj_set_style_text_color(k, lv_color_black(), 0);
    lv_obj_set_style_text_font(k, &lv_font_montserrat_22, 0);
    lv_obj_set_grid_cell(k,
                         LV_GRID_ALIGN_END, 1, 1,
                         LV_GRID_ALIGN_CENTER, i + 1, 1);

    lv_obj_t *titikdua = lv_label_create(information);
    lv_label_set_text(titikdua, keystitikduabodySecond[i]);
    lv_obj_set_style_text_color(titikdua, lv_color_black(), 0);
    lv_obj_set_style_text_font(titikdua, &lv_font_montserrat_22, 0);
    lv_obj_set_grid_cell(titikdua,
                         LV_GRID_ALIGN_CENTER, 2, 1,
                         LV_GRID_ALIGN_CENTER, i + 1, 1);

    labelsbodySecond[i] = lv_label_create(information);
    lv_label_set_text(labelsbodySecond[i], bufsbodySecond[i]);
    lv_obj_set_style_text_color(labelsbodySecond[i], lv_color_white(), 0);
    lv_obj_set_style_text_font(labelsbodySecond[i], &lv_font_montserrat_22, 0);
    lv_obj_set_grid_cell(labelsbodySecond[i],
                         LV_GRID_ALIGN_START, 3, 1,
                         LV_GRID_ALIGN_CENTER, i + 1, 1);
    // lv_obj_center(labelsbodySecond[i]);
  }
  lbl_mode = labelsbodySecond[0];
  lbl_device = labelsbodySecond[1];
  lbl_ssid = labelsbodySecond[2];
  lbl_statuswifi = labelsbodySecond[3];
  lbl_ip = labelsbodySecond[4];
  lbl_waktuaktif = labelsbodySecond[5];

  // Container Tengah 3===================================
  bodyThird = lv_obj_create(scr_main);
  lv_obj_set_style_bg_color(bodyThird, lv_color_hex(0x7ed856), 0);
  // lv_obj_set_style_border_width(bodyThird, 4, 0);
  // lv_obj_set_style_border_color(bodyThird, lv_color_hex(0xffffff), 0);
  // lv_obj_set_style_pad_all(bodyThird, 0, 0);
  lv_obj_set_style_pad_top(bodyThird, 1, 1);
  lv_obj_set_style_pad_bottom(bodyThird, 0, 0);
  lv_obj_set_style_pad_left(bodyThird, 0, 0);
  lv_obj_set_style_pad_right(bodyThird, 0, 0);

  static lv_coord_t bcolsbodyThird[] = { LV_GRID_FR(1), 75, 10, 250, 10, 75, 10, 250, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t browsbodyThird[] = { 40, 40, 20, 40, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  lv_obj_set_layout(bodyThird, LV_LAYOUT_GRID);
  lv_obj_set_grid_dsc_array(bodyThird, bcolsbodyThird, browsbodyThird);
  lv_obj_set_grid_cell(bodyThird,
                       LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_add_flag(bodyThird, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *lbl_device = lv_label_create(bodyThird);
  lv_label_set_text(lbl_device, "Device");
  lv_obj_set_style_text_font(lbl_device, &lv_font_montserrat_22, 0);
  lv_obj_set_grid_cell(lbl_device,
                       LV_GRID_ALIGN_START, 1, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);

  lv_obj_t *titikdua0 = lv_label_create(bodyThird);
  lv_label_set_text(titikdua0, ":");
  lv_obj_set_style_text_font(titikdua0, &lv_font_montserrat_22, 0);
  lv_obj_set_grid_cell(titikdua0,
                       LV_GRID_ALIGN_CENTER, 2, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);

  // Text area SSID
  ta_device = lv_textarea_create(bodyThird);
  lv_textarea_set_placeholder_text(ta_device, "Masukkan Nama Device");
  lv_obj_add_event_cb(ta_device, ta_event_cb, LV_EVENT_ALL, NULL);
  lv_textarea_set_one_line(ta_device, true);
  lv_obj_set_grid_cell(ta_device,
                       LV_GRID_ALIGN_STRETCH, 3, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);

  lv_obj_t *lbl_mode = lv_label_create(bodyThird);
  lv_label_set_text(lbl_mode, "Mode");
  lv_obj_set_style_text_font(lbl_mode, &lv_font_montserrat_22, 0);
  lv_obj_set_grid_cell(lbl_mode,
                       LV_GRID_ALIGN_START, 1, 1,
                       LV_GRID_ALIGN_CENTER, 1, 1);

  lv_obj_t *titikdua3 = lv_label_create(bodyThird);
  lv_label_set_text(titikdua3, ":");
  lv_obj_set_style_text_font(titikdua3, &lv_font_montserrat_22, 0);
  lv_obj_set_grid_cell(titikdua3,
                       LV_GRID_ALIGN_CENTER, 2, 1,
                       LV_GRID_ALIGN_CENTER, 1, 1);

  // Text area SSID
  ta_mode = lv_textarea_create(bodyThird);
  lv_textarea_set_placeholder_text(ta_mode, "Masukkan Mode");
  lv_obj_add_event_cb(ta_mode, ta_event_cb, LV_EVENT_ALL, NULL);
  lv_textarea_set_one_line(ta_mode, true);
  lv_obj_set_grid_cell(ta_mode,
                       LV_GRID_ALIGN_STRETCH, 3, 1,
                       LV_GRID_ALIGN_CENTER, 1, 1);

  lv_obj_t *lbl_ssid = lv_label_create(bodyThird);
  lv_label_set_text(lbl_ssid, "SSID");
  lv_obj_set_style_text_font(lbl_ssid, &lv_font_montserrat_22, 0);
  lv_obj_set_grid_cell(lbl_ssid,
                       LV_GRID_ALIGN_START, 5, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);

  lv_obj_t *titikdua1 = lv_label_create(bodyThird);
  lv_label_set_text(titikdua1, ":");
  lv_obj_set_style_text_font(titikdua1, &lv_font_montserrat_22, 0);
  lv_obj_set_grid_cell(titikdua1,
                       LV_GRID_ALIGN_CENTER, 6, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);

  // Text area SSID
  ta_ssid = lv_textarea_create(bodyThird);
  lv_textarea_set_placeholder_text(ta_ssid, "Masukkan SSID");
  lv_obj_add_event_cb(ta_ssid, ta_event_cb, LV_EVENT_ALL, NULL);
  lv_textarea_set_one_line(ta_ssid, true);
  lv_obj_set_grid_cell(ta_ssid,
                       LV_GRID_ALIGN_STRETCH, 7, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);

  // static lv_coord_t bcolsbodyThird[] = { LV_GRID_FR(1), 110, 20, 400, 10, 110, 20, 400, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

  // 2) Label Password
  lv_obj_t *lbl_pwd = lv_label_create(bodyThird);
  lv_label_set_text(lbl_pwd, "PASS");
  lv_obj_set_style_text_font(lbl_pwd, &lv_font_montserrat_22, 0);
  lv_obj_set_grid_cell(lbl_pwd,
                       LV_GRID_ALIGN_START, 5, 1,
                       LV_GRID_ALIGN_CENTER, 1, 1);

  lv_obj_t *titikdua2 = lv_label_create(bodyThird);
  lv_label_set_text(titikdua2, ":");
  lv_obj_set_style_text_font(titikdua2, &lv_font_montserrat_22, 0);
  lv_obj_set_grid_cell(titikdua2,
                       LV_GRID_ALIGN_CENTER, 6, 1,
                       LV_GRID_ALIGN_CENTER, 1, 1);

  // Text area Password (tampilkan ★, hide text)
  ta_pwd = lv_textarea_create(bodyThird);
  lv_textarea_set_password_mode(ta_pwd, true);
  lv_textarea_set_placeholder_text(ta_pwd, "Masukkan Password");
  lv_obj_add_event_cb(ta_pwd, ta_event_cb, LV_EVENT_ALL, NULL);
  lv_textarea_set_one_line(ta_pwd, true);
  lv_obj_set_grid_cell(ta_pwd,
                       LV_GRID_ALIGN_STRETCH, 7, 1,
                       LV_GRID_ALIGN_CENTER, 1, 1);

  // 3) Tombol Save
  btn_save = lv_btn_create(bodyThird);
  lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x0066CC), 0);
  lv_obj_set_style_radius(btn_save, 5, 0);
  lv_obj_set_style_pad_all(btn_save, 10, 0);
  lv_obj_add_event_cb(btn_save, btn_save_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_set_grid_cell(btn_save,
                       LV_GRID_ALIGN_CENTER, 0, 9,
                       LV_GRID_ALIGN_CENTER, 3, 1);
  lv_obj_t *lbl = lv_label_create(btn_save);
  lv_label_set_text(lbl, "Simpan");

  // 4) Keyboard di baris terakhir, otomatis muncul di event handler
  kb = lv_keyboard_create(scr_main);
  lv_obj_set_grid_cell(kb,
                       LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_END, 1, 1);
  lv_obj_set_size(kb, LV_HOR_RES, LV_VER_RES / 2);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, NULL);

  // Container Bottom===================================
  lv_obj_t *bodybottom = lv_obj_create(scr_main);
  // lv_obj_set_style_bg_color(bodybottom, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(bodybottom, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(bodybottom, 0, 0);
  // lv_obj_set_style_border_color(bodybottom, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_pad_top(bodybottom, 0, 0);
  lv_obj_set_style_pad_bottom(bodybottom, 0, 0);
  lv_obj_set_style_pad_left(bodybottom, 1, 1);
  lv_obj_set_style_pad_right(bodybottom, 1, 1);
  // lv_obj_set_style_pad_left(bodybottom, 1, 1);
  static lv_coord_t bodybottombcols[] = { 55, 55, 55, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t bodybottombrows[] = { 55, LV_GRID_TEMPLATE_LAST };
  lv_obj_set_layout(bodybottom, LV_LAYOUT_GRID);
  lv_obj_set_grid_dsc_array(bodybottom, bodybottombcols, bodybottombrows);
  lv_obj_set_grid_cell(bodybottom,
                       LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_STRETCH, 2, 1);
  // lv_obj_set_style_bg_img_src(bodybottom, "S:/WaprastaYazakiBottom.bin", 0);
  // lv_obj_set_style_bg_img_opa(bodybottom, LV_OPA_COVER, 0);
  // lv_obj_set_style_bg_img_tiled(bodybottom, false, 0);
  // lv_obj_set_style_bg_img_recolor_opa(bodybottom, LV_OPA_TRANSP, 0);

  // matikan scroll
  lv_obj_clear_flag(bodybottom, LV_OBJ_FLAG_SCROLLABLE);
  // dan kalau perlu:
  lv_obj_set_scrollbar_mode(bodybottom, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t *f = lv_label_create(bodybottom);
  lv_label_set_text(f, "Place Your Card !!!");
  lv_obj_set_style_text_color(f, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(f, &lv_font_montserrat_26, 0);
  lv_obj_set_grid_cell(f,
                       LV_GRID_ALIGN_CENTER, 3, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);

  // Button===================================
  btnHome = lv_btn_create(bodybottom);
  lv_obj_set_style_bg_color(btnHome, lv_color_white(), 0);
  lv_obj_set_style_border_width(btnHome, 0, 0);
  lv_obj_set_style_bg_opa(btnHome, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_img_src(btnHome, "S:/Home.bin", 0);
  lv_obj_add_event_cb(btnHome, btn_zoom_event_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(btnHome, btn_zoom_event_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(btnHome, btn_zoom_event_cb, LV_EVENT_PRESS_LOST, NULL);
  lv_obj_set_size(btnHome, 45, 45);
  lv_obj_align(btnHome, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(btnHome, btnHome_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_set_grid_cell(btnHome,
                       LV_GRID_ALIGN_START, 0, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);

  btnInformation = lv_btn_create(bodybottom);
  lv_obj_set_style_bg_color(btnInformation, lv_color_white(), 0);
  lv_obj_set_style_border_width(btnInformation, 0, 0);
  lv_obj_set_style_bg_opa(btnInformation, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_img_src(btnInformation, "S:/Information.bin", 0);
  lv_obj_add_event_cb(btnInformation, btn_zoom_event_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(btnInformation, btn_zoom_event_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(btnInformation, btn_zoom_event_cb, LV_EVENT_PRESS_LOST, NULL);
  lv_obj_set_size(btnInformation, 45, 45);
  lv_obj_align(btnInformation, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(btnInformation, btnInformation_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_set_grid_cell(btnInformation,
                       LV_GRID_ALIGN_START, 1, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);

  btnSetting = lv_btn_create(bodybottom);
  lv_obj_set_style_bg_color(btnSetting, lv_color_white(), 0);
  lv_obj_set_style_border_width(btnSetting, 0, 0);
  lv_obj_set_style_bg_opa(btnSetting, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_img_src(btnSetting, "S:/Setting.bin", 0);
  lv_obj_add_event_cb(btnSetting, btn_zoom_event_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(btnSetting, btn_zoom_event_cb, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(btnSetting, btn_zoom_event_cb, LV_EVENT_PRESS_LOST, NULL);
  lv_obj_set_size(btnSetting, 45, 45);
  lv_obj_align(btnSetting, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(btnSetting, btnSetting_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_set_grid_cell(btnSetting,
                       LV_GRID_ALIGN_START, 2, 1,
                       LV_GRID_ALIGN_CENTER, 0, 1);
}

void lvgl_sdcard_fs_init() {
  static lv_fs_drv_t drv;
  lv_fs_drv_init(&drv);

  drv.letter = 'S';    // nanti path-nya "S:/..."
  drv.cache_size = 0;  // cache tidak dipakai

  // open_cb: (drv, path, mode) → handle atau nullptr
  drv.open_cb = [](lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode) -> void * {
    File *f = nullptr;
    if (mode == LV_FS_MODE_RD) f = new File(SD.open(path, FILE_READ));
    else if (mode == LV_FS_MODE_WR) f = new File(SD.open(path, FILE_WRITE));
    if (!f || !*f) {
      delete f;
      return nullptr;
    }
    return f;
  };

  // close_cb: (drv, handle)
  drv.close_cb = [](lv_fs_drv_t *drv, void *fp) -> lv_fs_res_t {
    File *f = static_cast<File *>(fp);
    f->close();
    delete f;
    return LV_FS_RES_OK;
  };

  // read_cb: (drv, handle, buf, btr, &br)
  drv.read_cb = [](lv_fs_drv_t *drv, void *fp, void *buf, uint32_t btr, uint32_t *br) -> lv_fs_res_t {
    File *f = static_cast<File *>(fp);
    *br = f->read((uint8_t *)buf, btr);
    return LV_FS_RES_OK;
  };

  // seek_cb: (drv, handle, pos, whence)
  drv.seek_cb = [](lv_fs_drv_t *drv, void *fp, uint32_t pos, lv_fs_whence_t whence) -> lv_fs_res_t {
    File *f = static_cast<File *>(fp);
    if (whence == LV_FS_SEEK_SET) f->seek(pos, SeekSet);
    else if (whence == LV_FS_SEEK_CUR) f->seek(f->position() + pos, SeekSet);
    else /*END*/ f->seek(f->size() + pos, SeekSet);
    return LV_FS_RES_OK;
  };

  // tell_cb: (drv, handle, &pos)
  drv.tell_cb = [](lv_fs_drv_t *drv, void *fp, uint32_t *pos_p) -> lv_fs_res_t {
    File *f = static_cast<File *>(fp);
    *pos_p = f->position();
    return LV_FS_RES_OK;
  };

  // Daftarkan driver ke LVGL
  lv_fs_drv_register(&drv);
}

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  if (!SD.begin(10, SPI, 40000000U)) {
    Serial.println("SD card mount failed!");
  }
  Serial.println("Mounted");
  gfx->begin();

  // Inisialisasi LVGL core
  lv_init();
  touch_init();
  lvgl_sdcard_fs_init();

  // Setup backlight
  ledcSetup(BL_CHANNEL, BL_FREQ, BL_RES);
  ledcAttachPin(TFT_BL, BL_CHANNEL);
  curBrightness = 255;
  ledcWrite(BL_CHANNEL, curBrightness);
  lastTouchMs = millis();

  // Inisialisasi driver LVGL
  static lv_disp_draw_buf_t draw_buf;
  static lv_color_t *buf_draw;
  uint16_t w = 800, h = 480;  // sesuaikan resolusi
  buf_draw = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * w * h / 4,
                                            MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  lv_disp_draw_buf_init(&draw_buf, buf_draw, NULL, w * h / 4);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.draw_buf = &draw_buf;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.hor_res = w;
  disp_drv.ver_res = h;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t in_drv;
  lv_indev_drv_init(&in_drv);
  in_drv.type = LV_INDEV_TYPE_POINTER;
  in_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&in_drv);

  // Buat screen & widget
  create_main_screen();
  create_status_screens();
  lv_disp_load_scr(scr_main);

  // Buat queue
  serialQueue = xQueueCreate(8, sizeof(String));

  // Buat task pinned ke core
  xTaskCreatePinnedToCore(taskLVGL, "LVGL", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskSerial, "Serial2", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskBacklight, "Backlight", 2048, NULL, 1, NULL, 0);

  Serial.println("Ready");
  // Hapus default loop task
  vTaskDelete(NULL);
}

void loop() {
  // tidak terpakai
}

// ===== Task Definitions =====

// 1) Task LVGL: render & timer handler setiap 5 ms
void taskLVGL(void *pv) {
  const TickType_t xDelay = pdMS_TO_TICKS(1);
  for (;;) {
    lv_timer_handler();
    vTaskDelay(xDelay);
  }
}

// 2) Task Serial2: baca, parse, dan update UI
void taskSerial(void *pv) {
  String line;
  for (;;) {
    // baca baris
    if (Serial2.available()) {
      line = Serial2.readStringUntil('\n');
      line.trim();
      xQueueSend(serialQueue, &line, portMAX_DELAY);
    }
    // proses queue
    while (uxQueueMessagesWaiting(serialQueue)) {
      xQueueReceive(serialQueue, &line, 0);
      if (line.startsWith("MAIN;")) {
        String p[5];
        int idx = 0;
        for (int i = 0; i < 4; i++) {
          int t = line.indexOf(';', idx);
          p[i] = line.substring(idx, t);
          idx = t + 1;
        }
        p[4] = line.substring(idx);
        p[1].toCharArray(bufName, sizeof(bufName));
        p[2].toCharArray(bufTime, sizeof(bufTime));
        p[3].toCharArray(bufDate, sizeof(bufDate));
        p[4].toCharArray(bufStat, sizeof(bufStat));
        // p[5].toCharArray(bufDoor, sizeof(bufDoor));
        // p[6].toCharArray(bufDur, sizeof(bufDur));
        lv_label_set_text(lbl_name, bufName);
        lv_label_set_text(lbl_time, bufTime);
        lv_label_set_text(lbl_date, bufDate);
        lv_label_set_text(lbl_status, bufStat);
        // lv_label_set_text(lbl_door, bufDoor);

        // *** Reset timer idle ***
        lastTouchMs = millis();
        // Jika sebelumnya redup, kembalikan terang penuh
        if (curBrightness != 255) {
          curBrightness = 255;
          ledcWrite(BL_CHANNEL, curBrightness);
        }
      } else if (line.startsWith("INIT;")) {
        String p[7];
        int idx = 0;
        for (int i = 0; i < 6; i++) {
          int t = line.indexOf(';', idx);
          p[i] = line.substring(idx, t);
          idx = t + 1;
        }
        p[6] = line.substring(idx);
        p[1].toCharArray(bufMode, sizeof(bufMode));
        p[2].toCharArray(bufDevice, sizeof(bufDevice));
        p[3].toCharArray(bufssid, sizeof(bufssid));
        p[4].toCharArray(bufStatuswifi, sizeof(bufStatuswifi));
        p[5].toCharArray(bufip, sizeof(bufip));
        p[6].toCharArray(bufwaktuaktif, sizeof(bufwaktuaktif));
        // p[5].toCharArray(bufDoor, sizeof(bufDoor));
        // p[6].toCharArray(bufDur, sizeof(bufDur));
        lv_label_set_text(lbl_mode, bufMode);
        lv_label_set_text(lbl_device, bufDevice);
        lv_label_set_text(lbl_ssid, bufssid);
        lv_label_set_text(lbl_statuswifi, bufStatuswifi);
        lv_label_set_text(lbl_ip, bufip);
        lv_label_set_text(lbl_waktuaktif, bufwaktuaktif);

        // *** Reset timer idle ***
        lastTouchMs = millis();
        // Jika sebelumnya redup, kembalikan terang penuh
        if (curBrightness != 255) {
          curBrightness = 255;
          ledcWrite(BL_CHANNEL, curBrightness);
        }
      } else if (line.startsWith("DOOR;")) {
        String p[3];
        int idx = 0;
        for (int i = 0; i < 2; i++) {
          int t = line.indexOf(';', idx);
          p[i] = line.substring(idx, t);
          idx = t + 1;
        }
        p[2] = line.substring(idx);
        p[1].toCharArray(bufDoor, sizeof(bufDoor));
        p[2].toCharArray(bufDur, sizeof(bufDur));

        // lv_label_set_text(lbl_door, bufDoor);
        lv_label_set_text(lbl_duration, bufDur);

        // Ganti style background b1 sesuai kondisi
        if (p[1] == "LOCKED") {
          lv_obj_set_style_bg_img_src(b1, "S:/LOCKED.bin", 0);
        } else {
          lv_obj_set_style_bg_img_src(b1, "S:/UNLOCKED.bin", 0);
        }

        // Minta LVGL redraw hanya pada b1
        lv_obj_invalidate(b1);
        // lv_disp_load_scr(scr_main);
        // *** Reset timer idle ***
        lastTouchMs = millis();
        // Jika sebelumnya redup, kembalikan terang penuh
        if (curBrightness != 255) {
          curBrightness = 255;
          ledcWrite(BL_CHANNEL, curBrightness);
        }
      } else if (line == "OK") {
        lv_disp_load_scr(scr_ok);
        last = millis();
        // *** Reset timer idle ***
        lastTouchMs = millis();
        // Jika sebelumnya redup, kembalikan terang penuh
        if (curBrightness != 255) {
          curBrightness = 255;
          ledcWrite(BL_CHANNEL, curBrightness);
        }
      } else if (line == "FAIL") {
        lv_disp_load_scr(scr_fail);
        last = millis();
        // *** Reset timer idle ***
        lastTouchMs = millis();
        // Jika sebelumnya redup, kembalikan terang penuh
        if (curBrightness != 255) {
          curBrightness = 255;
          ledcWrite(BL_CHANNEL, curBrightness);
        }
      }

      // reset backlight
      lastTouchMs = millis();
      if (curBrightness != 255) {
        curBrightness = 255;
        ledcWrite(BL_CHANNEL, curBrightness);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

// 3) Task Backlight: redupkan setelah idle 30 detik
void taskBacklight(void *pv) {
  const TickType_t xCheck = pdMS_TO_TICKS(100);
  for (;;) {
    if ((millis() - lastTouchMs) > 30000 && curBrightness != 8) {
      curBrightness = 8;
      ledcWrite(BL_CHANNEL, curBrightness);
    }

    if ((lv_disp_get_scr_act(NULL) == scr_ok || lv_disp_get_scr_act(NULL) == scr_fail)
        && millis() - last > DUR) lv_disp_load_scr(scr_main);
    vTaskDelay(xCheck);
  }
}
