#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H
/* Pines y parametros comunes (identicos en RP2040 y RP2350) */

#define PIN_ENA   16   /* GP16 -> PWM slice 0, canal A  -> ENA del L298N  */
#define PIN_IN1   17   /* GP17 -> SIO (salida digital)  -> IN1            */
#define PIN_IN2   18   /* GP18 -> SIO (salida digital)  -> IN2            */

#define PWM_TOP          999u    /* contador 0..999 => 1000 pasos (0.1 %)  */
#define PWM_FREQ_DEFAULT 5000u   /* Hz                                     */
#define PWM_FREQ_MIN     1000u
#define PWM_FREQ_MAX     20000u

#endif
