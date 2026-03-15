#pragma once
#include "pico/stdlib.h"

class LimitSwitch
{
public:
    explicit LimitSwitch(uint pin);
    void init() const;
    [[nodiscard]] bool isPressed() const;

private:
    uint pin_;
};