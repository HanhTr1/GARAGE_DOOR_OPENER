#include "calibration.h"


Calibration::Calibration() {
    current_step = 0;
    average_steps = 0;
    calibrated = false;
}

void Calibration::init() {

    for (int stepper_pin : stepper_pins) {
        gpio_init(stepper_pin);
        gpio_set_dir(stepper_pin, GPIO_OUT);
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

}

void Calibration::set_coils(int step) {
    for (int i=0;i<PIN_AMOUNT;i++)
        gpio_put(stepper_pins[i], sequences[step][i]);
}


queue_t Calibration::encoder_queue;
int Calibration::encoder_steps = 0;
int Calibration::motor_step = 0;

void Calibration::encoder_isr(uint gpio, uint32_t events) {
    if (gpio == ROT_A)
    {
        int direction;
        if (gpio_get(ROT_B) == 0)
        {
            direction = 1;
        }else
        {
            direction = -1;
        }
        queue_try_add(&encoder_queue, &direction);
    }
}

void Calibration::step_motor(int direction) {
    if (direction==1)
    {
        current_step=(current_step+1)%SEQUENCES_AMOUNT;
        motor_step++;
        motor_since_encoder++;
    }
    if (direction==-1)
    {
        current_step=(current_step-1+SEQUENCES_AMOUNT) % SEQUENCES_AMOUNT;
        motor_step++;
        motor_since_encoder++;
    }
    if (direction != 1 && direction != -1)
    {
        return;
    }
    set_coils(current_step);
}

bool Calibration::door_stuck()
{
    if (motor_since_encoder > expected_ratio * 4)
    {
        cout << "error: Door stuck detected!" << endl;
        set_coils(0);
        calibrated =false; //just added
        return true;
    }
    return false;
}

int Calibration::motor_since_encoder = 0;
int Calibration::expected_ratio = 204;
void Calibration::calibration_process() {

    cout << "Press SW0 + SW2 to calibration\n";
    while (gpio_get(SW0) != 0 || gpio_get(SW2) != 0) {
    }

    while (!calibrated)
    {
        queue_init(&encoder_queue, sizeof(int), 100);
        gpio_set_irq_enabled_with_callback(ROT_A, GPIO_IRQ_EDGE_RISE, true, &encoder_isr);
        cout << "Finding the top limit...\n";
        while (gpio_get(LIMIT_TOP) != 0) {
            step_motor(1);
            sleep_ms(1);
        }
        cout <<"Door is now at TOP. Starting calibration...\n";

        encoder_steps = 0;
        int counts[CALIB_TIMES];
        for (int i = 0; i < CALIB_TIMES; i++) {
            cout << "Round " << i+1 << "..\n";
            counts[i]=0;
            int down_steps = 0;
            int up_steps = 0;
            encoder_steps = 0;
            motor_step = 0;
            while (gpio_get(LIMIT_BOTTOM) != 0) {
                step_motor(-1);
                sleep_ms(1);
                int q_step = 0; //set =0 to correct round 1 downsteps
                while (queue_try_remove(&encoder_queue, &q_step))
                {
                    encoder_steps += abs(q_step);
                    motor_since_encoder = 0;
                }
                if (door_stuck()) {
                    return;
                }

            }
            down_steps = encoder_steps;
            cout << "Down steps (encoder): " << encoder_steps << " steps\n";

            encoder_steps = 0;

            while (gpio_get(LIMIT_TOP) != 0)
            {
                step_motor(1);
                sleep_ms(1);
                int q_step = 0; //just added =0
                while (queue_try_remove(&encoder_queue, &q_step))
                {
                    encoder_steps+= abs(q_step);
                    motor_since_encoder = 0;
                }
                if (door_stuck()) {
                    return;
                }
            }

            up_steps = encoder_steps;
            cout << "UP steps (encoder): " << encoder_steps << " steps\n";

            counts[i] = (down_steps + up_steps)/2;

            cout << "Total steps : " << counts[i] << endl;
            cout << "Motor steps: "<< motor_step << endl;
        }

        int sum = 0;
        for (int count : counts) {
            sum += count;
        }
        average_steps = sum / CALIB_TIMES;
        expected_ratio = motor_step /average_steps;
        calibrated = true;
        cout << "Calibration finished.\n";
        cout << "Average encoder steps: " << average_steps << endl;
    }
}

