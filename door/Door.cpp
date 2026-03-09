#include "Door.h"
#include "../encoder/Encoder.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include "pico/stdlib.h"

using namespace std;

Door::Door(StepperMotor& motor,
           LimitSwitch& topLimit,
           LimitSwitch& bottomLimit,
           GarageMQTT& mqtt,
           StatusLeds& leds)
    : motor_(motor),
      topLimit_(topLimit),
      bottomLimit_(bottomLimit),
      mqtt_(mqtt),
      leds_(leds),
      doorState_(DoorState::CLOSED),
      errorState_(ErrorState::NORMAL),
      calibrationState_(CalibrationState::NOT_CALIBRATED),
      direction_(Direction::NONE),
      lastMotionDirection_(Direction::NONE),
      currentStepIndex_(0),
      currentPosition_(0),
      totalTravelSteps_(0),
      encoderDeltaAccumulator_(0),
      motorSinceEncoder_(0),
      motorStepCount_(0),
      encoderSteps_(0),
      roundCounts_{2},
      calibrationRound_(0),
      expectedRatio_(DEFAULT_EXPECTED_RATIO),
      calibrationPhase_(CalibrationPhase::IDLE),
      statusDirty_(true),
      lastStepMs_(0)
{
}

void Door::init()
{
    PersistentData data{};

    if (Eeprom::load(data) && data.calibrated == 1 && data.maxPosition > 0)
    {
        calibrationState_ = CalibrationState::CALIBRATED;
        totalTravelSteps_ = data.maxPosition;
        currentPosition_ = data.currentPosition;

        if (currentPosition_ < 0)
        {
            currentPosition_ = 0;
        }
        if (currentPosition_ > totalTravelSteps_)
        {
            currentPosition_ = totalTravelSteps_;
        }

        if (currentPosition_ == 0)
        {
            doorState_ = DoorState::CLOSED;
        }
        else if (currentPosition_ == totalTravelSteps_)
        {
            doorState_ = DoorState::OPEN;
        }
        else
        {
            doorState_ = DoorState::STOPPED;
        }

        if (data.lastDirection <= static_cast<uint8_t>(Direction::NONE))
        {
            lastMotionDirection_ = static_cast<Direction>(data.lastDirection);
        }
        else
        {
            lastMotionDirection_ = Direction::NONE;
        }
    }
    else
    {
        calibrationState_ = CalibrationState::NOT_CALIBRATED;
        doorState_ = DoorState::CLOSED;
        currentPosition_ = 0;
        totalTravelSteps_ = 0;
        lastMotionDirection_ = Direction::NONE;
    }

    errorState_ = ErrorState::NORMAL;
    direction_ = Direction::NONE;
    calibrationPhase_ = CalibrationPhase::IDLE;
    encoderDeltaAccumulator_ = 0;
    motorSinceEncoder_ = 0;
    motorStepCount_ = 0;
    encoderSteps_ = 0;
    calibrationRound_ = 0;
    expectedRatio_ = DEFAULT_EXPECTED_RATIO;
    statusDirty_ = true;
}

void Door::update()
{
    int delta = Encoder::consumeDelta();
    encoderDeltaAccumulator_ += delta;

    if (delta != 0)
    {
        motorSinceEncoder_ = 0;
    }

    if (calibrationState_ == CalibrationState::CALIBRATING)
    {
        updateCalibration();
    }
    else
    {
        updateNormalMovement();
    }

    updateLeds();
    publishStatusIfNeeded();
}

void Door::onSw1Pressed()
{
    if (calibrationState_ == CalibrationState::CALIBRATING)
    {
        return;
    }

    if (calibrationState_ != CalibrationState::CALIBRATED)
    {
        cout << "Door is not calibrated. Press SW0 + SW2 to calibrate first." << endl;
        return;
    }

    switch (doorState_)
    {
        case DoorState::CLOSED:
            startOpening();
            break;

        case DoorState::OPEN:
            startClosing();
            break;

        case DoorState::OPENING:
        case DoorState::CLOSING:
            stopDoor();
            break;

        case DoorState::STOPPED:
            if (lastMotionDirection_ == Direction::UP)
            {
                startClosing();
            }
            else if (lastMotionDirection_ == Direction::DOWN)
            {
                startOpening();
            }
            else
            {
                startOpening();
            }
            break;
    }
}

void Door::onCalibrationRequested()
{
    if (calibrationState_ == CalibrationState::CALIBRATING)
    {
        return;
    }

    motor_.stop();

    errorState_ = ErrorState::NORMAL;
    calibrationState_ = CalibrationState::CALIBRATING;
    calibrationPhase_ = CalibrationPhase::FIND_TOP;
    direction_ = Direction::UP;
    doorState_ = DoorState::STOPPED;
    lastMotionDirection_ = Direction::NONE;

    currentPosition_ = 0;
    totalTravelSteps_ = 0;
    resetCalibrationRoundData();
    lastStepMs_ = 0;

    statusDirty_ = true;
    cout << "Calibration requested" << endl;
}

void Door::onRemoteCommand(const char* command)
{
    if (command == nullptr)
    {
        mqtt_.publishResponse("ERROR: null command");
        return;
    }

    char cmd[64];
    strncpy(cmd, command, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';

    size_t len = strlen(cmd);
    while (len > 0 && (cmd[len - 1] == '\n' || cmd[len - 1] == '\r' || cmd[len - 1] == ' '))
    {
        cmd[len - 1] = '\0';
        --len;
    }

    for (size_t i = 0; cmd[i] != '\0'; ++i)
    {
        if (cmd[i] >= 'a' && cmd[i] <= 'z')
        {
            cmd[i] = static_cast<char>(cmd[i] - 'a' + 'A');
        }
    }

    cout << "Remote command parsed: [" << cmd << "]" << endl;

    if (strcmp(cmd, "STATUS") == 0)
    {
        statusDirty_ = true;
        publishStatusIfNeeded();
        mqtt_.publishResponse("OK: status published");
        return;
    }

    if (calibrationState_ == CalibrationState::CALIBRATING)
    {
        mqtt_.publishResponse("ERROR: calibration in progress");
        return;
    }

    if (strcmp(cmd, "CALIBRATE") == 0)
    {
        onCalibrationRequested();
        mqtt_.publishResponse("OK: calibration started");
        return;
    }

    if (calibrationState_ != CalibrationState::CALIBRATED)
    {
        mqtt_.publishResponse("ERROR: not calibrated");
        return;
    }

    if (strcmp(cmd, "OPEN") == 0)
    {
        if (doorState_ == DoorState::OPEN)
        {
            mqtt_.publishResponse("OK: already open");
            return;
        }

        startOpening();
        mqtt_.publishResponse("OK: opening");
        return;
    }

    if (strcmp(cmd, "CLOSE") == 0)
    {
        if (doorState_ == DoorState::CLOSED)
        {
            mqtt_.publishResponse("OK: already closed");
            return;
        }

        startClosing();
        mqtt_.publishResponse("OK: closing");
        return;
    }

    if (strcmp(cmd, "STOP") == 0)
    {
        if (doorState_ != DoorState::OPENING && doorState_ != DoorState::CLOSING)
        {
            mqtt_.publishResponse("OK: already stopped");
            return;
        }

        stopDoor();
        mqtt_.publishResponse("OK: stopped");
        return;
    }

    if (strcmp(cmd, "TOGGLE") == 0)
    {
        onSw1Pressed();
        mqtt_.publishResponse("OK: toggle executed");
        return;
    }

    mqtt_.publishResponse("ERROR: unknown command");
}

DoorState Door::getDoorState() const
{
    return doorState_;
}

ErrorState Door::getErrorState() const
{
    return errorState_;
}

CalibrationState Door::getCalibrationState() const
{
    return calibrationState_;
}

void Door::startOpening()
{
    errorState_ = ErrorState::NORMAL;
    direction_ = Direction::UP;
    lastMotionDirection_ = Direction::UP;
    doorState_ = DoorState::OPENING;
    motorSinceEncoder_ = 0;
    encoderDeltaAccumulator_ = 0;
    statusDirty_ = true;
}

void Door::startClosing()
{
    errorState_ = ErrorState::NORMAL;
    direction_ = Direction::DOWN;
    lastMotionDirection_ = Direction::DOWN;
    doorState_ = DoorState::CLOSING;
    motorSinceEncoder_ = 0;
    encoderDeltaAccumulator_ = 0;
    statusDirty_ = true;
}
void Door::stopDoor()
{
    motor_.stop();
    direction_ = Direction::NONE;

    if (bottomLimit_.isPressed() || currentPosition_ <= 0)
    {
        currentPosition_ = 0;
        doorState_ = DoorState::CLOSED;
    }
    else if (topLimit_.isPressed() ||
             (totalTravelSteps_ > 0 && currentPosition_ >= totalTravelSteps_))
    {
        currentPosition_ = totalTravelSteps_;
        doorState_ = DoorState::OPEN;
    }
    else
    {
        doorState_ = DoorState::STOPPED;
    }

    statusDirty_ = true;
    savePersistentState();
}
// void Door::stopDoor(bool userInitiated)
// {
//     DoorState previousState = doorState_;
//
//     motor_.stop();
//     direction_ = Direction::NONE;
//
//     if (bottomLimit_.isPressed() || currentPosition_ <= 0)
//     {
//         currentPosition_ = 0;
//         doorState_ = DoorState::CLOSED;
//     }
//     else if (topLimit_.isPressed() ||
//              (totalTravelSteps_ > 0 && currentPosition_ >= totalTravelSteps_))
//     {
//         currentPosition_ = totalTravelSteps_;
//         doorState_ = DoorState::OPEN;
//     }
//     else
//     {
//         doorState_ = DoorState::STOPPED;
//     }
//
//     statusDirty_ = true;
//     savePersistentState();
// }
void Door::updateNormalMovement()
{
    if (doorState_ != DoorState::OPENING && doorState_ != DoorState::CLOSING)
    {
        return;
    }

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - lastStepMs_ < NORMAL_STEP_INTERVAL_MS)
    {
        return;
    }
    lastStepMs_ = now;

    if (direction_ == Direction::UP)
    {
        if (topLimit_.isPressed())
        {
            /*motor_.stop();
            direction_ = Direction::NONE;
            currentPosition_ = totalTravelSteps_;
            doorState_ = DoorState::OPEN;
            statusDirty_ = true;
            savePersistentState();*/
            stopDoor();
            return;
        }

        stepTowardTop();
        motorSinceEncoder_++;

        if (encoderDeltaAccumulator_ != 0)
        {
            currentPosition_ += abs(encoderDeltaAccumulator_);
            encoderDeltaAccumulator_ = 0;
        }
    }
    else if (direction_ == Direction::DOWN)
    {
        if (bottomLimit_.isPressed())
        {
            /*motor_.stop();
            direction_ = Direction::NONE;
            currentPosition_ = 0;
            doorState_ = DoorState::CLOSED;
            statusDirty_ = true;
            savePersistentState();*/
            stopDoor();
            return;
        }

        stepTowardBottom();
        motorSinceEncoder_++;

        if (encoderDeltaAccumulator_ != 0)
        {
            currentPosition_ -= abs(encoderDeltaAccumulator_);
            encoderDeltaAccumulator_ = 0;
        }
    }

    if (currentPosition_ < 0)
    {
        currentPosition_ = 0;
    }
    if (totalTravelSteps_ > 0 && currentPosition_ > totalTravelSteps_)
    {
        currentPosition_ = totalTravelSteps_;
    }

    if (motorSinceEncoder_ > expectedRatio_ * 4)
    {
        enterStuckError();
    }
}

void Door::updateCalibration()
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - lastStepMs_ < CALIBRATION_STEP_INTERVAL_MS)
    {
        return;
    }
    lastStepMs_ = now;

    if (encoderDeltaAccumulator_ != 0)
    {
        encoderSteps_ += abs(encoderDeltaAccumulator_);
        encoderDeltaAccumulator_ = 0;
        motorSinceEncoder_ = 0;
    }

    switch (calibrationPhase_)
    {
        case CalibrationPhase::FIND_TOP:
        {
            if (topLimit_.isPressed())
            {
                motor_.stop();
                cout << "Door is now at TOP. Starting calibration..." << endl;
                calibrationPhase_ = CalibrationPhase::GO_BOTTOM;
                direction_ = Direction::DOWN;
                encoderSteps_ = 0;
                motorStepCount_ = 0;
                motorSinceEncoder_ = 0;
                return;
            }

            stepTowardTop();
            motorStepCount_++;
            motorSinceEncoder_++;
            break;
        }

        case CalibrationPhase::GO_BOTTOM:
        {
            if (bottomLimit_.isPressed())
            {
                motor_.stop();
                cout << "Round " << calibrationRound_ + 1 << " down steps: " << encoderSteps_ << endl;
                roundCounts_[calibrationRound_] = encoderSteps_;
                encoderSteps_ = 0;
                motorSinceEncoder_ = 0;
                currentPosition_ = 0;
                calibrationPhase_ = CalibrationPhase::GO_TOP;
                direction_ = Direction::UP;
                return;
            }

            stepTowardBottom();
            motorStepCount_++;
            motorSinceEncoder_++;
            break;
        }

        case CalibrationPhase::GO_TOP:
        {
            if (topLimit_.isPressed())
            {
                motor_.stop();
                cout << "Round " << calibrationRound_ + 1 << " up steps: " << encoderSteps_ << endl;

                roundCounts_[calibrationRound_] =
                    (roundCounts_[calibrationRound_] + encoderSteps_) / 2;

                cout << "Average steps this round: " << roundCounts_[calibrationRound_] << endl;

                calibrationRound_++;

                if (calibrationRound_ >= CALIB_TIMES)
                {
                    finishCalibrationSuccess();
                    return;
                }

                encoderSteps_ = 0;
                motorSinceEncoder_ = 0;
                calibrationPhase_ = CalibrationPhase::GO_BOTTOM;
                direction_ = Direction::DOWN;
                return;
            }

            stepTowardTop();
            motorStepCount_++;
            motorSinceEncoder_++;
            break;
        }

        case CalibrationPhase::COMPLETE:
        case CalibrationPhase::FAILED:
        case CalibrationPhase::IDLE:
            return;
    }

    if (motorSinceEncoder_ > expectedRatio_ * 4)
    {
        cout << "error: Door stuck detected!" << endl;
        calibrationPhase_ = CalibrationPhase::FAILED;
        enterStuckError();
    }
}

void Door::stepTowardTop()
{
    motor_.stepBackward();
}

void Door::stepTowardBottom()
{
    motor_.stepForward();
}

void Door::resetCalibrationRoundData()
{
    encoderDeltaAccumulator_ = 0;
    motorSinceEncoder_ = 0;
    motorStepCount_ = 0;
    encoderSteps_ = 0;
    calibrationRound_ = 0;
    roundCounts_[0] = 0;
    roundCounts_[1] = 0;
    roundCounts_[2] = 0;
}

void Door::finishCalibrationSuccess()
{
    int sum = 0;
    for (int i = 0; i < CALIB_TIMES; ++i)
    {
        sum += roundCounts_[i];
    }

    totalTravelSteps_ = sum / CALIB_TIMES;
    currentPosition_ = totalTravelSteps_;

    if (totalTravelSteps_ > 0)
    {
        expectedRatio_ = motorStepCount_ / totalTravelSteps_;
        if (expectedRatio_ <= 0)
        {
            expectedRatio_ = DEFAULT_EXPECTED_RATIO;
        }
    }
    else
    {
        expectedRatio_ = DEFAULT_EXPECTED_RATIO;
    }

    direction_ = Direction::NONE;
    lastMotionDirection_ = Direction::UP;
    doorState_ = DoorState::OPEN;
    errorState_ = ErrorState::NORMAL;
    calibrationState_ = CalibrationState::CALIBRATED;
    calibrationPhase_ = CalibrationPhase::COMPLETE;
    statusDirty_ = true;

    savePersistentState();

    cout << "Calibration finished." << endl;
    cout << "Average encoder steps: " << totalTravelSteps_ << endl;
}

void Door::enterStuckError()
{
    motor_.stop();
    direction_ = Direction::NONE;
    errorState_ = ErrorState::DOOR_STUCK;
    calibrationState_ = CalibrationState::NOT_CALIBRATED;
    totalTravelSteps_ = 0;
    doorState_ = (currentPosition_ <= 0) ? DoorState::CLOSED : DoorState::STOPPED;
    statusDirty_ = true;
    savePersistentState();
}

void Door::updateLeds()
{
    leds_.update(doorState_, calibrationState_, errorState_);
}

void Door::publishStatusIfNeeded()
{
    if (!statusDirty_)
    {
        return;
    }

    char payload[128];
    snprintf(payload,
             sizeof(payload),
             R"({"door":"%s","error":"%s","calibration":"%s"})",
             toStringDoorStateReport(doorState_),
             toStringErrorState(errorState_),
             toStringCalibrationState(calibrationState_));

    mqtt_.publishStatus(payload);
    statusDirty_ = false;
}

void Door::savePersistentState()
{
    PersistentData data{};
    data.calibrated = (calibrationState_ == CalibrationState::CALIBRATED) ? 1 : 0;
    data.doorState = static_cast<uint8_t>(doorState_);
    data.lastDirection = static_cast<uint8_t>(lastMotionDirection_);
    data.maxPosition = totalTravelSteps_;
    data.currentPosition = currentPosition_;
    Eeprom::save(data);
}