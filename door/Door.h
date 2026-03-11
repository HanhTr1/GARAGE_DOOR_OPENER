#pragma once

#include "../motor/StepperMotor.h"
#include "../encoder/Encoder.h"
#include "../limits/LimitSwitch.h"
#include "../eeprom/Eeprom.h"
#include "../network/GarageMQTT.h"
#include "../led/Led.h"
#include "DoorState.h"
#include <cstdint>

class Door
{
public:
    Door(StepperMotor& motor,
         //Encoder& encoder,
         LimitSwitch& topLimit,
         LimitSwitch& bottomLimit,
         GarageMQTT& mqtt,
         StatusLeds& leds);

    void init();
    void update();

    void onSw1Pressed();

    void savePersistentData();

    void onCalibrationRequested();
    void onRemoteCommand(const char* command);

    [[nodiscard]] DoorState getDoorState() const;
    [[nodiscard]] ErrorState getErrorState() const;
    [[nodiscard]] CalibrationState getCalibrationState() const;
    //[[nodiscard]] Direction getDirection() const;

private:
    enum class CalibrationPhase
    {
        IDLE,
        FIND_TOP,
        GO_BOTTOM,
        GO_TOP,
        COMPLETE,
        FAILED
    };

    void startOpening();
    void startClosing();
    void stopDoor();

    void updateMovement();
    void updatePositionFromEncoder(int direction);
    void detectStuck();
    void handleCalibration();

    void stepTowardTop();
    void stepTowardBottom();

    void updateLeds();
    void publishStatusUpdate();
    void savePersistentState();
    void enterStuckError();

    StepperMotor& motor_;
    //Encoder& encoder_;
    LimitSwitch& topLimit_;
    LimitSwitch& bottomLimit_;
    GarageMQTT& mqtt_;
    StatusLeds& leds_;

    DoorState doorState_;
    ErrorState errorState_;
    CalibrationState calibrationState_;
    Direction direction_;
    Direction lastMotionDirection_;

    int currentPosition_;
    int totalTravelSteps_;

    int encoderTotalSteps_;
    int commandedStepsSinceEncoderMove_;
    int calibrationMeasuredEncoderSteps_;

    CalibrationPhase calibrationPhase_;
    bool stoppedByUser_;
    bool statusDirty_;
    uint32_t lastStepMs_;

    static constexpr uint32_t STEP_INTERVAL_MS = 1;
    static constexpr int STUCK_STEP_THRESHOLD = 800;
    static constexpr int CALIBRATION_STUCK_THRESHOLD = 800;
};