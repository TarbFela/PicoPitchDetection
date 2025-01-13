cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(PitchDetection C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(PitchDetection
		main.c
		midi_device.c
		usb_descriptors.c
		pitch_analysis.c
		usb_debug.c
)

# stdio_usb = 1 for usb debugging, = 0 for midi functionality
pico_enable_stdio_usb(PitchDetection 1)
pico_enable_stdio_uart(PitchDetection 0)

target_include_directories(PitchDetection PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_add_extra_outputs(PitchDetection)
target_link_libraries(PitchDetection PRIVATE pico_stdlib tinyusb_device tinyusb_board
	hardware_dma
	hardware_adc
	pico_multicore
	pico_bootrom
)

