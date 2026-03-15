#include "pico/stdlib.h"
#include "door/Door.h"
#include "motor/StepperMotor.h"
#include "limitSwitch/LimitSwitch.h"
#include "encoder/Encoder.h"
#include "input/Button.h"
#include "mqtt/GarageMQTT.h"
#include "eeprom/Eeprom.h"
#include "led/Led.h"

constexpr uint MOTOR1 = 2;
constexpr uint MOTOR2 = 3;
constexpr uint MOTOR3 = 6;
constexpr uint MOTOR4 = 13;

constexpr uint ENC_A = 4;
constexpr uint ENC_B = 5;

constexpr uint LIMIT_TOP = 27;
constexpr uint LIMIT_BOTTOM = 28;

constexpr uint SW0 = 9;
constexpr uint SW1 = 8;
constexpr uint SW2 = 7;

constexpr uint LED0 = 20;
constexpr uint LED1 = 21;
constexpr uint LED2 = 22;

int main()
{
    stdio_init_all();

    Eeprom::init();

    StepperMotor motor(MOTOR1, MOTOR2, MOTOR3, MOTOR4);
    Encoder encoder(ENC_A, ENC_B);
    LimitSwitch topLimit(LIMIT_TOP);
    LimitSwitch bottomLimit(LIMIT_BOTTOM);
    Button buttons(SW0, SW1, SW2);

    Led led0(LED0);
    Led led1(LED1);
    Led led2(LED2);
    StatusLeds statusLeds(led0, led1, led2);

    GarageMQTT mqtt("SSID",
                    "PASSWORD",
                    "IP",
                    1883,
                    "garage-door");

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