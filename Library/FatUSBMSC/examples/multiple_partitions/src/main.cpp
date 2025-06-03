#include <Arduino.h>
#include <FatUSBMSC.h>
#include <USB.h>

FatUSBMSC fat0MSC;
FatUSBMSC fat1MSC;

void setup() {
    Serial.begin(115200);

    // Provide FAT partition with label "fat0" as 1st USB drive
    if (!fat0MSC.begin("fat0")) {
        Serial.println("FAT partition \"fat0\" not found!");
    }; 

    // Provide FAT partition with label "fat1" as 2nd USB drive
    if (!fat1MSC.begin("fat1")) {
        Serial.println("FAT partition \"fat1\" not found!");
    }; 

    USB.begin();
}

void loop() {
}