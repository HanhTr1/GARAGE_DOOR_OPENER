//
// Created by Tran Lan Anh on 8.3.2026.
//

#include "button.h"
void Button::init()
{
    gpio_init(SW0);
    gpio_set_dir(SW0, GPIO_IN);
    gpio_pull_up(SW0);

    gpio_init(SW1);
    gpio_set_dir(SW1, GPIO_IN);
    gpio_pull_up(SW1);

    gpio_init(SW2);
    gpio_set_dir(SW2, GPIO_IN);
    gpio_pull_up(SW2);

    lastSw0 = false;
    lastSw1 = false;
    lastSw2 = false;
}

bool Button::Sw1Pressed()
{
    bool current;
    if (gpio_get(SW1)==0)
    {
        current = true;
    }else
    {
        current = false;
    }

    bool pressed;
    if (current == true && lastSw1==false)
    {
        pressed = true;
    }else
    {
        pressed = false;
    }
    lastSw1 = current;
    return pressed;
}

bool Button::Sw0_Sw2Pressed()
{
    bool sw0 = (gpio_get(SW0)==0);
    bool sw2 = (gpio_get(SW2)==0);
    bool pressed;
    if (sw0 && sw2)
    {
        if (lastSw0==true && lastSw2==true)
        {
            pressed = false;
        }else
        {
            pressed = true;
        }
    }else
    {
        pressed = false;
    }
    lastSw0 = sw0;
    lastSw2 = sw2;
    return pressed;
}
