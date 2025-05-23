#include <SD.h>
#include "wrapHtmlhandleList.h"

// ...existing code...

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
      body += "<div class='file-size'>" + String(file.size() / 1024) + " KB</div>";
      body += "<div class='file-opsi'><a href='/delete?filename=" + displayName + "' class='delete-button'>Hapus</a></div>";
      body += "</div>";
      file = root.openNextFile();
    }
    body += "<div class='file-info'>Jumlah file: " + String(fileCount) + "</div>";
  }
  body += "<div class='links'><a href='/deleteAll'>Hapus Semua File</a><a href='/'>Kembali</a></div>";
  String html = wrapHtmlhandleList("Daftar File SD Card", body);
  sendHttpResponse(client, html.c_str());
}

// ...existing code...
