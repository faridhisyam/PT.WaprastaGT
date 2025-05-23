#include <SD.h>
#include "wrapHtmlhandleDelete.h"

// ...existing code...

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
      String html = wrapHtmlhandleDelete("Hapus File", response);
      sendHttpResponse(client, html.c_str());
    } else {
      String response = "<p>Gagal menghapus file " + filename + ".</p><br><a href='/list'>Kembali</a>";
      String html = wrapHtmlhandleDelete("Hapus File", response);
      sendHttpResponse(client, html.c_str());
    }
  } else {
    String response = "<p>File " + filename + " tidak ditemukan.</p><br><a href='/list'>Kembali</a>";
    String html = wrapHtmlhandleDelete("Hapus File", response);
    sendHttpResponse(client, html.c_str());
  }
}

// ...existing code...
