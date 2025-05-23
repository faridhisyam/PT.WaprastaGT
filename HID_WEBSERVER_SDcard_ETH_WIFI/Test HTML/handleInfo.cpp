#include <SD.h>
#include "wrapHtmlhandleInfo.h"

// ...existing code...

void handleInfo(Client &client) {
  total = SD.totalBytes();
  used = SD.usedBytes();
  freeSpace = total - used;
  
  String body = "";
  body += "<div class='circular-progress'><svg><circle class='free' cx='90' cy='90' r='70'></circle><circle class='used' cx='90' cy='90' r='70'></circle></svg><div class='text'><div id='used-percentage'>" + String((used * 100.0) / total, 2) + "%</div><div>Used</div></div></div>";
  body += "<div class='info-details'>";
  body += "<div class='info'><span>SD Card Type:</span><span class='value'>" + String(CARDTYPE) + "</span></div>";
  body += "<div class='info'><span>File System:</span><span class='value'>FAT32</span></div>";
  body += "<div class='info'><span>Total:</span><span id='total-size' class='value'>" + String(total / 1048576.0, 3) + " MB</span></div>";
  body += "<div class='info'><span>Used:</span><span id='used-size' class='value'>" + String(used / 1048576.0, 3) + " MB</span></div>";
  body += "<div class='info'><span>Free:</span><span id='free-size' class='value'>" + String(freeSpace / 1048576.0, 3) + " MB</span></div>";
  body += "</div>";
  
  String html = wrapHtmlhandleInfo("Informasi SD Card", body);
  sendHttpResponse(client, html.c_str());
}

// ...existing code...
