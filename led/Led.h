#pragma once
#include "pico/stdlib.h"
#include <cstdint>
#include "../door/DoorState.h"

class Led {
public:
    explicit Led(uint pin);
    void init();
    void on();
    void off();
    void toggle();
    [[nodiscard]] bool isOn() const;

private:
    uint pin_;
    bool state_;
};

class StatusLeds {
public:
    StatusLeds(Led& led0, Led& led1, Led& led2);

    void update(CalibrationState calibrationState,
                ErrorState errorState);

private:
    static bool shouldToggle(uint32_t now, uint32_t& lastToggle, uint32_t intervalMs);
    void updateCalibrationLed(CalibrationState state, uint32_t now);
    void updateErrorLed(ErrorState state, uint32_t now);

    Led& led0_;
    Led& led1_;
    Led& led2_;
    uint32_t lastErrorToggle_{};
};