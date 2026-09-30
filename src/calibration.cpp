#include "calibration.h"


Calibration::Calibration() {
    current_step = 0;
    average_steps = 0;
    calibrated = false;
    previous_state = 0;
}

void Calibration::init() {
    for (int stepper_pin : stepper_pins) {
        gpio_init(stepper_pin);
        gpio_set_dir(stepper_pin, GPIO_OUT);
    }

    gpio_init(ROT_A);
    gpio_set_dir(ROT_A, GPIO_IN);
    gpio_pull_up(ROT_A);

    gpio_init(ROT_B);
    gpio_set_dir(ROT_B, GPIO_IN);
    gpio_pull_up(ROT_B);

    gpio_init(LIMIT_TOP);
    gpio_set_dir(LIMIT_TOP, GPIO_IN);
    gpio_pull_up(LIMIT_TOP);

    gpio_init(LIMIT_BOTTOM);
    gpio_set_dir(LIMIT_BOTTOM, GPIO_IN);
    gpio_pull_up(LIMIT_BOTTOM);

    gpio_init(SW0);
    gpio_set_dir(SW0, GPIO_IN);
    gpio_pull_up(SW0);

    gpio_init(SW1);
    gpio_set_dir(SW1, GPIO_IN);
    gpio_pull_up(SW1);

    gpio_init(SW2);
    gpio_set_dir(SW2, GPIO_IN);
    gpio_pull_up(SW2);
}

void Calibration::set_coils(int step) {
    for (int i=0;i<PIN_AMOUNT;i++)
        gpio_put(stepper_pins[i], sequences[step][i]);
}


queue_t Calibration::step_queue;
int Calibration::encoder_steps = 0;
int Calibration::motor_step = 0;
void Calibration::encoder_isr(uint gpio, uint32_t events) {
    if (gpio == ROT_A)
    {
        int step_event;
        if (gpio_get(ROT_B) == 0)
        {
            step_event = 1;
        }else
        {
            step_event = -1;
        }
        queue_try_add(&step_queue, &step_event);
    }
}
void Calibration::update_encoder()
{
    int event;
    while (queue_try_remove(&step_queue, &event))
    {
        encoder_steps += event;
    }
}
void Calibration::step_motor(int direction) {
    if (direction==1)
        current_step=(current_step+1)%SEQUENCES_AMOUNT;
    else if (direction==-1)
        current_step=(current_step-1+SEQUENCES_AMOUNT) % SEQUENCES_AMOUNT;
    else return;

    set_coils(current_step);
    sleep_ms(SMALL_DEBOUNCE_TIME);
}

void Calibration::calibration_process() {

    if (gpio_get(SW0) != 0 && gpio_get(SW2) != 0) {
        return;
    }
    cout << "Press SW0 + SW2 to start calibration\n";

    cout << "Calibration start...\n";

    int step_count = 0;
    while (gpio_get(LIMIT_TOP) != 0 && step_count < MAX_STEPS) {
        step_motor(1);
        step_count++;
    }
    cout <<"Door is now at TOP. Starting calibration...\n";

    queue_init(&step_queue, sizeof(int), 100);
    gpio_set_irq_enabled_with_callback(ROT_A, GPIO_IRQ_EDGE_RISE, true, &encoder_isr);

    int counts[CALIB_TIMES];
    for (int i = 0; i < CALIB_TIMES; i++) {
        cout << "Round " << i+1 << "..\n";
        counts[i] = 0;

        queue_init(&step_queue, sizeof(int), 100);
        step_count = 0;
        while (gpio_get(LIMIT_BOTTOM) != 0 && step_count < MAX_STEPS) {
            step_motor(-1);
            step_count++;
            sleep_ms(10);

            int q_step;
            while (queue_try_remove(&step_queue, &q_step))
            {
                counts[i]++;
            }
        }
        cout << "Down steps (encoder): " << counts[i] << " steps\n";


        queue_init(&step_queue, sizeof(int), 100);
        step_count = 0;
        while (gpio_get(LIMIT_TOP) != 0 && step_count < MAX_STEPS)
        {
            step_motor(1);
            step_count++;
            int q_step;
            while (queue_try_remove(&step_queue, &q_step))
            {
                counts[i]++;
            }
        }
        cout << "UP steps (encoder): " << counts[i]<< " steps\n";
    }

    int sum = 0;
    for (int count : counts) {
        sum += count;
    }
    average_steps = sum / CALIB_TIMES;
    calibrated = true;
    cout << "Calibration finished.\n";
    cout << "Average encoder steps: " << average_steps << endl;
}

