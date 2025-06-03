#include <Arduino.h>
#include <FatUSBMSC.h>
#include <USB.h>

FatUSBMSC fatMSC;

void setup() {
    Serial.begin(115200);

    // Provide first available FAT partition as USB drive
    if (!fatMSC.begin()) {
        Serial.println("Failed to mount fat partition!");
    }

    USB.begin();
}

void loop() {
}