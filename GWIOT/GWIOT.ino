#include <Gwiot7941e.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <RTClib.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#define GWIOT_PIN1 15
#define GWIOT_PIN2 21
#define RXD2 18
#define TXD2 17
#define SD_CS 39


// Gunakan instance Wire khusus
#define SDA_RTC 10
#define SCL_RTC 11
TwoWire WireRTC = TwoWire(0);  // I2C bus 0 (bisa juga pakai 1 kalau bus 0 sudah dipakai)
File uploadFile;

// Session management
struct Session {
  String name;
  String role;
};
static String sessionToken;
static Session sessionData;

String exitDate;
String exitTime;
String ipStr;

// Objek RTC
RTC_DS1307 rtc;

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", 7 * 3600, 60000);  // Server NTP Indonesia (GMT+7)

SPIClass SPI2;
Gwiot7941e GWIOT_1;
Gwiot7941e GWIOT_2;
Preferences preferences;

String FLAGSIMPAN = "";
String MODE = "";
String DEVICE = "";
String WIFI_SSID = "";
String WIFI_PASSWORD = "";
unsigned long TIME_OUT = 0UL;
String newnewDEVICE = "";
String newnewSSID = "";
String newnewPassword = "";

// String DEVICE_NAME = "";
// String currentMac = "";
// String currentIp  = "";
// String str;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const int SensorPintu = 4;
const int BUZ = 40;
const int Relay = 6;

// Konfigurasi LEDC channel
const int ledcChannel = 0;
const int ledcTimer = 0;

// const int melody[] = { 659, 622, 659, 622, 659, 494, 587, 523 };
// const int noteDurations[] = { 4,   4,   4,   4,   4,   4,   4,   2  };

//Lumayan
// const int melody[] = { 262, 294, 330, 294, 262, 330, 392, 330 };
// const int noteDurations[] = { 4,4,4,4,4,4,4,2 };

//cepat lumayan
const int melody[] = { 330, 330, 392, 440, 494, 494, 440, 392 };
const int noteDurations[] = { 8, 8, 8, 4, 8, 8, 4, 4 };

// const int melody[] = { 262, 294, 330, 349, 392, 440, 494, 523 };
// const int noteDurations[] = { 4,4,4,4,4,4,4,2 };

const int noteCount = sizeof(melody) / sizeof(melody[0]);

int flagopenclock = 0;
// Buffer untuk menyimpan waktu dalam format string
char formattedTanggal[11];  // format "dd/mm/yyyy" + null terminator
char formattedJam[9];       // format "hh:mm:ss" + null terminator
char dataTFT1[100];
char dataTFT2[100];
char dataTFT3[100];

String statusPintu;
bool sudahKirim = false;
String name;
String akses;
String durasi;
String statuswifi;
// Variabel global untuk menyimpan data waktu masuk dan status log
String myLastuidString = "";
bool entryTimeRecorded = false;
String entryDate = "";
String entryTime = "";
String uidString;
// Replace with your network credentials
const char *ssid = "PT.nyoba";
const char *password = "123456789";

long timezone = 0;
byte daysavetime = 1;

const char *PARAM_INPUT_1 = "uid";
const char *PARAM_INPUT_2 = "role";
const char *PARAM_INPUT_3 = "delete";
const char *PARAM_INPUT_4 = "delete-user";
const char *PARAM_INPUT_5 = "name";

String inputMessage;
String inputParam;

const unsigned long openThreshold = 5000;  // ambang waktu 5000 ms (5 detik)

unsigned long doorOpenStartTime = 0;  // untuk menyimpan waktu mulai ketika sensor HIGH
bool timerActive = false;             // flag apakah timer telah berjalan

void OTA() {
  Serial.println("Booting");
  // WiFi.mode(WIFI_STA);

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("GWIOT");

  // No authentication by default
  ArduinoOTA.setPassword("waprasta");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  // Serial.print("IP address: ");
  // Serial.println(WiFi.softAP());
}

// Write to the SD card
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card
void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);

  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  String finalString = String(message) + "\n";
  if (file.print(finalString.c_str())) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void updateLogFile(fs::FS &fs, const char *path, String uid) {
  Serial.printf("Updating file: %s\n", path);

  File file = fs.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  String updatedContent = "";  // Simpan seluruh file yang akan diperbarui
  bool found = false;          // Flag untuk cek apakah UID ditemukan

  myTime();
  String exitTime = String(formattedTanggal) + "," + String(formattedJam);

  // Baca file baris per baris
  while (file.available()) {
    String line = file.readStringUntil('\n');

    // Periksa apakah UID sudah ada dalam baris ini
    if (line.indexOf(uid) != -1 && !found) {
      // Jika UID ditemukan, tambahkan waktu keluar ke baris ini
      line += "," + exitTime;
      found = true;
    }

    updatedContent += line + "\n";  // Simpan ke buffer
  }
  file.close();

  // Jika UID ditemukan, tulis ulang file dengan data yang diperbarui
  if (found) {
    File fileWrite = fs.open(path, FILE_WRITE);
    if (!fileWrite) {
      Serial.println("Failed to open file for writing");
      return;
    }

    fileWrite.print(updatedContent);
    fileWrite.close();
    Serial.println("Log file updated successfully.");
  } else {
    Serial.println("UID not found in log.");
  }
}


// Append data to the SD card
void appendUserFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  String finalString = String(message) + "\n";

  if (file.print(finalString.c_str())) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

String processor(const String &var) {
  return String(" Gagal Memproses Data "
                "<br><a href=\"/add-user\"><button class=\"button button-home\">Kembali</button> </a>");
}

void deleteLineFromFile(const char *filename, int lineNumber) {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file for reading.");
    return;
  }

  // Read all lines except the one to delete
  String lines = "";
  int currentLine = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (currentLine != lineNumber) {
      lines += line + "\n";
    }
    currentLine++;
  }
  file.close();

  // Write back all lines except the deleted one
  file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing.");
    return;
  }

  file.print(lines);
  file.close();
  Serial.println("Line deleted successfully.");
}

String getRoleFromFile(const char *filename, String uid) {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file for reading.");
    return "";
  }

  file.readStringUntil('\n');  // Skip header

  while (file.available()) {
    String line = file.readStringUntil('\n');

    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);  // Cari koma kedua

    if (firstComma > 0 && secondComma > firstComma) {
      String fileUID = line.substring(0, firstComma);
      String role = line.substring(firstComma + 1, secondComma);  // Ambil hanya role

      if (fileUID == uid) {
        file.close();
        role.trim();
        return role;
      }
    }
  }
  file.close();
  return "";
}


String getNameFromFile(const char *filename, String uid) {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file for reading.");
    return "";
  }

  file.readStringUntil('\n');  // Skip header

  while (file.available()) {
    String line = file.readStringUntil('\n');

    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);  // Posisi koma kedua

    if (firstComma > 0 && secondComma > firstComma) {
      String fileUID = line.substring(0, firstComma);
      String name = line.substring(secondComma + 1);  // Ambil setelah koma kedua

      if (fileUID == uid) {
        file.close();
        name.trim();  // Hilangkan spasi atau karakter aneh
        return name;
      }
    }
  }
  file.close();
  return "";
}


void initLittleFS() {
  if (!LittleFS.begin()) {  // Argumen `true` akan memformat ulang jika gagal
    Serial.println("LittleFS mount failed! Trying to format...");
    return;
  }
  Serial.println("LittleFS mounted successfully");
}

void initWiFi() {
  if (MODE == "CLIENT" || MODE == "client") initWiFiClient();
  else if (MODE == "AP" || MODE == "ap") initWiFiAP();
  else initWiFiAP();
}

void initWiFiClient() {
  // Connect to Wi-Fi
  // WiFi.begin("PT.WaprastaGT", "123456789");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    statuswifi = "Connecting";
    Serial.println(statuswifi);
  }
  // Print ESP32 Local IP Address
  ipStr = WiFi.localIP().toString();
  Serial.println(ipStr);

  statuswifi = "Connected";
  Serial.println(statuswifi);
}

void initWiFiAP() {
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  ipStr = WiFi.softAPIP().toString();
  Serial.println(ipStr);
  delay(1000);
  statuswifi = "Connected";
  Serial.println(statuswifi);
}


void initRTC() {
  // Inisialisasi I2C di pin 10 dan 11
  WireRTC.begin(SDA_RTC, SCL_RTC);

  // Inisialisasi RTC dengan Wire yang sudah kita atur
  if (!rtc.begin(&WireRTC)) {
    Serial.println("RTC tidak ditemukan!");
    while (1)
      ;
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC tidak berjalan, menyetel waktu manual...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set waktu saat kompilasi
  }

  // Sinkronisasi waktu RTC dengan NTP (jika kamu punya fungsi syncRTC)
  syncRTC();
}

// Fungsi untuk menyinkronkan waktu RTC dengan NTP atau fallback ke RTC jika gagal
void syncRTC() {
  timeClient.begin();
  Serial.print("Mengambil waktu dari NTP...");
  const int maxAttempts = 5;  // batas maksimal percobaan
  int attempts = 0;
  bool ntpSuccess = false;

  // Coba update waktu dari NTP
  while (attempts < maxAttempts && !timeClient.update()) {
    Serial.print(".");
    timeClient.forceUpdate();
    delay(1000);
    attempts++;
  }

  // Jika percobaan melebihi batas, gunakan waktu dari RTC
  if (attempts >= maxAttempts) {
    Serial.println("\nNTP gagal mendapatkan waktu, menggunakan waktu RTC.");
    DateTime now = rtc.now();
    Serial.print("Waktu RTC saat ini: ");
    Serial.print(now.year());
    Serial.print("-");
    Serial.print(now.month());
    Serial.print("-");
    Serial.print(now.day());
    Serial.print(" ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
  } else {
    // Jika sukses mendapatkan waktu dari NTP
    Serial.println("\nSukses mendapatkan waktu dari NTP!");
    unsigned long epochTime = timeClient.getEpochTime();
    Serial.print("Epoch Time: ");
    Serial.println(epochTime);

    // Konversi epoch ke format RTC dan set RTC
    DateTime ntpTime(epochTime);
    Serial.print("Set RTC ke: ");
    Serial.print(ntpTime.year());
    Serial.print("-");
    Serial.print(ntpTime.month());
    Serial.print("-");
    Serial.print(ntpTime.day());
    Serial.print(" ");
    Serial.print(ntpTime.hour());
    Serial.print(":");
    Serial.print(ntpTime.minute());
    Serial.print(":");
    Serial.println(ntpTime.second());

    rtc.adjust(ntpTime);
    Serial.println("Waktu RTC telah diperbarui!");
  }
}

void myTime() {
  DateTime now = rtc.now();

  // Menggunakan sprintf untuk membentuk string waktu
  sprintf(formattedTanggal, "%02d/%02d/%04d", now.day(), now.month(), now.year());

  sprintf(formattedJam, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // Serial.println(formattedJam);
  // Serial.println();
  // Serial.println(formattedTanggal);
  // Serial.println();
}

void initSDCard() {
  // Inisialisasi SD Card menggunakan SPI2
  SPI2.begin(36, 37, 35, SD_CS);
  if (!SD.begin(SD_CS, SPI2, 200000000)) {
    Serial.println("Inisialisasi SD Card gagal!");
  } else {
    Serial.println("SD Card berhasil diinisialisasi.");
  }

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // If the log.txt file doesn't exist, create a file on the SD card and write the header
  File file = SD.open("/log.txt");
  if (!file) {
    Serial.println("log.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/log.txt", "");
  } else {
    Serial.println("log.txt file already exists");
  }
  file.close();

  // If the users.txt file doesn't exist, create a file on the SD card and write the header
  file = SD.open("/users.txt");
  if (!file) {
    Serial.println("users.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/users.txt", "UID,Role,Name\r\nadmin,admin,admin\n");
  } else {
    Serial.println("users.txt file already exists");
  }
  file.close();
}

void initGWIOT() {
  GWIOT_1.begin(GWIOT_PIN1, 1);
  GWIOT_2.begin(GWIOT_PIN2, 2);

  Serial.println("init GWIOT");
}

// Fungsi untuk mencari entri terakhir UID dalam log
String findLastEntry(const char *path, String uid) {
  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial.println("Gagal membuka file untuk membaca");
    return "";
  }

  String lastEntry = "";
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.indexOf(uid) != -1) {
      lastEntry = line;  // Simpan baris terakhir yang cocok
    }
  }
  file.close();
  return lastEntry;
}

bool hasExitTime(String lastEntry) {
  int commaCount = 0;
  for (int i = 0; i < lastEntry.length(); i++) {
    if (lastEntry[i] == ',') commaCount++;
  }
  return (commaCount >= 5);  // Jika ada 5 koma berarti sudah ada waktu keluar
}

// Fungsi untuk menghitung durasi berdasarkan dua string waktu
// Format waktu diasumsikan HH:MM (bisa dimodifikasi sesuai format kamu)
String calculateDuration(String startTime, String endTime) {
  // Ekstrak jam, menit, dan detik dari startTime (format "HH:MM:SS")
  int startHour = startTime.substring(0, 2).toInt();
  int startMin = startTime.substring(3, 5).toInt();
  int startSec = startTime.substring(6, 8).toInt();

  // Ekstrak jam, menit, dan detik dari endTime (format "HH:MM:SS")
  int endHour = endTime.substring(0, 2).toInt();
  int endMin = endTime.substring(3, 5).toInt();
  int endSec = endTime.substring(6, 8).toInt();

  // Konversi masing-masing waktu ke detik
  int startTotal = startHour * 3600 + startMin * 60 + startSec;
  int endTotal = endHour * 3600 + endMin * 60 + endSec;

  // Hitung selisih total dalam detik
  int diff = endTotal - startTotal;

  // Konversi kembali ke jam, menit, dan detik
  int diffHour = diff / 3600;
  int diffMin = (diff % 3600) / 60;
  int diffSec = diff % 60;

  char buffer[9];  // "HH:MM:SS" membutuhkan 8 karakter + null terminator
  // Format hasil dengan leading zero menggunakan sprintf
  sprintf(buffer, "%02d:%02d:%02d", diffHour, diffMin, diffSec);
  return String(buffer);
}



// Fungsi untuk update baris terakhir dengan menambahkan waktu masuk
void updateLogWithEntryTime(const char *path, String inDate, String inTime) {
  // Baca seluruh isi file ke dalam variabel 'content'
  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial.println("Gagal membuka file untuk membaca update entry time");
    return;
  }

  String content = "";
  while (file.available()) {
    // Baca tiap baris tanpa menambahkan "\n" tambahan pada akhir file
    content += file.readStringUntil('\n');
    if (file.available()) {
      content += "\n";  // Hanya tambahkan newline antar baris, bukan di akhir file
    }
  }
  file.close();

  // Jika ada newline di akhir, hapus agar entri log tetap satu baris
  if (content.endsWith("\n")) {
    content.remove(content.length() - 1);
  }

  // Pastikan file tidak kosong
  if (content.length() == 0) {
    Serial.println("File kosong, tidak ada yang diperbarui.");
    return;
  }

  // Pisahkan konten menjadi prefix (seluruh baris kecuali baris terakhir) dan lastLine (baris terakhir)
  int lastNewline = content.lastIndexOf('\n');
  String prefix = "";
  String lastLine = "";
  if (lastNewline != -1) {
    prefix = content.substring(0, lastNewline + 1);  // Termasuk newline
    lastLine = content.substring(lastNewline + 1);
  } else {
    // Hanya ada satu baris dalam file
    lastLine = content;
  }

  // Update baris terakhir: tambahkan tanggal dan waktu masuk
  // Pastikan tidak ada newline yang ditambahkan
  lastLine += "," + inDate + "," + inTime;

  // Gabungkan kembali prefix dan lastLine untuk membentuk konten yang diperbarui
  String updatedContent = prefix + lastLine;

  // Tulis ulang file dengan isi yang telah diperbarui
  File writeFile = SD.open(path, FILE_WRITE);
  if (!writeFile) {
    Serial.println("Gagal membuka file untuk update entry time");
    return;
  }

  writeFile.print(updatedContent);
  writeFile.close();

  FLAGSIMPAN = "1";

  preferences.begin("webData", false);
  preferences.putString("flagsimpan", FLAGSIMPAN);
  preferences.end();

  Serial.println("Waktu masuk telah diperbarui.");
}



// Fungsi untuk update baris terakhir dengan menambahkan waktu keluar dan durasi
void updateLogWithExitTime(const char *path, String outDate, String outTime, String duration) {
  // Baca semua baris
  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial.println("Gagal membuka file untuk update exit time");
    return;
  }

  String allLines = "";
  String lastLine = "";
  int lineCount = 0;
  int lastIndex = -1;

  // Simpan seluruh isi file dalam buffer
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      lastLine = line;
      lastIndex = lineCount;
    }
    allLines += line + "\n";
    lineCount++;
  }
  file.close();

  // Ubah baris terakhir dengan menambahkan exit time dan duration
  lastLine += "," + outDate + "," + outTime + "," + duration;

  // Pecah isi file menjadi baris dan gantikan baris terakhir
  String updatedContent = "";
  int currentLine = 0;
  int startIdx = 0;
  while (currentLine < lineCount) {
    int newLineIdx = allLines.indexOf("\n", startIdx);
    String line = allLines.substring(startIdx, newLineIdx);
    if (currentLine == lastIndex) {
      updatedContent += lastLine + "\n";
    } else {
      updatedContent += line + "\n";
    }
    startIdx = newLineIdx + 1;
    currentLine++;
  }

  // Tulis ulang file dengan updatedContent
  File writeFile = SD.open(path, FILE_WRITE);
  if (!writeFile) {
    Serial.println("Gagal membuka file untuk menulis ulang update exit time");
    return;
  }
  writeFile.print(updatedContent);
  writeFile.close();

  FLAGSIMPAN = "0";

  preferences.begin("webData", false);
  preferences.putString("flagsimpan", FLAGSIMPAN);
  preferences.end();

  Serial.println("Waktu keluar dan durasi telah diperbarui.");
}

void playTone(int freq, int duration) {
  // Set frekuensi dan duty cycle 50%
  ledcWriteTone(ledcChannel, freq);
  ledcWrite(ledcChannel, 128);
  delay(duration);
  ledcWriteTone(ledcChannel, 0);  // Matikan nada
}

void BUZZ() {
  for (int i = 0; i < noteCount; i++) {
    // Hitung durasi nada dalam ms: misal noteDurations=4 → 250 ms
    int duration = 1000 / noteDurations[i];
    // Mainkan nada
    ledcWriteTone(ledcChannel, melody[i]);
    ledcWrite(ledcChannel, 128);
    delay(duration);
    // Jedah singkat antar nada
    ledcWriteTone(ledcChannel, 0);
    delay(20);
  }
}

void ambilDataEEPROM() {
  preferences.begin("webData", true);
  MODE = preferences.getString("mode", "");
  DEVICE = preferences.getString("device", "");
  WIFI_SSID = preferences.getString("ssid", "");
  WIFI_PASSWORD = preferences.getString("password", "");
  // TIME_OUT = preferences.getString("timeout", "");
  TIME_OUT = preferences.getULong("timeout", 0UL);
  FLAGSIMPAN = preferences.getString("flagsimpan", "");

  Serial.println(DEVICE);
  Serial.println(WIFI_SSID);
  Serial.println(WIFI_PASSWORD);
  Serial.println(TIME_OUT);
  // DEVICE_NAME = preferences.getString("device", "Default");
  // currentMac = preferences.getString("mac", "");
  // currentIp = preferences.getString("ip", "");
  // updateMacAndIp(currentMac, currentIp);

  // myMAC = currentMac.c_str();
  // deviceName = DEVICE_NAME.c_str();
  // wifiSSIDClient     = WIFI_SSID.c_str();
  // wifiPasswordClient = WIFI_PASSWORD.c_str();
  preferences.end();
}

void sendTFT2(String screen, String pintu, String durasi) {
  sprintf(dataTFT2,
          "%s;%s;%s",
          screen.c_str(),
          pintu.c_str(),
          durasi.c_str());
  Serial2.printf(dataTFT2);
  Serial.println(dataTFT2);
  delay(2000);
}

void sendTFT1(String screen, String nama, String time, String date, String akses) {
  sprintf(dataTFT1,
          "%s;%s;%s;%s;%s",
          screen.c_str(),
          nama.c_str(),
          time.c_str(),
          date.c_str(),
          akses.c_str());
  Serial2.printf(dataTFT1);
  Serial.println(dataTFT1);
}

void sendTFT3(String screen, String mode, String device, String ssid, String statuswifi, String ip, String waktuaktif) {
  sprintf(dataTFT3,
          "%s;%s;%s;%s;%s;%s;%s",
          screen.c_str(),
          mode.c_str(),
          device.c_str(),
          ssid.c_str(),
          statuswifi.c_str(),
          ip.c_str(),
          waktuaktif.c_str());
  Serial2.printf(dataTFT3);
  Serial.println(dataTFT3);
}

// Fungsi callback untuk upload
void handleFileUpload(AsyncWebServerRequest *request,
                      const String &filename,
                      size_t index,
                      uint8_t *data,
                      size_t len,
                      bool final) {
  // Hanya menangani file dengan nama users.txt
  if (filename != "users.txt") return;

  // Jika potongan pertama, hapus file lama dan buka file baru
  if (index == 0) {
    if (SD.exists("/users.txt")) {
      SD.remove("/users.txt");
      Serial.println(">> Menghapus file lama /users.txt");
    }
    uploadFile = SD.open("/users.txt", FILE_WRITE);
    if (!uploadFile) {
      Serial.println("!! Gagal membuka /users.txt untuk ditulis");
      return;
    }
    Serial.println(">> Mulai menulis /users.txt");
  }

  // Tulis potongan data ke file
  if (len && uploadFile) {
    uploadFile.write(data, len);
  }

  // Jika ini chunk terakhir, flush & close di bagian onSuccess di setup()
  if (final) {
    Serial.printf(">> Terima chunk terakhir (%u bytes)\n", len);
  }
}

// Generate a random token for session
String genToken() {
  return String(random(0xFFFFFF), HEX) + String(millis(), HEX);
}

// Check if request has valid session cookie
bool isAuthenticated(AsyncWebServerRequest *req) {
  if (!req->hasHeader("Cookie")) return false;
  String cookie = req->header("Cookie");
  int idx = cookie.indexOf("session=");
  if (idx < 0) return false;
  String tok = cookie.substring(idx + 8);
  int end = tok.indexOf(';');
  if (end > 0) tok = tok.substring(0, end);
  return tok == sessionToken;
}

// Helper to serve protected files
void serveProtected(const char *url, const char *filepath) {
  server.on(url, HTTP_GET, [=](AsyncWebServerRequest *req) {
    if (!isAuthenticated(req)) {
      req->redirect("/login.html");
      return;
    }
    req->send(LittleFS, filepath);
  });
}

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  ambilDataEEPROM();
  pinMode(SensorPintu, INPUT_PULLUP);
  // pinMode(BUZ, OUTPUT);
  pinMode(Relay, OUTPUT);

  ledcSetup(ledcChannel, 2000, 8);  // 2 kHz base frekuensi, 8-bit resolusi
  ledcAttachPin(BUZ, ledcChannel);

  BUZZ();
  randomSeed(esp_random());

  initGWIOT();
  initLittleFS();
  initWiFi();
  OTA();
  initRTC();
  initSDCard();
  myTime();
  String waktu = String(formattedJam) + " | " + String(formattedTanggal);
  sendTFT3("INIT", MODE, DEVICE, WIFI_SSID, statuswifi, ipStr, waktu);

  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(LittleFS, "/login.html");
  // });
  // // Route for root / web page
  // server.on("/full-log", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(LittleFS, "/full-log.html");
  // });
  // // Route for root /add-user web page
  // server.on("/add-user", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(LittleFS, "/add-user.html");
  // });
  // // Route for root /manage-users web page
  // server.on("/manage-users", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(LittleFS, "/manage-users.html");
  // });

  // // NEWW
  // server.on("/configuration", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(LittleFS, "/configuration.html");
  // });


  // server.on("/handleGantiDevice", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(LittleFS, "/handleGantiDevice.html");
  // });
  // server.on("/handleGantiMacIp", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(LittleFS, "/handleGantiMacIp.html");
  // });
  // server.on("/handlepanduan", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(LittleFS, "/handlepanduan.html");
  // });

  // // Serve Static files
  // server.serveStatic("/", LittleFS, "/");

  // // Loads the log.txt file
  // server.on("/view-log", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(SD, "/log.txt", "text/plain", false);
  // });
  // // Loads the users.txt file
  // server.on("/view-users", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(SD, "/users.txt", "text/plain", false);
  // });

  // // Endpoint untuk download file users.txt
  // server.on("/download/users.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   AsyncWebServerResponse *response = request->beginResponse(SD, "/users.txt", "text/plain");
  //   response->addHeader("Content-Disposition", "attachment; filename=users.txt");
  //   request->send(response);
  // });

  // // Endpoint untuk download file log.txt
  // server.on("/download/log.txt", HTTP_GET, [](AsyncWebServerRequest *request){
  //     AsyncWebServerResponse *response = request->beginResponse(SD, "/log.txt", "text/plain");
  //     response->addHeader("Content-Disposition", "attachment; filename=log.txt");
  //     request->send(response);
  // });

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // LOGIN endpoint (POST)
  server.on("/authenticate", HTTP_POST, [](AsyncWebServerRequest *req) {
    String uid, nama;

    if (req->hasParam("uid", true) && req->hasParam("nama", true)) {
      uid = req->getParam("uid", true)->value();
      nama = req->getParam("nama", true)->value();
    } else {
      req->send(400, "application/json", "{\"error\":\"Missing uid or nama\"}");
      return;
    }

    // Validate against users.txt
    File f = SD.open("/users.txt");
    bool found = false;
    String role;
    if (f) {
      f.readStringUntil('\n');  // skip header
      while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        int uidIdx = line.indexOf(',');
        int roleIdx = line.indexOf(',', uidIdx + 1);
        int nameIdx = line.indexOf(',', roleIdx + 1);

        String f_uid = line.substring(0, uidIdx);
        String f_role = line.substring(uidIdx + 1, roleIdx);
        String f_nama = line.substring(roleIdx + 1);

        if (f_uid == uid && f_nama == nama) {
          role = f_role;
          found = true;
          break;
        }
      }
      f.close();
    }

    if (!found) {
      req->send(401, "application/json", "{\"ok\":false}");
      return;
    }

    // Buat token dan simpan sesi
    String token = genToken();
    sessionToken = token;
    sessionData.name = nama;
    sessionData.role = role;

    // Kirim balik JSON + Set-Cookie
    DynamicJsonDocument doc(128);
    doc["ok"] = true;
    doc["role"] = role;
    String payload;
    serializeJson(doc, payload);

    auto res = req->beginResponse(200, "application/json", payload);
    res->addHeader("Set-Cookie", "session=" + token + "; Path=/; HttpOnly");
    req->send(res);
  });

  // LOGOUT endpoint
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *req) {
    sessionToken = "";
    AsyncWebServerResponse *res = req->beginResponse(302, "text/plain", "");
    res->addHeader("Set-Cookie", "session=; Max-Age=0");
    res->addHeader("Location", "/login.html");
    req->send(res);
  });

  // ME endpoint for client-side JS
  server.on("/me", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!isAuthenticated(req)) {
      req->send(401);
      return;
    }
    DynamicJsonDocument doc(128);
    doc["nama"] = sessionData.name;
    doc["role"] = sessionData.role;
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  // Serve login and static assets without auth
  server.serveStatic("/login.html", LittleFS, "/login.html");
  server.serveStatic("/js/xlsx.full.min.js", LittleFS, "/js/xlsx.full.min.js");
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/img/logo.png", LittleFS, "/img/logo.png");
  // Protected routes
  serveProtected("/", "/full-log.html");
  serveProtected("/full-log", "/full-log.html");
  serveProtected("/add-user", "/add-user.html");
  serveProtected("/manage-users", "/manage-users.html");
  // serveProtected("/configuration", "/configuration.html");

  // Protected data endpoints
  server.on("/view-log", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!isAuthenticated(req)) {
      req->redirect("/login.html");
      return;
    }
    req->send(SD, "/log.txt", "text/plain");
  });
  server.on("/view-users", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!isAuthenticated(req)) {
      req->redirect("/login.html");
      return;
    }
    req->send(SD, "/users.txt", "text/plain");
  });

  // Download endpoints
  server.on("/download/users.txt", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!isAuthenticated(req)) {
      req->redirect("/login.html");
      return;
    }
    auto r = req->beginResponse(SD, "/users.txt", "text/plain");
    r->addHeader("Content-Disposition", "attachment; filename=users.txt");
    req->send(r);
  });

  // ROUTE: GantiSSID (GET + manual placeholder replace)
  server.on("/configuration", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!isAuthenticated(req)) {
      req->redirect("/login.html");
      return;
    }

    File f = LittleFS.open("/configuration.html", "r");
    if (!f) {
      req->send(500, "text/plain", "File configuration.html tidak ditemukan");
      return;
    }
    String html = f.readString();
    f.close();

    // ganti placeholder
    html.replace("%MODE%", MODE);
    html.replace("%DEVICE%", DEVICE);
    html.replace("%OLD_SSID%", WIFI_SSID);
    html.replace("%OLD_PASSWORD%", WIFI_PASSWORD);
    html.replace("%TIME_OUT%", String(TIME_OUT));

    req->send(200, "text/html", html);
  });

  // ROUTE: proses form
  server.on("/ButtonGantiSSID", HTTP_POST, [](AsyncWebServerRequest *req) {
    if (!isAuthenticated(req)) {
      req->redirect("/login.html");
      return;
    }
    String newMODE = req->arg("mode");
    String newDEVICE = req->arg("device");
    String newSSID = req->arg("ssid");
    String newPassword = req->arg("password");
    String newTimeout = req->arg("timeout");

    unsigned long timeoutUL = (unsigned long)newTimeout.toInt();

    // simpan di Preferences
    preferences.begin("webData", false);
    preferences.putString("mode", newMODE);
    preferences.putString("device", newDEVICE);
    preferences.putString("ssid", newSSID);
    preferences.putString("password", newPassword);
    preferences.putULong("timeout", timeoutUL);
    preferences.end();

    // update variabel
    MODE = newMODE;
    DEVICE = newDEVICE;
    WIFI_SSID = newSSID;
    WIFI_PASSWORD = newPassword;
    TIME_OUT = timeoutUL;

    // kirim halaman konfirmasi sederhana
    String resp = "<!DOCTYPE html><html lang='id'><head>"
                  "<meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>"
                  "<title>Berhasil</title></head><body style='font-family:Arial,sans-serif;text-align:center;padding:40px;'>"
                  "<h2>✅ Berhasil disimpan!</h2>"
                  "<p><strong>MODE:</strong> "
                  + newMODE + "</p>"
                              "<p><strong>Device:</strong> "
                  + newDEVICE + "</p>"
                                "<p><strong>SSID:</strong> "
                  + newSSID + "</p>"
                              "<p><strong>Password:</strong> "
                  + newPassword + "</p>"
                                  "<p><strong>Time Out:</strong> "
                  + timeoutUL + "</p>"
                                "<p><a href='/'>Kembali</a></p>"
                                "</body></html>";
    req->send(200, "text/html", resp);
  });


  // Receive HTTP GET requests on <ESP_IP>/get?input=<inputMessage>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
      request->redirect("/login.html");
      return;
    }
    // GET input1 and input2 value on <ESP_IP>/get?input1=<inputMessage1>&input2=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2) && request->hasParam(PARAM_INPUT_5)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = String(PARAM_INPUT_1);
      inputMessage += " " + request->getParam(PARAM_INPUT_2)->value();
      inputParam += " " + String(PARAM_INPUT_2);
      inputMessage += " " + request->getParam(PARAM_INPUT_5)->value();
      inputParam += " " + String(PARAM_INPUT_5);

      String finalMessageInput = String(request->getParam(PARAM_INPUT_1)->value()) + "," + String(request->getParam(PARAM_INPUT_2)->value()) + "," + String(request->getParam(PARAM_INPUT_5)->value());
      Serial.println(finalMessageInput);
      appendUserFile(SD, "/users.txt", finalMessageInput.c_str());
    } else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = String(PARAM_INPUT_3);
      if (request->getParam(PARAM_INPUT_3)->value() == "users") {
        deleteFile(SD, "/users.txt");
      } else if (request->getParam(PARAM_INPUT_3)->value() == "log") {
        deleteFile(SD, "/log.txt");
      }
    } else if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_4)->value();
      inputParam = String(PARAM_INPUT_4);
      deleteLineFromFile("/users.txt", inputMessage.toInt());
    } else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    request->send(LittleFS, "/manage-users.html");
  });

  // Route untuk upload
  server.on(
    "/upload", HTTP_POST,
    // Saat request berakhir
    [](AsyncWebServerRequest *request) {
      if (!isAuthenticated(request)) {
        request->redirect("/login.html");
        return;
      }
      if (uploadFile) {
        uploadFile.close();
        request->send(LittleFS, "/manage-users.html");
        Serial.println(">> Upload selesai, file ditulis.");
      } else {
        request->send(LittleFS, "/get.html", "text/html", false, processor);
      }
    },
    // Handler untuk tiap potongan data upload
    handleFileUpload);


  // server.on("/search-log", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   if (!isAuthenticated(request)) {
  //     request->redirect("/login.html");
  //     return;
  //   }
  //   if (!request->hasParam("date")) {
  //     request->send(400, "text/plain", "Parameter 'date' tidak disediakan. Contoh: /search-log?date=2025/03/25");
  //     return;
  //   }
  //   String searchDate = request->getParam("date")->value();

  //   File file = SD.open("/log.txt", FILE_READ);
  //   if (!file) {
  //     request->send(500, "text/plain", "Gagal membuka file log.");
  //     return;
  //   }

  //   // Buat stream respons chunked
  //   AsyncResponseStream *response = request->beginResponseStream("text/plain");

  //   const size_t BUF_SIZE = 128;
  //   char buf[BUF_SIZE];
  //   size_t idx = 0;
  //   bool anyFound = false;

  //   // Setiap kali kita baca satu karakter:
  //   while (file.available()) {
  //     char c = file.read();
  //     if (c == '\r') continue;  // abaikan carriage return
  //     if (c == '\n' || idx >= BUF_SIZE - 1) {
  //       // Akhir baris atau buffer penuh
  //       buf[idx] = '\0';
  //       idx = 0;

  //       // Cek apakah baris mengandung tanggal
  //       if (strstr(buf, searchDate.c_str()) != nullptr) {
  //         response->print(buf);
  //         response->print('\n');
  //         anyFound = true;
  //       }
  //       // Beri napas ke RTOS agar watchdog tak reset
  //       delay(1);

  //     } else {
  //       buf[idx++] = c;
  //     }
  //   }
  //   file.close();

  //   if (!anyFound) {
  //     response->print("Tidak ditemukan log untuk tanggal: ");
  //     response->print(searchDate);
  //     response->print('\n');
  //   }

  //   request->send(response);
  // });


  server.begin();

  if (FLAGSIMPAN == "1") {
    updateLogWithExitTime("/log.txt", " ", " ", " ");
  }
}

void loop() {
  ArduinoOTA.handle();

  if (Serial.available()) {
    String L = Serial.readStringUntil('\n');
    L.trim();
    if (L.startsWith("IKI;")) {
      String p[5];
      int idx = 0;
      for (int i = 0; i < 4; i++) {
        int t = L.indexOf(';', idx);
        p[i] = L.substring(idx, t);
        idx = t + 1;
      }
      p[4] = L.substring(idx);

      preferences.begin("webData", false);
      preferences.putString("device", p[1]);
      preferences.putString("mode", p[2]);
      preferences.putString("ssid", p[3]);
      preferences.putString("password", p[4]);
      preferences.end();

      Serial.println(p[1]);
      Serial.println(p[2]);
      Serial.println(p[3]);
      Serial.println(p[4]);
      ESP.restart();
    }
  }

  // ============ BACA RFID 1 ============
  if (GWIOT_1.update()) {

    if (entryTimeRecorded && myLastuidString != "") {
      // Tutup entri sebelumnya dengan waktu keluar dan durasi
      myTime();
      exitDate = String(formattedTanggal);
      exitTime = String(formattedJam);
      durasi = calculateDuration(entryTime, exitTime);
      updateLogWithExitTime("/log.txt", exitDate, exitTime, durasi);

      // Reset flag agar entri sebelumnya dianggap sudah ditutup
      entryTimeRecorded = false;
      myLastuidString = "";
    }

    uint32_t uid = GWIOT_1.getLastTagId();
    String uidString = String(uid);
    myLastuidString = uidString;

    Serial.print("Card UID 1 : ");
    Serial.println(uidString);

    String role = getRoleFromFile("/users.txt", uidString);
    name = getNameFromFile("/users.txt", uidString);
    akses = "Masuk";

    if (role == "") {
      Serial.println("UID tidak terdaftar, tidak menulis ke log.");
      Serial2.println("FAIL");
      return;
    }

    Serial2.println("OK");

    // Tulis entri awal ke file log (tanpa waktu masuk/keluar)
    String logEntry = uidString + "," + name + "," + role + "," + akses;
    Serial.println(logEntry);
    appendFile(SD, "/log.txt", logEntry.c_str());

    Serial.println("Catat Waktu Awal PINTU");
    myTime();
    entryDate = String(formattedTanggal);
    entryTime = String(formattedJam);
    updateLogWithEntryTime("/log.txt", entryDate, entryTime);
    sendTFT1("MAIN", name, entryTime, entryDate, akses);

    entryTimeRecorded = false;
    delay(100);
    tone(BUZ, 1000);
    delay(100);
    noTone(BUZ);
    delay(1000);
    digitalWrite(Relay, HIGH);
  }

  // ============ BACA RFID 2 ============
  if (GWIOT_2.update()) {

    if (entryTimeRecorded && myLastuidString != "") {
      // Tutup entri sebelumnya dengan waktu keluar dan durasi
      myTime();
      exitDate = String(formattedTanggal);
      exitTime = String(formattedJam);
      durasi = calculateDuration(entryTime, exitTime);
      updateLogWithExitTime("/log.txt", exitDate, exitTime, durasi);

      // Reset flag agar entri sebelumnya dianggap sudah ditutup
      entryTimeRecorded = false;
      myLastuidString = "";
    }

    uint32_t uid = GWIOT_2.getLastTagId();
    String uidString = String(uid);
    myLastuidString = uidString;

    Serial.print("Card UID 2 : ");
    Serial.println(uidString);

    String role = getRoleFromFile("/users.txt", uidString);
    name = getNameFromFile("/users.txt", uidString);
    akses = "Keluar";

    if (role == "") {
      Serial.println("UID tidak terdaftar, tidak menulis ke log.");
      Serial2.println("FAIL");
      return;
    }

    Serial2.println("OK");

    // Tulis entri awal ke file log (tanpa waktu masuk/keluar)
    String logEntry = uidString + "," + name + "," + role + "," + akses;
    Serial.println(logEntry);
    appendFile(SD, "/log.txt", logEntry.c_str());

    Serial.println("Catat Waktu Awal PINTU");
    myTime();
    entryDate = String(formattedTanggal);
    entryTime = String(formattedJam);
    updateLogWithEntryTime("/log.txt", entryDate, entryTime);
    sendTFT1("MAIN", name, entryTime, entryDate, akses);

    entryTimeRecorded = false;
    delay(100);
    tone(BUZ, 1000);
    delay(100);
    noTone(BUZ);
    delay(1000);
    digitalWrite(Relay, HIGH);
  }

  // Jika sensor pintu HIGH pintu terbuka
  if (digitalRead(SensorPintu) == HIGH) {
    // Jika timer belum aktif, simpan waktu mulai
    statusPintu = "UNLOCKED";
    if (!timerActive) {
      doorOpenStartTime = millis();
      timerActive = true;
    }

    // Bila sudah melewati ambang waktu (threshold), aktifkan buzzer
    if (millis() - doorOpenStartTime >= (TIME_OUT * 1000)) {
      tone(BUZ, 1000);
    }

    // Bila pintu terbuka dan ada proses entri, simpan waktu masuk log seperti sebelumnya
    if (!entryTimeRecorded && myLastuidString != "") {
      // Serial.println("Catat Waktu Awal PINTU");
      // myTime();
      // entryDate = String(formattedTanggal);
      // entryTime = String(formattedJam);
      // updateLogWithEntryTime("/log.txt", entryDate, entryTime);
      // sendTFT1("MAIN", name, entryTime, entryDate, akses);
      // delay(100);
      sendTFT2("DOOR", statusPintu, "00:00:00");
      entryTimeRecorded = true;
    }
    // Serial.println("Pintu Terbuka");
  }
  // Jika sensor pintu LOW (pintu tertutup)
  else if (digitalRead(SensorPintu) == LOW) {

    statusPintu = "LOCKED";
    noTone(BUZ);
    timerActive = false;
    doorOpenStartTime = 0;

    // Jika terdapat entri yang sedang aktif, catat waktu keluar dan durasi
    if (entryTimeRecorded && myLastuidString != "") {
      Serial.println("Catat Waktu Akhir");
      myTime();
      exitDate = String(formattedTanggal);
      exitTime = String(formattedJam);
      durasi = calculateDuration(entryTime, exitTime);
      updateLogWithExitTime("/log.txt", exitDate, exitTime, durasi);

      digitalWrite(Relay, LOW);
      sendTFT2("DOOR", statusPintu, durasi);

      entryTimeRecorded = false;
      myLastuidString = "";
    }
    // Serial.println("Pintu Tertutup");
  }

  delay(10);
}
