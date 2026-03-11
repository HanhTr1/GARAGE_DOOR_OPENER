#include "pico/stdlib.h"
#include "door/Door.h"
#include "motor/StepperMotor.h"
#include "limits/LimitSwitch.h"
#include "encoder/Encoder.h"
#include "input/Button.h"
#include "network/GarageMQTT.h"
#include "eeprom/Eeprom.h"
#include "led/Led.h"
int main()
{
    stdio_init_all();

    Eeprom::init();

    StepperMotor motor(2, 3, 6, 13);
    Encoder encoder(4, 5);
    LimitSwitch topLimit(27);
    LimitSwitch bottomLimit(28);
    Button buttons(9, 8, 7);

    Led led0(20);
    Led led1(21);
    Led led2(22);
    StatusLeds statusLeds(led0, led1, led2);

    GarageMQTT mqtt("MP-IOT",
                    "ID2vOcYrWi",
                    "10.161.6.53",
                    1883,
                    "garage-door-opener");

    motor.init();
    encoder.init();
    topLimit.init();
    bottomLimit.init();
    buttons.init();
    led0.init();
    led1.init();
    led2.init();

    const bool mqttConnected = mqtt.connect();
    printf("Initial MQTT connection: %s\n", mqttConnected ? "OK" : "FAILED");

    Door door(motor, topLimit, bottomLimit, mqtt, statusLeds);
    door.init();

    while (true)
    {
        const bool isCalibrating =
            (door.getCalibrationState() == CalibrationState::CALIBRATING);

        if (!isCalibrating)
        {
            if (buttons.sw1Pressed())
            {
                door.onSw1Pressed();
            }

            if (buttons.sw0AndSw2Pressed())
            {
                door.onCalibrationRequested();
            }
        }

        char cmd[64];
        if (mqtt.hasPendingCommand() && mqtt.popPendingCommand(cmd, sizeof(cmd)))
        {
            door.onRemoteCommand(cmd);
        }
        door.update();
        mqtt.loop();

        sleep_ms(1);
    }
}