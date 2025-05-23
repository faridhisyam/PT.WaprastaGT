#include "MyWebServer.h"          // Pastikan WebServer.h sudah disesuaikan untuk WiFi AP 
 
// Pin chip select SD Card
#define SD_CS 39 
#define W5500_CS_PIN 10

// Definisi untuk tampilan OLED
#define SCREEN_WIDTH 128 // Lebar OLED dalam piksel
#define SCREEN_HEIGHT 32 // Tinggi OLED dalam piksel
#define OLED_RESET     -1       // Tidak menggunakan pin reset khusus
#define SCREEN_ADDRESS 0x3C     // Alamat I2C OLED 

const int myWIFI = 4;
const int myETHERNET = 5;

int WIFI = 0, ETHERNET = 0;  

String CARDTYPE = "";

// Variabel global untuk pengaturan sektor SD card
int m_sector_size = 512;  // Ukuran sektor standar (512 byte)
uint32_t m_sector_count = 0;
int flagSetup = 1;

// Variabel untuk menyimpan listing file terakhir
String lastDirListing = ""; 
// Variabel untuk menyimpan IP dari WiFi AP
String ipStr;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); 
USBMSC msc;  
SPIClass SPI2;

// Konfigurasi Ethernet 1  
// byte mac[]    = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// Konfigurasi Ethernet 2  
byte mac[6]    = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress ip(192, 168, 1, 10);  // Ganti sesuai dengan device
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

unsigned int localUdpPort = 8888; // Port untuk komunikasi UDP
EthernetUDP Udp;
char packetBuffer[255]; // Buffer untuk menerima data 
 
// Variabel informasi device
// const char* deviceName = DEVICE_NAME.c_str();
// const char* wifiSSIDClient     = WIFI_SSID.c_str();
// const char* wifiPasswordClient = WIFI_PASSWORD.c_str();  

const char* deviceName = "0";
const char* wifiSSIDClient     = "0";
const char* wifiPasswordClient = "0";
const char* myMAC = "0";  

// Fungsi untuk mengirim respons
void sendResponse() {
  unsigned long uptimeMillis = millis();
  unsigned long uptimeSeconds = uptimeMillis / 1000;  // Konversi ke detik

  // Konversi waktu ke format HH:MM:SS untuk tampilan awal
  unsigned long seconds = uptimeSeconds % 60;
  unsigned long minutes = (uptimeSeconds / 60) % 60;
  unsigned long hours = (uptimeSeconds / 3600);

  char uptimeStr[10];
  sprintf(uptimeStr, "%02lu:%02lu:%02lu", hours, minutes, seconds);

  // Buat string IP
  char ipStr[16];
  sprintf(ipStr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  
  // Format respon (dipisahkan dengan koma)
  // Perhatikan bahwa untuk waktu aktif, kita gunakan prefix "Active:" bukan "MAC:" kedua kalinya.
  char reply[255];
  sprintf(reply, "Device:%s, SSID:%s, Password:%s, IP:%s, MAC:%s, Active:%s", 
          deviceName, wifiSSIDClient, wifiPasswordClient, ipStr, myMAC, uptimeStr);
  
  Serial.print("Mengirim respon: ");
  Serial.println(reply);
  
  // Kirim respons ke alamat pengirim (master)
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.write(reply);
  Udp.endPacket();
} 

// Fungsi untuk menerima pesan DISCOVER
void receiveDiscovery() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';
    }
    Serial.print("Pesan diterima: ");
    Serial.println(packetBuffer);
    
    // Jika pesan yang diterima adalah "DISCOVER", maka kirim respons
    if (strcmp(packetBuffer, "DISCOVER") == 0) {
      delay(random(0, 200)); // Delay acak untuk menghindari tabrakan respon jika banyak device
      sendResponse();
    }
  }
}

void initUdp() { 
  // Mulai Udp pada port yang telah ditentukan
  Udp.begin(localUdpPort);
  randomSeed(analogRead(0)); // Untuk delay acak
  Serial.print("Client berjalan dengan IP: "); 
  Tulis("initUdp ", "Tengah");

  delay(1000); 
  }

String getDirectoryListing() {
  String listing = "";
  File root = SD.open("/CB100/");
  if (!root) {
    Serial.println("Gagal membuka direktori root.");
    return listing;
  }
  
  File entry = root.openNextFile();
  while (entry) {
    listing += entry.name();
    listing += ",";
    listing += String(entry.size());
    listing += ";";
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
  return listing;
}

// Fungsi untuk menulis data ke SD card secara langsung (satu sektor penuh per iterasi)
bool writeSectors(uint8_t *src, size_t start_sector, size_t sector_count) {
  digitalWrite(GPIO_NUM_2, HIGH);  // Gunakan GPIO_NUM_2 (misalnya LED) sebagai indikator
  bool res = true;
  for (size_t i = 0; i < sector_count; i++) {
    // Pastikan fungsi SD.writeRAW() tersedia pada library SD yang Anda gunakan!
    res = SD.writeRAW(src, start_sector + i);
    if (!res) {
      break;
    }
    src += m_sector_size;
  }
  digitalWrite(GPIO_NUM_2, LOW);
  return res;
}

// Fungsi untuk membaca data secara langsung dari SD card
bool readSectors(uint8_t *dst, size_t start_sector, size_t sector_count) {
  digitalWrite(GPIO_NUM_2, HIGH);
  bool res = true;
  for (size_t i = 0; i < sector_count; i++) {
    // Pastikan fungsi SD.readRAW() tersedia pada library SD yang Anda gunakan!
    res = SD.readRAW(dst, start_sector + i);
    if (!res) {
      break;
    }
    dst += m_sector_size;
  }
  digitalWrite(GPIO_NUM_2, LOW);
  return res;
}

// Callback untuk operasi tulis (write)
static int32_t onWrite(uint32_t lba, uint32_t offset, unsigned char* buffer, uint32_t bufsize) {
  if (writeSectors(buffer, lba, bufsize / m_sector_size)) {
    return bufsize;  // Berhasil menulis seluruh data
  }
  return -1;         // Gagal menulis
}

// Callback untuk operasi baca (read)
static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  if (readSectors((uint8_t*)buffer, lba, bufsize / m_sector_size)) {
    return bufsize;  // Berhasil membaca seluruh data
  }
  return -1;         // Gagal membaca
}

// Callback untuk start/stop media (misalnya, untuk load/eject)
static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  Serial.printf("StartStop: power_condition=%d, start=%d, load_eject=%d\n", power_condition, start, load_eject);
  if (load_eject) {
    msc.end();  // Matikan USB MSC jika media dilepas (eject)
  }
  return true;
}

static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == ARDUINO_USB_EVENTS) {
    switch(event_id) {
      case ARDUINO_USB_STARTED_EVENT: Serial.println("USB PLUGGED"); break;
      case ARDUINO_USB_STOPPED_EVENT: Serial.println("USB UNPLUGGED"); break;
      case ARDUINO_USB_SUSPEND_EVENT: Serial.println("USB SUSPENDED"); break;
      case ARDUINO_USB_RESUME_EVENT:  Serial.println("USB RESUMED"); break;
      default: break;
    }
  }
} 

void stopUSBMSC(){ 
  msc.mediaPresent(false);
  delay(1000); 
  msc.end(); 
  Serial.println("USB OFF");
}

void startUSBMSC(){
  // Hitung jumlah sektor dari SD card
  uint64_t totalBytes = SD.totalBytes();
  m_sector_count = totalBytes / m_sector_size;  

  // Konfigurasi identitas dan callback USB MSC
  msc.vendorID("ESP32");            // Identitas vendor
  msc.productID("USB_MSC");           // Identitas produk
  msc.productRevision("1.0");         // Revisi produk
  msc.onRead(onRead);                 // Daftarkan callback baca
  msc.onWrite(onWrite);               // Daftarkan callback tulis
  msc.onStartStop(onStartStop);       // Daftarkan callback start/stop
  msc.mediaPresent(true);             // Tandai bahwa media (SD card) tersedia
  msc.isWritable(true); 
  // Mulai USB MSC dengan kapasitas berdasarkan jumlah sektor dan ukuran sektor
  msc.begin(m_sector_count, m_sector_size);
  Serial.println("USB ON");
}  

// Fungsi untuk menampilkan teks di OLED
void Tulis(String teks, String posisi) {
  int y; // Variabel untuk menyimpan posisi Y
  bool hapusSaja = teks == ""; // Jika teks kosong, hanya hapus bagian tersebut

  if (posisi == "Atas") {
    y = 1;
    display.fillRect(0, 0, SCREEN_WIDTH, 10, SSD1306_BLACK);
  } 
  else if (posisi == "Tengah") {
    y = 12;
    display.fillRect(0, 11, SCREEN_WIDTH, 10, SSD1306_BLACK);
  } 
  else if (posisi == "Bawah") {
    y = 23;
    display.fillRect(0, 22, SCREEN_WIDTH, 10, SSD1306_BLACK);
  } 
  else {
    return; // Jika posisi tidak valid, keluar dari fungsi
  }

  if (!hapusSaja) { // Jika tidak hanya hapus, tulis teksnya
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(1, y);
    display.println(teks);
  }
  
  display.display(); 
}

void HapusTulis() {
  display.clearDisplay(); 
  display.display();
}


void initWiFiClient(){
  // Koneksi WiFi 
  Tulis("init WiFi Client", "Atas");  
  Tulis((String("SSID ") + WIFI_SSID), "Tengah");
  Tulis((String("PASS ") + WIFI_PASSWORD), "Bawah");   
  delay(2000);
  HapusTulis();

  WiFi.config(ip, gateway, subnet, dns);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { 
    Tulis("Connecting", "Tengah");
    delay(1000);
  }
  Tulis(" ", "Tengah");
  Tulis("Connected ", "Tengah");  
  ipStr = WiFi.localIP().toString(); 
  delay(1000);
}

void initWiFiAP(){ 
  Tulis("init WiFi AP", "Atas");  
  Tulis((String("S Device") + DEVICE_NAME), "Tengah");
  Tulis((String("P Device") + DEVICE_NAME), "Bawah");   
  delay(2000);  
  
  WiFi.softAP((String("Device") + DEVICE_NAME).c_str(), (String("Device") + DEVICE_NAME).c_str()); 
  // WiFi.softAP(DEVICE_NAME.c_str(), DEVICE_NAME.c_str());  
  ipStr = WiFi.softAPIP().toString();   
}

void initETH(){ 
  Tulis("init Ethernet", "Tengah");   
  delay(2000);
  Tulis(" ", "Tengah"); 
 
  SPI.begin(12, 13, 11, W5500_CS_PIN);
  Ethernet.init(W5500_CS_PIN); 
  // Ethernet.begin(mac); 
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  IPAddress ipku = Ethernet.localIP();     
  ipStr = ipku.toString();   

  initUdp();
} 

// Fungsi untuk memperbarui nilai mac dan ip
void updateMacAndIp(const String &currentMacStr, const String &currentIpStr) {
  // Perbarui MAC Address
  byte newMac[6];
  int macIndex = 0;
  int start = 0;
  for (int i = 0; i <= currentMacStr.length() && macIndex < 6; i++) {
    if (i == currentMacStr.length() || currentMacStr.charAt(i) == ':') {
      String token = currentMacStr.substring(start, i);
      newMac[macIndex] = (byte) strtol(token.c_str(), NULL, 16);
      macIndex++;
      start = i + 1;
    }
  }
  // Salin nilai baru ke array global mac
  memcpy(mac, newMac, sizeof(newMac));

  // Perbarui IP Address
  int ipParts[4];
  sscanf(currentIpStr.c_str(), "%d.%d.%d.%d", &ipParts[0], &ipParts[1], &ipParts[2], &ipParts[3]);
  ip = IPAddress(ipParts[0], ipParts[1], ipParts[2], ipParts[3]);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); 
  }

  pinMode(myWIFI, INPUT_PULLDOWN);
  pinMode(myETHERNET, INPUT_PULLDOWN);

  ambilDataEEPROM();
  
  USB.begin(); 
   
  // Inisialisasi SD Card menggunakan SPI2
  SPI2.begin(36, 37, 35, SD_CS);
  if (!SD.begin(SD_CS, SPI2, 200000000)) {
    Serial.println("Inisialisasi SD Card gagal!"); 
  } else {
    Serial.println("SD Card berhasil diinisialisasi.");
  }   

  // Menampilkan tipe kartu
  uint8_t cardType = SD.cardType(); 
  if (cardType == CARD_NONE) { 
    CARDTYPE = "NO SD CARD";
  } else if (cardType == CARD_MMC) { 
    CARDTYPE = "MMC";
  } else if (cardType == CARD_SD) { 
    CARDTYPE = "SDSC";
  } else if (cardType == CARD_SDHC) { 
    CARDTYPE = "SDHC/SDXC";
  } 

  // Inisialisasi USB MSC
  stopUSBMSC();
  delay(1000);
  startUSBMSC(); 

  WIFI = digitalRead(myWIFI);
  ETHERNET = digitalRead(myETHERNET);
}

void loop() { 
  if (flagSetup == 1) {  
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);   
    HapusTulis();
    delay(200);
    Tulis("Initialize", "Tengah");
    delay(100);
    Tulis("Initialize.", "Tengah");
    delay(100);
    Tulis("Initialize..", "Tengah");
    delay(100);
    Tulis("Initialize...", "Tengah");
    delay(100);
    Tulis("Initialize....", "Tengah");
    delay(100);
    Tulis("Initialize.....", "Tengah");
    delay(100);
    Tulis("Initialize......", "Tengah");
    delay(100);
    Tulis("Initialize.......", "Tengah");
    delay(100); 

    if(WIFI == 1 && ETHERNET == 0)
      {   
        initWiFiClient();
        initWebServer(); 
        Tulis("WiFi Client Mode", "Atas");
        Tulis((String("IP ") + ipStr.c_str()), "Tengah");    
      } 
    else if(WIFI == 0 && ETHERNET == 1) 
      { 
        initETH();
        initWebServer(); 
        Tulis("Ethernet Mode", "Atas");
        Tulis((String("IP ") + ipStr.c_str()), "Tengah");   
      } 
    else if(WIFI == 1 && ETHERNET == 1)
      {
        initWiFiAP();
        initWebServer(); 
        Tulis("Configuration Mode", "Atas");  
        Tulis((String("S/P Device") + DEVICE_NAME), "Tengah");
        Tulis((String("IP ") + ipStr.c_str()), "Bawah");    
      }
    else if(WIFI == 0 && ETHERNET == 0) 
      { 
        initWiFiAP();
        initWebServer(); 
        Tulis("WiFi AP Mode", "Atas");  
        Tulis((String("S/P Device") + DEVICE_NAME), "Tengah");
        Tulis((String("IP ") + ipStr.c_str()), "Bawah");    
        // Tulis("Only Flashdisk", "Tengah"); 
      }  
    flagSetup = 0;
  } 
  else 
  {
    handleClientConnections();
    receiveDiscovery();
  }
}
