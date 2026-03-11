#include "Led.h"
#include "pico/stdlib.h"

//static constexpr uint32_t SLOW_BLINK_MS = 500;
static constexpr uint32_t FAST_BLINK_MS = 200;

Led::Led(uint pin)
    : pin_(pin),
      state_(false)
{
}

void Led::init()
{
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_OUT);
    off();
}

void Led::on()
{
    state_ = true;
    gpio_put(pin_, true);
}

void Led::off()
{
    state_ = false;
    gpio_put(pin_, false);
}

void Led::toggle()
{
    state_ = !state_;
    gpio_put(pin_, state_);
}

bool Led::isOn() const
{
    return state_;
}

StatusLeds::StatusLeds(Led& led0, Led& led1, Led& led2)
    : led0_(led0),
      led1_(led1),
      led2_(led2),
      lastErrorToggle_(0)
      /*lastCalToggle_(0),
      lastDoorToggle_(0),*/

{
}

bool StatusLeds::shouldToggle(uint32_t now, uint32_t& lastToggle, uint32_t intervalMs)
{
    if (now - lastToggle >= intervalMs)
    {
        lastToggle = now;
        return true;
    }
    return false;
}

/*void StatusLeds::update(CalibrationState calibrationState,
                        ErrorState errorState)
{
    const uint32_t now = to_ms_since_boot(get_absolute_time());

    updateCalibrationLed(calibrationState, now);
    updateErrorLed(errorState, now);
}*/

void StatusLeds::updateCalibrationLed(CalibrationState state, uint32_t now)
{
    switch (state)
    {
        case CalibrationState::CALIBRATED:
            led0_.off();
            led1_.off();
            led2_.off();

            break;

        // case CalibrationState::NOT_CALIBRATED:
        //     led0_.off();
        //     led1_.off();
        //     led2_.off();
        //     break;

        case CalibrationState::CALIBRATING:
            led0_.on();
            led1_.on();
            led2_.on();
            break;
        default:
            led0_.off();
            led1_.off();
            led2_.off();
    }
    /*switch (state)
    {
        case CalibrationState::CALIBRATED:
            led0_.on();
            break;

        case CalibrationState::NOT_CALIBRATED:
            led0_.off();
            break;

        case CalibrationState::CALIBRATING:
            if (shouldToggle(now, lastCalToggle_, SLOW_BLINK_MS))
            {
                led0_.toggle();
            }
            break;
    }*/
}
void StatusLeds::update(CalibrationState calibrationState,
                        ErrorState errorState)
{
    const uint32_t now = to_ms_since_boot(get_absolute_time());

    if (errorState == ErrorState::DOOR_STUCK)
    {
        updateErrorLed(errorState, now);
        return;
    }

    updateCalibrationLed(calibrationState, now);
    updateErrorLed(errorState, now);
}
/*
void StatusLeds::updateDoorLed(DoorState state, uint32_t now)
{
    switch (state)
    {
        case DoorState::OPEN:
            led1_.on();
            break;

        case DoorState::CLOSED:
            led1_.off();
            break;

        case DoorState::OPENING:
        case DoorState::CLOSING:
            if (shouldToggle(now, lastDoorToggle_, SLOW_BLINK_MS))
            {
                led1_.toggle();
            }
            break;

        case DoorState::STOPPED:
            if (shouldToggle(now, lastDoorToggle_, MEDIUM_BLINK_MS))
            {
                led1_.toggle();
            }
            break;
    }
}*/

void StatusLeds::updateErrorLed(ErrorState state, uint32_t now)
{

    switch (state)
    {
        case ErrorState::NORMAL:
            break;

        case ErrorState::DOOR_STUCK:
            if (shouldToggle(now, lastErrorToggle_, FAST_BLINK_MS))
            {
                led0_.toggle();
                led1_.toggle();
                led2_.toggle();
            }
            break;
    }
}