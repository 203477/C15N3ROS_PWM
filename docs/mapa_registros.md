# Mapa de registros utilizado — PWM slice 0 (GP16 canal A)

Todos los registros son de **32 bits**, alineados a 4 bytes. Se accede con `volatile uint32_t *`.
Fórmulas comunes a ambos chips:

- `PWM_SLICE(n)  = PWM_BASE + n * 0x14`
- `GPIOn_CTRL    = IO_BANK0_BASE + 8*n + 4`
- `PADS GPIOn    = PADS_BANK0_BASE + 4 + 4*n`
- GPIO0..29 → `slice = (gpio >> 1) & 7`, `canal = gpio & 1` (0 = A, 1 = B). GP16 → slice 0, canal A.

## 1. Direcciones por chip

| Bloque | RP2040 | RP2350 |
|---|---|---|
| RESETS base | `0x4000C000` | `0x40020000` |
| IO_BANK0 base | `0x40014000` | `0x40028000` |
| PADS_BANK0 base | `0x4001C000` | `0x40038000` |
| PWM base | `0x40050000` | `0x40058000` |
| SIO base | `0xD0000000` | `0xD0000000` |

## 2. PWM (slice 0)

| Registro | Offset | RP2040 | RP2350 | Acceso |
|---|---|---|---|---|
| CSR | `0x00` | `0x40050000` | `0x40058000` | R/W |
| DIV | `0x04` | `0x40050004` | `0x40058004` | R/W |
| CTR | `0x08` | `0x40050008` | `0x40058008` | R/W |
| CC | `0x0C` | `0x4005000C` | `0x4005800C` | R/W |
| TOP | `0x10` | `0x40050010` | `0x40058010` | R/W |
| EN (global) | `0xA0` / `0xF0` | `0x400500A0` | `0x400580F0` | R/W |

### Campos y significado

**CSR** — control del slice
| Bits | Campo | Significado | Valor usado |
|---|---|---|---|
| 0 | EN | 1 = el contador corre | 1 (al final de la configuración) |
| 1 | PH_CORRECT | 1 = modo phase-correct (cuenta arriba y abajo) | 0 |
| 2 | A_INV | invierte la salida A | 0 |
| 3 | B_INV | invierte la salida B | 0 |
| 5:4 | DIVMODE | 0 = el divisor corre libre (modo normal) | 0 |
| 6 | PH_RET | retarda la fase 1 ciclo | 0 |
| 7 | PH_ADV | adelanta la fase 1 ciclo | 0 |

**DIV** — divisor de reloj de punto fijo 8.4
| Bits | Campo | Significado |
|---|---|---|
| 11:4 | INT | parte entera (1..255) |
| 3:0 | FRAC | parte fraccionaria en 1/16 |

El valor del registro es `div × 16`. `f_PWM = clk_sys / (div × (TOP+1))`.

| f deseada | RP2040 (125 MHz) | RP2350 (150 MHz) |
|---|---|---|
| 1 kHz | `DIV=0x7D0` (125.0) | `DIV=0x960` (150.0) |
| 5 kHz | `DIV=0x190` (25.0) | `DIV=0x1E0` (30.0) |
| 10 kHz | `DIV=0x0C8` (12.5) | `DIV=0x0F0` (15.0) |
| 20 kHz | `DIV=0x064` (6.25) | `DIV=0x078` (7.5) |

**CTR** — contador actual (16 bits).

**CC** — nivel de comparación
| Bits | Campo | Significado |
|---|---|---|
| 15:0 | A | la salida A está en alto mientras `CTR < A` |
| 31:16 | B | ídem para el canal B |

Con `TOP = 999`: `CC_A = 500` → 50 %; `CC_A = 1000` (> TOP) → siempre en alto (100 %); `CC_A = 0` → siempre en bajo.

**TOP** — valor de wrap (bits 15:0). El contador va de 0 a TOP y vuelve a 0 → periodo = `TOP+1` ciclos de `clk_sys/div`. Con 999 se tienen 1000 pasos (0.1 % de resolución).

**EN (global)** — un bit por slice (bit n = slice n); permite arrancar varios slices a la vez. Se usó `CSR.EN` por slice.

## 3. Selección de función y pad

**GPIOn_CTRL** (IO_BANK0, offset `8n+4`) — `GPIO16_CTRL` = `+0x84`: RP2040 `0x40014084`, RP2350 `0x40028084`.
| Bits | Campo | Valor |
|---|---|---|
| 4:0 | FUNCSEL | 4 = PWM · 5 = SIO (GPIO por software) |

**PADS_BANK0 GPIOn** — `GPIO16` = `+0x44`: RP2040 `0x4001C044`, RP2350 `0x40038044`.
| Bit | Campo | Nota |
|---|---|---|
| 0 | SLEWFAST | velocidad de flanco |
| 1 | SCHMITT | entrada Schmitt trigger |
| 2 / 3 | PDE / PUE | pull-down / pull-up |
| 5:4 | DRIVE | 2, 4, 8, 12 mA |
| 6 | IE | habilita entrada |
| 7 | OD | deshabilita salida |
| **8** | **ISO** | **Solo RP2350**: aislamiento del pad. Sale en 1 del reset; se debe poner en 0. |

## 4. RESETS

| Registro | Offset | Acceso | Uso |
|---|---|---|---|
| RESET | `0x00` | R/W | 1 = periférico en reset. Se limpian los bits requeridos |
| RESET_DONE | `0x08` | R | 1 = el periférico ya salió de reset (esperar antes de usarlo) |

| Periférico | Bit RP2040 | Bit RP2350 |
|---|---|---|
| IO_BANK0 | 5 | 6 |
| PADS_BANK0 | 8 | 9 |
| PWM | 14 | 16 |

## 5. SIO (pines de dirección IN1/IN2)

| Registro | RP2040 | RP2350 | Acceso |
|---|---|---|---|
| GPIO_OUT_SET | `0xD0000014` | `0xD0000018` | W |
| GPIO_OUT_CLR | `0xD0000018` | `0xD0000020` | W |
| GPIO_OE_SET | `0xD0000024` | `0xD0000038` | W |

Un `1` en el bit *n* afecta solo al GPIO *n*; los `0` no cambian nada (por eso no hace falta leer-modificar-escribir).

## 6. Alias atómicos (APB)

Cada registro de periférico APB se repite en 4 direcciones:

| Dirección | Efecto de escribir `v` |
|---|---|
| `reg + 0x0000` | `reg = v` |
| `reg + 0x1000` | `reg ^= v` |
| `reg + 0x2000` | `reg \|= v` (SET) |
| `reg + 0x3000` | `reg &= ~v` (CLR) |

Evita leer-modificar-escribir y condiciones de carrera entre núcleos o interrupciones.

## 7. Conceptos de punteros usados

| Concepto | En este proyecto |
|---|---|
| Dirección | `0x4005000C` es donde vive `PWM0_CC` |
| Puntero | `(volatile uint32_t *)0x4005000C` |
| Desreferenciación | `*(ptr) = 500;` escribe en el periférico, no en RAM |
| `volatile` | Evita que el compilador elimine, junte o reordene accesos: el hardware cambia por sí solo y escribir tiene efectos |
| Tamaño | 32 bits → `uint32_t`; un acceso de 8/16 bits puede ser ignorado o dar error |
| Máscara | `(v & ~0x1F) \| 4` cambia solo FUNCSEL sin tocar otros campos |
| Campo | `CC_A` = bits 15:0; `CC_B` = bits 31:16 |

## 8. Ejemplo numérico completo — `motor 50 derecha` @ 5 kHz

| Registro | RP2040 | RP2350 |
|---|---|---|
| `GPIO16_CTRL.FUNCSEL` | 4 | 4 |
| `PWM0_TOP` | 999 | 999 |
| `PWM0_DIV` | `0x190` | `0x1E0` |
| `PWM0_CC` | `0x000001F4` | `0x000001F4` |
| `PWM0_CSR` | `0x00000001` | `0x00000001` |
| SIO IN1 / IN2 | 1 / 0 | 1 / 0 |
