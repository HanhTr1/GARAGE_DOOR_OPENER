#include "StepperMotor.h"

const int StepperMotor::sequence_[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

StepperMotor::StepperMotor(uint pin1, uint pin2, uint pin3, uint pin4)
    : pins_{pin1, pin2, pin3, pin4},
      sequenceIndex_(0)
{
}

void StepperMotor::init()
{
    for (uint pin : pins_)
    {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, false);
    }
}

void StepperMotor::applySequence(int index) {
    for (int i = 0; i < 4; ++i)
    {
        gpio_put(pins_[i], sequence_[index][i]);
    }
}

void StepperMotor::stepForward()
{
    sequenceIndex_ = (sequenceIndex_ + 1) % 8;
    applySequence(sequenceIndex_);
}

void StepperMotor::stepBackward()
{
    sequenceIndex_ = (sequenceIndex_ - 1 + 8) % 8;
    applySequence(sequenceIndex_);
}

void StepperMotor::stop()
{
    for (uint pin : pins_)
    {
        gpio_put(pin, false);
    }
}