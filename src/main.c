#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "chip_map.h"
#include "motor_cmd.h"
#include "repl.h"

int main(void)
{
    stdio_init_all();

    /* Esperar a que la PC abra el puerto USB (max. 5 s; en Wokwi no bloquea) */
    for (int i = 0; i < 50 && !stdio_usb_connected(); i++) sleep_ms(100);

    motor_init();            /* arranca con implementacion por REGISTROS, 0 % */

    printf("\n=== Actividad integradora: PWM + Motor DC (L298N) ===\n");
    printf("Ana Cisneros / Leslie Sanchez\n");
    motor_print_chip();
    printf("Escribe 'help'. Ejemplo: motor 50 derecha\n");

    repl_run();
    return 0;
}
