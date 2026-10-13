#ifndef PWM_BACKEND_H
#define PWM_BACKEND_H
#include <stdint.h>
#include "board_config.h"

typedef enum { DIR_COAST = 0, DIR_FWD, DIR_REV, DIR_BRAKE } motor_dir_t;

/* Interfaz comun: la misma API implementada con registros y con el SDK */
typedef struct {
    const char *name;
    void (*init)(uint32_t freq_hz);    /* GPIO + slice, salida en 0 %        */
    void (*set_freq)(uint32_t freq_hz);
    void (*set_level)(uint16_t level); /* 0..PWM_TOP+1 (PWM_TOP+1 = 100 %)   */
    void (*set_dir)(motor_dir_t d);    /* IN1/IN2 del puente H               */
    void (*deinit)(void);
    void (*dump)(void);                /* imprime registros del slice        */
} pwm_backend_t;

extern const pwm_backend_t pwm_backend_regs;
extern const pwm_backend_t pwm_backend_sdk;

/* DIV (8.4) = clk_sys / (f * (TOP+1)), expresado como entero x16.
 * PWM_DIV tiene INT en [11:4] y FRAC en [3:0]: el valor del registro ES div*16.
 * Rango valido: 16 (1.0) .. 4095 (255.9375). */
static inline uint32_t pwm_div16_for(uint32_t sysclk_hz, uint32_t freq_hz)
{
    uint64_t den = (uint64_t)freq_hz * (PWM_TOP + 1u);
    uint64_t d16 = ((uint64_t)sysclk_hz * 16u + den / 2u) / den;  /* redondeo */
    if (d16 < 16u)   d16 = 16u;
    if (d16 > 4095u) d16 = 4095u;
    return (uint32_t)d16;
}

#endif
