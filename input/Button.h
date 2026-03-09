#pragma once
#include "pico/stdlib.h"
#include <cstdint>

class Button
{
public:
    Button(uint sw0, uint sw1, uint sw2);
    void init();
    bool sw1Pressed();
    bool sw0AndSw2Pressed();

private:
    static bool detectPressEdge(uint pin, bool& lastState);

    uint sw0_;
    uint sw1_;
    uint sw2_;
    bool lastSw0_;
    bool lastSw1_;
    bool lastSw2_;
    uint32_t lastComboPressMs_;
};