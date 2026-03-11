#pragma once
#include "pico/stdlib.h"

class StepperMotor
{
public:
    StepperMotor(uint pin1, uint pin2, uint pin3, uint pin4);
    void init();

    void applySequence(int index) const;

    void stepForward();
    void stepBackward();
    void stop();

private:
    void applySequence(int index);

    uint pins_[4];
    int sequenceIndex_;
    static const int sequence_[8][4];
};