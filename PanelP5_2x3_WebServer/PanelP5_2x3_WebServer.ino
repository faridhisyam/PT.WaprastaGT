// 15 Januari 2025
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Preferences.h> //Buat nyimpan data terakhir ke EEPROM
#include "index_html.h" // Header untuk HTML
#include "GlobalVariabel.h"

// Konfigurasi Access Point
IPAddress local_IP(192, 168, 0, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);

MatrixPanel_I2S_DMA *dma_display = nullptr;
VirtualMatrixPanel  *virtualDisp = nullptr;
Preferences preferences;

// Variable for color.
uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myRED = dma_display->color565(250, 0, 0);
uint16_t myGREEN = dma_display->color565(0, 255, 0);
uint16_t myBLUE = dma_display->color565(0, 0, 255);
uint16_t myYELLOW = dma_display->color565(255, 255, 50);

void handleRoot() {
  server.send(200, "text/html", webpage);
}

void handleSubmit() {
  String code = server.arg("code");
  String text1 = server.arg("text1");
  String text2 = server.arg("text2");
  String text3 = server.arg("text3");
  String text4 = server.arg("text4");
  String option = server.arg("option");

  if (code == uniqueCode) {
    flagclean = 1;
    receivedText1 = text1;
    receivedText2 = text2;
    receivedText3 = text3;
    receivedText4 = text4;
    receivedOption = option;

    preferences.begin("webData", false);
    preferences.putString("savedText1", receivedText1);
    preferences.putString("savedText2", receivedText2);
    preferences.putString("savedText3", receivedText3);
    preferences.putString("savedText4", receivedText4);
    preferences.putString("option", receivedOption);
    preferences.end();

    // Kirim respons ke klien
    String response = "Data Web Server :<br>";
    if (receivedText1 != "") response += "Teks 1 : " + receivedText1 + "<br>";
    if (receivedText2 != "") response += "Teks 2 : " + receivedText2 + "<br>";
    if (receivedText3 != "") response += "Teks 3 : " + receivedText3 + "<br>";
    if (receivedText4 != "") response += "Teks 4 : " + receivedText4 + "<br>";
    response += "<a href='/'>Kembali</a>";

    server.send(200, "text/html", response);
  } 
  else 
  {
    Serial.println("Kode salah, akses ditolak.");
    server.send(200, "text/html", "Kode salah!<br><a href='/'>Kembali</a>");
  }
}

// void centerText(String str, int width) {
//   virtualDisp->setFont(&FreeSansBold12pt7b);
//   int len = str.length();
//   int spaces = (width - (len * 18)) / 2;
//   int car = spaces;

//   if(flagclean == 1){
//     virtualDisp->fillRect(0, 0, 192, 31, myBLACK);
//     flagclean = 0;
//   }

//   virtualDisp->setCursor(car, 24);
//   virtualDisp->setTextColor(dma_display->color565(0, 255, 0));   
//   virtualDisp->print(str);
// }

void centerText(String str, int width) {
  virtualDisp->setFont(&FreeSansBold12pt7b); 
  int16_t xcenterText, ycenterText;        // Koordinat kiri atas teks
  uint16_t wcenterText, hcenterText;         // (lebar dan tinggi)
 
  virtualDisp->getTextBounds(str, 0, 0, &xcenterText, &ycenterText, &wcenterText, &hcenterText);
 
  int car = (width - wcenterText) / 2;
 
  if (flagclean == 1) {
    virtualDisp->fillRect(0, 0, width, 31, myBLACK);
    flagclean = 0;
  }
 
  virtualDisp->setCursor(car, 24);  
  virtualDisp->setTextColor(dma_display->color565(0, 255, 0));  
  virtualDisp->print(str);
} 

void setup() {
  Serial.begin(115200);
  Serial.println("Start");
  
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
      Serial.println("Konfigurasi IP gagal.");
      while (true);
  }

  WiFi.softAP(ssid, password);
  Serial.println("Access Point dimulai");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  
  server.on("/", handleRoot);
  server.on("/submit", handleSubmit);

  server.begin();
  Serial.println("Web server dimulai");

  preferences.begin("webData", true);
  receivedText1 = preferences.getString("savedText1", "");
  receivedText2 = preferences.getString("savedText2", "");
  receivedText3 = preferences.getString("savedText3", "");
  receivedText4 = preferences.getString("savedText4", "");
  receivedOption = preferences.getString("option", "");
  preferences.end();

  preferences.begin("RCVData", true);
  receivedData = preferences.getInt("savedSerial", 0);
  preferences.end();

  //======new=====//
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);
  pinMode(S6, INPUT);
  //===================//
  
  //----------------------------------------Module configuration.
  HUB75_I2S_CFG::i2s_pins _pins={R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, 23, OE_PIN, CLK_PIN};
  // HUB75_I2S_CFG::i2s_pins _pins2={R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};
  delay(10);
  
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,   //--> module width.
    PANEL_RES_Y,   //--> module height.
    PANEL_CHAIN,   //--> Chain length.
    _pins          //--> pin mapping.
  );
  delay(10);
  
  // HUB75_I2S_CFG mxconfig2(
  //   PANEL_RES_X,   //--> module width.
  //   PANEL_RES_Y,   //--> module height.
  //   PANEL_CHAIN,   //--> Chain length.
  //   _pins2          //--> pin mapping.
  // );
  // delay(10);

  // Set I2S clock speed.
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;  // I2S clock speed, better leave as-is unless you want to experiment.
  delay(10);

  // mxconfig2.i2sspeed = HUB75_I2S_CFG::HZ_10M;  // I2S clock speed, better leave as-is unless you want to experiment.
  // delay(10);

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(80); //--> 0-255.

  // dma_display = new MatrixPanel_I2S_DMA(mxconfig2);
  // dma_display->begin();
  // dma_display->setBrightness8(80); //--> 0-255.

  virtualDisp = new VirtualMatrixPanel((*dma_display), NUM_ROWS, NUM_COLS, PANEL_RES_X, PANEL_RES_Y, VIRTUAL_MATRIX_CHAIN_TYPE2);//Panel Baru
  // virtualDisp = new VirtualMatrixPanel((*dma_display), NUM_ROWS, NUM_COLS, PANEL_RES_X, PANEL_RES_Y, VIRTUAL_MATRIX_CHAIN_TYPE); //Panel Lama (yang sudah dikirim di SAI)

  dma_display->clearScreen();
  
  dma_display->fillScreen(myBLUE);
  delay(1000);
  
  dma_display->clearScreen();
  delay(1000);
}

void displayStep(int step) {
  if (step == 0) {
      flagclean = 1; 
      centerText(receivedText1, 192);
  } else if (step == 1) {
      flagclean = 1;
      centerText(receivedText2, 192);
  } else if (step == 2) {
      flagclean = 1;
      centerText(receivedText3, 192);
  } else if (step == 3) {
      flagclean = 1;
      centerText(receivedText4, 192);
  }
}

void loop(){
  unsigned long currentMillis = millis();

  server.handleClient();
  mainDisplay();
  receivedSerial();

  if(receivedOption == "1"){ 
    centerText(receivedText1, 192);
  } 
  else if(receivedOption == "4"){
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        // displayStep(currentStep); 

        // currentStep++;

        // if (currentStep > 3) {
        //     currentStep = 0;
        // } 

        secondDisplay(currentStep); 

        currentStep++;

        if (currentStep > 1) {
         currentStep = 0;
        }

        // if(receivedData == 0 || receivedData == 1 || receivedData == 2 || receivedData == 3 || receivedData == 4 || receivedData == 5 ||  receivedData == 6 || receivedData == 8){
        //   if (currentStep > 1) {
        //       currentStep = 0;
        //   }
        //   Serial.println("2 bit");
        // } else if(receivedData == 7){
        //   if (currentStep > 2) {
        //       currentStep = 0;
        //   }
        //   Serial.println("3 bit");
        // } else if(receivedData == 15){
        //   if (currentStep > 3) {
        //       currentStep = 0;
        //   }
        //   Serial.println("4 bit");
        // }
    }
  }
}

// void secondDisplay(int step){
//   if(receivedData == 0){
//     switch (step) {
//       case 0:
//         flagclean = 1;
//         centerText(" ", 192);
//       break;
//       case 1:
//         flagclean = 1;
//         centerText(" ", 192);
//       break;
//     }
//     Serial.println("masuk 0");
//   }
//   else if(receivedData == 1){
//     switch (step) {
//       case 0:
//         flagclean = 1;
//         centerText(receivedText1, 192);
//       break;
//       case 1:
//         flagclean = 1;
//         centerText("  ", 192);
//       break;
//     }
//     Serial.println("masuk 1");
//   } 
//   else if(receivedData == 2){
//     switch (step) {
//       case 0:
//         flagclean = 1;
//         centerText(receivedText2, 192);
//       break;
//       case 1:
//         flagclean = 1;
//         centerText(" ", 192);
//       break;
//     }
//     Serial.println("masuk 2");
//   }
//   else if(receivedData == 3){
//     switch (step) {
//       case 0:
//         flagclean = 1;
//         centerText(receivedText1, 192);
//       break;
//       case 1:
//         flagclean = 1;
//         centerText(receivedText2, 192);
//       break;
//     }
//     Serial.println("masuk 3");
//   }
//   else if(receivedData == 4){
//     switch (step) {
//       case 0:
//         flagclean = 1;
//         centerText(receivedText3, 192);
//       break;
//       case 1:
//         flagclean = 1;
//         centerText("  ", 192);
//       break;
//     }
//     Serial.println("masuk 4");
//   }
//   else if(receivedData == 5){
//     switch (step) {
//       case 0:
//         flagclean = 1;
//         centerText(receivedText1, 192);
//       break;
//       case 1:
//         flagclean = 1;
//         centerText(receivedText3, 192);
//       break;
//     }
//     Serial.println("masuk 5");
//   }
//   else if(receivedData == 6){
//     switch (step) {
//       case 0:
//         flagclean = 1;
//         centerText(receivedText2, 192);
//       break;
//       case 1:
//         flagclean = 1;
//         centerText(receivedText3, 192);
//       break;
//     }
//     Serial.println("masuk 6");
//   } 
//   else if(receivedData == 7){ //3bit
//     switch (step) {
//       case 0:
//         flagclean = 1;
//         centerText(receivedText1, 192);
//       break;
//       case 1:
//         flagclean = 1;
//         centerText(receivedText2, 192);
//       break;
//       case 2:
//         flagclean = 1;
//         centerText(receivedText3, 192);
//       break;
//     }
//     Serial.println("masuk 7");
//   }
//   else if(receivedData == 8){
//     switch (step) {
//       case 0:
//         flagclean = 1;
//         centerText(receivedText1, 192);
//       break;
//       case 1:
//         flagclean = 1;
//         centerText(" ", 192);
//       break;
//     }
//     Serial.println("masuk 8");
//   } 
//   else if(receivedData == 15){
//     displayStep(step);
//     Serial.println("masuk 15");
//   } 
// }

void secondDisplay(int step){
  if(receivedData == 1 || receivedData == 3 || receivedData == 5 || receivedData == 7){
    switch (step) {
      case 0:
        flagclean = 1;
        centerText(" ", 192);
      break;
      case 1:
        flagclean = 1;
        centerText(" ", 192);
      break;
    }
    Serial.println("mati");
  }
  else if(receivedData == 2){
    switch (step) {
      case 0:
        flagclean = 1;
        centerText(receivedText1, 192);
      break;
      case 1:
        flagclean = 1;
        centerText("  ", 192);
      break;
    }
    Serial.println("receivedText1");
  } 
  else if(receivedData == 4){
    switch (step) {
      case 0:
        flagclean = 1;
        centerText(receivedText2, 192);
      break;
      case 1:
        flagclean = 1;
        centerText(" ", 192);
      break;
    }
    Serial.println("receivedText2");
  }
  else if(receivedData == 6){
    switch (step) {
      case 0:
        flagclean = 1;
        centerText(receivedText3, 192); 
      break;
      case 1:
        flagclean = 1;
        centerText(" ", 192);
      break;
    }
    Serial.println("receivedText3");
  }
  else if(receivedData == 8){
    switch (step) {
      case 0:
        flagclean = 1;
        centerText(receivedText4, 192);
      break;
      case 1:
        flagclean = 1;
        centerText("  ", 192);
      break;
    }
    Serial.println("receivedText4");
  }
  // else if(receivedData == 5){
  //   switch (step) {   
  //     case 0:
  //       flagclean = 1;
  //       centerText(receivedText1, 192);
  //     break;
  //     case 1:
  //       flagclean = 1;
  //       centerText(receivedText3, 192);
  //     break;
  //   }
  //   Serial.println("masuk 5");
  // }
  // else if(receivedData == 6){
  //   switch (step) {
  //     case 0:
  //       flagclean = 1;
  //       centerText(receivedText2, 192);
  //     break;
  //     case 1:
  //       flagclean = 1;
  //       centerText(receivedText3, 192);
  //     break;
  //   }
  //   Serial.println("masuk 6");
  // } 
  // else if(receivedData == 7){ //3bit
  //   switch (step) {
  //     case 0:
  //       flagclean = 1;
  //       centerText(receivedText1, 192);
  //     break;
  //     case 1:
  //       flagclean = 1;
  //       centerText(receivedText2, 192);
  //     break;
  //     case 2:
  //       flagclean = 1;
  //       centerText(receivedText3, 192);
  //     break;
  //   }
  //   Serial.println("masuk 7");
  // }
  // else if(receivedData == 8){
  //   switch (step) {
  //     case 0:
  //       flagclean = 1;
  //       centerText(receivedText1, 192);
  //     break;
  //     case 1:
  //       flagclean = 1;
  //       centerText(" ", 192);
  //     break;
  //   }
  //   Serial.println("masuk 8");
  // } 
  // else if(receivedData == 15){
  //   displayStep(step);
  //   Serial.println("masuk 15");
  // } 
}

void receivedSerial() {
  if (Serial.available() > 0) {
    String incomingData = Serial.readStringUntil('\n'); 
    int convertedData = incomingData.toInt();

    if (convertedData >= 0 && convertedData <= 15) {
      receivedData = (uint8_t)convertedData;
      preferences.begin("RCVData", false);
      preferences.putInt("savedSerial", receivedData);
      preferences.end();
    }
  }
}

void mainDisplay() {
  virtualDisp->setFont(&FreeMonoBold9pt7b);
  dma_display->setTextSize(1);    
  virtualDisp->setTextSize(1);

  //==================BOARD BARU=========================//
    condition_Led[1] = !digitalRead(S1);
    condition_Led[2] = !digitalRead(S2);
    condition_Led[3] = !digitalRead(S3);
    condition_Led[4] = !digitalRead(S4);
    condition_Led[5] = !digitalRead(S5);
    condition_Led[6] = !digitalRead(S6);   

  //====================================SETELAH REVISI(DI SAI)====================================//
  //============YCM============//
  if(condition_Led[1] == 1 && condition_Led[6] == 1){
    if(condiRect[1] == 1){
      virtualDisp->fillRect(2,33,L_Rect_Red,T_Rect_Red,myRED);
      virtualDisp->fillRect(XR1-1,YR1-11,L_Rect,T_Rect,myBLACK);
      condiRect[1] = 2;
      con_black[1] = 1;
    } 
  }else{
    if(con_black[1] == 1){
      virtualDisp->fillRect(2,33,L_Rect_Red,T_Rect_Red,myBLACK);
      virtualDisp->fillRect(XR1-1,YR1-11,L_Rect,T_Rect,myBLACK);
      con_black[1] = 2;
      condiRect[1] = 1;
    }
  }

  // if(condition_Led[1] == 1 && condition_Led[6] == 1){
  //   if(condiRect[1] == 1){
  //     virtualDisp->getTextBounds("YCM", XR1, YR1, &xYCM, &yYCM, &wYCM, &hYCM);
  //     virtualDisp->fillRect(xYCM, yYCM, wYCM, hYCM, myBLACK);
  //     virtualDisp->setCursor(XR1, YR1);  
  //     virtualDisp->setTextColor(myGREEN);
  //     virtualDisp->println("YCM");
  //     condiRect[1] = 2;
  //     con_black[1] = 1;
  //   } 
  // }else{
  //   if(con_black[1] == 1){
  //     virtualDisp->getTextBounds("YCM", XR1, YR1, &xYCM, &yYCM, &wYCM, &hYCM);
  //     virtualDisp->fillRect(xYCM, yYCM, wYCM, hYCM, myRED);
  //     virtualDisp->setCursor(XR1, YR1);  
  //     virtualDisp->setTextColor(myGREEN);
  //     virtualDisp->println("YCM");
  //     con_black[1] = 2;
  //     condiRect[1] = 1;
  //   }
  // }
  
  //============TWS============//
  if(condition_Led[2] == 1 && condition_Led[6] == 1){
    if(condiRect[2] == 1){
      virtualDisp->fillRect(41-1,33,L_Rect_Red,T_Rect_Red,myRED);
      virtualDisp->fillRect(XR2-1,YR2-11,L_Rect,T_Rect,myBLACK);
      condiRect[2] = 2;
      con_black[2] = 1;
    }
  }else{
    if(con_black[2] == 1 ){
      virtualDisp->fillRect(41-1,33,L_Rect_Red,T_Rect_Red,myBLACK);
      virtualDisp->fillRect(XR2-1,YR2-11,L_Rect,T_Rect,myBLACK);
      con_black[2] = 2;
      condiRect[2] = 1;
    }
  } 
  
  //============BON============//   
  if(condition_Led[3] == 1 && condition_Led[6] == 1){
    if(condiRect[3] == 1){
      virtualDisp->fillRect(79-1,33,L_Rect_Red,T_Rect_Red,myRED);
      virtualDisp->fillRect(XR3,YR3-11,L_Rect,T_Rect,myBLACK);
      condiRect[3] = 2;
      con_black[3] = 1;
    }
  }else{
    if(con_black[3] == 1 ){
        virtualDisp->fillRect(79-1,33,L_Rect_Red,T_Rect_Red,myBLACK);
        virtualDisp->fillRect(XR3,YR3-11,L_Rect,T_Rect,myBLACK);
        con_black[3] = 2;
        condiRect[3] = 1;
    }
  }

  //============SLD============// 
  if(condition_Led[4] == 1 &&  condition_Led[6] == 1){
    if(condiRect[4] == 1){
      virtualDisp->fillRect(117-1,33,L_Rect_Red,T_Rect_Red,myRED);
      virtualDisp->fillRect(XR4-1,YR4-11,L_Rect,T_Rect,myBLACK);
      condiRect[4] = 2;
      con_black[4] = 1;
    }
  }else{
    if(con_black[4] == 1){
        virtualDisp->fillRect(117-1,33,L_Rect_Red,T_Rect_Red,myBLACK);
        virtualDisp->fillRect(XR4-1,YR4-11,L_Rect,T_Rect,myBLACK);
        con_black[4] = 2;
        condiRect[4] = 1;
    }
  }

  //============RAY============// 
  if(condition_Led[5] == 1 && condition_Led[6] == 1){   
    if(condiRect[5] == 1){
      virtualDisp->fillRect(155-1,33,L_Rect_Red,T_Rect_Red,myRED);
      virtualDisp->fillRect(XR5,YR5-11,L_Rect,T_Rect,myBLACK);
      condiRect[5] = 2;
      con_black[5] = 1;
    }
  }else{
    if(con_black[5] == 1){
        virtualDisp->fillRect(155-1,33,L_Rect_Red,T_Rect_Red,myBLACK);
        virtualDisp->fillRect(XR5,YR5-11,L_Rect,T_Rect,myBLACK);
        con_black[5] = 2;
        condiRect[5] = 1;
    }
  }

  //============END============//
        // if(Serial.available() > 0){
        //   dma_display->clearScreen();
        //   condiRect[1]=1;
        //   condiRect[2]=1;
        //   condiRect[3]=1;
        //   condiRect[4]=1;
        //   condiRect[5]=1;
        //   str = Serial.readStringUntil('\n');
          // centerText(receivedText1, 192);
        //   Serial.println(str);  
        // } 

  virtualDisp->setCursor(XR1, YR1);  
  virtualDisp->setTextColor(dma_display->color565(0, 255, 0));
  virtualDisp->println("YCM");

  virtualDisp->setCursor(XR2, YR2);  
  virtualDisp->setTextColor(dma_display->color565(0, 255, 0));
  virtualDisp->println("TWS");

  virtualDisp->setCursor(XR3, YR3);  
  virtualDisp->setTextColor(dma_display->color565(0, 255, 0));
  virtualDisp->println("BON");

  virtualDisp->setCursor(XR4, YR4);  
  virtualDisp->setTextColor(dma_display->color565(0, 255, 0));
  virtualDisp->println("SLD");

  virtualDisp->setCursor(XR5, YR5);  
  virtualDisp->setTextColor(dma_display->color565(0, 255, 0));
  virtualDisp->println("RAY");
        
  //============GARIS PEMBATAS============//
  virtualDisp->fillRect(0,31,2,33,myWHITE);
  virtualDisp->fillRect(39-1,31,2,33,myWHITE);
  virtualDisp->fillRect(77-1,31,2,33,myWHITE);
  virtualDisp->fillRect(115-1,31,2,33,myWHITE);
  virtualDisp->fillRect(153-1,31,2,33,myWHITE);
  virtualDisp->fillRect(190,31,2,33,myWHITE);
  
  virtualDisp->fillRect(0,31,192,2,myWHITE);
  virtualDisp->fillRect(0,62,192,2,myWHITE);
  //---------------------------------------
}
//________________________________________________________________________________
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
