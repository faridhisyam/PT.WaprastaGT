#define S1 33 
#define S2 32
#define S3 35
#define S4 34
#define S5 39
#define S6 36

#define R1_PIN 19
#define G1_PIN 13
#define B1_PIN 18
#define R2_PIN 5
#define G2_PIN 12
#define B2_PIN 17

#define A_PIN 16
#define B_PIN 14
#define C_PIN 4
#define D_PIN 27
#define E_PIN -1  

#define LAT_PIN 23  //26 | 23
#define OE_PIN 15
#define CLK_PIN 2*1//25 //2*1

//----------------------------------------Defines the P5 Panel configuration.
#define PANEL_RES_X 64  //--> Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 32  //--> Number of pixels tall of each INDIVIDUAL panel module.
#define NUM_ROWS 2 // Number of rows of chained INDIVIDUAL PANELS
#define NUM_COLS 3 // Number of INDIVIDUAL PANELS per ROW
#define PANEL_CHAIN NUM_ROWS*NUM_COLS    // total number of panels chained one to another 
#define VIRTUAL_MATRIX_CHAIN_TYPE2 CHAIN_TOP_LEFT_DOWN
#define VIRTUAL_MATRIX_CHAIN_TYPE CHAIN_TOP_RIGHT_DOWN
//----------------------------------------

// Konfigurasi Access Point
const char* ssid = "Display_1";
const char* password = "12345678";

const String uniqueCode = "1"; // Kode unik ESP32
String receivedText1 = "";
String receivedText2 = "";
String receivedText3 = "";
String receivedText4 = "";
String receivedOption = "";
String str;
// String code;

int L_Rect_Red = 36 , T_Rect_Red = 29;//40
int L_Rect = 34, T_Rect = 13;//40
int flagclean = 0;

int condiRect[11]={1,1,1,1,1,1,1,1,1,1,1};
int condition_Led[9];
int con_black[11]={1,1,1,1,1,1,1,1,1,1,1};

int XR1 = 4;//10-4-2;
int YR1 = 52;//44-7+15;

int XR2 = 42;//50-7-1;
int YR2 = 52;//44-7+15;

int XR3 = 79;//89-7-2-1;
int YR3 = 52;//44-7+15;

int XR4 = 118;//126-7-1;
int YR4 = 52;//44-7+15;

int XR5 = 155;//164-7-1-1;
int YR5 = 52;//44-7+15;

int XRC = 58;
int YRC = 20;
//----------------------------------------

unsigned long previousMillis = 0;  
const unsigned long interval = 1500;  //Cepat lambat ganti teks
int currentStep = 0; 

uint8_t myDATA = 0;
uint8_t receivedData = 0;
uint8_t mydata = 0;

// =============================COBA 
// int16_t xYCM, yYCM;
// uint16_t wYCM, hYCM;