#include "Door.h"
#include "DoorState.h"
#include "../eeprom/Eeprom.h"
#include "calibration.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>

#include "encoder/Encoder.h"
#include "pico/stdlib.h"

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
      calibration_(motor, topLimit, bottomLimit),
      doorState_(DoorState::CLOSED),
      errorState_(ErrorState::NORMAL),
      calibrationState_(CalibrationState::NOT_CALIBRATED),
      direction_(Direction::NONE),
      lastMotionDirection_(Direction::NONE),
      currentPosition_(0),
      totalTravelSteps_(0),
      encoderTotalSteps_(0),
      commandedStepsSinceEncoderMove_(0),
      stoppedByUser_(false),
      status_(true),
      lastStepMs_(0) {}

void Door::init()
{
    PersistentData data{};

    if (Eeprom::load(data) && data.calibrated == 1 && data.maxPosition > 0)
    {
        calibrationState_ = CalibrationState::CALIBRATED;
        totalTravelSteps_ = data.maxPosition;
        currentPosition_ = data.currentPosition;
        stoppedByUser_ = (data.stoppedByUser == 1);

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
            if (data.doorState <= static_cast<uint8_t>(DoorState::STOPPED))
            {
                doorState_ = static_cast<DoorState>(data.doorState);
            }
            else
            {
                doorState_ = DoorState::STOPPED;
            }

            if (doorState_ != DoorState::OPENING &&
                doorState_ != DoorState::CLOSING &&
                doorState_ != DoorState::STOPPED)
            {
                doorState_ = DoorState::STOPPED;
            }
        }

        direction_ = Direction::NONE;

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
        errorState_ = ErrorState::NORMAL;
        direction_ = Direction::NONE;
        lastMotionDirection_ = Direction::NONE;
        currentPosition_ = 0;
        totalTravelSteps_ = 0;
    }

    errorState_ = ErrorState::NORMAL;
    encoderTotalSteps_ = 0;
    commandedStepsSinceEncoderMove_ = 0;
    status_ = true;
}

void Door::update()
{
    const int direction = Encoder::encoderSteps();
    encoderTotalSteps_ += direction;

    if (calibrationState_ == CalibrationState::CALIBRATING)
    {
        calibration_.update(encoderTotalSteps_);
        encoderTotalSteps_ = 0;

        if (calibration_.isFinished())
        {
            totalTravelSteps_ = calibration_.measuredSteps();
            currentPosition_ = totalTravelSteps_;
            direction_ = Direction::NONE;
            lastMotionDirection_ = Direction::UP;
            doorState_ = DoorState::OPEN;
            errorState_ = ErrorState::NORMAL;
            calibrationState_ = CalibrationState::CALIBRATED;
            stoppedByUser_ = false;
            status_ = true;
            savePersistentState();

            std::cout << "Calibration finished. Travel steps: " << totalTravelSteps_ << std::endl;
        }
        else if (calibration_.hasFailed())
        {
            enterStuckError();
            std::cout << "Calibration failed: door stuck detected." << std::endl;
            std::cout << "Check the door and press SW0 + SW2 to try calibration again." << std::endl;
        }
    }
    else
    {
        updatePositionFromEncoder(direction);
        updateMovement();
        detectStuck();
    }

    updateLeds();
    publishStatusUpdate();
}

void Door::onSw1Pressed()
{
    if (calibrationState_ != CalibrationState::CALIBRATED)
    {
        if (calibrationState_ == CalibrationState::CALIBRATING){
        return;
        }
        std::cout << "Door is not calibrated. Press SW0 + SW2 to calibrate first." << std::endl;
        return;
    }

    switch (doorState_)
    {
        case DoorState::CLOSED:
            startOpening();
            mqtt_.publishResponse("LOCAL: OPEN");
            break;

        case DoorState::OPEN:
            startClosing();
            mqtt_.publishResponse("LOCAL: CLOSE");
            break;

        case DoorState::OPENING:
        case DoorState::CLOSING:
            stoppedByUser_ = true;
            stopDoor();
            mqtt_.publishResponse("LOCAL: STOP");
            break;

        case DoorState::STOPPED:
            if (stoppedByUser_) //opposite dir
            {
                if (lastMotionDirection_ == Direction::UP)
                {
                    startClosing();
                    mqtt_.publishResponse("LOCAL: CLOSE");
                }
                else if (lastMotionDirection_ == Direction::DOWN)
                {
                    startOpening();
                    mqtt_.publishResponse("LOCAL: OPEN");
                }
            }
            else
            {
                if (lastMotionDirection_ == Direction::UP)
                {
                    startOpening();
                    mqtt_.publishResponse("LOCAL: OPEN");
                }
                else if (lastMotionDirection_ == Direction::DOWN)
                {
                    startClosing();
                    mqtt_.publishResponse("LOCAL: CLOSE");
                }

            }
            break;
        default: ;
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

    direction_ = Direction::DOWN;
    lastMotionDirection_ = Direction::NONE;
    doorState_ = DoorState::STOPPED;

    currentPosition_ = 0;
    totalTravelSteps_ = 0;
    encoderTotalSteps_ = 0;
    commandedStepsSinceEncoderMove_ = 0;
    lastStepMs_ = 0;

    calibration_.start();

    status_ = true;
    savePersistentState();
    mqtt_.publishResponse("LOCAL: CALIBRATE");
    std::cout << "Calibration requested" << std::endl;
}

void Door::onRemoteCommand(const char* command)
{
    // check the input command is existed or not.
    if (command == nullptr)
    {
        mqtt_.publishResponse("ERROR: null command");
        return;
    }

    char cmd[64];
    std::strncpy(cmd, command, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';

    size_t len = std::strlen(cmd);
    while (len > 0 && (cmd[len - 1] == '\n' || cmd[len - 1] == '\r' || cmd[len - 1] == ' '))
    {
        cmd[len - 1] = '\0';
        --len;
    }

    std::cout << "Remote command parsed: [" << cmd << "]" << std::endl;

    if (std::strcmp(cmd, "STATUS") == 0)
    {
        status_ = true;
        publishStatusUpdate();
        mqtt_.publishResponse("OK: status published");
        return;
    }
    //if calibrating do nothing

    if (std::strcmp(cmd, "CALIBRATE") == 0)
    {
        if (calibrationState_ == CalibrationState::CALIBRATING)
        {
            mqtt_.publishResponse("Calibration in progress");
            return;
        }
        onCalibrationRequested();
        mqtt_.publishResponse("OK: calibration started");

        return;
    }

    // if door hasn't calibrated yet, do not close or open by anyways

    if (calibrationState_ != CalibrationState::CALIBRATED)
    {
        mqtt_.publishResponse("ERROR: not calibrated");
        return;
    }

    if (std::strcmp(cmd, "OPEN") == 0)
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

    if (std::strcmp(cmd, "CLOSE") == 0)
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

    if (std::strcmp(cmd, "STOP") == 0)
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
    commandedStepsSinceEncoderMove_ = 0;
    encoderTotalSteps_ = 0;
    stoppedByUser_ = false;
    status_ = true;
    savePersistentState();
}

void Door::startClosing()
{
    errorState_ = ErrorState::NORMAL;
    direction_ = Direction::DOWN;
    lastMotionDirection_ = Direction::DOWN;
    doorState_ = DoorState::CLOSING;
    commandedStepsSinceEncoderMove_ = 0;
    encoderTotalSteps_ = 0;
    stoppedByUser_ = false;
    status_ = true;
    savePersistentState();
}

void Door::stopDoor()
{
    //const DoorState previousState = doorState_;
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

    status_ = true;
    savePersistentState();
}


void Door::updatePositionFromEncoder(int direction)
{
    if (calibrationState_ != CalibrationState::CALIBRATED)
    {
        return;
    }

    if (doorState_ != DoorState::OPENING && doorState_ != DoorState::CLOSING)
    {
        return;
    }

    if (direction == 0)
    {
        return;
    }
    currentPosition_ += direction;

    if (currentPosition_ < 0)
    {
        currentPosition_ = 0;
    }

    if (totalTravelSteps_ > 0 && currentPosition_ > totalTravelSteps_)
    {
        currentPosition_ = totalTravelSteps_;
    }
}

void Door::updateMovement()
{
    if (doorState_ != DoorState::OPENING && doorState_ != DoorState::CLOSING)
    {
        return;
    }

    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - lastStepMs_ < STEP_INTERVAL_MS)
    {
        return;
    }
    lastStepMs_ = now;

    if (direction_ == Direction::UP)
    {
        if (topLimit_.isPressed())
        {
            motor_.stop();
            direction_ = Direction::NONE;
            currentPosition_ = totalTravelSteps_;
            doorState_ = DoorState::OPEN;
            status_ = true;
            savePersistentState();
            std::cout << "Door reached TOP and is now OPEN." << std::endl;
            return;
        }
        motor_.stepBackward();
        commandedStepsSinceEncoderMove_++;
    }
    else if (direction_ == Direction::DOWN)
    {
        if (bottomLimit_.isPressed())
        {
            motor_.stop();
            direction_ = Direction::NONE;
            currentPosition_ = 0;
            doorState_ = DoorState::CLOSED;
            status_ = true;
            savePersistentState();
            std::cout << "Door reached BOTTOM and is now CLOSED." << std::endl;
            return;
        }

        motor_.stepForward();
        commandedStepsSinceEncoderMove_++;
    }
}

void Door::detectStuck()
{
    if (doorState_ != DoorState::OPENING && doorState_ != DoorState::CLOSING)
    {
        return;
    }

    if (encoderTotalSteps_ != 0)
    {
        encoderTotalSteps_ = 0;
        commandedStepsSinceEncoderMove_ = 0;
        return;
    }

    if (commandedStepsSinceEncoderMove_ > STUCK_STEP_THRESHOLD)
    {
        enterStuckError();
        std::cout << "Door stuck detected. Calibration lost." << std::endl;
        std::cout << "Check the door and press SW0 + SW2 to calibrate again." << std::endl;
    }
}

void Door::updateLeds()
{
    leds_.update(calibrationState_, errorState_);
}

void Door::publishStatusUpdate()
{
    if (!status_)
    {
        return;
    }

    char payload[128];
    std::snprintf(payload,
                  sizeof(payload),
                  R"({"door":"%s","error":"%s","calibration":"%s"})",
                  toStringDoorStateReport(doorState_),
                  toStringErrorState(errorState_),
                  toStringCalibrationState(calibrationState_));

    mqtt_.publishStatus(payload);
    status_ = false;
}

void Door::savePersistentState()
{
    PersistentData data{};
    data.calibrated = (calibrationState_ == CalibrationState::CALIBRATED) ? 1 : 0;
    data.doorState = static_cast<uint8_t>(doorState_);
    data.lastDirection = static_cast<uint8_t>(lastMotionDirection_);
    data.stoppedByUser = stoppedByUser_ ? 1 : 0;

    if (calibrationState_ == CalibrationState::CALIBRATED)
    {
        data.maxPosition = totalTravelSteps_;
        data.currentPosition = currentPosition_;
    }
    else
    {
        data.maxPosition = 0;
        data.currentPosition = 0;
    }

    const bool ok = Eeprom::save(data);
    std::cout << "EEPROM save: " << (ok ? "OK" : "FAILED")
              << ", calibrated=" << static_cast<int>(data.calibrated)
              << ", maxPosition=" << data.maxPosition
              << ", currentPosition=" << data.currentPosition
              << std::endl;
}

void Door::enterStuckError()
{
    motor_.stop();
    direction_ = Direction::NONE;
    errorState_ = ErrorState::DOOR_STUCK;
    calibrationState_ = CalibrationState::NOT_CALIBRATED;
    totalTravelSteps_ = 0;
    doorState_ = (currentPosition_ <= 0) ? DoorState::CLOSED : DoorState::STOPPED;
    status_ = true;
    savePersistentState();
}