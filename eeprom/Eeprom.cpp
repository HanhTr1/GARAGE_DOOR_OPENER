#include "Eeprom.h"
#include <cstring>

void Eeprom::init()
{
    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(16, GPIO_FUNC_I2C);
    gpio_set_function(17, GPIO_FUNC_I2C);
    gpio_pull_up(16);
    gpio_pull_up(17);
}

uint16_t Eeprom::crc16(const uint8_t* data, size_t length)
{
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--)
    {
        x = (crc >> 8) ^ *data++;
        x ^= x >> 4;
        crc = (crc << 8) ^
              static_cast<uint16_t>(x << 12) ^
              static_cast<uint16_t>(x << 5) ^
              static_cast<uint16_t>(x);
    }
    return crc;
}

bool Eeprom::load(PersistentData& data)
{
    uint8_t addr[2];
    addr[0] = static_cast<uint8_t>(MEMORY_OFFSET >> 8);
    addr[1] = static_cast<uint8_t>(MEMORY_OFFSET & 0xFF);

    int written = i2c_write_blocking(i2c0, ADDRESS, addr, 2, true);
    if (written != 2)
    {
        return false;
    }

    int read = i2c_read_blocking(i2c0,
                                 ADDRESS,
                                 reinterpret_cast<uint8_t*>(&data),
                                 sizeof(PersistentData),
                                 false);
    if (read != sizeof(PersistentData))
    {
        return false;
    }

    if (data.magic != MAGIC)
    {
        return false;
    }

    uint16_t expected = crc16(reinterpret_cast<const uint8_t*>(&data),
                              sizeof(PersistentData) - sizeof(data.crc16));

    return data.crc16 == expected;
}

bool Eeprom::save(const PersistentData& data)
{
    PersistentData copy = data;
    copy.magic = MAGIC;
    copy.crc16 = crc16(reinterpret_cast<const uint8_t*>(&copy),
                       sizeof(PersistentData) - sizeof(copy.crc16));

    uint8_t buffer[sizeof(PersistentData) + 2];
    buffer[0] = static_cast<uint8_t>(MEMORY_OFFSET >> 8);
    buffer[1] = static_cast<uint8_t>(MEMORY_OFFSET & 0xFF);

    std::memcpy(&buffer[2], &copy, sizeof(PersistentData));

    int written = i2c_write_blocking(i2c0, ADDRESS, buffer, sizeof(buffer), false);
    if (written != static_cast<int>(sizeof(buffer)))
    {
        return false;
    }

    sleep_ms(10);
    return true;
}