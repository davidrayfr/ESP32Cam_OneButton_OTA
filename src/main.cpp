// version du programme avec gestion de :
// OTA
// One Button
// EEPROM
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <wifikeys.h>
#include "eeprom_Sauv.h"
#include "OTA.h"

unsigned long previousMillis;

EEPROM_Data (memory){STRUCT_MAGIC,
                    "MaisonRay300",
                    "CamilleEmilie",
                    "123456",
                    "Esp32_Cam",
                    true,
                    true,
                    554
                    };

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  confOTA(memory.hostname,memory.ota_password);
}

void loop() {
  ArduinoOTA.handle();
}
