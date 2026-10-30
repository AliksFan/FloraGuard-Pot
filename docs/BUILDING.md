# Build status and future setup

**This candidate is not directly buildable as supplied.** It contains application C files and local headers, rather than the complete original toolchain project. No build, installation, simulation, flashing or board test was performed during its preparation.

## Deliberately absent items

- `STARTUP.A51`: the original Keil startup file contains a third-party copyright notice. It is withheld pending redistribution review.
- `FloraGuard_Pot.uvproj`: the original project references that withheld startup file. It is not included as a misleadingly complete build entry.
- `STC8H.h`: the external STC device header was not present in the inspected source set. Its correct version and permitted source need to be identified.
- `intrins.h`: the application uses a Keil C51 compiler header, which is not bundled.
- Historical HEX files: none has a verified association with this source snapshot, configuration and physical board.

No replacement startup code or device headers have been invented or obtained from an unverified source.

## Procedure for a later authorized build

The following is a proposed setup procedure, not a record of successful execution:

1. Confirm the physical MCU and board revision. Identify the legitimate Keil C51/MCS-51 environment and matching STC device support used for the project.
2. After the relevant redistribution rights are resolved, restore or create a reviewed target configuration and appropriate startup support in a separate working copy. The recorded original target was STC8H1K08 Series.
3. Add the main application files listed below. Configure the local firmware include directory and the legitimate external device/compiler headers.
4. Check `config.h`: the snapshot sets `BATTERY_CALIBRATION_MODE=0`, `FOSC_HZ=11059200UL` and `BAUD_RATE=9600UL`. Reconcile these calculations with the actual oscillator and programming configuration; the previously inspected original project metadata used a different clock value.
5. Review the startup UART ordering described in the [firmware notes](FIRMWARE_NOTES.md) before attempting device execution. Then build in the working copy and retain the exact toolchain, input hashes, configuration, warnings and output digest.
6. Treat board programming and measurement as a separate verification step. A generated HEX alone does not establish correct operation.

Main application C files: `Main.c`, `UART.c`, `delay.c`, `io.c`, `adc.c`, `PWM.c`, `rgb.c`, `sleep.c` and `isr.c`. Their local headers, including `config.h`, are retained beside them.

## Calibration entry points

Normal `Main.c` contains an integrated battery-calibration mode selected by `BATTERY_CALIBRATION_MODE`. Changing that switch and rebuilding would be a future operation in a working copy; no source configuration was changed here.

`firmware/reference_unbuilt/Main_BatCal.c` is a separate, unbuilt entry point. If reviewed for later use, it needs its own target, the parent `firmware/` include directory and the required dependencies. Do not compile it together with normal `Main.c`, because both define an entry point.

The source's battery divider and gain constants are implementation assumptions, not a calibration report. Compare serial readings with independent measurements before claiming voltage accuracy.
