#include "Calibration.h"
#include "pico/stdlib.h"
#include <cstdlib>
#include <iostream>

CalibrationManager::CalibrationManager(StepperMotor& motor,
                                       LimitSwitch& topLimit,
                                       LimitSwitch& bottomLimit)
    : motor_(motor),
      topLimit_(topLimit),
      bottomLimit_(bottomLimit),
      phase_(Phase::IDLE),
      calibrationMeasuredEncoderSteps_(0),
      commandedStepsSinceEncoderMove_(0),
      lastStepMs_(0),
      finished_(false),
      failed_(false)
{
}

void CalibrationManager::start()
{
    motor_.stop();

    phase_ = Phase::GO_BOTTOM;
    calibrationMeasuredEncoderSteps_ = 0;
    commandedStepsSinceEncoderMove_ = 0;
    lastStepMs_ = 0;
    finished_ = false;
    failed_ = false;
}

void CalibrationManager::update(int encoderTotalSteps)
{
    if (phase_ == Phase::IDLE)
    {
        return;
    }

    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - lastStepMs_ < STEP_INTERVAL_MS)
    {
        return;
    }
    lastStepMs_ = now;

    if (encoderTotalSteps != 0)
    {
        calibrationMeasuredEncoderSteps_ += std::abs(encoderTotalSteps);
        commandedStepsSinceEncoderMove_ = 0;
    }

    switch (phase_)
    {
        case Phase::GO_BOTTOM:
        {
            if (bottomLimit_.isPressed())
            {
                motor_.stop();
                calibrationMeasuredEncoderSteps_ = 0; // start from bottom to top
                commandedStepsSinceEncoderMove_ = 0;
                phase_ = Phase::GO_TOP;
                std::cout << "Bottom found. Moving to top..." << std::endl;
                return;
            }

           motor_.stepForward();
            commandedStepsSinceEncoderMove_++;
            break;
        }

        case Phase::GO_TOP:
        {
            if (topLimit_.isPressed())
            {
                motor_.stop();
                phase_ = Phase::IDLE;
                finished_ = true;
                return;
            }

            motor_.stepBackward();
            commandedStepsSinceEncoderMove_++;
            break;
        }

        case Phase::IDLE:
            return;
    }

    if (commandedStepsSinceEncoderMove_ > STUCK_STEP_THRESHOLD)
    {
        motor_.stop();
        phase_ = Phase::IDLE;
        failed_ = true;
    }
}

bool CalibrationManager::isRunning() const
{
    return phase_ != Phase::IDLE && !finished_ && !failed_;
}

bool CalibrationManager::isFinished() const
{
    return finished_;
}

bool CalibrationManager::hasFailed() const
{
    return failed_;
}

int CalibrationManager::measuredSteps() const
{
    return calibrationMeasuredEncoderSteps_;
}

CalibrationManager::Phase CalibrationManager::phase() const
{
    return phase_;
}
