/*
 * Implementacion B: la MISMA funcion con el Raspberry Pi Pico SDK.
 * El SDK oculta direcciones base, offsets, resets y el mapeo GPIO->slice,
 * y es el mismo codigo fuente para RP2040 y RP2350 (cambia el hardware_pwm
 * que se compila segun PICO_BOARD).
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/structs/pwm.h"   /* pwm_hw: puntero al bloque de registros */
#include "chip_map.h"
#include "pwm_backend.h"

static void sdk_set_freq(uint32_t freq_hz)
{
    uint slice = pwm_gpio_to_slice_num(PIN_ENA);
    uint32_t d16 = pwm_div16_for(PWM_SYSCLK_HZ, freq_hz);
    pwm_set_clkdiv(slice, d16 / 16.0f);              /* 8.4 -> float exacto */
}

static void sdk_set_level(uint16_t level)
{
    pwm_set_gpio_level(PIN_ENA, level);              /* escribe PWM_CC (A o B) */
}

static void sdk_set_dir(motor_dir_t d)
{
    bool in1 = (d == DIR_FWD || d == DIR_BRAKE);
    bool in2 = (d == DIR_REV || d == DIR_BRAKE);
    gpio_put(PIN_IN1, in1);
    gpio_put(PIN_IN2, in2);
}

static void sdk_init(uint32_t freq_hz)
{
    gpio_init(PIN_IN1); gpio_set_dir(PIN_IN1, GPIO_OUT);
    gpio_init(PIN_IN2); gpio_set_dir(PIN_IN2, GPIO_OUT);
    sdk_set_dir(DIR_COAST);

    gpio_set_function(PIN_ENA, GPIO_FUNC_PWM);       /* FUNCSEL=4 (+ISO en RP2350) */
    uint slice = pwm_gpio_to_slice_num(PIN_ENA);

    pwm_config cfg = pwm_get_default_config();       /* CSR=0, DIV=1.0, TOP=0xFFFF */
    pwm_config_set_wrap(&cfg, PWM_TOP);              /* TOP */
    pwm_config_set_clkdiv(&cfg, pwm_div16_for(PWM_SYSCLK_HZ, freq_hz) / 16.0f);
    pwm_init(slice, &cfg, false);                    /* escribe CSR/DIV/TOP */
    pwm_set_gpio_level(PIN_ENA, 0);
    pwm_set_enabled(slice, true);                    /* CSR.EN = 1 */
}

static void sdk_deinit(void)
{
    uint slice = pwm_gpio_to_slice_num(PIN_ENA);
    pwm_set_gpio_level(PIN_ENA, 0);
    pwm_set_enabled(slice, false);
    sdk_set_dir(DIR_COAST);
}

static void sdk_dump(void)
{
    uint slice = pwm_gpio_to_slice_num(PIN_ENA);
    uint chan  = pwm_gpio_to_channel(PIN_ENA);
    /* pwm_hw es un puntero a la estructura que SE SUPERPONE a los registros reales */
    printf("  [SDK] %s  pwm_hw=%p  slice=%u canal=%c\n",
           CHIP_NAME, (void *)pwm_hw, slice, chan ? 'B' : 'A');
    printf("    pwm_hw->slice[%u].csr = 0x%08lX\n", slice, (unsigned long)pwm_hw->slice[slice].csr);
    printf("    pwm_hw->slice[%u].div = 0x%08lX\n", slice, (unsigned long)pwm_hw->slice[slice].div);
    printf("    pwm_hw->slice[%u].top = %lu\n",     slice, (unsigned long)pwm_hw->slice[slice].top);
    printf("    pwm_hw->slice[%u].cc  = 0x%08lX\n", slice, (unsigned long)pwm_hw->slice[slice].cc);
}

const pwm_backend_t pwm_backend_sdk = {
    .name = "sdk", .init = sdk_init, .set_freq = sdk_set_freq,
    .set_level = sdk_set_level, .set_dir = sdk_set_dir,
    .deinit = sdk_deinit, .dump = sdk_dump,
};
