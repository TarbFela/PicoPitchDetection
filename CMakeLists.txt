cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(Pitch_Detection)

set(CMAKE_C_STANDARD 11)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project

add_executable(Pitch_Detection
	main.c 
	pitch_analysis.c
)

target_include_directories(Pitch_Detection
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../
)

target_link_libraries(Pitch_Detection
	pico_stdlib 
	hardware_spi
	hardware_dma
	hardware_gpio
	hardware_adc
	pico_multicore
	pico_bootrom
	hardware_i2c)

pico_enable_stdio_usb(Pitch_Detection 1) 
pico_enable_stdio_uart(Pitch_Detection 0) 

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(Pitch_Detection)

