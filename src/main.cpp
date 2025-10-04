#include <Arduino.h>
#include <CC1101.h>

CC1101 radio;

void setup()
{
    // Initialize USB Serial
    Serial.begin(115200);
    Serial.println("=== ESP32-S3 Started! ===");
    Serial.println("Serial monitor is working!");
    Serial.println("Starting main loop...");
}

void loop()
{
    Serial.println("Hello world!");
    delay(1000);
}