#include "music_mode.h"

MusicMode::MusicMode() : isActive(false)
{
}

void MusicMode::begin()
{
    Serial.println("[MusicMode] Initialized");
}

void MusicMode::setActive(bool active)
{
    isActive = active;
    if (active)
    {
        Serial.println("[MusicMode] Activated - White light");
    }
    else
    {
        Serial.println("[MusicMode] Deactivated");
    }
}

void MusicMode::getRGB(int &r, int &g, int &b)
{
    r = 255;
    g = 255;
    b = 255;
}

void MusicMode::loop()
{
}
