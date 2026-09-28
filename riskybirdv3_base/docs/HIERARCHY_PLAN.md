# riskybird v3 — Schematic Hierarchy / Sheet-Split Plan

**Goal:** convert the flat `riskybirdv3.kicad_sch` (179 parts, 1 sheet) into a hierarchical design of functional sheets, **behavior-preserving** (one commit), *before* the ECO-001 power redesign goes into the Power sheet.

## Key simplification — use global labels, not sheet pins

The cross-sheet signals below are currently **local labels** (sheet-scoped once we split, so they'd stop connecting). The cleanest fix: **convert each cross-sheet signal to a *global label*** (right-click label → Change to Global Label). KiCAD global labels connect across all sheets with no sheet-pin wiring — far less work than hierarchical pins, and it scales as the SoM/camera get wired. Power rails use **power symbols** (already global). Reserve hierarchical sheet pins only if you specifically want a port to be sheet-local.

So the workflow per sheet is: **(1)** create the subsheet, **(2)** cut/paste its components in, **(3)** globalize the labels in its "ports" list, **(4)** verify (below).

## Sheets

| Sheet | # parts | File |
|---|---|---|
| Power | 48 | reuse `riskybirdv3_power.kicad_sch` (currently empty stub) |
| FPGA SoM | 6 | new |
| Sensors | 27 | new (ToF → reuse `ToF.kicad_sch` as a sub-sheet for U9) |
| Camera | 1 | new |
| ESP32-C6 | 1 (+ its decoupling) | new |
| USB/Debug | 28 | new |
| Motors & I/O | 60 | new |
| Mechanical | 8 | keep on root (no nets) |

## Components per sheet

**Power (48):** C1, C3, C5, C19, C23, C24, C34, C41, C42, C44, C45, C47, C48, C49, C50, C51, C52, C53, C54, C55, D7, D13, D14, J18, L3, R2, R3, R34, R37, R38, R39, R45, R46, R47, R48, R49, R50, R51, R52, R53, R54, SW3, U4, U7, U10, U11, U12, U13

**FPGA SoM (6):** R4, R6, R7, R8, R9, R10 *(+ JB1/JB2/JB3 — the connectors live here; they currently carry few real nets)*

**Sensors (27):** C6, C7, C8, C9, C10, C11, C12, C13, C14, C15, C16, C17, C18, C21, C22, C25, C38, C39, IC1, R25, R30, R31, R42, U1, U3, U5, U9

**Camera (1):** J13

**ESP32-C6 (1):** U14 *(see caveat — grab its local bypass caps too)*

**USB/Debug (28):** C4, C20, C26, C27, C28, C29, C30, C31, C32, C33, D10, D11, D12, FB1, FB2, J1, J8, R5, R11, R21, R24, R26, R27, R28, R29, U6, U8, Y1

**Motors & I/O (60):** C2, C35, C36, C37, C40, C43, C46, C56, C57, C58, C59, D1, D2, D3, D4, D5, D6, D8, D9, J2, J3, J4, J5, J6, J7, J9, J10, J11, J19, Q1, Q2, Q3, Q4, Q8, R1, R12, R13–R20, R22, R23, R32, R33, R35, R36, R40, R41, R43, R44, R55, R56, R57, SW1, SW2, SW4

**Mechanical (8, stay on root):** FID1, G***, H1–H4, TP1, TP6

> JB1/JB2/JB3 are listed for the SoM sheet but were auto-classed separately because they're mostly unwired today — place them on the SoM sheet.

## Cross-sheet signals to globalize (per sheet)

| Sheet | Signals to make global labels |
|---|---|
| Power | `/VSENSE` *(→ becomes `/VBAT_SENSE` per ECO-001)* |
| Sensors | `/SDA /SCL /MISO /MOSI /SCLK /NCS` (buses), `/GYRO_INT /ACCEL_INT /INT_BARO /TOF_Int /MOTION`, `/CamRST`, `/GPIO1–4`, `/LED_N` |
| ESP32 | `/SDA /SCL /MISO /MOSI /SCLK /NCS`, `/MOTOR1–4`, `/U0RXD /U0TXD`, `/CHIP_EN`, `/RGB_CTRL`, `/GPIO9`, `/CamRST`, `/LED_N`, `/VSENSE` |
| USB/Debug | `/JTAG_TCK_3V3 /JTAG_TDI_3V3 /JTAG_TDO_3V3 /JTAG_TMS_3V3 /JTAG_TRSTn_3V3`, `/UART0_RX_3V3 /UART0_TX_3V3`, `/U0RXD /U0TXD`, `/DTR /RTS` |
| Motors & I/O | union of the above (J19 mezzanine fans out **all** domains — see note) |
| SoM / Camera | none yet (wired in their workstreams) |

**Global power nets (use power symbols, not labels):** `GND`, `+3V3`, `+5V`, `+BATT`, `+1V8`. *(`+3V3_DRONE` / `+3V3_USB` exist today but are deleted by ECO-001.)*

## Notes / caveats

- **J19 (30-pin mezzanine) is cross-domain** — it carries JTAG, UART, SPI, I²C, all 4 motor lines, sensor INTs and CamRST, which is why Motors&I/O shows ~33 cross-sheet signals. Keeping it on Motors&I/O is fine (all those become global labels anyway); alternatively give connectors their own sheet later.
- **ESP32 decoupling:** the proximity pass may have filed U14's bypass caps under a physically-adjacent sheet (e.g. USB/Debug). When moving U14, visually grab its local 100 nF caps with it. Generally: **verify each IC's local decoupling moves with its IC.**
- **ToF:** `ToF.kicad_sch` already exists — reuse it as the U9/VL53L1 sub-sheet under Sensors instead of making a new file.

## Verification (proves the split changed nothing electrically)

1. **Before** the split (already captured): `kicad-cli sch export netlist --format kicadsexpr -o before.net riskybirdv3.kicad_sch`
2. **After** the split: export `after.net` the same way.
3. Compare **node membership of every net** (set of `ref.pin` per net), ignoring net *names* (local→global renames are expected). Identical sets = connectivity preserved. I'll run this diff for you.
4. `kicad-cli sch erc` clean.
5. Commit the split on its own, then ECO-001 lands as a separate commit in the Power sheet.
