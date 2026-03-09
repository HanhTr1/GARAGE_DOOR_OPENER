#pragma once

#include "../motor/StepperMotor.h"
#include "../limits/LimitSwitch.h"
#include "../network/GarageMQTT.h"
#include "../led/Led.h"
#include "DoorState.h"
#include <cstdint>

#include "eeprom/Eeprom.h"

class Door
{
public:
    Door(StepperMotor& motor,
         LimitSwitch& topLimit,
         LimitSwitch& bottomLimit,
         GarageMQTT& mqtt,
         Eeprom& eeprom,
         StatusLeds& leds);

    Door(StepperMotor &motor, LimitSwitch &topLimit, LimitSwitch &bottomLimit, GarageMQTT &mqtt, StatusLeds &leds);

    void init();
    void update();

    void onSw1Pressed();
    void onCalibrationRequested();

    void stopDoor();

    void onRemoteCommand(const char* command);

    [[nodiscard]] DoorState getDoorState() const;
    [[nodiscard]] ErrorState getErrorState() const;
    [[nodiscard]] CalibrationState getCalibrationState() const;

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

    void updateNormalMovement();
    void updateCalibration();
    void stepTowardTop();
    void stepTowardBottom();

    void resetCalibrationRoundData();
    void finishCalibrationSuccess();
    void enterStuckError();
    void updateLeds();
    void publishStatusIfNeeded();
    void savePersistentState();

    StepperMotor& motor_;
    LimitSwitch& topLimit_;
    LimitSwitch& bottomLimit_;
    GarageMQTT& mqtt_;
    StatusLeds& leds_;

    DoorState doorState_;
    ErrorState errorState_;
    CalibrationState calibrationState_;
    Direction direction_;
    Direction lastMotionDirection_;

    int currentStepIndex_;
    int currentPosition_;
    int totalTravelSteps_;

    int encoderDeltaAccumulator_;
    int motorSinceEncoder_;
    int motorStepCount_;
    int encoderSteps_;
    int roundCounts_[1];
    int calibrationRound_;
    int expectedRatio_;

    CalibrationPhase calibrationPhase_;

    bool statusDirty_;
    uint32_t lastStepMs_;

    static constexpr uint32_t NORMAL_STEP_INTERVAL_MS = 1;
    static constexpr uint32_t CALIBRATION_STEP_INTERVAL_MS = 3;
    static constexpr int DEFAULT_EXPECTED_RATIO = 204;
    static constexpr int CALIB_TIMES = 3;
};
