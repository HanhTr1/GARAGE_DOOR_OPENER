//
// Created by Tran Lan Anh on 25.2.2026.
//
#ifndef GARAGEDOOR_CALIBRATION_H
#define GARAGEDOOR_CALIBRATION_H

#include "hardware/gpio.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "iostream"
#include "pico/util/queue.h"
#include <cstdint>

using namespace std;


#define SW0 9
#define SW1 8
#define SW2 7

#define A_PIN 13
#define B_PIN 6
#define C_PIN 3
#define D_PIN 2

#define ROT_A 4
#define ROT_B 5

#define LIMIT_TOP 27
#define LIMIT_BOTTOM 28

#define PIN_AMOUNT 4
#define SEQUENCES_AMOUNT 8
#define CALIB_TIMES 3
#define MAX_STEPS 18
#define SMALL_DEBOUNCE_TIME 2
#define MAX_QUEUE_SIZE 100

class Calibration {
public:
    Calibration();
    void init();
    void calibration_process();
    void step_motor(int direction);
    void set_coils(int step);
    static int encoder_steps;
    static int motor_step;
    static int step_counts;
    static int motor_since_encoder;
    static int expected_ratio;
    static void encoder_isr(uint gpio, uint32_t events);

private:
    int current_step;
    int average_steps;
    bool calibrated;

    const uint stepper_pins[PIN_AMOUNT] = {A_PIN, B_PIN, C_PIN, D_PIN};
    const int sequences[SEQUENCES_AMOUNT][PIN_AMOUNT] = {
        {1,0,0,0},
        {1,1,0,0},
        {0,1,0,0},
        {0,1,1,0},
        {0,0,1,0},
        {0,0,1,1},
        {0,0,0,1},
        {1,0,0,1}
    };
    static queue_t encoder_queue;
};

#endif