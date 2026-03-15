#include "LimitSwitch.h"

LimitSwitch::LimitSwitch(uint pin): pin_(pin){}

void LimitSwitch::init() const{
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_IN);
    gpio_pull_up(pin_);
}

bool LimitSwitch::isPressed() const{
    return gpio_get(pin_) == 0;
}