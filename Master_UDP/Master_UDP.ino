#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <string.h>

// Struktur untuk menyimpan informasi device
struct DeviceInfo {
  char deviceName[32];
  char wifiSSIDClient[32];
  char wifiPasswordClient[32];
  char ipStr[16];
};

const int MAX_DEVICES = 16;
DeviceInfo devices[MAX_DEVICES];
int deviceCount = 0;

// Konfigurasi Ethernet Master
byte macMaster[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02 };
IPAddress ipMaster(192, 168, 1, 100);       // IP statis master
IPAddress broadcastIP(192, 168, 1, 255);      // Broadcast address
unsigned int localUdpPort = 8888;             // Port UDP untuk discovery

EthernetUDP Udp;
char packetBuffer[255]; // Buffer untuk menerima data UDP

// Web server pada port 80
EthernetServer server(80);

// Fungsi mengirim DISCOVER via UDP
void sendDiscovery() {
  Serial.println("Mengirim DISCOVER ke semua device...");
  Udp.beginPacket(broadcastIP, localUdpPort);
  Udp.print("DISCOVER");
  Udp.endPacket();
}

// Fungsi mengecek apakah device dengan IP tertentu sudah tersimpan
bool isDeviceStored(const char* ip) {
  for (int i = 0; i < deviceCount; i++) {
    if (strcmp(devices[i].ipStr, ip) == 0)
      return true;
  }
  return false;
}

// Fungsi untuk memparsing string respons dan menyimpannya ke struktur DeviceInfo
bool parseResponse(const char* response, DeviceInfo &dev) {
  memset(&dev, 0, sizeof(dev));
  char buf[255];
  strcpy(buf, response);
  
  // Tokenisasi menggunakan tanda koma sebagai delimiter
  char* token = strtok(buf, ",");
  while (token != NULL) {
    // Hilangkan spasi awal jika ada
    while (*token == ' ') token++;
    
    if (strncmp(token, "Device:", 7) == 0) {
      strcpy(dev.deviceName, token + 7);
    }
    else if (strncmp(token, "SSID:", 5) == 0) {
      strcpy(dev.wifiSSIDClient, token + 5);
    }
    else if (strncmp(token, "Password:", 9) == 0) {
      strcpy(dev.wifiPasswordClient, token + 9);
    }
    else if (strncmp(token, "IP:", 3) == 0) {
      strcpy(dev.ipStr, token + 3);
    }
    token = strtok(NULL, ",");
  }
  return true;
}

// Fungsi untuk mengurutkan data device berdasarkan nama (deviceName) secara ascending
void sortDevices() {
  for (int i = 0; i < deviceCount - 1; i++) {
    for (int j = i + 1; j < deviceCount; j++) {
      if (strcmp(devices[i].deviceName, devices[j].deviceName) > 0) {
        DeviceInfo temp = devices[i];
        devices[i] = devices[j];
        devices[j] = temp;
      }
    }
  }
}

// Fungsi untuk memproses respon UDP dan menyimpan data ke array devices
void processResponse(char* response) {
  Serial.println("Memproses respons:");
  Serial.println(response);
  
  // Jika respon hanya "DISCOVER" atau tidak valid, abaikan
  if (strcmp(response, "DISCOVER") == 0 || strstr(response, "Device:") == NULL) {
    Serial.println("Respon tidak valid, abaikan.");
    return;
  }
  
  DeviceInfo dev;
  if (parseResponse(response, dev)) {
    if (strlen(dev.ipStr) == 0) {
      Serial.println("Respon tidak memiliki IP, abaikan.");
      return;
    }
    if (!isDeviceStored(dev.ipStr)) {
      if (deviceCount < MAX_DEVICES) {
        devices[deviceCount++] = dev;
        Serial.print("Device disimpan di index ");
        Serial.println(deviceCount - 1);
      }
      else {
        Serial.println("Data device sudah penuh.");
      }
    }
    else {
      Serial.print("Device dengan IP ");
      Serial.print(dev.ipStr);
      Serial.println(" sudah ada, tidak ditambahkan.");
    }
  }
  
  // Debug: Tampilkan daftar device yang tersimpan
  Serial.println("Daftar device:");
  for (int i = 0; i < deviceCount; i++) {
    Serial.print(i);
    Serial.print(": Device: ");
    Serial.print(devices[i].deviceName);
    Serial.print(" | SSID: ");
    Serial.print(devices[i].wifiSSIDClient);
    Serial.print(" | Password: ");
    Serial.print(devices[i].wifiPasswordClient);
    Serial.print(" | IP: ");
    Serial.println(devices[i].ipStr);
  }
}

// Fungsi untuk mengirim halaman HTML secara terpisah ke web client
void sendHtmlPage(EthernetClient &client) {
  // Urutkan data device terlebih dahulu
  sortDevices();
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  
  client.println("<!DOCTYPE html>");
  client.println("<html>");
  client.println("<head>");
  client.println("<meta charset='UTF-8'>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  client.println("<title>Data Device Tersimpan</title>");
  client.println("<style>");
  client.println("body { font-family: 'Roboto', sans-serif; background: #f4f7f9; margin: 0; padding: 20px; }");
  client.println("header { background: #2c3e50; color: #fff; padding: 20px; text-align: center; }");
  client.println(".container { display: grid; grid-template-columns: repeat(auto-fill, minmax(280px, 1fr)); gap: 20px; margin-top: 20px; }");
  client.println(".device-box { background: #fff; border-radius: 8px; padding: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); transition: transform 0.2s ease; text-align: center; }");
  client.println(".device-box:hover { transform: translateY(-5px); }");
  client.println(".device-box h2 { margin-top: 0; font-size: 20px; color: #34495e; }");
  client.println(".device-box p { margin: 10px 0; font-size: 14px; color: #555; }");
  client.println(".button { display: inline-block; margin-top: 10px; padding: 10px 15px; background: #2980b9; color: #fff; text-decoration: none; border-radius: 4px; font-weight: 500; transition: background 0.3s ease; }");
  client.println(".button:hover { background: #1c5980; }");
  client.println("@media (max-width: 480px) { .container { grid-template-columns: 1fr; } }");
  client.println("</style>");
  client.println("</head>");
  client.println("<body>");
  client.println("<header><h1>Data Device Tersimpan</h1></header>");
  client.println("<div class='container'>");
  
  // Buat kotak untuk setiap device yang tersimpan
  for (int i = 0; i < deviceCount; i++) {
    client.println("<div class='device-box'>");
    client.print("<h2>");
    client.print(devices[i].deviceName);
    client.println("</h2>");
    client.print("<p><strong>SSID:</strong> ");
    client.print(devices[i].wifiSSIDClient);
    client.println("</p>");
    client.print("<p><strong>Password:</strong> ");
    client.print(devices[i].wifiPasswordClient);
    client.println("</p>");
    client.print("<p><strong>IP:</strong> ");
    client.print(devices[i].ipStr);
    client.println("</p>");
    client.print("<a class='button' href='http://");
    client.print(devices[i].ipStr);
    client.println("' target='_blank'>Buka IP</a>");
    client.println("</div>");
  }
  
  client.println("</div>");  // Tutup container
  client.println("</body>");
  client.println("</html>");
}

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi SPI dan Ethernet
  SPI.begin(12, 13, 11, 10);   // Sesuaikan dengan wiring modul W5500
  Ethernet.init(10);           // CS pin untuk modul W5500
  Ethernet.begin(macMaster, ipMaster);
  
  Serial.print("Master berjalan dengan IP: ");
  Serial.println(Ethernet.localIP());
  
  Udp.begin(localUdpPort);
  server.begin();
  
  delay(1000);
  sendDiscovery();  // Kirim DISCOVER saat startup
}

void loop() {
  // --- Bagian UDP Discovery ---
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';
    }
    Serial.print("Respons UDP diterima: ");
    Serial.println(packetBuffer);
    processResponse(packetBuffer);
  }
  
  // Kirim ulang DISCOVER tiap 10 detik untuk mencari device baru
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 10000) {
    sendDiscovery();
    lastSend = millis();
  }
  
  // --- Bagian Webserver ---
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Web client terhubung.");
    // Tunggu hingga ada request
    while (client.connected() && !client.available()) {
      delay(1);
    }
    // Baca request (tidak diproses lebih lanjut)
    while (client.available()) {
      client.read();
    }
    
    // Kirim halaman HTML menggunakan fungsi terpisah
    sendHtmlPage(client);
    
    delay(1);
    client.stop();
    Serial.println("Web client diputus.");
  }
  
  delay(100);
}
