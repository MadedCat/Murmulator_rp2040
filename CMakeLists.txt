# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_LIST_DIR}/boards")
set(PICO_BOARD murmulator2 CACHE STRING "Board pinout: murmulator or murmulator2")
set(PICO_PLATFORM rp2350 CACHE STRING "Processor: rp2040 or rp2350 (ARM)")
set(MURM_VIDEO vga-hdmi-tft CACHE STRING "Video backend")
set_property(CACHE MURM_VIDEO PROPERTY STRINGS vga-hdmi-tft composite soft-composite)

if(PICO_BOARD STREQUAL "murmulator")
    set(board_id m1)
elseif(PICO_BOARD STREQUAL "murmulator2")
    set(board_id m2)
else()
    message(FATAL_ERROR "Unsupported PICO_BOARD: ${PICO_BOARD}")
endif()

if(PICO_PLATFORM STREQUAL "rp2040")
    set(cpu_id p1)
elseif(PICO_PLATFORM MATCHES "^rp2350(-arm-s)?$")
    set(cpu_id p2)
else()
    message(FATAL_ERROR "Unsupported PICO_PLATFORM: ${PICO_PLATFORM}")
endif()

if(MURM_VIDEO STREQUAL "vga-hdmi-tft")
    set(video_definitions VGA_HDMI=1)
elseif(MURM_VIDEO STREQUAL "composite")
    set(video_definitions COMPOSITE_TV=1)
    set(video_source lib/tv_out/tv_out.c)
elseif(MURM_VIDEO STREQUAL "soft-composite")
    set(video_definitions SOFT_COMPOSITE_TV=1 SOFT_CVBS_NO_HUD)
    set(video_source lib/tv_out_soft/tv_out.c)
else()
    message(FATAL_ERROR "Unsupported MURM_VIDEO: ${MURM_VIDEO}")
endif()

include(pico_sdk_import.cmake)
project(murmulator VERSION 0.96.21 LANGUAGES C CXX ASM)

set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(SOFT_VERSION ${PROJECT_VERSION})
configure_file(src/config.h.in generated/config.h @ONLY)

set(PICO_HEAP_SIZE 1024)
set(PICO_STACK_SIZE 1024)
pico_sdk_init()

add_subdirectory(lib/fatfs)
add_subdirectory(lib/sdcard)
add_subdirectory(lib/joysticks)
add_subdirectory(lib/sound)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.c src/*.cpp)
add_executable(murmulator ${sources} ${video_source})
set_target_properties(murmulator PROPERTIES
    OUTPUT_NAME "murmulator-${board_id}${cpu_id}-${MURM_VIDEO}-v${PROJECT_VERSION}"
)

pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
pico_set_boot_stage2(murmulator slower_boot2)

target_include_directories(murmulator PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_definitions(murmulator PRIVATE ${video_definitions})
target_link_libraries(murmulator PRIVATE
    fatfs sdcard joysticks sound
    pico_stdlib pico_multicore pico_rand
    hardware_spi hardware_i2c hardware_pwm hardware_adc
    hardware_pio hardware_irq hardware_dma hardware_timer
)

if(MURM_VIDEO STREQUAL "vga-hdmi-tft")
    add_subdirectory(lib/video)
    target_link_libraries(murmulator PRIVATE video)
endif()

pico_enable_stdio_uart(murmulator 0)
pico_enable_stdio_usb(murmulator 1)
pico_add_extra_outputs(murmulator)
target_link_options(murmulator PRIVATE LINKER:--print-memory-usage)
