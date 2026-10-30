# Firmware source review

This document records static findings in the preserved application source. Application files were copied without code edits. No compiler, simulator, programmer or board was run during preparation.

## Source behavior

| Signal or setting | Implementation in this snapshot |
| --- | --- |
| Battery input | P3.5 / ADC13 |
| Threshold potentiometer | P3.4 / ADC12 |
| Soil-sensor input | P3.6 / ADC14 |
| Sensor power | P1.1 PMOS control; low enables power |
| Sensor excitation | PWM6 mapped to P5.4; requests 150 kHz / 50% |
| RGB | P1.3 / P1.4 / P1.5; common-anode initialization |
| Charging-state inputs | P3.2 / P3.3 |
| Button | P3.7; configured 1200 ms hold before software reset |
| UART1 | P3.0 / P3.1; configured 9600 baud |

The application powers and excites the sensing circuit, reads analog inputs, compares relative mapped values and selects indication/sleep behavior. No pump, valve, relay or irrigation-actuator control path was identified. PWM serves sensor excitation.

## Findings requiring interpretation or verification

- **Soil states:** the decision function in `Main.c` returns state 1 or 2. A state 3 appears in comments and diagnostics without an assignment path. The mapped scale is not a validated volumetric water-content percentage.
- **Sleep naming:** `sleep.c` assigns `10800UL` to `SEC_12H`, a nominal three-hour base despite the name and old messages. Exact elapsed sampling intervals are not measured and include indication and wake behavior.
- **Button naming:** `Hook_OnLongPress_Sleep` calls software reset. The short-press hook is empty despite an older header description. Documentation should follow the function body rather than the old name.
- **Timer comments:** some comments refer to Timer2 for the interrupt tick, while initialization uses Timer1. UART baud-rate generation uses Timer2.
- **Clock assumptions:** `config.h` supplies 11.0592 MHz for calculations. The physical oscillator, programming settings and original target metadata have not been reconciled.
- **Startup UART ordering:** normal `Main.c` writes a boot message before `Board_Init()` initializes UART. The transmit routine polls a flag. This is a potential startup-blocking path requiring review, not a reproduced hardware failure.
- **Measurement and timing assumptions:** busy-loop delays are uncalibrated. ADC code assumes a 10-bit result and 3300 mV reference. Battery conversion uses a divider factor of 6 and gain of 1101/1000. Accuracy, timing and current consumption need measurements against the intended board.

## Scope of the source subset

The main application comprises nine C files and nine local headers. The separate `reference_unbuilt/Main_BatCal.c` is retained for calibration context and is not part of the main target. Startup support, the original Keil project and external headers are excluded; see the [build guide](BUILDING.md).

Scoped scans of the candidate application files did not identify common personal-path, contact-address, private-key, token or credential-assignment patterns. This is not proof that every possible sensitive string or third-party contribution has been identified. No explicit third-party notice was found in these C/H files, but that does not establish original authorship or permission to publish them.

Before future additions or releases, review the provenance, redistribution terms and exact publication scope of new material. No project license or tested release status is assigned by this review.
