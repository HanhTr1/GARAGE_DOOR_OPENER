#include "DoorState.h"

const char* toStringDoorStateReport(DoorState state)
{
    switch (state)
    {
        case DoorState::OPEN:
            return "Open";
        case DoorState::CLOSED:
            return "Closed";
        case DoorState::OPENING:
        case DoorState::CLOSING:
        case DoorState::STOPPED:
            return "In between";
        default:
            return "Unknown";
    }
}

const char* toStringErrorState(ErrorState state)
{
    switch (state)
    {
        case ErrorState::NORMAL:
            return "Normal";
        case ErrorState::DOOR_STUCK:
            return "Door stuck";
    }
    return "Normal";
}

const char* toStringCalibrationState(CalibrationState state)
{
    switch (state)
    {
        case CalibrationState::CALIBRATED:
            return "Calibrated";
        case CalibrationState::CALIBRATING:
            return "Calibrating";
        case CalibrationState::NOT_CALIBRATED:
            return "Not calibrated";
    }
    return "Not calibrated";
}