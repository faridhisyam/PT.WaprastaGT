#include <USBHIDKeyboard.h>
#include <SD.h>
#include <SPI.h>
#include <USB.h>

const int myBACA = 4;       // Pin tombol
int lastButtonState = HIGH;   // Untuk deteksi edge (perubahan)
int currentButtonState = LOW;
  
const int maxFiles = 20;     // Maksimal file yang akan diproses
String fileNames[maxFiles];
int fileCount = 0;
int currentFileIndex = 0;    // Indeks file yang sedang aktif

USBHIDKeyboard Keyboard;

void initSerial() {
  Serial1.begin(115200, SERIAL_8N1, 44, 43);  //RX, TX 
 } 

void setup() {
  Serial.begin(115200);
  initSerial();
  pinMode(myBACA, INPUT_PULLDOWN);

  USB.begin();
  delay(2000); // Tunggu inisialisasi USB
  Keyboard.begin();
  delay(2000); // Tunggu inisialisasi USB

  // Inisialisasi SD Card
  SPI.begin(36, 37, 35, 39);
  if (!SD.begin(39, SPI, 200000000)) {
    Serial.println("Inisialisasi SD Card gagal!");
    return;
  } else {
    Serial.println("SD Card berhasil diinisialisasi.");
  }

  // Buka folder "CB100"
  File folder = SD.open("/CB100");
  if (!folder) {
    Serial.println("Folder CB100 tidak ditemukan!");
    return;
  }
  
  // Kumpulkan nama file .txt dari folder "CB100"
  while (true) {
    File entry = folder.openNextFile();
    if (!entry) break; // tidak ada file lagi
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith(".txt")) {
        fileNames[fileCount++] = name;
        Serial.print("Ditemukan file: ");
        Serial.println(name);
      }
    }
    entry.close();
  }
  folder.close();

  if (fileCount == 0) {
    Serial.println("Tidak ada file .txt di folder CB100.");
  }
}

void loop() {
  // Deteksi perubahan tombol (edge detection)
  currentButtonState = digitalRead(myBACA);
  if (currentButtonState == HIGH && lastButtonState == LOW) {
    delay(50);  // debounce
    processCurrentFileOneLine();
  }
  lastButtonState = currentButtonState;
}

void processCurrentFileOneLine() {
  // Jika sudah tidak ada file yang tersisa, hentikan proses
  if (currentFileIndex >= fileCount) {
    Serial.println("Tidak ada file lagi untuk diproses.");
    return;
  }
  
  // Bentuk path lengkap file, misal: "/CB100/namaFile.txt"
  String fullPath = String("/CB100/") + fileNames[currentFileIndex];
  Serial.print("Memproses file: ");
  Serial.println(fullPath);

  if (!SD.exists(fullPath.c_str())) {
    Serial.print("File tidak ditemukan: ");
    Serial.println(fullPath);
    return;
  }
  
  // Buka file untuk membaca
  File file = SD.open(fullPath.c_str(), FILE_READ);
  if (!file) {
    Serial.print("Gagal membuka file: ");
    Serial.println(fullPath);
    return;
  }
  
  // Jika file kosong, hapus file dan pindah ke file berikutnya
  if (file.size() == 0) {
    file.close();
    Serial.print("File kosong: ");
    Serial.println(fullPath);
    SD.remove(fullPath.c_str());
    currentFileIndex++;
    return;
  }
  
  // Baca baris pertama
  String line = file.readStringUntil('\n');
  file.close();

  // Kirim baris ke keyboard
  Keyboard.print(line);
  Keyboard.write(KEY_RETURN);
  Serial.print("Diketik: ");
  Serial.println(line);
  Serial1.write(line.c_str());  

  // Buka ulang file untuk membuat versi baru tanpa baris yang sudah dibaca
  File original = SD.open(fullPath.c_str(), FILE_READ);
  if (!original) {
    Serial.print("Gagal membuka file untuk update: ");
    Serial.println(fullPath);
    return;
  }
  
  // Buat file temporary di folder yang sama
  String tempPath = String("/CB100/") + "temp.txt";
  File tempFile = SD.open(tempPath.c_str(), FILE_WRITE);
  if (!tempFile) {
    Serial.println("Gagal membuat file temporary!");
    original.close();
    return;
  }
  
  // Lewati baris pertama (yang telah dibaca) dan salin sisa baris ke file temporary
  bool skipFirst = true;
  while (original.available()) {
    String currentLine = original.readStringUntil('\n');
    if (skipFirst) {
      skipFirst = false;
      continue;
    }
    tempFile.println(currentLine);
  }
  original.close();
  tempFile.close();
  
  // Hapus file asli dan ganti dengan file temporary
  if (!SD.remove(fullPath.c_str())) {
    Serial.print("Gagal menghapus file: ");
    Serial.println(fullPath);
    return;
  }
  if (!SD.rename(tempPath.c_str(), fullPath.c_str())) {
    Serial.print("Gagal mengganti nama file temporary: ");
    Serial.println(fullPath);
    return;
  }
  
  // Cek apakah file sekarang kosong, jika ya, pindah ke file berikutnya
  File checkFile = SD.open(fullPath.c_str(), FILE_READ);
  if (checkFile) {
    if (checkFile.size() == 0) {
      Serial.print("File telah kosong, pindah ke file berikutnya: ");
      Serial.println(fileNames[currentFileIndex]);
      checkFile.close();
      currentFileIndex++;
    } else {
      checkFile.close();
    }
  }
  delay(500);
}
