#pragma once

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <cstdint>

struct __attribute__((packed)) PersistentData
{
    uint32_t magic;
    uint8_t calibrated;
    uint8_t doorState;
    uint8_t lastDirection;
    int32_t maxPosition;
    uint8_t stoppedByUser;
    int32_t currentPosition;
    uint16_t crc16;
};

class Eeprom {
public:
    static void init();
    static bool load(PersistentData& data);
    static bool save(const PersistentData& data);

private:
    static uint16_t crc16Calc(const uint8_t* data, size_t length);

    static constexpr uint8_t ADDRESS = 0x50;
    static constexpr uint16_t OFFSET = 0x0000;
    static constexpr uint32_t MAGIC = 0xDEADBEEF;
};