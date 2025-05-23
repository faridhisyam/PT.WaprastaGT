#include <WiFi.h>
#include <ETH.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <string.h>

// Struktur untuk menyimpan informasi device
struct DeviceInfo {
  char deviceName[32];
  char wifiSSIDClient[32];
  char wifiPasswordClient[32];
  char ipStr[16];
  char macAddress[18];       // MAC address
  unsigned long baseActive;  // waktu aktif awal (detik) dari client
  unsigned long discoveryTime; // waktu (millis) saat pertama respons diterima
  unsigned long lastSeen;      // waktu (millis) saat terakhir respons diterima
};

const int MAX_DEVICES = 16;
DeviceInfo devices[MAX_DEVICES];
int deviceCount = 0;

// Konfigurasi Ethernet Master (static IP)
byte macMaster[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02 };
IPAddress ipMaster(192, 168, 1, 100);  // IP statis master
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

IPAddress broadcastIP(192, 168, 1, 255); // Broadcast address untuk UDP
unsigned int localUdpPort = 8888;          // Port UDP untuk discovery

WiFiUDP Udp;
char packetBuffer[255]; // Buffer untuk menerima data UDP

// Web server pada port 80
WiFiServer server(80);

// Waktu reset webserver (1 menit)
unsigned long lastServerReset = 0;

// Fungsi mengirim DISCOVER via UDP
void sendDiscovery() {
  Serial.println("Mengirim DISCOVER ke semua device...");
  Udp.beginPacket(broadcastIP, localUdpPort);
  Udp.print("DISCOVER");
  Udp.endPacket();
}

// Fungsi mencari index device berdasarkan IP
int findDeviceIndexByIP(const char* ip) {
  for (int i = 0; i < deviceCount; i++) {
    if (strcmp(devices[i].ipStr, ip) == 0)
      return i;
  }
  return -1;
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
    else if (strncmp(token, "MAC:", 4) == 0) {
      strcpy(dev.macAddress, token + 4);
    }
    else if (strncmp(token, "Active:", 7) == 0) {
      // Parsing waktu aktif yang dikirim dalam format HH:MM:SS
      unsigned long h = 0, m = 0, s = 0;
      if (sscanf(token + 7, "%lu:%lu:%lu", &h, &m, &s) == 3) {
        dev.baseActive = h * 3600 + m * 60 + s;
      }
      else {
        // Jika parsing gagal, fallback konversi langsung (dalam detik)
        dev.baseActive = strtoul(token + 7, NULL, 10);
      }
    }
    token = strtok(NULL, ",");
  }
  // Set waktu discovery dan lastSeen ke waktu sekarang
  dev.discoveryTime = millis();
  dev.lastSeen = millis();
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

// Fungsi untuk menghapus device yang sudah tidak terhubung (tidak respons selama >30 detik)
void removeDisconnectedDevices() {
  const unsigned long disconnectThreshold = 20000; // 30 detik
  for (int i = 0; i < deviceCount; ) {
    if (millis() - devices[i].lastSeen > disconnectThreshold) {
      Serial.print("Menghapus device dengan IP ");
      Serial.println(devices[i].ipStr);
      // Geser elemen array untuk menghapus device
      for (int j = i; j < deviceCount - 1; j++) {
        devices[j] = devices[j + 1];
      }
      deviceCount--;
    } else {
      i++;
    }
  }
}

// Fungsi untuk memproses respon UDP dan menyimpan data ke array devices
void processResponse(char* response) {
  Serial.println("Memproses respons:");
  Serial.println(response);
  
  // Abaikan respon yang hanya "DISCOVER" atau tidak memiliki "Device:"
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
    int index = findDeviceIndexByIP(dev.ipStr);
    if (index >= 0) {
      // Jika sudah ada, perbarui lastSeen
      devices[index].lastSeen = millis();
      Serial.print("Update lastSeen untuk device IP ");
      Serial.println(dev.ipStr);
    }
    else {
      if (deviceCount < MAX_DEVICES) {
        devices[deviceCount++] = dev;
        Serial.print("Device disimpan di index ");
        Serial.println(deviceCount - 1);
      }
      else {
        Serial.println("Data device sudah penuh.");
      }
    }
  }
  
  // Tampilkan daftar device untuk debug
  Serial.println("Daftar device:");
  for (int i = 0; i < deviceCount; i++) {
    unsigned long currentActive = devices[i].baseActive + (millis() - devices[i].discoveryTime) / 1000;
    Serial.print(i);
    Serial.print(": Device: ");
    Serial.print(devices[i].deviceName);
    Serial.print(" | SSID: ");
    Serial.print(devices[i].wifiSSIDClient);
    Serial.print(" | Password: ");
    Serial.print(devices[i].wifiPasswordClient);
    Serial.print(" | IP: ");
    Serial.print(devices[i].ipStr);
    Serial.print(" | MAC: ");
    Serial.print(devices[i].macAddress);
    Serial.print(" | Active: ");
    Serial.println(currentActive);
  }
}

// Fungsi untuk mengirim halaman HTML ke web client
void sendHtmlPage(WiFiClient &client) {
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
  // Menambahkan auto-refresh (misalnya setiap 30 detik)
  client.println("<meta http-equiv='refresh' content='20'>");
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
  client.println("<header><h1>Device Aktif</h1></header>");
  client.println("<div class='container'>");
  
  // Buat kotak untuk setiap device yang tersimpan
  for (int i = 0; i < deviceCount; i++) {
    unsigned long currentActive = devices[i].baseActive + (millis() - devices[i].discoveryTime) / 1000;
    // Format waktu aktif ke format HH:MM:SS
    unsigned long seconds = currentActive % 60;
    unsigned long minutes = (currentActive / 60) % 60;
    unsigned long hours = (currentActive / 3600);
    char formattedTime[10];
    sprintf(formattedTime, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    
    client.println("<div class='device-box'>");
    client.print("<h2>");
    client.print(devices[i].deviceName);
    client.println("</h2>");
    client.print("<p><strong>SSID :</strong> ");
    client.print(devices[i].wifiSSIDClient);
    client.println("</p>");
    client.print("<p><strong>PASS :</strong> ");
    client.print(devices[i].wifiPasswordClient);
    client.println("</p>");
    client.print("<p><strong>IP :</strong> ");
    client.print(devices[i].ipStr);
    client.println("</p>");
    client.print("<p><strong>MAC :</strong> ");
    client.print(devices[i].macAddress);
    client.println("</p>");
    
    // Setiap device diberi elemen <span> dengan class 'activeTime' dan atribut data-uptime
    client.print("<p><strong>Waktu Aktif :</strong> <span class='activeTime' data-uptime='");
    client.print(currentActive);
    client.print("'>");
    client.print(formattedTime);
    client.println("</span></p>");
    
    client.print("<a class='button' href='http://");
    client.print(devices[i].ipStr);
    client.println("' target='_blank'>Buka</a>");
    client.println("</div>");
  }
  
  client.println("</div>");  // Tutup container
  
  // JavaScript untuk update waktu aktif setiap detik untuk semua device
  client.println("<script>");
  client.println("setInterval(function() {");
  client.println("  var elements = document.getElementsByClassName('activeTime');");
  client.println("  for (var i = 0; i < elements.length; i++) {");
  client.println("    var uptime = parseInt(elements[i].getAttribute('data-uptime'));");
  client.println("    uptime++;");
  client.println("    elements[i].setAttribute('data-uptime', uptime);");
  client.println("    var hrs = Math.floor(uptime / 3600);");
  client.println("    var mins = Math.floor((uptime % 3600) / 60);");
  client.println("    var secs = uptime % 60;");
  client.println("    var formatted = ('0' + hrs).slice(-2) + ':' + ('0' + mins).slice(-2) + ':' + ('0' + secs).slice(-2);");
  client.println("    elements[i].innerText = formatted;");
  client.println("  }");
  client.println("}, 1000);");
  client.println("</script>");
  
  client.println("</body>");
  client.println("</html>");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Memulai Ethernet...");

  // Konfigurasi static IP sebelum inisialisasi Ethernet
  WiFi.config(ipMaster, gateway, subnet, dns);

  // Inisialisasi built-in Ethernet (contoh: WT32-ETH01 dengan LAN8720)
  if (!ETH.begin(ETH_PHY_LAN8720, 1, 23, 18, -1, ETH_CLOCK_GPIO0_OUT)) {
    Serial.println("Inisialisasi Ethernet gagal");
    while (1) {
      delay(1000);
    }
  }
  
  // Polling untuk menunggu hingga mendapatkan IP
  Serial.println("Menunggu mendapatkan IP...");
  while (ETH.localIP() == IPAddress(0, 0, 0, 0)) {
    delay(100);
  }
  
  Serial.print("Master berjalan dengan IP: ");
  Serial.println(ETH.localIP());
  
  // Inisialisasi UDP dan web server
  Udp.begin(localUdpPort);
  server.begin();
  lastServerReset = millis();
  
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
  
  // Hapus device yang tidak aktif (tidak respons selama >30 detik)
  removeDisconnectedDevices();
  
  // --- Bagian Webserver ---
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Web client terhubung.");
    while (client.connected() && !client.available()) {
      delay(1);
    }
    while (client.available()) {
      client.read();
    }
    sendHtmlPage(client);
    delay(1);
    client.stop();
    Serial.println("Web client diputus.");
  }
  
  // // Reset web server setiap 1 menit
  // if (millis() - lastServerReset > 60000) {
  //   Serial.println("Mereset web server...");
  //   server.end();
  //   server.begin();
  //   lastServerReset = millis();
  // }
  
  delay(100);
}
