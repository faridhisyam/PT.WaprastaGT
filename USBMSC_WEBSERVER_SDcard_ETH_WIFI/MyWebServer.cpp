#include "MyWebServer.h" 
 
String WIFI_SSID = "";
String WIFI_PASSWORD = ""; 
String DEVICE_NAME = "";  
String currentMac = "";
String currentIp  = "";
String str;

uint64_t total, used, freeSpace;

WebServer server(80);
WiFiServer* wifiServer = nullptr;
EthernetServer* ethernetServer = nullptr;
Preferences preferences;

// Fungsi utilitas untuk mendecode URL-encoded string
String urldecode(String str) {
  String decoded = "";
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%' && i + 2 < str.length()) {
      String hex = str.substring(i + 1, i + 3);
      decoded += (char)strtol(hex.c_str(), NULL, 16);
      i += 2;
    } else {
      decoded += c;
    }
  }
  return decoded;
}

// Fungsi pembantu untuk membungkus konten HTML dengan template standar
String wrapHtml(String title, String bodyContent) {
  String html = "<!DOCTYPE html><html><head><title>" + title + "</title>";
  html += "<style>";
  // Menggunakan tema warna hijau muda dengan container yang lebih besar
  html += "body { background-color: #e8f5e9; font-family: Arial, sans-serif; margin: 20px; }";
  html += ".container { background-color: #ffffff; padding: 20px; border-radius: 8px; ";
  html += "box-shadow: 0 0 10px rgba(0,0,0,0.1); ";
  html += "width: 90%; max-width: auto; min-height: 3000px; margin: auto; ";
  html += "border-top: 4px solid #4CAF50; }";
  html += "h1 { color: #4CAF50; }";
  html += "a { color: #4CAF50; text-decoration: none; margin-right: 10px; }";
  html += "a:hover { text-decoration: underline; }";
  html += "input[type='file'], input[type='submit'] { padding: 10px; margin-top: 10px; margin-bottom: 10px; }";
  html += "label { font-weight: bold; }";
  html += "</style></head><body><div class='container'>";
  html += bodyContent;
  html += "</div></body></html>";
  return html;
}

String wrapHtmlhandleIndex(const String& title, const String& body) {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>" + title + "</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); color: #e0e0e0; font-family: 'Roboto', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }"
         ".container { background: #1e1e1e; padding: 40px; border-radius: 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.4); max-width: 600px; width: 100%; margin: auto; border-top: 8px solid #4CAF50; transition: transform 0.3s ease-in-out; }"
         ".container:hover { transform: scale(1.02); }"
         ".info-header { display: flex; justify-content: space-between; gap: 20px; margin-bottom: 20px; margin-top: 0px; }"
         ".info-item { flex: 1; display: flex; flex-direction: column; align-items: center; margin-top: 0px; }"
         ".info-label { font-size: 14px; color: #aaa; margin-bottom: 5px; }"
         ".info-box { background: #2e2e2e; padding: 8px 12px; border-radius: 10px; width: 100%; text-align: center; font-size: 16px; font-weight: bold; }"
         "@media (max-width: 600px) { .info-header { flex-direction: column; gap: 10px; } }"
         "h2 { color: #4CAF50; margin-top: 50px; margin-bottom: 30px; font-size: 24px; text-align: center; }"
         "form { display: flex; flex-direction: column; align-items: center; gap: 20px; margin-bottom: 30px; }"
         "input[type='file'] { display: none; }"
         ".custom-file-upload { display: block; width: 55%; padding: 12px 24px; border: 1px solid #4CAF50; border-radius: 25px; background-color: rgba(255,255,255,0.05); color: #e0e0e0; font-size: 16px; text-align: center; cursor: pointer; transition: background-color 0.3s ease, color 0.3s ease; margin: 0 auto; }"
         ".custom-file-upload:hover { background-color: #4CAF50; color: #121212; }"
         "input[type='submit'] { display: block; width: 55%; padding: 12px; border: 1px solid #4CAF50; border-radius: 25px; background-color: rgba(255,255,255,0.05); color: #e0e0e0; font-size: 16px; cursor: pointer; transition: background-color 0.3s ease, color 0.3s ease; margin: 0 auto; }"
         "input[type='submit']:hover { background-color: #4CAF50; color: #121212; }"
         ".file-info { text-align: center; font-size: 14px; color: #aaa; }"
         ".file-info span { display: block; margin-top: 5px; }"
         ".links { display: flex; justify-content: space-around; margin-top: 20px; }"
         ".links a { text-decoration: none; color: #4CAF50; font-size: 16px; transition: color 0.3s ease; }"
         ".links a:hover { color: #66BB6A; }"
         "@media (max-width: 600px) { .container { padding: 20px; } h2 { font-size: 18px; } input[type='submit'] { width: 100%; } .links { flex-direction: column; gap: 10px; } }"
         "</style>"
         "<script>"
         "document.addEventListener('DOMContentLoaded', function() {"
         "  const fileInput = document.querySelector('input[type=file]');"
         "  const fileInfo = document.querySelector('.file-info');"
         "  fileInput.addEventListener('change', function() {"
         "    const files = fileInput.files;"
         "    let fileNames = '';"
         "    for (let i = 0; i < files.length; i++) {"
         "      fileNames += files[i].name + '<br>';"
         "    }"
         "    fileInfo.innerHTML = 'Selected files (' + files.length + '):<br>' + fileNames;"
         "  });"
         "});"
         "</script>"
         "</head>"
         "<body>"
         "<div class='container'>" + body + "</div>"
         "</body>"
         "</html>";
}

String wrapHtmlhandleList(const String& title, const String& body) {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>" + title + "</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); color: #e0e0e0; font-family: 'Roboto', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }"
         ".container { background: #1e1e1e; padding: 40px; border-radius: 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.4); max-width: 600px; width: 100%; margin: auto; border-top: 8px solid #4CAF50; transition: transform 0.3s ease-in-out; }"
         ".container:hover { transform: scale(1.02); }"
         "h1 { color: #4CAF50; margin-bottom: 20px; font-size: 24px; text-align: center; }"
         ".file-list-header { display: flex; justify-content: space-between; align-items: center; font-weight: bold; background: rgba(255,255,255,0.05); padding: 10px; border-radius: 10px; margin-bottom: 10px; font-size: 16px; }"
         ".header-name { flex: 2; }"
         ".header-size { flex: 1; text-align: center; }"
         ".header-opsi { flex: 1; text-align: right; }"
         ".file-item { display: flex; justify-content: space-between; align-items: center; padding: 10px; border-bottom: 1px solid #4CAF50; font-size: 14px; }"
         ".file-name { flex: 2; }"
         ".file-size { flex: 1; text-align: center; }"
         ".file-opsi { flex: 1; text-align: right; }"
         ".file-item:last-child { border-bottom: none; }"
         ".delete-button { padding: 5px 10px; background-color: #4CAF50; color: #ffffff; border: none; border-radius: 5px; cursor: pointer; text-decoration: none; font-size: 14px; transition: background-color 0.3s, color 0.3s; }"
         ".delete-button:hover { background-color: #ffffff; color: #4CAF50; }"
         ".file-info { text-align: right; margin-top: 15px; font-size: 16px; color: #4CAF50; }"
         ".links { display: flex; justify-content: space-around; margin-top: 20px; }"
         ".links a { text-decoration: none; color: #4CAF50; font-size: 16px; transition: color 0.3s ease; }"
         ".links a:hover { color: #66BB6A; }"
         "@media (max-width: 600px) { .container { padding: 20px; } h1 { font-size: 18px; } .file-list-header, .file-item { flex-direction: column; align-items: flex-start; } .header-size, .header-opsi, .file-size, .file-opsi { text-align: left; width: 100%; margin-top: 5px; } .links { flex-direction: column; gap: 10px; } }"
         "</style>"
         "</head>"
         "<body>"
         "<div class='container'>"
         "<h1>" + title + "</h1>"
         "<div class='file-list-header'>"
         "<div class='header-name'>Nama File</div>"
         "<div class='header-size'>Ukuran</div>"
         "<div class='header-opsi'>Opsi</div>"
         "</div>"
         + body +
         "</div>"
         "</body>"
         "</html>";
}

String wrapHtmlhandleDelete(const String& title, const String& body) {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>" + title + "</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); color: #e0e0e0; font-family: 'Roboto', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }"
         ".container { background: #1e1e1e; padding: 40px; border-radius: 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.4); max-width: 600px; width: 100%; margin: auto; border-top: 8px solid #4CAF50; transition: transform 0.3s ease-in-out; text-align: center; }"
         ".container:hover { transform: scale(1.02); }"
         "h1 { color: #4CAF50; margin-bottom: 20px; font-size: 24px; }"
         "a { text-decoration: none; color: #4CAF50; font-size: 16px; transition: color 0.3s ease; }"
         "a:hover { color: #66BB6A; }"
         "@media (max-width: 600px) { .container { padding: 20px; } h1 { font-size: 20px; } a { font-size: 14px; } }"
         "</style>"
         "</head>"
         "<body>"
         "<div class='container'>"
         "<h1>" + title + "</h1>"
         + body +
         "</div>"
         "</body>"
         "</html>";
}

String wrapHtmlhandleUpload(const String& title, const String& body) {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>" + title + "</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); color: #e0e0e0; font-family: 'Roboto', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }"
         ".container { background: #1e1e1e; padding: 40px; border-radius: 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.4); max-width: 600px; width: 100%; margin: auto; border-top: 8px solid #4CAF50; transition: transform 0.3s ease-in-out; text-align: center; }"
         ".container:hover { transform: scale(1.02); }"
         "h1 { color: #4CAF50; margin-bottom: 20px; font-size: 24px; }"
         ".links { display: flex; justify-content: space-around; margin-top: 20px; }"
         ".links a { text-decoration: none; color: #4CAF50; font-size: 16px; transition: color 0.3s ease; }"
         ".links a:hover { color: #66BB6A; }"
         "@media (max-width: 600px) { .container { padding: 20px; } h1 { font-size: 20px; } a { font-size: 14px; } }"
         "</style>"
         "</head>"
         "<body>"
         "<div class='container'>"
         "<h1>" + title + "</h1>"
         + body +
         "</div>"
         "</body>"
         "</html>";
}

String wrapHtmlhandleDeleteAll(const String& title, const String& body) {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>" + title + "</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); color: #e0e0e0; font-family: 'Roboto', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }"
         ".container { background: #1e1e1e; padding: 40px; border-radius: 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.4); max-width: 600px; width: 100%; margin: auto; border-top: 8px solid #4CAF50; transition: transform 0.3s ease-in-out; text-align: center; }"
         ".container:hover { transform: scale(1.02); }"
         "h1 { color: #4CAF50; margin-bottom: 20px; font-size: 24px; }"
         "a { text-decoration: none; color: #4CAF50; font-size: 16px; transition: color 0.3s ease; }"
         "a:hover { color: #66BB6A; }"
         "@media (max-width: 600px) { .container { padding: 20px; } h1 { font-size: 20px; } a { font-size: 14px; } }"
         "</style>"
         "</head>"
         "<body>"
         "<div class='container'>"
         "<h1>" + title + "</h1>"
         + body +
         "</div>"
         "</body>"
         "</html>";
}

String wrapHtmlhandleInfo(const String& title, const String& body) {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>" + title + "</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); font-family: 'Roboto', sans-serif; display: flex; align-items: center; justify-content: center; min-height: 100vh; padding: 20px; }"
         ".container { background: #1e1e1e; border-radius: 20px; box-shadow: 0 4px 18px rgba(0, 0, 0, 0.4); max-width: 600px; width: 100%; padding: 30px; border-top: 8px solid #4CAF50; transition: transform 0.3s ease; }"
         ".container:hover { transform: scale(1.02); }"
         "h1 { color: #4CAF50; margin-bottom: 20px; font-size: 24px; text-align: center; }"
         ".info-container { display: flex; flex-wrap: wrap; justify-content: center; align-items: center; gap: 20px; }"
         ".circular-progress { position: relative; width: 180px; height: 180px; }"
         ".circular-progress svg { width: 100%; height: 100%; transform: rotate(-90deg); }"
         ".circular-progress circle { fill: none; stroke-width: 20; stroke-linecap: round; }"
         ".circular-progress .free { stroke: #FFEB3B; transition: stroke-dasharray 0.5s, stroke-dashoffset 0.5s; }"
         ".circular-progress .used { stroke: #2196F3; transition: stroke-dasharray 0.5s; }"
         ".circular-progress .text { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); text-align: center; color: #e0e0e0; }"
         ".circular-progress .text div { font-size: 18px; font-weight: 500; }"
         ".info-details { display: flex; flex-direction: column; gap: 10px; min-width: 200px; }"
         ".info { display: flex; justify-content: space-between; align-items: center; font-size: 16px; padding: 8px 12px; background: rgba(255, 255, 255, 0.05); border-radius: 6px; }"
         ".info span:first-child { flex: 1; color: #aaa; }"
         ".info .value { font-weight: bold; color: #e0e0e0; display: flex; align-items: center; gap: 8px; }"
         ".legend-box { width: 12px; height: 12px; border-radius: 3px; }"
         ".legend-box.used-box { background-color: #2196F3; }"
         ".legend-box.free-box { background-color: #FFEB3B; }"
         ".back-btn { display: inline-block; text-align: left; margin-top: 20px; text-decoration: none; color: #4CAF50; font-size: 16px; transition: color 0.3s ease; }"
         ".back-btn:hover { color: #66BB6A; }"
         "@media (max-width: 600px) { .info-container { flex-direction: column; } .container { padding: 20px; } h1 { font-size: 20px; } .info { font-size: 14px; } }"
         "</style>"
         "</head>"
         "<body>"
         "<div class='container'>"
         "<h1>" + title + "</h1>"
         "<div class='info-container'>" + body + "</div>"
         "<a href='/' class='back-btn'>Kembali</a>"
         "</div>"
         "</body>"
         "</html>";
}

void sendHttpResponse(Client &client, const char *content) {
  size_t len = strlen(content);
  size_t chunkSize = 1024; // Gunakan buffer lebih besar untuk efisiensi
  
  for (size_t i = 0; i < len; i += chunkSize) {
    size_t currentChunk = (len - i < chunkSize) ? (len - i) : chunkSize;
    client.write(reinterpret_cast<const uint8_t*>(content + i), currentChunk);
  }
  client.stop();
}

// void sendHttpResponse(Client &client, const char *content) {
//   size_t len = strlen(content);
//   size_t chunkSize = 1024; // jumlah byte per chunk
//   for (size_t i = 0; i < len; i += chunkSize) {
//     size_t currentChunk = (len - i < chunkSize) ? (len - i) : chunkSize;
//     client.write(reinterpret_cast<const uint8_t*>(content + i), currentChunk);
//     client.flush();
//     // delay(1); // atur delay agar data tidak dikirim terlalu cepat
//   }
//   delay(10); // delay tambahan sebelum stop
//   client.stop();
// }

// -----------------------
// Fungsi Handler Halaman
// -----------------------

// Halaman Index
// Parameter 'mode' menunjukkan koneksi (misalnya "WiFi AP Mode" atau "Ethernet Mode")
void handleIndex(Client &client, String mode) {
  unsigned long uptimeMillis = millis();
  unsigned long uptimeSeconds = uptimeMillis / 1000;  // Konversi ke detik

  // Konversi waktu ke format HH:MM:SS untuk tampilan awal
  unsigned long seconds = uptimeSeconds % 60;
  unsigned long minutes = (uptimeSeconds / 60) % 60;
  unsigned long hours = (uptimeSeconds / 3600);

  char uptimeStr[10];
  sprintf(uptimeStr, "%02lu:%02lu:%02lu", hours, minutes, seconds);

  String body = "";
  body += "<div class='info-header'>";
  body += "<div class='info-item'><div class='info-label'>Device</div><div class='info-box'>" + String(DEVICE_NAME) + "</div></div>";
  body += "<div class='info-item'><div class='info-label'>Mode</div><div class='info-box'>" + mode + "</div></div>";
  body += "<div class='info-item'><div class='info-label'>Waktu Aktif</div><div class='info-box' id='active-time'>" + String(uptimeStr) + "</div></div>";
  body += "</div>";
  body += "<h2>Tambahkan File ke SD Card</h2>";
  body += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  body += "<label for='file-upload' class='custom-file-upload'>Pilih File</label>";
  body += "<input type='file' name='upload[]' id='file-upload' multiple>";
  body += "<div class='file-info' id='file-info'><span id='file-names'>Belum ada file yang dipilih</span><span id='file-count'></span></div>";
  body += "<input type='submit' value='Upload'>";
  body += "</form>";
  body += "<div class='links'><a href='/list'>Daftar File</a><a href='/info'>Informasi</a></div>";
  
    // **Tambahkan JavaScript untuk update waktu aktif**
  body += "<script>";
  body += "var uptimeSeconds = " + String(uptimeSeconds) + ";";  // Konversi uptime ke angka
  body += "function updateUptime() {";
  body += "  uptimeSeconds++;";  // Tambahkan waktu setiap detik
  body += "  var hours = Math.floor(uptimeSeconds / 3600);";
  body += "  var minutes = Math.floor((uptimeSeconds % 3600) / 60);";
  body += "  var seconds = uptimeSeconds % 60;";
  body += "  var formattedTime = ('0' + hours).slice(-2) + ':' + ('0' + minutes).slice(-2) + ':' + ('0' + seconds).slice(-2);";
  body += "  document.getElementById('active-time').innerText = formattedTime;";
  body += "}";
  body += "setInterval(updateUptime, 1000);";  // Perbarui setiap 1 detik
  body += "</script>";
  
  String html = wrapHtmlhandleIndex("(" + DEVICE_NAME + ") " + mode + "", body);
  sendHttpResponse(client, html.c_str());
}

// ----------------------------------------------------------------
// Menampilkan daftar file di folder /CB100/ beserta link hapus
// ----------------------------------------------------------------
// Menampilkan daftar file di folder /CB100/
void handleList(Client &client) {
  String body = "";
  File root = SD.open("/CB100/");
  if (!root) {
    body += "<p>Gagal membuka direktori /CB100/.</p>";
  } else {
    int fileCount = 0;
    File file = root.openNextFile();
    if (!file) {
      body += "<p>Tidak ada file.</p>";
    }
    while (file) {
      fileCount++;
      String fname = file.name();  
      String displayName = fname;
      if (fname.startsWith("/CB100/"))
        displayName = fname.substring(7);
      body += "<div class='file-item'>";
      body += "<div class='file-name'>" + displayName + "</div>";
      body += "<div class='file-size'>" + String(file.size() / 1024.0, 2) + " KB</div>";
      body += "<div class='file-opsi'><a href='/delete?filename=" + displayName + "' class='delete-button'>Hapus</a></div>";
      body += "</div>";
      file = root.openNextFile();
    }
    body += "<div class='file-info'>Jumlah file : " + String(fileCount) + "</div>";
  }
  body += "<div class='links'> <a href='/'>Kembali</a> <a href='/deleteAll'>Hapus Semua File</a></div>";
  String html = wrapHtmlhandleList("Daftar File SD Card (" + DEVICE_NAME + ")", body);
  sendHttpResponse(client, html.c_str());
}

// ----------------------------------------------------------------
// Menampilkan informasi penyimpanan SD Card
// ----------------------------------------------------------------
// Menampilkan informasi SD Card
void handleInfo(Client &client) {
  total = SD.totalBytes();
  used = SD.usedBytes();
  freeSpace = total - used;

  String body = "";
  body += "<div class='circular-progress'><svg><circle class='free' cx='90' cy='90' r='70'></circle><circle class='used' cx='90' cy='90' r='70'></circle></svg><div class='text'><div id='used-percentage'>" + String((used * 100.0) / total, 3) + "%</div><div>Used</div></div></div>";
  body += "<div class='info-details'>";
  body += "<div class='info'><span>Type</span><span class='value'>" + String(CARDTYPE) + "</span></div>";
  body += "<div class='info'><span>System</span><span class='value'>FAT32</span></div>";
  body += "<div class='info'><span>Total</span><span id='total-size' class='value'>" + String(total / 1048576.00, 3) + " MB</span></div>";
  body += "<div class='info'><span>Used</span><span id='used-size' class='value'>" + String(used / 1048576.00, 3) + " MB</span></div>";
  body += "<div class='info'><span>Free</span><span id='free-size' class='value'>" + String(freeSpace / 1048576.00, 3) + " MB</span></div>";
  body += "</div>";
  
  String html = wrapHtmlhandleInfo("Informasi SD Card (" + DEVICE_NAME + ")", body);
  sendHttpResponse(client, html.c_str());
}


// ----------------------------------------------------------------
// Menghapus satu file berdasarkan parameter filename dari query string
// Contoh request: GET /delete?filename=test.txt HTTP/1.1
// ----------------------------------------------------------------
// Menghapus satu file berdasarkan query parameter (contoh: /delete?filename=test.txt)
void handleDelete(Client &client, String requestLine) {
  int idx = requestLine.indexOf("?");
  if (idx == -1) {
    String html = wrapHtmlhandleDelete("Error", "<p>Parameter tidak ditemukan.</p>");
    sendHttpResponse(client, html.c_str());
    return;
  }
  String query = requestLine.substring(idx + 1);
  int endIdx = query.indexOf(" ");
  if (endIdx != -1) query = query.substring(0, endIdx);
  int pos = query.indexOf("filename=");
  if (pos == -1) {
    String html = wrapHtmlhandleDelete("Error", "<p>Filename tidak diberikan.</p>");
    sendHttpResponse(client, html.c_str());
    return;
  }
  String filename = query.substring(pos + 9);
  filename = urldecode(filename);
  if (!filename.startsWith("/CB100/")) filename = "/CB100/" + filename;
  
  if (SD.exists(filename)) {
    if (SD.remove(filename)) {
      String response = "<p>File " + filename + " berhasil dihapus.</p><br><a href='/list'>Kembali</a>";
      String html = wrapHtmlhandleDelete("Hapus File (" + DEVICE_NAME + ")", response);
      sendHttpResponse(client, html.c_str());
    } else {
      String response = "<p>Gagal menghapus file " + filename + ".</p><br><a href='/list'>Kembali</a>";
      String html = wrapHtmlhandleDelete("Hapus File (" + DEVICE_NAME + ")", response);
      sendHttpResponse(client, html.c_str());
    }
  } else {
    String response = "<p>File " + filename + " tidak ditemukan.</p><br><a href='/list'>Kembali</a>";
    String html = wrapHtmlhandleDelete("Hapus File (" + DEVICE_NAME + ")", response);
    sendHttpResponse(client, html.c_str());
  }
}

// ----------------------------------------------------------------
// Menghapus semua file di folder /CB100/ dengan membuka ulang direktori
// ----------------------------------------------------------------
// Menghapus semua file di folder /CB100/
void handleDeleteAll(Client &client) {
  String failedFiles = "";
  File root = SD.open("/CB100/");
  if (!root) {
    String html = wrapHtmlhandleDeleteAll("Error", "<p>Gagal membuka direktori /CB100/.</p>");
    sendHttpResponse(client, html.c_str());
    return;
  }
  
  String fileNames = "";
  while (true) {
    File file = root.openNextFile();
    if (!file) break;
    String fname = file.name();
    file.close();
    fileNames += fname + "\n";
  }
  root.close();
  
  int currentIndex = 0;
  while (currentIndex < fileNames.length()) {
    int nextIndex = fileNames.indexOf('\n', currentIndex);
    if (nextIndex == -1) break;
    String fname = fileNames.substring(currentIndex, nextIndex);
    currentIndex = nextIndex + 1;
    if (!fname.startsWith("/CB100/")) fname = "/CB100/" + fname;
    if (!SD.remove(fname)) {
      failedFiles += fname + " ";
    }
    delay(10);
  }
  
  String response;
  if (failedFiles.length() == 0)
    response = "<p>Semua file di CB100 berhasil dihapus.</p><br><a href='/list'>Kembali</a>";
  else
    response = "<p>Gagal menghapus file: " + failedFiles + "</p><br><a href='/list'>Kembali</a>";
    
  String html = wrapHtmlhandleDeleteAll("Hapus Semua File (" + DEVICE_NAME + ")", response);
  sendHttpResponse(client, html.c_str());
}

// ----------------------------------------------------------------
// Menangani upload file (multiple file) dengan ekstensi .txt
// Pendekatan: Baca seluruh body request ke buffer, kemudian parsing berdasarkan boundary
// ----------------------------------------------------------------
// Menangani upload file (.txt) via multipart/form-data
void handleUpload(Client &client, String requestLine) {
  stopUSBMSC();
  
  // Baca header HTTP untuk Content-Length dan boundary
  String header;
  int contentLength = 0;
  String boundary = "";
  while (client.available()) {
    header = client.readStringUntil('\n');
    header.trim();
    if (header.length() == 0) break;
    if (header.startsWith("Content-Type:")) {
      int bPos = header.indexOf("boundary=");
      if (bPos != -1) {
        boundary = "--" + header.substring(bPos + 9);
        boundary.trim();
      }
    } else if (header.startsWith("Content-Length:")) {
      contentLength = header.substring(16).toInt();
    }
  }
  
  if (boundary == "") {
    String html = wrapHtml("Error", "<p>Error: Boundary tidak ditemukan.</p>");
    sendHttpResponse(client, html.c_str());
    return;
  }
  
  Serial.print("Content-Length: ");
  Serial.println(contentLength);
  Serial.print("Boundary: ");
  Serial.println(boundary);
  
  char *bodyBuffer = new char[contentLength + 1];
  int br = client.readBytes(bodyBuffer, contentLength);
  bodyBuffer[contentLength] = '\0';
  String body = String(bodyBuffer);
  delete[] bodyBuffer;
  
  Serial.print("Body length: ");
  Serial.println(body.length());
  
  int currentPos = 0;
  while (true) {
    int partStart = body.indexOf(boundary, currentPos);
    if (partStart == -1) break;
    int partEnd = body.indexOf(boundary, partStart + boundary.length());
    if (partEnd == -1) break;
    
    String part = body.substring(partStart + boundary.length(), partEnd);
    part.trim();
    if (part == "--") break;
    
    int headerEnd = part.indexOf("\r\n\r\n");
    if (headerEnd == -1) {
      currentPos = partEnd;
      continue;
    }
    String partHeaders = part.substring(0, headerEnd);
    String partContent = part.substring(headerEnd + 4);
    
    int fnPos = partHeaders.indexOf("filename=");
    if (fnPos != -1) {
      int firstQuote = partHeaders.indexOf('"', fnPos);
      int secondQuote = partHeaders.indexOf('"', firstQuote + 1);
      if (firstQuote != -1 && secondQuote != -1) {
        String filename = partHeaders.substring(firstQuote + 1, secondQuote);
        filename.trim();
        if (filename.endsWith(".txt") && filename.length() > 0) {
          String filePath = "/CB100/" + filename;
          File file = SD.open(filePath, FILE_WRITE);
          if (file) {
            file.print(partContent);
            file.close();
            Serial.println("Uploaded: " + filename);
          } else {
            Serial.println("Gagal membuka file: " + filename);
          }
        } else {
          Serial.println("File bukan .txt atau nama kosong: " + filename);
        }
      }
    }
    currentPos = partEnd;
  }
  
  String responseContent = "<p>Upload selesai.</p><br><div class='links'><a href='/list'>Daftar File</a><br><a href='/'>Kembali</a></div>";
  String html = wrapHtmlhandleDeleteAll("Upload File (" + DEVICE_NAME + ")", responseContent);
  sendHttpResponse(client, html.c_str());
  startUSBMSC();
}

// -----------------------
// Fungsi untuk Memproses Request Client (dengan runtime mode)
// -----------------------
void handleClientRequest(Client &client) {
  // Tunggu hingga client mengirim data
  while (!client.available()) {
    delay(1);
  }
  String reqLine = client.readStringUntil('\n');
  reqLine.trim();
  
  if (reqLine.startsWith("GET")) {
    if (reqLine.indexOf(" /deleteAll") != -1)
      handleDeleteAll(client);
    else if (reqLine.indexOf(" /delete") != -1)
      handleDelete(client, reqLine);
    else if (reqLine.indexOf(" /list") != -1)
      handleList(client);
    else if (reqLine.indexOf(" /info") != -1)
      handleInfo(client);
    else {
      // Tentukan mode koneksi berdasarkan digitalRead
      if (WIFI == HIGH && ETHERNET == LOW)
        handleIndex(client, "WiFi Client");
      else if (WIFI == LOW && ETHERNET == HIGH)
        handleIndex(client, "Ethernet");
      else if (WIFI == LOW && ETHERNET == LOW)
        handleIndex(client, "WiFi AP");
      // else
      //   handleIndex(client, "Mode Tidak Diketahui");
    }
  }
  else if (reqLine.startsWith("POST")) {
    if (reqLine.indexOf(" /upload") != -1)
      handleUpload(client, reqLine);
    else {
      String html = wrapHtml("Error", "<p>Request POST tidak dikenali.</p>");
      sendHttpResponse(client, html.c_str());
    }
  }
  else {
    String html = wrapHtml("Error", "<p>Request tidak dikenali.</p>");
    sendHttpResponse(client, html.c_str());
  }
  // // Pastikan data terkirim dengan flush dan delay yang cukup
  // client.flush();
  // delay(10);  // Ubah delay sesuai kebutuhan
  client.stop();
}

// ----------------------------------------------------------------
// Routing: Menangani request klien dan mengarahkan ke fungsi terkait
// ----------------------------------------------------------------
// -----------------------
// Fungsi Utama untuk Menangani Client dari Server
// -----------------------
void handleClientConnections() {
  
  // Jika hanya WiFi aktif
  if (WIFI == HIGH && ETHERNET == LOW && wifiServer != nullptr) {
    WiFiClient client = wifiServer->available();
    if (client) handleClientRequest(client);
  }
  // Jika hanya Ethernet aktif
  else if (WIFI == LOW && ETHERNET == HIGH && ethernetServer != nullptr) {
    EthernetClient client = ethernetServer->available();
    if (client) handleClientRequest(client);
  }
  else if (WIFI == HIGH && ETHERNET == HIGH) {
    server.handleClient();
    delay(10); 
  }
  else if (WIFI == LOW && ETHERNET == LOW && wifiServer != nullptr) {
    WiFiClient client = wifiServer->available();
    if (client) handleClientRequest(client);
  }

  // Jika kedua aktif, periksa keduanya
  // else if (WIFI == HIGH && ETHERNET == HIGH) {
  //   WiFiClient wifiClient = wifiServer->available();
  //   if (wifiClient) handleClientRequest(wifiClient);
  //   EthernetClient ethClient = ethernetServer->available();
  //   if (ethClient) handleClientRequest(ethClient);
  // }
  // Jika tidak ada mode aktif, tidak melakukan apa-apa (atau tambahkan penanganan error)
}

String getConfigurationPage() {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>Configuration</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); color: #e0e0e0; font-family: 'Roboto', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }"
         ".container { background: #1e1e1e; padding: 40px; border-radius: 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.4); max-width: 600px; width: 100%; margin: auto; border-top: 8px solid #4CAF50; transition: transform 0.3s ease-in-out; text-align: center; }"
         ".container:hover { transform: scale(1.02); }"
         "h1 { color: #4CAF50; margin-bottom: 30px; font-size: 24px; text-align: center; }"
         ".menu-buttons { display: flex; flex-direction: column; gap: 20px; margin-bottom: 30px; align-items: center; }"
         ".menu-button { display: block; width: 55%; padding: 12px 24px; border: 1px solid #4CAF50; border-radius: 25px; background-color: rgba(255,255,255,0.05); color: #e0e0e0; font-size: 16px; text-align: center; cursor: pointer; transition: background-color 0.3s ease, color 0.3s ease; text-decoration: none; }"
         ".menu-button:hover { background-color: #4CAF50; color: #121212; }"
         ".footer-text { color: #4CAF50; font-size: 14px; margin-top: 20px; }"
         "@media (max-width: 600px) { .container { padding: 20px; } h1 { font-size: 18px; } .menu-button { width: 100%; } }"
         "</style>"
         "</head>"
         "<body>"
         "  <div class='container'>"
         "    <h1>Configuration</h1>"
         "    <div class='menu-buttons'>"
         "      <a href='/GantiSSID' class='menu-button'>Ganti SSID</a>"
         "      <a href='/handleGantiDevice' class='menu-button'>Ganti Device</a>"
         "      <a href='/handleGantiMacIp' class='menu-button'>Ganti MAC & IP</a>"
         "      <a href='/handlepanduan' class='menu-button'>Panduan</a>"
         "    </div>"
         "  </div>"
         "</body>"
         "</html>";
}

void handleRoot() {
  server.send(200, "text/html", getConfigurationPage());
}
 

String WrapHtmlGantiSSID(const String& title, const String& body) {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>" + title + "</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); color: #e0e0e0; font-family: 'Roboto', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }"
         ".container { background: #1e1e1e; padding: 40px; border-radius: 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.4); max-width: 600px; width: 100%; margin: auto; border-top: 8px solid #4CAF50; transition: transform 0.3s ease-in-out; text-align: center; }"
         ".container:hover { transform: scale(1.02); }"
         "h1 { color: #4CAF50; margin-bottom: 30px; font-size: 24px; text-align: center; }"
         ".info-box { background: #2e2e2e; padding: 12px; border-radius: 10px; margin-bottom: 30px; font-size: 16px; text-align: left; }"
         ".info-box p { margin: 8px 0; }"
         "form { display: flex; flex-direction: column; gap: 20px; align-items: center; }"
         "label { font-size: 16px; font-weight: bold; }"
         "input[type='text'], input[type='password'] { width: 55%; padding: 12px 24px; border: 1px solid #4CAF50; border-radius: 25px; background-color: rgba(255,255,255,0.05); color: #e0e0e0; font-size: 16px; }"
         "input[type='submit'] { display: block; width: 55%; padding: 12px 24px; border: 1px solid #4CAF50; border-radius: 25px; background-color: rgba(255,255,255,0.05); color: #e0e0e0; font-size: 16px; cursor: pointer; transition: background-color 0.3s ease, color 0.3s ease; }"
         "input[type='submit']:hover { background-color: #4CAF50; color: #121212; }"
         "#toggle-password { background: none; border: none; color: #4CAF50; cursor: pointer; font-size: 16px; margin-left: 10px; }"
         ".links { display: flex; justify-content: space-around; margin-top: 20px; }"
         ".links a { text-decoration: none; color: #4CAF50; font-size: 16px; transition: color 0.3s ease; }"
         ".links a:hover { color: #66BB6A; }"
         "@media (max-width: 600px) { .container { padding: 20px; } h1 { font-size: 18px; } input[type='text'], input[type='password'], input[type='submit'] { width: 100%; } }"
         "</style>"
         "</head>"
         "<body>"
         "<div class='container'>"
         "<h1>" + title + "</h1>"
         + body +
         "</div>"
         "</body>"
         "</html>";
}

void GantiSSID() {
  // Jika request adalah GET, tampilkan halaman form
  if (server.method() == HTTP_GET) {
    String formContent = "";
    // Tampilkan WiFi lama (sebelumnya)
    formContent += "<div class='info-box'>";
    formContent += "<p><strong>WiFi Lama :</strong> " + WIFI_SSID + "</p>";
    formContent += "<p><strong>Password Lama :</strong> " + WIFI_PASSWORD + "</p>";
    formContent += "</div>";
    // Form untuk memasukkan WiFi baru
    formContent += "<form action='/GantiSSID' method='post'>";
    formContent += "<label for='ssid'>WiFi Baru :</label>";
    formContent += "<input type='text' id='ssid' name='ssid' placeholder='Tulis nama WiFi baru' required>";
    formContent += "<label for='password'>Password Baru :</label>";
    formContent += "<input type='password' id='password' name='password' placeholder='Tulis password WiFi baru' required>";
    formContent += "<input type='submit' value='Ubah WiFi'>";
    formContent += "</form>";
    formContent += "<div class='links'><a href='/'>Kembali</a></div>";

    server.send(200, "text/html", WrapHtmlGantiSSID("Ganti WiFi (" + DEVICE_NAME + ")", formContent));
  }
  // Jika request adalah POST, proses data yang dikirim
  else if (server.method() == HTTP_POST) {
    // Simpan WiFi lama untuk ditampilkan nanti
    String oldSSID = WIFI_SSID;
    String oldPassword = WIFI_PASSWORD;

    // Ambil nilai baru dari form
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");

    // Update konfigurasi WiFi
    WIFI_SSID = newSSID;
    WIFI_PASSWORD = newPassword;

    // Simpan ke Preferences
    preferences.begin("webData", false);
    preferences.putString("ssid", WIFI_SSID);
    preferences.putString("password", WIFI_PASSWORD);
    preferences.end();

    Serial.println("Konfigurasi WiFi Baru:");
    Serial.println("SSID: " + newSSID);
    Serial.println("Password: " + newPassword);

    // Buat halaman konfirmasi yang menampilkan WiFi lama dan baru
    String confirmContent = "";
    confirmContent += "<div class='info-box'>";
    confirmContent += "<p><strong>WiFi Lama :</strong> " + oldSSID + "</p>";
    confirmContent += "<p><strong>Password Lama :</strong> " + oldPassword + "</p>";
    confirmContent += "<p><strong>WiFi Baru :</strong> " + WIFI_SSID + "</p>";
    confirmContent += "<p><strong>Password Baru :</strong> " + WIFI_PASSWORD + "</p>";
    confirmContent += "</div>";
    confirmContent += "<div class='links'><a href='/'>Kembali</a></div>";

    server.send(200, "text/html", WrapHtmlGantiSSID("Konfigurasi WiFi (" + DEVICE_NAME + ")", confirmContent));
  }
}

// Fungsi pembungkus HTML untuk halaman Ganti Device
String WrapHtmlhandleGantiDevice(const String& title, const String& body) {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>" + title + "</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); color: #e0e0e0; font-family: 'Roboto', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }"
         ".container { background: #1e1e1e; padding: 40px; border-radius: 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.4); max-width: 600px; width: 100%; margin: auto; border-top: 8px solid #4CAF50; transition: transform 0.3s ease-in-out; text-align: center; }"
         ".container:hover { transform: scale(1.02); }"
         "h1 { color: #4CAF50; margin-bottom: 30px; font-size: 24px; text-align: center; }"
         ".info-box { background: #2e2e2e; padding: 12px; border-radius: 10px; margin-bottom: 30px; font-size: 16px; text-align: left; }"
         ".info-box p { margin: 8px 0; }"
         "form { display: flex; flex-direction: column; gap: 20px; align-items: center; }"
         "label { font-size: 16px; font-weight: bold; }"
         "input[type='text'] { width: 55%; padding: 12px 24px; border: 1px solid #4CAF50; border-radius: 25px; background-color: rgba(255,255,255,0.05); color: #e0e0e0; font-size: 16px; }"
         "input[type='submit'] { display: block; width: 55%; padding: 12px 24px; border: 1px solid #4CAF50; border-radius: 25px; background-color: rgba(255,255,255,0.05); color: #e0e0e0; font-size: 16px; cursor: pointer; transition: background-color 0.3s ease, color 0.3s ease; }"
         "input[type='submit']:hover { background-color: #4CAF50; color: #121212; }"
         ".links { display: flex; justify-content: space-around; margin-top: 20px; }"
         ".links a { text-decoration: none; color: #4CAF50; font-size: 16px; transition: color 0.3s ease; }"
         ".links a:hover { color: #66BB6A; }"
         "@media (max-width: 600px) { .container { padding: 20px; } h1 { font-size: 18px; } input[type='text'], input[type='submit'] { width: 100%; } }"
         "</style>"
         "</head>"
         "<body>"
         "<div class='container'>"
         "<h1>" + title + "</h1>"
         + body +
         "</div>"
         "</body>"
         "</html>";
}

// Fungsi handler untuk Ganti Device (menangani GET dan POST)
void handleGantiDevice() {
  if (server.method() == HTTP_GET) {
    // Tampilkan halaman form dengan informasi device lama
    String formContent = "";
    formContent += "<div class='info-box'>";
    formContent += "<p><strong>Device Lama :</strong> " + DEVICE_NAME + "</p>";
    formContent += "</div>";
    formContent += "<form action='/handleGantiDevice' method='post'>";
    formContent += "<label for='device'>Nama Device Baru :</label>";
    formContent += "<input type='text' id='device' name='device' placeholder='Minimal 4 karakter' required>";
    formContent += "<input type='submit' value='Ganti Device'>";
    formContent += "</form>";
    formContent += "<div class='links'><a href='/'>Kembali</a></div>";
    
    server.send(200, "text/html", WrapHtmlhandleGantiDevice("Ganti Device (" + DEVICE_NAME + ")", formContent));
  }
  else if (server.method() == HTTP_POST) {
    // Simpan device lama untuk konfirmasi
    String oldDevice = DEVICE_NAME;
    // Ambil nilai device baru dari form
    String newDevice = server.arg("device");
    
    // Update nama device
    DEVICE_NAME = newDevice;

    // Simpan ke Preferences
    preferences.begin("webData", false);
    preferences.putString("device", DEVICE_NAME);
    preferences.end();
    
    Serial.println("Device telah diubah");
    Serial.println("Device Lama : " + oldDevice);
    Serial.println("Device Baru : " + newDevice);
    
    // Tampilkan halaman konfirmasi
    String confirmContent = "";
    confirmContent += "<div class='info-box'>";
    confirmContent += "<p><strong>Device Lama :</strong> " + oldDevice + "</p>";
    confirmContent += "<p><strong>Device Baru :</strong> " + newDevice + "</p>";
    confirmContent += "</div>";
    confirmContent += "<div class='links'><a href='/'>Kembali</a></div>";
    
    server.send(200, "text/html", WrapHtmlhandleGantiDevice("Ganti Device (" + DEVICE_NAME + ")", confirmContent));
  }
}

String wrapHtmlPanduan(const String& title, const String& body) {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>" + title + "</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); color: #e0e0e0; font-family: 'Roboto', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; line-height: 1.6; }"
         ".container { background: #1e1e1e; padding: 40px; border-radius: 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.4); max-width: 600px; width: 100%; margin: auto; border-top: 8px solid #4CAF50; transition: transform 0.3s ease; }"
         ".container:hover { transform: scale(1.02); }"
         "h1 { color: #4CAF50; margin-bottom: 30px; font-size: 28px; text-align: center; }"
         "p { font-size: 18px; margin-bottom: 20px; text-align: justify; }"
         "ul { text-align: justify; font-size: 18px; margin-bottom: 20px; padding-left: 20px; }"
         "li { margin-bottom: 12px; }"
         ".links { margin-top: 20px; display: flex; justify-content: center; gap: 20px; }"
         ".links a { text-decoration: none; color: #4CAF50; font-size: 16px; transition: color 0.3s ease; }"
         ".links a:hover { color: #66BB6A; }"
         ".info-header { display: flex; justify-content: space-between; gap: 20px; margin-bottom: 20px; margin-top: 0px; }"
         ".info-box { background: #2e2e2e; padding: 8px 12px; border-radius: 10px; width: 100%; text-align: left; font-size: 16px; font-weight: thin; font-family: 'Roboto', 'Courier New', Courier, monospace; }"
         "@media (max-width: 600px) { .container { padding: 20px; } h1 { font-size: 22px; } p, ul { font-size: 16px; } .links a { font-size: 16px; } }"
         "</style>"
         "</head>"
         "<body>"
         "<div class='container'>"
         "<h1>" + title + "</h1>" + body +
         "</div>"
         "</body>"
         "</html>";
} 

void handlepanduan() {    
  String panduanBody = "<p>Berikut adalah beberapa informasi penting yang perlu Anda ketahui untuk memastikan konfigurasi perangkat Anda berjalan dengan lancar dan sesuai dengan kebutuhan :</p>"
                      "<ul>"
                      "<li><strong>Ganti SSID :</strong> Gunakan menu Ganti SSID untuk mengubah nama dan kata sandi WiFi. Pastikan untuk menggunakan nama yang sesuai dengan Access Point (AP) atau Router yang digunakan.</li>"
                      "<li><strong>Ganti Device :</strong> Di menu Ganti Device, Anda dapat mengubah nama perangkat agar lebih mudah dikenali. Nama Device juga akan mengganti nama WiFi Modul untuk konfigurasi dan juga mode upload</li>"
                      "<li>Pastikan semua perubahan disimpan dengan benar dan konfigurasi telah diperbarui.</li>"
                      "<li>Periksa kembali setiap konfigurasi setelah melakukan perubahan untuk menghindari kesalahan.</li>"
                      "<li>Atur selector pada modul sesuai dengan informasi yang tersedia untuk memastikan mode yang benar.</li>"
                      "<div class='info-header'>"
                      "  <div class='info-box' id='device-text'>"
                      "    <b>1 1</b> > WiFi AP (Configuration)<br>"
                      "    <b>1 0</b> > WiFi Client (Upload Mode)<br>"
                      "    <b>0 1</b> > Ethernet Client (Upload Mode)<br>"
                      "    <b>0 0</b> > WiFi AP (Upload Mode)<br>"
                      "  </div>"
                      "</div>"
                      "<li>Pilih selector sesuai fungsi perangkat Anda dengan seksama.</li>"
                      "</ul>"
                      "<div class='links'>"
                      "  <a href='/'>Kembali</a>"
                      "</div>";
 
  server.send(200, "text/html", wrapHtmlPanduan("Panduan", panduanBody) );
}  

// Fungsi untuk membungkus konten HTML ke dalam template halaman
String wrapHtmlGantiMacIp(const String &title, const String &body) {
  return "<!DOCTYPE html>"
         "<html lang='id'>"
         "<head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>" + title + "</title>"
         "<style>"
         "/* Reset & dasar */"
         "* { box-sizing: border-box; margin: 0; padding: 0; }"
         "body { background: linear-gradient(135deg, #1a1a1a, #2a2a2a); color: #e0e0e0; font-family: 'Roboto', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }"
         ".container { background: #1e1e1e; padding: 40px; border-radius: 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.4); max-width: 600px; width: 100%; margin: auto; border-top: 8px solid #4CAF50; transition: transform 0.3s ease-in-out; text-align: center; }"
         ".container:hover { transform: scale(1.02); }"
         "h1 { color: #4CAF50; margin-bottom: 30px; font-size: 24px; text-align: center; }"
         ".info-box { background: #2e2e2e; padding: 12px; border-radius: 10px; margin-bottom: 30px; font-size: 16px; text-align: left; }"
         ".info-box p { margin: 8px 0; }"
         "form { display: flex; flex-direction: column; gap: 20px; align-items: center; }"
         "label { font-size: 16px; font-weight: bold; }"
         "input[type='text'] { width: 55%; padding: 12px 24px; border: 1px solid #4CAF50; border-radius: 25px; background-color: rgba(255,255,255,0.05); color: #e0e0e0; font-size: 16px; }"
         "input[type='submit'] { display: block; width: 55%; padding: 12px 24px; border: 1px solid #4CAF50; border-radius: 25px; background-color: rgba(255,255,255,0.05); color: #e0e0e0; font-size: 16px; cursor: pointer; transition: background-color 0.3s ease, color 0.3s ease; }"
         "input[type='submit']:hover { background-color: #4CAF50; color: #121212; }"
         ".links { display: flex; justify-content: space-around; margin-top: 20px; }"
         ".links a { text-decoration: none; color: #4CAF50; font-size: 16px; transition: color 0.3s ease; }"
         ".links a:hover { color: #66BB6A; }"
         "@media (max-width: 600px) { .container { padding: 20px; } h1 { font-size: 18px; } input[type='text'], input[type='submit'] { width: 100%; } }"
         "</style>"
         "</head>"
         "<body>"
         "<div class='container'>"
         "<h1>" + title + "</h1>" +
         body +
         "</div>"
         "</body>"
         "</html>";
}

// Fungsi handler untuk mengganti MAC & IP Address
void handleGantiMacIp() {
  if (server.method() == HTTP_GET) {
    // Tampilkan halaman form dengan informasi MAC & IP Address lama
    String formContent = "";
    formContent += "<div class='info-box'>";
    formContent += "<p><strong>MAC Address :</strong> " + currentMac + "</p>";
    formContent += "<p><strong>IP Address :</strong> " + currentIp + "</p>";
    formContent += "</div>";
    formContent += "<form action='/handleGantiMacIp' method='post'>";
    formContent += "<label for='mac'>MAC Address baru :</label>";
    formContent += "<input type='text' id='mac' name='mac' placeholder='Format DE:AD:BE:EF:FE:EE' required>";
    formContent += "<label for='ip'>IP Address baru :</label>";
    formContent += "<input type='text' id='ip' name='ip' placeholder='Format 192.168.1.10' required>";
    formContent += "<input type='submit' value='Perbarui'>";
    formContent += "</form>";
    formContent += "<div class='links'><a href='/'>Kembali</a></div>";
    
    server.send(200, "text/html", wrapHtmlGantiMacIp("Ganti MAC & IP Address (" + DEVICE_NAME + ")", formContent));
  }
  else if (server.method() == HTTP_POST) {
    // Simpan data MAC & IP Address lama untuk konfirmasi
    String oldMac = currentMac;
    String oldIp = currentIp;
    // Ambil nilai MAC dan IP Address baru dari form
    String newMac = server.arg("mac");
    String newIp  = server.arg("ip");
    
    // Update variabel global dengan nilai baru
    currentMac = newMac;
    currentIp  = newIp;
    
    // Simpan ke Preferences (misalnya EEPROM)
    preferences.begin("webData", false);
    preferences.putString("mac", currentMac);
    preferences.putString("ip", currentIp);
    preferences.end();
    
    Serial.println("MAC & IP Address telah diubah");
    Serial.println("MAC Lama : " + oldMac);
    Serial.println("MAC Baru : " + newMac);
    Serial.println("IP Lama  : " + oldIp);
    Serial.println("IP Baru  : " + newIp);
    
    // Tampilkan halaman konfirmasi
    String confirmContent = "";
    confirmContent += "<div class='info-box'>";
    confirmContent += "<p><strong>MAC Address Lama :</strong> " + oldMac + "</p>";
    confirmContent += "<p><strong>MAC Address Baru :</strong> " + newMac + "</p>";
    confirmContent += "<p><strong>IP Address Lama :</strong> " + oldIp + "</p>";
    confirmContent += "<p><strong>IP Address Baru :</strong> " + newIp + "</p>";
    confirmContent += "</div>";
    confirmContent += "<div class='links'><a href='/'>Kembali</a></div>";
    
    server.send(200, "text/html", wrapHtmlGantiMacIp("Ganti MAC & IP Address (" + DEVICE_NAME + ")", confirmContent));
  }
} 

void ambilDataEEPROM(){
  preferences.begin("webData", true);
  WIFI_SSID = preferences.getString("ssid", "");
  WIFI_PASSWORD = preferences.getString("password", ""); 
  DEVICE_NAME = preferences.getString("device", "Default"); 
  currentMac = preferences.getString("mac", ""); 
  currentIp = preferences.getString("ip", "");  
  updateMacAndIp(currentMac, currentIp);
  
  myMAC = currentMac.c_str(); 
  deviceName = DEVICE_NAME.c_str();
  wifiSSIDClient     = WIFI_SSID.c_str();
  wifiPasswordClient = WIFI_PASSWORD.c_str();  
  preferences.end();
} 

// ----------------------------------------------------------------
// Inisialisasi server web menggunakan WiFi Access Point
// ----------------------------------------------------------------
void initWebServer() {
  if (WIFI == 1 && ETHERNET == 0) {
    wifiServer = new WiFiServer(80);
    wifiServer->begin();
    Serial.println("WiFi Server Client telah dimulai.");
  } 
  else if (WIFI == 0 && ETHERNET == 1) {
    ethernetServer = new EthernetServer(80);
    ethernetServer->begin();
    Serial.println("Ethernet Server telah dimulai.");
  } 
  else if (WIFI == 1 && ETHERNET == 1) { 
    server.on("/", handleRoot);
    server.on("/GantiSSID", GantiSSID);
    server.on("/handleGantiDevice", handleGantiDevice); 
    server.on("/handleGantiMacIp", handleGantiMacIp);
    server.on("/handlepanduan", handlepanduan); 
    server.begin(); 
    Serial.println("WiFi Server AP telah dimulai.");
  }
  else if (WIFI == 0 && ETHERNET == 0) {
    wifiServer = new WiFiServer(80);
    wifiServer->begin();
    Serial.println("WiFi Server AP telah dimulai.");
  } 
} 
