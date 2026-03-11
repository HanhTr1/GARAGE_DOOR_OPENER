#pragma once
#include "pico/stdlib.h"
#include "pico/util/queue.h"

class Encoder
{
public:
    Encoder(uint pinA, uint pinB);
    void init();
    static int encoderSteps();

private:
    static void isr(uint gpio, uint32_t events);

    uint pinA_;
    uint pinB_;

    static Encoder* instance_;
    static queue_t eventQueue_;
};