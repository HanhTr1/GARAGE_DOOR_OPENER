#pragma once

#include "../motor/StepperMotor.h"
#include "../limitSwitch/LimitSwitch.h"
#include "../mqtt/GarageMQTT.h"
#include "../led/Led.h"
#include "calibration.h"
#include "DoorState.h"
#include <cstdint>

class Door
{
public:
    Door(StepperMotor& motor,
         LimitSwitch& topLimit,
         LimitSwitch& bottomLimit,
         GarageMQTT& mqtt,
         StatusLeds& leds);

    void init();
    void update();

    void onSw1Pressed();
    void onCalibrationRequested();
    void onRemoteCommand(const char* command);

    [[nodiscard]] DoorState getDoorState() const;
    [[nodiscard]] ErrorState getErrorState() const;
    [[nodiscard]] CalibrationState getCalibrationState() const;

private:
    void startOpening();
    void startClosing();
    void stopDoor();

    void updateMovement();
    void updatePositionFromEncoder(int direction);
    void detectStuck();

    void updateLeds();
    void publishStatusUpdate();
    void savePersistentState();
    void enterStuckError();

    StepperMotor& motor_;
    LimitSwitch& topLimit_;
    LimitSwitch& bottomLimit_;
    GarageMQTT& mqtt_;
    StatusLeds& leds_;
    CalibrationManager calibration_;

    DoorState doorState_;
    ErrorState errorState_;
    CalibrationState calibrationState_;
    Direction direction_;
    Direction lastMotionDirection_;

    int currentPosition_;
    int totalTravelSteps_;

    int encoderTotalSteps_;
    int commandedStepsSinceEncoderMove_;

    bool stoppedByUser_;
    bool status_;
    uint32_t lastStepMs_;

    static constexpr uint32_t STEP_INTERVAL_MS = 1;
    static constexpr int STUCK_STEP_THRESHOLD = 800;
};