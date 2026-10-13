/*
 * REPL minimo (lectura de linea + tokenizador argc/argv + tabla de comandos).
 *
 * >>> INTEGRACION CON EL REPL DEL CURSO <<<
 * Si su REPL ya tiene tabla de comandos, solo copien motor_cmd.c/.h, los
 * backends (pwm_regs.c, pwm_sdk.c, chip_map.h, ...) y agreguen:
 *      { "motor", "motor <0-100> <derecha|izquierda> | ...", cmd_motor }
 * a SU tabla. cmd_motor() usa la misma firma int f(int argc, char **argv).
 */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "repl.h"
#include "motor_cmd.h"

static int cmd_help(int argc, char **argv);
static int cmd_chip(int argc, char **argv);

static const repl_cmd_t commands[] = {
    { "help",  "lista de comandos",                              cmd_help  },
    { "chip",  "muestra chip y reloj PWM asumidos",              cmd_chip  },
    { "motor", "motor <0-100> <derecha|izquierda> | parar | freno | "
               "freq <hz> | modo <reg|sdk> | rampa <0-100> <ms> | info", cmd_motor },
};
#define NCMD (sizeof(commands) / sizeof(commands[0]))

static int cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    for (unsigned i = 0; i < NCMD; i++)
        printf("  %-6s %s\n", commands[i].name, commands[i].help);
    return 0;
}

static int cmd_chip(int argc, char **argv)
{
    (void)argc; (void)argv;
    motor_print_chip();
    return 0;
}

/* Divide la linea en tokens (modifica el buffer). Devuelve argc. */
static int tokenize(char *line, char **argv, int max)
{
    int argc = 0;
    char *p = line;
    while (*p && argc < max) {
        while (*p == ' ' || *p == '\t') *p++ = '\0';
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
    }
    return argc;
}

static void dispatch(char *line)
{
    char *argv[REPL_MAX_ARGS];
    int argc = tokenize(line, argv, REPL_MAX_ARGS);
    if (argc == 0) return;

    for (unsigned i = 0; i < NCMD; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            int rc = commands[i].fn(argc, argv);
            if (rc != 0) printf("error (%d)\n", rc);
            return;
        }
    }
    printf("comando desconocido: '%s' (escribe help)\n", argv[0]);
}

void repl_run(void)
{
    char line[REPL_MAX_LINE];
    int n = 0;
    printf("\npico> ");
    for (;;) {
        int c = getchar_timeout_us(10000);
        if (c == PICO_ERROR_TIMEOUT) continue;

        if (c == '\r' || c == '\n') {
            putchar('\n');
            line[n] = '\0';
            dispatch(line);
            n = 0;
            printf("pico> ");
        } else if ((c == 0x7F || c == 0x08) && n > 0) {
            n--;
            printf("\b \b");
        } else if (c >= 32 && c < 127 && n < REPL_MAX_LINE - 1) {
            line[n++] = (char)c;
            putchar(c);
        }
    }
}
