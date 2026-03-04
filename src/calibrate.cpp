//
// Created by Tran Lan Anh on 4.3.2026.
//
#include "calibration.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"

Calibration::Calibration() {
    current_step = 0;
    average_steps = 0;
    calibrated = false;
    previous_state = 0;
}

void Calibration::init() {
    for (int i=0;i<PIN_AMOUNT;i++) {
        gpio_init(stepper_pins[i]);
        gpio_set_dir(stepper_pins[i], GPIO_OUT);
    }

    gpio_init(ROT_A);
    gpio_set_dir(ROT_A, GPIO_IN);
    gpio_init(ROT_B);
    gpio_set_dir(ROT_B, GPIO_IN);

    gpio_init(LIMIT_TOP);
    gpio_set_dir(LIMIT_TOP, GPIO_IN);
    gpio_pull_up(LIMIT_TOP);

    gpio_init(LIMIT_BOTTOM);
    gpio_set_dir(LIMIT_BOTTOM, GPIO_IN);
    gpio_pull_up(LIMIT_BOTTOM);

    gpio_init(SW0);
    gpio_set_dir(SW0, GPIO_IN);
    gpio_pull_up(SW0);

    gpio_init(SW2);
    gpio_set_dir(SW2, GPIO_IN);
    gpio_pull_up(SW2);
}

void Calibration::set_coils(int step) {
    for (int i=0;i<PIN_AMOUNT;i++)
        gpio_put(stepper_pins[i], sequences[step][i]);
}

void Calibration::step_motor(int direction) {
    if (direction==1)
        current_step=(current_step+1)%SEQUENCES_AMOUNT;
    else if (direction==-1)
        current_step=(current_step-1+SEQUENCES_AMOUNT)%SEQUENCES_AMOUNT;
    else return;

    set_coils(current_step);
    sleep_ms(SMALL_DEBOUNCE_TIME);
}

void Calibration::calibration_process() {
    if (gpio_get(SW0) || gpio_get(SW2)) {
        printf("Press SW0 + SW2 to start calibration\n");
        return;
    }

    printf("Calibration started...\n");
    int step_count=0;

    while (gpio_get(LIMIT_TOP) != 0 && step_count < MAX_STEPS) {
        step_motor(1);
        step_count++;
    }

    if (step_count >= MAX_STEPS) {
        printf("Error: top limit not detected\n");
        return;
    }

    printf("Top limit reached\n");

    int counts[CALIB_TIMES];

    for (int i=0;i<CALIB_TIMES;i++) {
        step_count=0;
        while (gpio_get(LIMIT_BOTTOM)!=0 && step_count<MAX_STEPS) {
            step_motor(-1);
            step_count++;
        }

        if (step_count>=MAX_STEPS){
            printf("Error: bottom limit not detected\n");
            return;
        }

        counts[i]=step_count;
        printf("Round %d: %d steps\n", i+1, step_count);

        while (gpio_get(LIMIT_TOP)!=0)
            step_motor(1);
    }

    average_steps=(counts[0]+counts[1]+counts[2])/3;

    for (int i=0;i<20;i++)
        step_motor(1);

    calibrated=true;
    printf("Calibration finished\n");
}

int Calibration::get_average_steps() { return average_steps; }
bool Calibration::is_calibrated() { return calibrated; }