#ifndef CHIP_MAP_H
#define CHIP_MAP_H
/*
 * Mapa de direcciones/campos que DIFIEREN entre RP2040 y RP2350.
 * El resto (offsets internos de cada slice PWM, formato de CSR/DIV/CC/TOP,
 * alias atomicos) es identico y vive en pwm_regs.c.
 */
#include <stdint.h>

#if PICO_RP2350
  #define CHIP_NAME              "RP2350"
  #define PWM_BASE               0x40058000u
  #define PWM_NUM_SLICES         12
  #define PWM_GLOBAL_EN_OFFS     0x0F0u      /* 12 slices * 0x14 = 0xF0    */
  #define RESETS_BASE            0x40020000u
  #define IO_BANK0_BASE          0x40028000u
  #define PADS_BANK0_BASE        0x40038000u
  #define RST_BIT_IO_BANK0       (1u << 6)
  #define RST_BIT_PADS_BANK0     (1u << 9)
  #define RST_BIT_PWM            (1u << 16)
  #define PAD_HAS_ISO            1           /* bit 8: aislamiento del pad */
  #define SIO_GPIO_OUT_SET_OFFS  0x018u
  #define SIO_GPIO_OUT_CLR_OFFS  0x020u
  #define SIO_GPIO_OE_SET_OFFS   0x038u
  #define PWM_SYSCLK_HZ          150000000u  /* clk_sys por defecto (SDK)  */
#elif PICO_RP2040
  #define CHIP_NAME              "RP2040"
  #define PWM_BASE               0x40050000u
  #define PWM_NUM_SLICES         8
  #define PWM_GLOBAL_EN_OFFS     0x0A0u      /* 8 slices * 0x14 = 0xA0     */
  #define RESETS_BASE            0x4000C000u
  #define IO_BANK0_BASE          0x40014000u
  #define PADS_BANK0_BASE        0x4001C000u
  #define RST_BIT_IO_BANK0       (1u << 5)
  #define RST_BIT_PADS_BANK0     (1u << 8)
  #define RST_BIT_PWM            (1u << 14)
  #define PAD_HAS_ISO            0
  #define SIO_GPIO_OUT_SET_OFFS  0x014u
  #define SIO_GPIO_OUT_CLR_OFFS  0x018u
  #define SIO_GPIO_OE_SET_OFFS   0x024u
  #define PWM_SYSCLK_HZ          125000000u
#else
  #error "Chip no soportado: compile con PICO_BOARD=pico (RP2040) o pico2_w (RP2350)"
#endif

#define SIO_BASE                 0xD0000000u   /* igual en ambos chips */

#endif
