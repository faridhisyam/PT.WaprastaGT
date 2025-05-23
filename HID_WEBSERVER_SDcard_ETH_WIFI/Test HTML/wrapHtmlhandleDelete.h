#ifndef WRAPHTMLHANDLEDELETE_H
#define WRAPHTMLHANDLEDELETE_H

#include <Arduino.h>

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

#endif // WRAPHTMLHANDLEDELETE_H
