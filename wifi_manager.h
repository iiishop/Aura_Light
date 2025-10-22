#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFiNINA.h>

void setupWiFi();
bool checkWiFiConnection();
void reconnectWiFi();

#endif
