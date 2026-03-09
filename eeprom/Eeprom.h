#pragma once

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <cstdint>

struct __attribute__((packed)) PersistentData
{
    uint32_t magic;
    uint8_t calibrated;
    uint8_t door_state;
    uint8_t direction;
    uint8_t reserved;

    int32_t max_position;
    int32_t current_position;

    uint16_t crc16;
};

class Eeprom
{
public:
    static void init();
    bool load(PersistentData& data);
    bool save(const PersistentData& data);

private:
    static uint16_t crc16(const uint8_t* data, size_t length);

private:
    static constexpr uint8_t ADDRESS = 0x50;
    static constexpr uint16_t MEMORY_OFFSET = 0x0000;
    static constexpr uint32_t MAGIC = 0xDEADBEEF;
};