#ifndef MOTOR_CMD_H
#define MOTOR_CMD_H
#include "pwm_backend.h"

void motor_init(void);
void motor_print_chip(void);
int  cmd_motor(int argc, char **argv);   /* comando del REPL */

#endif
