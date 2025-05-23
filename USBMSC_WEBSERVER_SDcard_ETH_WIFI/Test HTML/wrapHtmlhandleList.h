#ifndef WRAPHTMLHANDLELIST_H
#define WRAPHTMLHANDLELIST_H

#include <Arduino.h>

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

#endif // WRAPHTMLHANDLELIST_H
