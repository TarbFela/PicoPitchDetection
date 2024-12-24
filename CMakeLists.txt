cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(adc_display)

set(CMAKE_C_STANDARD 11)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project

add_executable(adc_display
	main.c 
	../ssd1306.c
	bootrom_api.c
)

target_include_directories(adc_display
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../
)

target_link_libraries(adc_display
	pico_stdlib 
	hardware_spi
	hardware_gpio
	hardware_adc
	pico_multicore
	pico_bootrom
	hardware_i2c)

pico_enable_stdio_usb(adc_display 1) 
pico_enable_stdio_uart(adc_display 0) 

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(adc_display)

