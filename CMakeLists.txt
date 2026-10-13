cmake_minimum_required(VERSION 3.13)

# Placa por defecto: Raspberry Pi Pico (RP2040).
# Para Pico 2 W (RP2350):  cmake -DPICO_BOARD=pico2_w ..
if(NOT PICO_BOARD)
  set(PICO_BOARD pico CACHE STRING "Placa destino")
endif()

include(pico_sdk_import.cmake)   # copiar desde $PICO_SDK_PATH/external/

project(pico_pwm_motor C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_executable(pico_pwm_motor
    src/main.c
    src/repl.c
    src/motor_cmd.c
    src/pwm_regs.c      # Implementacion A: registros + punteros
    src/pwm_sdk.c       # Implementacion B: Pico SDK
)

target_include_directories(pico_pwm_motor PRIVATE src)

target_link_libraries(pico_pwm_motor
    pico_stdlib
    hardware_gpio
    hardware_pwm
    hardware_clocks
)

# REPL por USB-CDC (el puerto serial del curso). UART desactivada.
pico_enable_stdio_usb(pico_pwm_motor 1)
pico_enable_stdio_uart(pico_pwm_motor 0)

pico_add_extra_outputs(pico_pwm_motor)   # genera .uf2 / .elf / .bin
