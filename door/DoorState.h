#pragma once

enum class DoorState
{
    OPEN,
    CLOSED,
    OPENING,
    CLOSING,
    STOPPED,
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

const char* toStringDoorStateReport(DoorState state);
const char* toStringErrorState(ErrorState state);
const char* toStringCalibrationState(CalibrationState state);