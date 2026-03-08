#include "Led.h"
#include "pico/stdlib.h"

Led::Led(uint pin) : pin_(pin), state_(false) {}

void Led::init() {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_OUT);
    off();
}

void Led::on() {
    state_ = true;
    gpio_put(pin_, true);
}

void Led::off() {
    state_ = false;
    gpio_put(pin_, false);
}

void Led::toggle() {
    state_ = !state_;
    gpio_put(pin_, state_);
}

bool Led::isOn() const {
    return state_;
}

static constexpr uint32_t SLOW_BLINK_MS = 500;
static constexpr uint32_t MEDIUM_BLINK_MS = 250;
static constexpr uint32_t FAST_BLINK_MS = 120;

StatusLeds::StatusLeds(Led& led0, Led& led1, Led& led2)
    : led0_(led0),
      led1_(led1),
      led2_(led2),
      lastCalToggle_(0),
      lastDoorToggle_(0),
      lastErrorToggle_(0) {}

bool StatusLeds::shouldToggle(uint32_t now, uint32_t& lastToggle, uint32_t intervalMs) {
    if (now - lastToggle >= intervalMs) {
        lastToggle = now;
        return true;
    }
    return false;
}

void StatusLeds::update(DoorState doorState,
                        CalibrationState calibrationState,
                        ErrorState errorState) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    updateCalibrationLed(calibrationState, now);
    updateDoorLed(doorState, now);
    updateErrorLed(errorState, now);
}

void StatusLeds::updateCalibrationLed(CalibrationState state, uint32_t now) {
    switch (state) {
        case CalibrationState::CALIBRATED:
            led0_.on();
            break;

        case CalibrationState::NOT_CALIBRATED:
            led0_.off();
            break;

        case CalibrationState::CALIBRATING:
            if (shouldToggle(now, lastCalToggle_, SLOW_BLINK_MS)) {
                led0_.toggle();
            }
            break;
    }
}

void StatusLeds::updateDoorLed(DoorState state, uint32_t now) {
    switch (state) {
        case DoorState::OPEN:
            led1_.on();
            break;

        case DoorState::CLOSED:
            led1_.off();
            break;

        case DoorState::OPENING:
        case DoorState::CLOSING:
            if (shouldToggle(now, lastDoorToggle_, SLOW_BLINK_MS)) {
                led1_.toggle();
            }
            break;

        case DoorState::STOPPED:
        case DoorState::IN_BETWEEN:
            if (shouldToggle(now, lastDoorToggle_, MEDIUM_BLINK_MS)) {
                led1_.toggle();
            }
            break;
    }
}

void StatusLeds::updateErrorLed(ErrorState state, uint32_t now) {
    switch (state) {
        case ErrorState::NORMAL:
            led2_.off();
            break;

        case ErrorState::DOOR_STUCK:
            if (shouldToggle(now, lastErrorToggle_, FAST_BLINK_MS)) {
                led2_.toggle();
            }
            break;
    }
}