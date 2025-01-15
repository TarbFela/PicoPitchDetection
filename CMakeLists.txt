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
)


# stdio_usb = 1 for usb debugging, 0 for midi functionality
pico_enable_stdio_usb(PitchDetection 0)
pico_enable_stdio_uart(PitchDetection 1)



target_include_directories(PitchDetection PRIVATE ${CMAKE_CURRENT_LIST_DIR})


pico_add_extra_outputs(PitchDetection)


target_link_libraries(PitchDetection PRIVATE pico_stdlib tinyusb_device tinyusb_board
	hardware_dma
	hardware_adc
	pico_multicore
	pico_bootrom
)

add_executable(USB_Debug
		usb_debug.c
		pitch_analysis.c
)
pico_enable_stdio_usb(USB_Debug 1)
pico_enable_stdio_uart(USB_Debug 0)
#target_include_directories(USB_Debug PRIVATE ${CMAKE_CURRENT_LIST_DIR})
pico_add_extra_outputs(USB_Debug)
target_link_libraries(USB_Debug PRIVATE pico_stdlib
	hardware_dma
	hardware_adc
	pico_multicore
	pico_bootrom
)

