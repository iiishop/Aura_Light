#ifndef MUSIC_MODE_H
#define MUSIC_MODE_H

#include <Arduino.h>

class MusicMode
{
private:
    bool isActive;

public:
    MusicMode();

    void begin();

    void setActive(bool active);
    bool getActive() const { return isActive; }

    void getRGB(int &r, int &g, int &b);

    void loop();
};

#endif
