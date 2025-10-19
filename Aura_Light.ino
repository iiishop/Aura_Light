#include "wifi_manager.h"
#include "geography.h"

void setup()
{
  // Initialize serial communication
  Serial.begin(9600);

  // Wait for serial port to connect (useful for debugging)
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("    Aura Light System Starting...     ");
  Serial.println("========================================\n");

  // Connect to WiFi
  setupWiFi();

  Serial.println("[System] Initialization complete!");
  Serial.println("[System] System ready.\n");
  // Get current city
  String city = getCurrentCity();
  Serial.println("========================================");
  Serial.print("[System] Current city: ");
  Serial.println(city);
  Serial.println("========================================\n");
}

void loop()
{
  // put your main code here, to run repeatedly:
}
