#pragma once
#include "../motor/StepperMotor.h"
#include "../encoder/Encoder.h"
#include "../limits/LimitSwitch.h"
#include "../led/Led.h"
#include "../eeprom/Eeprom.h"
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