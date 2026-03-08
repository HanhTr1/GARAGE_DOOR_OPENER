#pragma once

#include "../motor/StepperMotor.h"
#include "../encoder/Encoder.h"
#include "../limits/LimitSwitch.h"
#include "../eeprom/Eeprom.h"
#include "../network/GarageMQTT.h"
#include "../led/Led.h"


class StatusLeds;

enum class DoorState
{
    OPEN,
    CLOSED,
    OPENING,
    CLOSING,
    STOPPED,
    IN_BETWEEN
};

enum class ErrorState
{
    NORMAL,
    DOOR_STUCK
};

enum class CalibrationState
{
    CALIBRATED,
    CALIBRATING,
    NOT_CALIBRATED
};

enum class Direction
{
    UP,
    DOWN,
    NONE
};

class Door
{
public:
    Door(StepperMotor& motor,
         Encoder& encoder,
         LimitSwitch& topLimit,
         LimitSwitch& bottomLimit,
         Eeprom& eeprom,
         ///GarageMQTT& mqtt,
         StatusLeds& statusLeds);

    void init();
    void update();

    void onSw1Pressed();
    void onCalibrationRequested();
    void onRemoteCommand(const char* command);

    DoorState getDoorState() const;
    ErrorState getErrorState() const;
    CalibrationState getCalibrationState() const;
    Direction getDirection() const;

private:
    void updateLeds();
    void publishStatusIfNeeded();

    void startOpening();
    void startClosing();
    void stopDoor();

    void updateMovement();
    void detectStuck();
    void handleCalibration();
    StepperMotor& motor_;
    Encoder& encoder_;
    LimitSwitch& topLimit_;
    LimitSwitch& bottomLimit_;
    Eeprom& eeprom_;
    //GarageMQTT& mqtt_;
    StatusLeds& statusLeds_;

    DoorState doorState_;
    ErrorState errorState_;
    CalibrationState calibrationState_;
    Direction direction_;

    int currentStepPosition_;
    int totalTravelSteps_;

    int commandedStepsSinceEncoderMove_;
    int encoderDeltaAccumulator_;

    bool statusDirty_;
    enum class CalibrationPhase
    {
        IDLE,
        SEEK_TOP,
        SEEK_BOTTOM,
        MEASURE_UP,
        COMPLETE,
        FAILED
    };
    CalibrationPhase calibrationPhase_;

    int calibrationStartPosition_;
    int calibrationMeasuredSteps_;
    int motorSinceEncoder_;
    int expectedRatio_;
};
