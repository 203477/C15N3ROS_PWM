# Comparación técnica RP2040 vs RP2350 (enfocada al PWM)

| Aspecto | RP2040 (Pico) | RP2350 (Pico 2 W) | Impacto en este proyecto |
|---|---|---|---|
| Núcleos | 2× Cortex-M0+ | 2× Cortex-M33 **o** 2× Hazard3 RISC-V (seleccionable) | Mismo código C |
| clk_sys por defecto (SDK) | 125 MHz | 150 MHz | `DIV` para 5 kHz: 25 → 30 |
| SRAM | 264 KB | 520 KB | Sin impacto |
| GPIO | 30 | 30 (RP2350A) / 48 (RP2350B) | Pico 2 W usa RP2350A |
| Slices PWM | 8 (16 canales) | 12 (24 canales) | Slices 8–11 solo alcanzan GPIO 40–47 (RP2350B) |
| Base PWM | `0x40050000` | `0x40058000` | Direcciones distintas |
| `PWM_EN` global | `+0xA0` | `+0xF0` | 8×0x14 vs 12×0x14 |
| Formato de CSR/DIV/CC/TOP | Igual | Igual | Misma lógica |
| Base RESETS / IO_BANK0 / PADS | `0x4000C000` / `0x40014000` / `0x4001C000` | `0x40020000` / `0x40028000` / `0x40038000` | Hay que cambiarlas |
| Bit de reset PWM | 14 | 16 | Bits de reset reordenados |
| Pad al salir de reset | Sin aislamiento | `ISO=1` (aislado) | Limpiar bit 8 de `PADS GPIOn` |
| SIO `OUT_SET/CLR/OE_SET` | `0x14 / 0x18 / 0x24` | `0x18 / 0x20 / 0x38` | Offsets distintos (se añadieron registros HI) |
| Seguridad | — | TrustZone-M, arranque seguro, OTP | No usada |
| Inalámbrico (Pico W) | CYW43439 (Pico W) | CYW43439 (Pico 2 W) | LED integrado va por el CYW43, no por GP25 |
| SDK | ≥ 1.x | ≥ 2.0 | `PICO_BOARD=pico` vs `pico2_w` |
| Macros de compilación | `PICO_RP2040` | `PICO_RP2350` | Usadas en `chip_map.h` |
| Estructuras `hardware/structs` | `pwm_hw->slice[0..7]` | `pwm_hw->slice[0..11]` | El SDK oculta la diferencia |

## ¿Qué abstrae el SDK?

El mismo `pwm_sdk.c` compila en ambos chips sin `#if`. El SDK:
- elige las direcciones base correctas (`PWM_BASE`, `RESETS_BASE`…),
- calcula slice y canal (`pwm_gpio_to_slice_num`, `pwm_gpio_to_channel`),
- saca los periféricos del reset durante el arranque,
- limpia `ISO` en RP2350 dentro de `gpio_set_function`,
- empaqueta `DIV` 8.4 desde un `float`.

**Ventajas:** portabilidad, menos errores, código legible. **Limitaciones:** oculta lo que pasa en el hardware, el `float` introduce redondeo (aquí se calcula a partir del valor entero ×16 para que sea exacto), y no permite operaciones específicas (por ejemplo arrancar varios slices en el mismo ciclo con `PWM_EN` salvo `pwm_set_mask_enabled`).

## Verificación de la comparación (a completar en laboratorio)

Con el comando `motor info` en cada tarjeta, registren una captura con los valores de CSR, DIV, TOP y CC en modo `reg` y modo `sdk`, y confirmen que coinciden entre sí y con la tabla de `docs/mapa_registros.md`.
