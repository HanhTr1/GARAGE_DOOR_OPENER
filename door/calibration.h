#pragma once

#include "../motor/StepperMotor.h"
#include "../limitSwitch/LimitSwitch.h"
#include <cstdint>

class CalibrationManager
{
public:
    enum class Phase
    {
        IDLE,
        GO_BOTTOM,
        GO_TOP
    };

    CalibrationManager(StepperMotor& motor,
                       LimitSwitch& topLimit,
                       LimitSwitch& bottomLimit);

    void start();
    void update(int encoderTotalSteps);

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] bool isFinished() const;
    [[nodiscard]] bool hasFailed() const;

    [[nodiscard]] int measuredSteps() const;
    [[nodiscard]] Phase phase() const;

private:
    StepperMotor& motor_;
    LimitSwitch& topLimit_;
    LimitSwitch& bottomLimit_;

    Phase phase_;
    int calibrationMeasuredEncoderSteps_;
    int commandedStepsSinceEncoderMove_;
    uint32_t lastStepMs_;

    bool finished_;
    bool failed_;

    static constexpr uint32_t STEP_INTERVAL_MS = 1;
    static constexpr int STUCK_STEP_THRESHOLD = 800;
};