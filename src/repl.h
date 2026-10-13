#ifndef REPL_H
#define REPL_H

#define REPL_MAX_LINE 80
#define REPL_MAX_ARGS 8

/* Firma de un comando: igual que main(): argc/argv. Devuelve 0 = OK. */
typedef int (*repl_fn_t)(int argc, char **argv);

typedef struct {
    const char *name;   /* argv[0] */
    const char *help;
    repl_fn_t   fn;
} repl_cmd_t;

void repl_run(void);    /* bucle infinito: leer linea -> tokenizar -> despachar */

#endif
