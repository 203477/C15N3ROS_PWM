/*
 * Comando REPL "motor": REPL -> C (argc/argv) -> periferico PWM -> L298N -> motor
 *
 *   motor 50 derecha     50 % de velocidad, giro a la derecha
 *   motor 80 izquierda   80 %, giro a la izquierda
 *   motor parar          rueda libre (ENA=0, IN1=IN2=0)
 *   motor freno          freno rapido (IN1=IN2=1, ENA=100 %)
 *   motor freq 10000     cambia la frecuencia PWM (1000..20000 Hz)
 *   motor modo reg|sdk   elige la implementacion (registros o SDK)
 *   motor rampa 80 2000  rampa suave hasta 80 % en 2000 ms (misma direccion)
 *   motor info           estado y registros del slice PWM
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "chip_map.h"
#include "motor_cmd.h"

static struct {
    const pwm_backend_t *be;
    motor_dir_t dir;
    uint32_t    pct;      /* 0..100 */
    uint32_t    freq;     /* Hz     */
} m;

static uint16_t pct_to_level(uint32_t pct) { return (uint16_t)((pct * (PWM_TOP + 1u)) / 100u); }

static void apply(motor_dir_t dir, uint32_t pct)
{
    /* Cambio de sentido: parar y esperar para no estresar el puente H / motor */
    if ((dir == DIR_FWD || dir == DIR_REV) && (m.dir == DIR_FWD || m.dir == DIR_REV)
        && dir != m.dir && m.pct > 0) {
        m.be->set_level(0);
        m.be->set_dir(DIR_COAST);
        sleep_ms(150);
    }
    m.be->set_dir(dir);
    m.be->set_level(pct_to_level(pct));
    m.dir = dir;
    m.pct = pct;
}

void motor_init(void)
{
    m.be = &pwm_backend_regs;
    m.freq = PWM_FREQ_DEFAULT;
    m.dir = DIR_COAST;
    m.pct = 0;
    m.be->init(m.freq);
}

void motor_print_chip(void)
{
    printf("Chip: %s | clk_sys asumido: %lu Hz | PWM: %d slices | base 0x%08lX\n",
           CHIP_NAME, (unsigned long)PWM_SYSCLK_HZ, PWM_NUM_SLICES, (unsigned long)PWM_BASE);
}

/* Convierte texto a entero validando rango. 0 = OK. */
static int parse_u32(const char *s, uint32_t lo, uint32_t hi, uint32_t *out)
{
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v < (long)lo || v > (long)hi) return -1;
    *out = (uint32_t)v;
    return 0;
}

static int parse_dir(const char *s, motor_dir_t *d)
{
    if (!strcmp(s, "derecha")   || !strcmp(s, "der")) { *d = DIR_FWD; return 0; }
    if (!strcmp(s, "izquierda") || !strcmp(s, "izq")) { *d = DIR_REV; return 0; }
    return -1;
}

static int usage(void)
{
    printf("uso: motor <0-100> <derecha|izquierda>\n"
           "     motor parar | freno | info\n"
           "     motor freq <1000-20000> | modo <reg|sdk> | rampa <0-100> <ms>\n");
    return -1;
}

static void print_state(void)
{
    const char *dn[] = { "libre", "derecha", "izquierda", "freno" };
    uint32_t lvl = pct_to_level(m.pct);
    printf("modo=%s  dir=%s  velocidad=%lu%%  f=%lu Hz  CC_A=%lu/%u\n",
           m.be->name, dn[m.dir], (unsigned long)m.pct, (unsigned long)m.freq,
           (unsigned long)lvl, PWM_TOP + 1u);
}

int cmd_motor(int argc, char **argv)
{
    uint32_t v, t;
    motor_dir_t d;

    if (argc < 2) return usage();

    /* motor <0-100> <dir> : argv[1] = velocidad, argv[2] = sentido */
    if (parse_u32(argv[1], 0, 100, &v) == 0) {
        if (argc != 3 || parse_dir(argv[2], &d) != 0) return usage();
        apply(d, v);
        print_state();
        return 0;
    }

    if (!strcmp(argv[1], "parar") && argc == 2) { apply(DIR_COAST, 0); print_state(); return 0; }
    if (!strcmp(argv[1], "freno")  && argc == 2) { apply(DIR_BRAKE, 100); print_state(); return 0; }
    if (!strcmp(argv[1], "info")   && argc == 2) {
        motor_print_chip(); print_state(); m.be->dump(); return 0;
    }

    if (!strcmp(argv[1], "freq") && argc == 3) {
        if (parse_u32(argv[2], PWM_FREQ_MIN, PWM_FREQ_MAX, &v) != 0) return usage();
        m.freq = v;
        m.be->set_freq(v);
        print_state();
        return 0;
    }

    if (!strcmp(argv[1], "modo") && argc == 3) {
        const pwm_backend_t *nb = !strcmp(argv[2], "reg") ? &pwm_backend_regs
                                : !strcmp(argv[2], "sdk") ? &pwm_backend_sdk : NULL;
        if (!nb) return usage();
        motor_dir_t d0 = m.dir; uint32_t p0 = m.pct;
        m.be->deinit();
        m.be = nb;
        m.be->init(m.freq);
        m.dir = DIR_COAST; m.pct = 0;      /* estado limpio tras re-inicializar */
        apply(d0, p0);
        print_state();
        return 0;
    }

    if (!strcmp(argv[1], "rampa") && argc == 4) {
        if (parse_u32(argv[2], 0, 100, &v) != 0 || parse_u32(argv[3], 10, 20000, &t) != 0)
            return usage();
        if (m.dir != DIR_FWD && m.dir != DIR_REV) {
            printf("rampa requiere un sentido activo (use antes: motor <v> <dir>)\n");
            return -2;
        }
        int32_t from = (int32_t)m.pct, to = (int32_t)v;
        int32_t steps = from > to ? from - to : to - from;
        if (steps == 0) { print_state(); return 0; }
        uint32_t dt = t / (uint32_t)steps;
        for (int32_t i = 1; i <= steps; i++) {
            m.pct = (uint32_t)(from + (to > from ? i : -i));
            m.be->set_level(pct_to_level(m.pct));
            sleep_ms(dt);
        }
        print_state();
        return 0;
    }

    return usage();
}
