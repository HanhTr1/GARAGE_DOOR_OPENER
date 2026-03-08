#pragma once
#include "Led.h"
#include "door/Door.h"
#include <cstdint>


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

    void update(DoorState doorState,
                CalibrationState calibrationState,
                ErrorState errorState);

private:
    void updateCalibrationLed(CalibrationState state, uint32_t now);
    void updateDoorLed(DoorState state, uint32_t now);
    void updateErrorLed(ErrorState state, uint32_t now);

    bool shouldToggle(uint32_t now, uint32_t& lastToggle, uint32_t intervalMs);

    Led& led0_;
    Led& led1_;
    Led& led2_;

    uint32_t lastCalToggle_;
    uint32_t lastDoorToggle_;
    uint32_t lastErrorToggle_;
};