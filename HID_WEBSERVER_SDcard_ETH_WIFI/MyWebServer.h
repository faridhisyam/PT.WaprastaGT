#ifndef MYWEBSERVER_H
#define MYWEBSERVER_H 

#include <SPI.h>
#include <WiFi.h>   
#include <Ethernet.h>
#include <SD.h> 
#include "USB.h"
#include "USBMSC.h" 
#include <WebServer.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h> //Buat nyimpan data terakhir ke EEPROM
// #include <Fonts/FreeSans9pt7b.h>  
#include <EthernetUdp.h>
#include <USBHIDKeyboard.h>

extern int WIFI;
extern int ETHERNET; 
extern int USBorKEY;
extern String WIFI_SSID;
extern String WIFI_PASSWORD;
extern String DEVICE_NAME; 
extern String currentMac;
extern String currentIp;
extern String CARDTYPE;  

extern const int myBACA;       // Pin tombol
extern int lastButtonState;   // Untuk deteksi edge (perubahan)
extern int currentButtonState;

extern const char* deviceName;
extern const char* wifiSSIDClient;
extern const char* wifiPasswordClient; 
extern const char* myMAC; 

// Deklarasi fungsi-fungsi web server 
void processCurrentFileOneLine();
void RunKeyboard();
void initKeyboard();
void initWebServer();
void stopUSBMSC();
void startUSBMSC();
void ambilDataEEPROM();
void handleClientConnections();
void handleClientRequest(Client &client);
void updateMacAndIp(const String &newMacStr, const String &newIpStr);

#endif // WEBSERVER_H
