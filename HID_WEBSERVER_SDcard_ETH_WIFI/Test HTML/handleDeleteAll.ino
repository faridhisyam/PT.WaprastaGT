#include <SD.h>
#include "wrapHtmlhandleDeleteAll.h"

// ...existing code...

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
    
  String html = wrapHtmlhandleDeleteAll("Hapus Semua File", response);
  sendHttpResponse(client, html.c_str());
}

// ...existing code...
