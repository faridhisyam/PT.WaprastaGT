#include "wrapHtmlhandleIndex.h"

// ...existing code...

void handleIndex(Client &client, String mode) {
  String body = "";
  body += "<div class='info-header'>";
  body += "<div class='info-item'><div class='info-label'>Device</div><div class='info-box'>#CB1</div></div>";
  body += "<div class='info-item'><div class='info-label'>Mode</div><div class='info-box'>" + mode + "</div></div>";
  body += "<div class='info-item'><div class='info-label'>Waktu Aktif</div><div class='info-box' id='active-time'>00:00:00</div></div>";
  body += "</div>";
  body += "<h2>Tambahkan File ke SD Card</h2>";
  body += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  body += "<label for='file-upload' class='custom-file-upload'>Pilih File</label>";
  body += "<input type='file' name='upload[]' id='file-upload' multiple>";
  body += "<div class='file-info' id='file-info'><span id='file-names'>Belum ada file yang dipilih</span><span id='file-count'></span></div>";
  body += "<input type='submit' value='Upload'>";
  body += "</form>";
  body += "<div class='links'><a href='/list'>Daftar File</a><a href='/info'>Informasi</a></div>";
  
  String html = wrapHtmlhandleIndex("#CB1 (" + mode + ")", body);
  sendHttpResponse(client, html.c_str());
}

// ...existing code...
