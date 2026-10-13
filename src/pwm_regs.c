/*
 * Implementacion A: PWM mediante acceso DIRECTO A REGISTROS (punteros).
 *
 * Cadena de acceso:
 *   direccion base -> offset -> direccion del registro -> puntero volatile
 *   -> desreferenciacion (*p) -> valor -> periferico
 */
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"          /* solo para sleep/printf; NO se usa API de GPIO/PWM */
#include "chip_map.h"
#include "pwm_backend.h"

/* ---- Acceso a un registro de 32 bits ---------------------------------- */
/* volatile: el compilador NO puede cachear/eliminar/reordenar el acceso,
 * porque el valor puede cambiar por hardware y escribir tiene efectos.   */
#define REG32(addr)   (*(volatile uint32_t *)(uintptr_t)(addr))

/* Alias atomicos de los perifericos APB (RP2040 y RP2350):
 *   +0x0000 normal R/W | +0x1000 XOR | +0x2000 SET | +0x3000 CLR          */
#define ALIAS_SET  0x2000u
#define ALIAS_CLR  0x3000u

/* ---- RESETS ------------------------------------------------------------ */
#define RESETS_RESET_OFFS       0x00u   /* 1 = periferico en reset         */
#define RESETS_RESET_DONE_OFFS  0x08u   /* 1 = ya salio del reset (RO)     */

/* ---- IO_BANK0 / PADS_BANK0 (misma formula en ambos chips) --------------- */
#define GPIO_CTRL(n)     (IO_BANK0_BASE  + 8u * (n) + 4u)   /* GPIOn_CTRL   */
#define PAD_GPIO(n)      (PADS_BANK0_BASE + 4u + 4u * (n))  /* PADS GPIOn   */
#define FUNCSEL_MASK     0x1Fu
#define FUNCSEL_PWM      4u
#define FUNCSEL_SIO      5u
#define PAD_ISO_BIT      (1u << 8)       /* solo RP2350                      */

/* ---- PWM: offsets dentro de un slice (iguales en ambos chips) ----------- */
#define PWM_SLICE(s)     (PWM_BASE + (s) * 0x14u)
#define PWM_CSR(s)       (PWM_SLICE(s) + 0x00u)
#define PWM_DIV(s)       (PWM_SLICE(s) + 0x04u)
#define PWM_CTR(s)       (PWM_SLICE(s) + 0x08u)
#define PWM_CC(s)        (PWM_SLICE(s) + 0x0Cu)
#define PWM_TOPR(s)      (PWM_SLICE(s) + 0x10u)
#define PWM_EN           (PWM_BASE + PWM_GLOBAL_EN_OFFS)

#define CSR_EN           (1u << 0)
#define CSR_PH_CORRECT   (1u << 1)
#define CSR_A_INV        (1u << 2)
#define CSR_B_INV        (1u << 3)
#define CC_A_MASK        0x0000FFFFu     /* canal A: bits [15:0]             */

/* GPIO -> slice/canal.  GPIO0..29: slice = (gpio>>1)&7, canal = gpio&1      */
#define SLICE_OF(gpio)   (((gpio) >> 1) & 7u)
#define CHAN_OF(gpio)    ((gpio) & 1u)

static void unreset(uint32_t mask)
{
    REG32(RESETS_BASE + ALIAS_CLR + RESETS_RESET_OFFS) = mask;      /* RESET &= ~mask */
    while ((REG32(RESETS_BASE + RESETS_RESET_DONE_OFFS) & mask) != mask)
        ;                                                           /* esperar RESET_DONE */
}

static void pad_and_function(uint32_t pin, uint32_t funcsel)
{
#if PAD_HAS_ISO
    /* RP2350: los pads salen del reset AISLADOS (ISO=1). Hay que limpiarlo. */
    REG32(PAD_GPIO(pin) + ALIAS_CLR) = PAD_ISO_BIT;
#endif
    uint32_t v = REG32(GPIO_CTRL(pin));            /* leer-modificar-escribir */
    v = (v & ~FUNCSEL_MASK) | funcsel;
    REG32(GPIO_CTRL(pin)) = v;
}

static void regs_set_freq(uint32_t freq_hz)
{
    uint32_t s = SLICE_OF(PIN_ENA);
    REG32(PWM_DIV(s)) = pwm_div16_for(PWM_SYSCLK_HZ, freq_hz);
}

static void regs_set_level(uint16_t level)
{
    uint32_t s = SLICE_OF(PIN_ENA);
    uint32_t cc = REG32(PWM_CC(s));
    cc = (cc & ~CC_A_MASK) | level;                 /* conserva canal B */
    REG32(PWM_CC(s)) = cc;
}

static void regs_set_dir(motor_dir_t d)
{
    uint32_t in1 = 1u << PIN_IN1, in2 = 1u << PIN_IN2;
    uint32_t set = 0, clr = 0;
    switch (d) {
        case DIR_FWD:   set = in1;       clr = in2;       break;
        case DIR_REV:   set = in2;       clr = in1;       break;
        case DIR_BRAKE: set = in1 | in2; clr = 0;         break;
        default:        set = 0;         clr = in1 | in2; break;  /* COAST */
    }
    REG32(SIO_BASE + SIO_GPIO_OUT_CLR_OFFS) = clr;
    REG32(SIO_BASE + SIO_GPIO_OUT_SET_OFFS) = set;
}

static void regs_init(uint32_t freq_hz)
{
    uint32_t s = SLICE_OF(PIN_ENA);

    /* 1) Sacar de reset IO_BANK0, PADS_BANK0 y PWM */
    unreset(RST_BIT_IO_BANK0 | RST_BIT_PADS_BANK0 | RST_BIT_PWM);

    /* 2) Pines de direccion: funcion SIO, salida, en bajo */
    pad_and_function(PIN_IN1, FUNCSEL_SIO);
    pad_and_function(PIN_IN2, FUNCSEL_SIO);
    REG32(SIO_BASE + SIO_GPIO_OE_SET_OFFS)  = (1u << PIN_IN1) | (1u << PIN_IN2);
    regs_set_dir(DIR_COAST);

    /* 3) Slice PWM: deshabilitar, configurar, habilitar al final */
    REG32(PWM_CSR(s) + ALIAS_CLR) = CSR_EN;
    REG32(PWM_CSR(s)) = 0;                          /* modo libre, sin phase-correct, sin inversion */
    REG32(PWM_TOPR(s)) = PWM_TOP;
    regs_set_freq(freq_hz);
    REG32(PWM_CC(s)) = 0;                           /* 0 % */
    REG32(PWM_CTR(s)) = 0;

    /* 4) Pin ENA -> funcion PWM (GPIO16 = PWM0 A) */
    pad_and_function(PIN_ENA, FUNCSEL_PWM);

    /* 5) Habilitar slice */
    REG32(PWM_CSR(s) + ALIAS_SET) = CSR_EN;
}

static void regs_deinit(void)
{
    uint32_t s = SLICE_OF(PIN_ENA);
    REG32(PWM_CC(s)) = 0;
    REG32(PWM_CSR(s) + ALIAS_CLR) = CSR_EN;
    regs_set_dir(DIR_COAST);
}

static void regs_dump(void)
{
    uint32_t s = SLICE_OF(PIN_ENA);
    uint32_t div = REG32(PWM_DIV(s));
    printf("  [REGISTROS] %s  PWM_BASE=0x%08lX  slice=%lu canal=%c\n",
           CHIP_NAME, (unsigned long)PWM_BASE, (unsigned long)s,
           CHAN_OF(PIN_ENA) ? 'B' : 'A');
    printf("    CSR  @0x%08lX = 0x%08lX  (EN=%lu PH_CORRECT=%lu)\n",
           (unsigned long)PWM_CSR(s), (unsigned long)REG32(PWM_CSR(s)),
           (unsigned long)(REG32(PWM_CSR(s)) & 1u),
           (unsigned long)((REG32(PWM_CSR(s)) >> 1) & 1u));
    printf("    DIV  @0x%08lX = 0x%08lX  (INT=%lu FRAC=%lu/16)\n",
           (unsigned long)PWM_DIV(s), (unsigned long)div,
           (unsigned long)((div >> 4) & 0xFFu), (unsigned long)(div & 0xFu));
    printf("    TOP  @0x%08lX = %lu\n",
           (unsigned long)PWM_TOPR(s), (unsigned long)REG32(PWM_TOPR(s)));
    printf("    CC   @0x%08lX = 0x%08lX  (A=%lu B=%lu)\n",
           (unsigned long)PWM_CC(s), (unsigned long)REG32(PWM_CC(s)),
           (unsigned long)(REG32(PWM_CC(s)) & 0xFFFFu),
           (unsigned long)(REG32(PWM_CC(s)) >> 16));
}

const pwm_backend_t pwm_backend_regs = {
    .name = "registros", .init = regs_init, .set_freq = regs_set_freq,
    .set_level = regs_set_level, .set_dir = regs_set_dir,
    .deinit = regs_deinit, .dump = regs_dump,
};
