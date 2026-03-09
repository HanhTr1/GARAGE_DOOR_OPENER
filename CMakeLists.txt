cmake_minimum_required(VERSION 4.0)

set(PICO_BOARD pico_w)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
set(PICO_NO_PICOTOOL 1)
project(garage_04_03 C CXX ASM)
set(CMAKE_CXX_STANDARD 20)


# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
)

add_executable(${PROJECT_NAME}
        main.cpp
        door/Door.cpp
        motor/StepperMotor.cpp
        limits/LimitSwitch.cpp
        encoder/Encoder.cpp
        input/Button.cpp
        network/GarageMQTT.cpp
        eeprom/Eeprom.cpp
        led/Led.cpp
        door/DoorState.cpp
        pico-modbus/mqtt/IPStack.cpp
        pico-modbus/mqtt/Countdown.cpp
        pico-modbus/paho.mqtt.embedded-c/MQTTPacket/src/MQTTConnectClient.c
        pico-modbus/paho.mqtt.embedded-c/MQTTPacket/src/MQTTDeserializePublish.c
        pico-modbus/paho.mqtt.embedded-c/MQTTPacket/src/MQTTFormat.c
        pico-modbus/paho.mqtt.embedded-c/MQTTPacket/src/MQTTPacket.c
        pico-modbus/paho.mqtt.embedded-c/MQTTPacket/src/MQTTSerializePublish.c
        pico-modbus/paho.mqtt.embedded-c/MQTTPacket/src/MQTTSubscribeClient.c
        pico-modbus/paho.mqtt.embedded-c/MQTTPacket/src/MQTTUnsubscribeClient.c
        pico-modbus/i2c/PicoI2CBus.cpp
        pico-modbus/i2c/PicoI2CDevice.cpp

)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        pico_time
        hardware_pwm
        hardware_gpio
        hardware_i2c
        hardware_rtc
        pico_cyw43_arch_lwip_poll
)
target_include_directories(${PROJECT_NAME} PRIVATE
        pico-modbus/mqtt
        pico-modbus/paho.mqtt.embedded-c/MQTTPacket/src
        pico-modbus/paho.mqtt.embedded-c/MQTTClient/src
        pico-modbus
        pico-modbus/display/fonts
        pico-modbus/display
        pico-modbus/i2c

        ${CMAKE_CURRENT_LIST_DIR}/include
)
pico_add_extra_outputs(${PROJECT_NAME})
# Disable usb output, enable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)