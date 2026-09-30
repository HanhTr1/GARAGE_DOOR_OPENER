//
// Created by Tran Lan Anh on 8.3.2026.
//

#ifndef GARAGE_DOOR_OPENER_BUTTON_CPP_H
#define GARAGE_DOOR_OPENER_BUTTON_CPP_H

#include <iostream>
#include "hardware/gpio.h"
using namespace std;

#define SW0 9
#define SW1 8
#define SW2 7

class Button
{
    public:
    void init();
    bool Sw1Pressed();
    bool Sw0_Sw2Pressed();

    private:
    bool lastSw1;
    bool lastSw0;
    bool lastSw2;
};

#endif //GARAGE_DOOR_OPENER_BUTTON_CPP_H