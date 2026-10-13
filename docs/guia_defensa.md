# Guía de defensa individual (ambas integrantes deben poder responder todo)

**¿Cuál es la dirección base?** PWM: `0x40050000` en RP2040, `0x40058000` en RP2350. Cada slice ocupa `0x14` bytes; el slice 0 empieza en la base.

**¿Qué registro modificas?** Para la velocidad, `PWM0_CC` (offset `0x0C`), campo A (bits 15:0). Para frecuencia, `PWM0_DIV`. Para el periodo, `PWM0_TOP`. Para arrancar, `PWM0_CSR.EN`. Para conectar el pin, `GPIO16_CTRL.FUNCSEL = 4`.

**¿Qué significa este bit?** Ejemplos: `CSR.EN` = habilita el contador; `CSR.PH_CORRECT` = cuenta ascendente y descendente; `A_INV` = invierte la salida; `PADS.ISO` (RP2350) = pad aislado del circuito; `FUNCSEL` = qué periférico controla el pin.

**¿Por qué utilizarías `volatile`?** Porque el registro está mapeado a hardware: su valor puede cambiar sin que el programa lo escriba, y escribirlo produce un efecto. Sin `volatile`, el compilador puede eliminar escrituras “repetidas”, reordenarlas o guardar la lectura en un registro de CPU.

**¿Qué hace esta función del SDK?** Ejemplos: `gpio_set_function(16, GPIO_FUNC_PWM)` escribe FUNCSEL=4 (y limpia ISO en RP2350). `pwm_set_wrap` escribe TOP. `pwm_set_clkdiv` escribe DIV. `pwm_set_gpio_level` escribe CC_A o CC_B según el pin. `pwm_set_enabled` pone/quita CSR.EN.

**¿Qué cambia entre RP2040 y RP2350?** Base del PWM, número de slices (8 → 12), `PWM_EN` (`0xA0` → `0xF0`), bases y bits de reset, `ISO` en pads, offsets de SIO, `clk_sys` por defecto (125 → 150 MHz, por tanto el `DIV`), núcleos y SDK ≥ 2.0.

**¿Qué contiene `argv[1]`?** En `motor 50 derecha`: `argc = 3`, `argv[0] = "motor"`, `argv[1] = "50"` (texto, no número; se convierte con `strtol` y se valida 0–100), `argv[2] = "derecha"`.

**¿Qué ocurre físicamente al ejecutar el comando?** El REPL tokeniza la línea → `cmd_motor` valida → `apply()` pone IN1=1, IN2=0 (registros SIO) y escribe `CC_A = 500` → el contador del slice 0 cuenta de 0 a 999 a 5 kHz y mantiene GP16 en alto mientras `CTR < 500` → el L298N entrega a OUT1/OUT2 pulsos del 50 % con la tensión de la batería → la inductancia del motor promedia la corriente → gira a ~50 % de su velocidad máxima.

## Preguntas extra probables
- ¿Por qué `TOP = 999`? 1000 pasos = resolución de 0.1 % y la división de reloj resulta exacta a 5 kHz.
- ¿Por qué 5 kHz? Por encima del rango de mayor zumbido audible y dentro de lo que el L298N (bipolar) maneja con pocas pérdidas.
- ¿Por qué se requiere tierra común? El voltaje de IN1/IN2/ENA se mide respecto a la tierra del L298N.
- ¿Por qué pausar al invertir el sentido? Evita el pico de corriente por contra-fem y estrés mecánico.
- ¿Qué pasa con `CC > TOP`? La salida queda siempre en alto (100 %).
- ¿Para qué sirven los alias +0x2000 / +0x3000? Para poner o limpiar bits de forma atómica sin leer-modificar-escribir.
