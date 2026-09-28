# RESPIN_FPGA_PINS — FPGA SoM B2B Interface Correctness Audit (riskybird v3)

**Scope:** Full correctness audit of the Trenz 4×5 SoM board-to-board (B2B) interface on
`riskybirdv3_base` — power delivery, IO-bank voltage compatibility, and programming
interfaces (FPGA config JTAG + separate RISC-V JTAG). Goes beyond the already-fixed
JB1.13 short and re-verifies **every** power/GND/signal pin with fresh eyes.

**Primary target:** TE0712 (Artix-7 XC7A200T), JTAG-alive on hardware (IDCODE 0x03636093).
**Secondary/drop-in target:** TE0841 (Kintex UltraScale KU040).

**Method / evidence base**
- Carrier pins & nets: fresh netlist export `kicad-cli sch export netlist` from
  `riskybirdv3_base/riskybirdv3.kicad_sch` (read-only; parsed with scratchpad `analyze.py`,
  `trace.py`). The exported netlist **differs from the one used in ECO §8.11** — the schematic
  was reworked since (camera DVP moved JB2→**JB3**; RISC-V JTAG + RISC-V/TSI UART added to
  **JB1/JB3**). All pin/net facts below are from the current netlist.
- Module pinout (JM1/JM2/JM3): transcribed pin-by-pin from the **TE0712 REV02 schematic
  SCH-TE0712-02-82I36-A, sheet 2 "B2B-Connectors"** and cross-checked against the cleaner
  **TE0712 REV03 SCH-TE0712-03-72C36-L, page 6** (which annotates PWR-IN/PWR-OUT direction and
  per-bank IO counts). Both rendered from the official Trenz PDFs.
- Mating convention: **Trenz "4×5 SoM Integration Guide"** — hermaphroditic even/odd swap
  (*"pin1 connects to pin2, and pin2 to pin1"*), i.e. carrier pin N ↔ module pin N±1
  (odd→+1, even→−1). Independently reconfirmed by the JTAG and power self-consistency below.
- TE0841/KU040 banks: **TE0841 TRM v.69 (TRM-TE0841-02.pdf)** §4.1 Table 2 (B2B↔bank map)
  and §6.5 Table 14 (bank voltage ranges).
- Bank rules: Xilinx **DS181** (Artix-7 HR I/O), UltraScale HP/HR bank limits (KU040).

---

## Executive summary

The interface is **fundamentally sound** — the hermaphroditic even/odd swap is implemented
correctly for all 260 terminals, power delivery is adequate, and the two-independent-JTAG
programming architecture is present and correctly separated. However the audit found **one
still-unfixed dead short and one new critical bank-voltage error**, plus several important
dual-target and debug-interface issues:

- 🔴 **JB1.13→JM1.14 3.3V→GND short is STILL in the schematic** (fix not yet applied).
- 🔴 **The 3.3 V optical-flow SPI + MOTION are routed to a 1.8 V FPGA bank (B15)** → SPI
  non-functional on TE0712 and **destructive on KU040** (HP bank, 1.8 V max).
- 🟠 **The 3.3 V TSI debug UART is routed to the 1.8 V camera bank (B13)** → level mismatch
  with the 3.3 V debug header; destructive on KU040.
- 🟠 **Dual-target hazard:** the entire JM2 connector is HP (≤1.8 V) on KU040 but has a 3.3 V
  HR bank on TE0712 — the drop-in claim is only valid if **nothing 3.3 V** rides JM2 (today the
  flow SPI does) and if VCCO feed-pin locations align.
- 🟠 **/RV_TRST orphaned** (RISC-V JTAG reset never reaches the FPGA).

Programming: both JTAG domains exist, are independent, and are at compatible voltage —
**config JTAG on JM2 config pins (3.3 V VREF_JTAG)**, **RISC-V soft-TAP on JM1/B16 user I/O
(3.3 V)**. Straps all correctly pulled; JTAGEN = 10 kΩ↓GND (selects JTAG-to-FPGA), correct.

**TE0842 does not exist** as a Trenz product — only **TE0841** carries the KU040 (verified via
Trenz shop + Digikey). "TE0842" is a typo/confusion for TE0841. Use TE0841 in all docs.

### Must-fix-for-respin (ranked)
1. **JB1.13: reassign net GND → +3V3** (the known 3.3V→GND short; still present). [CRITICAL]
2. **Move optical-flow SPI + MOTION off JM2/B15 onto JM1/B16** (a 3.3V-capable bank on *both*
   modules), or otherwise guarantee they see 3.3 V VCCO. [CRITICAL]
3. **Move the TSI UART off JB3/B13 onto a 3.3 V bank** (JM1/B16, with the RISC-V UART). [IMPORTANT]
4. **Verify the KU040 drop-in**: nothing 3.3 V may ride JM2 (HP ≤1.8 V); confirm VCCO feed-pin
   alignment (VCCIO16/13/15 ↔ B64/66/67/68_VCCO); confirm TE0712-NC pins are NC on KU040 too. [IMPORTANT]
5. **Route /RV_TRST to a spare B16 GPIO** (or confirm TRST-less RISC-V debug). [IMPORTANT]
6. Add pulls on the RISC-V soft-JTAG; resolve /FPGA_PROG_B & /VBAT dead-ends; confirm R46 DNP. [MINOR]

---

## Reference: authoritative module pinout used (TE0712 REV02/REV03 B2B sheet)

Bank/VCCO summary as wired by the carrier (mating applies the even/odd swap):

| Module bank | TE0712 type | Carrier VCCO feed | Resulting VCCO | On KU040 (TE0841) |
|---|---|---|---|---|
| **B16** (JM1 main I/O) | HR | VCCIO16 = JM1.9,11 ← **+3V3** | **3.3 V** | Bank **64 HR**, 1.2–3.3 V (carrier) ✅ 3.3 V OK |
| **B14** (JM1/JM2 I/O) | HR | module-internal (tied to 3.3VIN) | **3.3 V** | Bank **65 HR** 3.3 V on-module / part of **67 HP** on JM2 ⚠ |
| **B13** (JM3 / camera) | HR | VCCIO13 = JM2.1,3 ← **+1V8** | **1.8 V** | Bank **66 HP**, 1.2–1.8 V (carrier) — 1.8 V OK |
| **B15** (JM2 I/O) | HR | VCCIO15 = JM2.7,9 ← **+1V8** | **1.8 V** | Bank **67/68 HP**, ≤1.8 V (carrier) |
| config (JTAG) | — | VREF_JTAG = JM2.91 ← **+3V3** (R39) | **3.3 V** | config bank 3.3 V |

Module power **inputs** (must be fed 3.3 V by carrier): JM1 VIN 1/3/5, JM1 3.3VIN 13/14/15,
JM2 VIN 2/4/6/8 (= 10 pins). Module power **outputs** (offered back, may be left NC): JM2
10/12 = 3.3 V@1 A, JM1.39 = 1.8 V@1 A, JM2.19 = 1.5 V@1 A.

> **Correction to ECO §8.11.3:** JM2.10/12 are the module's **3.3 V PWR-OUT** (not "3.3VIN"
> inputs), and **JM2.1 = VCCIO13** (ECO said "NC"). Neither changes the short analysis, but the
> corrected map is used throughout this report. JM1.14 = **3.3VIN** is reconfirmed (the short pin).

---

## Findings

### CRITICAL

#### C1 — JB1.13 (GND) → JM1.14 (3.3VIN): 3.3 V→GND dead short, STILL UNFIXED
- **Evidence:** netlist `JB1.13 = GND`; under the even/odd swap JB1.13→JM1.14; TE0712 sheet 2
  shows JM1.13/14/15 = 3.3VIN (JM1.14 is the lone even-row 3.3VIN pin). Scan `analyze.py`
  Section A flags exactly this one collision.
- This is the day-one short from ECO §8.11 that killed U2. **The current schematic still has
  JB1.13 assigned to GND** — the reassignment has not been made. It reproduces the hard
  `+3V3`→GND short on every mate, on every board.
- **Fix (1 net):** reassign carrier **JB1 pin 13: GND → +3V3**. Then JB1.13(+3V3)↔JM1.14(3.3VIN)
  is correct and adds an 11th module-input pin. Re-run ERC + the Section-A scan (expect zero
  collisions). Also carry the input current-limit/eFuse from ECO §8.5/§8.9.7.

#### C2 — Optical-flow SPI + MOTION (3.3 V) land on FPGA Bank 15, which the carrier sets to 1.8 V
- **Evidence (netlist `trace.py`):** PMW3901 IC1 is powered from **+3V3** (`IC1.3 = +3V3`) →
  its SPI/MOTION I/O are **3.3 V**. Those nets route to JM2 / Bank 15:

  | Carrier net | series R | JB2 pin | → module pin | bank/VCCO |
  |---|---|---|---|---|
  | /NCS (IC1.19) | R27 47R | JB2.61 | JM2.62 = B15_L10_N | B15 @ **1.8 V** |
  | /MISO (IC1.18) | R28 47R | JB2.63 | JM2.64 = B15_L10_P | B15 @ **1.8 V** |
  | /SCLK (IC1.17) | R29 47R | JB2.65 | JM2.66 = B15_L18_P | B15 @ **1.8 V** |
  | /MOSI (IC1.16) | R34 47R | JB2.67 | JM2.68 = B15_L18_N | B15 @ **1.8 V** |
  | /MOTION (IC1.15)| —      | JB2.73 | JM2.74 = B15_L20_N | B15 @ **1.8 V** |

  (The same SPI nets fan out to the ESP32 via R63–R66 100R — a shared 3.3 V bus, confirming the
  3.3 V domain.) VCCIO15 = JM2.7,9 fed **+1V8** (`analyze.py` Section A) → **Bank 15 = 1.8 V**.
- **Why it's a fault:**
  - *TE0712 (HR bank @ 1.8 V VCCO):* PMW3901 outputs (/MISO, /MOTION) drive **3.3 V into a
    1.8 V-VCCO input** → exceeds Artix-7 HR recommended max (Vin ≤ VCCO+0.55 ≈ 2.35 V, DS181) →
    clamp-diode conduction / reliability/latch-up risk. FPGA outputs (/MOSI,/SCLK,/NCS) drive
    only ~1.8 V into a part needing VIH ≈ 0.7·3.3 ≈ 2.31 V → **SPI never reads logic-high →
    optical flow is non-functional.**
  - *KU040 (TE0841):* these pins are **Bank 67/68 = HP, max 1.8 V** (TRM §6.5). 3.3 V on an HP
    pin **exceeds absolute-max → destroys the bank.**
- **Fix:** relocate the flow SPI + MOTION to **JM1 / Bank 16** (TE0712 HR 3.3 V; KU040 Bank 64
  HR 3.3 V — 3.3 V-capable on both), grouping them with the other 3.3 V peripherals. (Merely
  changing VCCIO15 to +3V3 is *not* a valid dual-target fix: JM2 is HP-only on KU040, so 3.3 V
  VCCO there is illegal.)

---

### IMPORTANT

#### I1 — TSI debug UART (3.3 V) routed to the 1.8 V camera bank (B13)
- **Evidence:** `/TSI_UART_TX` = JB3.59→JM3.60 = B13_L12_N; `/TSI_UART_RX` = JB3.60→JM3.59 =
  B13_L13_N — both **Bank 13 @ 1.8 V** (camera VCCIO13). Their carrier ends are the debug
  header **J1.17 / J1.20**, whose reference power is **+3V3** (J1.2/10). Everything else on J1
  (both JTAG chains, RISC-V UART) is 3.3 V.
- The TSI (Chipyard tethered-serial) console therefore sits at **1.8 V on a 3.3 V-referenced
  debug connector**. An off-board FTDI driving /TSI_UART_RX at 3.3 V overstresses the 1.8 V B13
  input; the FPGA's 1.8 V TX may not meet the FTDI's 3.3 V VIH. On **KU040 this is HP (Bank 66,
  ≤1.8 V) → 3.3 V FTDI drive is destructive.**
- Note the RISC-V UART is done correctly (JB1.42/44 → B16 @ 3.3 V); only the TSI UART is
  mis-banked. **Fix:** move /TSI_UART_TX/RX to JM1/B16 (3.3 V) with the rest of J1's signals
  (recommended), or provide a dedicated 1.8 V VTRef pin on J1 and a 1.8 V FTDI channel for TSI.

#### I2 — Dual-target (KU040) bank-voltage divergence on JM2, and VCCO feed-pin alignment
- **Evidence (TE0841 TRM §4.1 Table 2 / §6.5 Table 14):** on KU040, **JM1 = Bank 64/65 HR
  (1.2–3.3 V)**, **JM3 = Bank 66 HP (≤1.8 V)**, **JM2 = Bank 67/68 HP (≤1.8 V)**. On TE0712,
  JM2 instead contains **B14 = HR @ 3.3 V** (module-internal) plus B15.
- Consequence: **JM2 can carry 3.3 V on TE0712 but not on KU040.** Today the only carrier 3.3 V
  content on JM2 is the flow SPI (finding C2) — fixing C2 also removes this KU040 hazard. Keep
  the rule: **no 3.3 V signal may ride JM2** for the design to remain KU040 drop-in. The
  dedicated config JTAG on JM2.93-99 is exempt (config bank, 3.3 V on both).
- **VCCO feed-pin alignment (verify):** the carrier feeds +3V3 to VCCIO16 (JM1.9,11), +1V8 to
  VCCIO13 (JM2.1,3) and VCCIO15 (JM2.7,9). Confirm from the TE0841 pin-out table that
  **B64_VCCO, B66_VCCO, B67/68_VCCO occupy the *same* JMx pins** — otherwise on KU040 those bank
  supplies land on the wrong pins (unpowered banks / wrong-rail feed). In particular verify the
  KU040 camera bank (B66, on JM3) gets its VCCO from the carrier's +1V8; on TE0712 that VCCO
  (VCCIO13) is delivered on **JM2**, not JM3, so the physical feed pin may not carry over.

#### I3 — /RV_TRST orphaned: RISC-V JTAG reset never reaches the FPGA
- **Evidence:** `/RV_TRST` is a **1-node net** (only `J1.16`); it is not on any JBx pin. The
  RISC-V soft-TAP has TCK/TMS/TDI/TDO wired (JB1.45/47/49/51 → B16) but **no TRST path to the
  FPGA fabric.**
- Many RISC-V debug DTMs can run TRST-less (5-TMS reset), but if the soft-TAP or the debugger
  expects TRST it is missing. **Fix:** route /RV_TRST to a spare B16 GPIO, or confirm the
  Chipyard/OpenOCD flow needs no hardware TRST and drop the header pin.

---

### MINOR

#### M1 — /FPGA_PROG_B (J1.7) dead-ends
`/FPGA_PROG_B` is a 1-node net (only J1.7). The TE0712 does **not** expose PROGRAM_B on the B2B
(reconfig is via JTAG / the on-module System-Controller CPLD), so there is no module pin to route
it to. The debug-header pin is non-functional as drawn — document it, or repurpose the position.

#### M2 — RISC-V soft-JTAG has no idle pulls → floats when the cable is unplugged
`/RV_TCK,/RV_TMS,/RV_TDI,/RV_TDO` connect only to J1 + the FPGA (B16); no carrier pull
resistors. With the debug cable off, these B16 inputs float (spurious-clock risk on TCK). Add
**TCK↓, TMS↑ (and TRST↑ if wired)**, or define internal PULLUP/PULLDOWN in the FPGA XDC.
(The config JTAG on JM2 is fine — pulled on-module.)

#### M3 — /VBAT (JB1.80) is a dead net on a TE0712-NC pin
`/VBAT` is a 1-node net (only JB1.80) → JM1.79 = **VBAT_IN, "Not used in TE0712"** (REV03
sheet). It connects to nothing on the carrier and nothing on the module: fully dead. If the
intent was battery-voltage sense into the FPGA, it is not wired. On **TE0841, VBAT_IN may be a
real RTC-backup input** — confirm intent before relying on it.

#### M4 — Module hard-reset (RESIN) not driven by the carrier
`RESIN` (JM2.18, via JB2.17) is only pulled up by R8 10 k — it is **not** connected to the
carrier system reset `/RST`. `/RST` instead goes to a B16 GPIO (JM1.33) so the FPGA fabric can
*read* it. Net effect: the carrier cannot assert a hard module reset. Confirm this is intended
(module self-configures; fabric handles reset in logic).

#### M5 — NOSEQ has both a 10 kΩ pull-up and a 0 Ω pull-down; confirm R46 is DNP
`NOSEQ` (JB1.8→JM1.7) is pulled up by R9 10 k→+3V3 **and** has R46 0 Ω→GND on the same net. If
R46 is populated it hard-grounds NOSEQ (and wastes ~0.33 mA through R9). ECO/memory says R46 is
DNP — **verify DNP in the schematic** so NOSEQ is a clean pull-up.

#### M6 — Carrier GND/1V8 on TE0712-NC module pins (harmless on TE0712, verify on KU040)
`JB2.30→JM2.29(NC)`, `JB2.40→JM2.39(NC)`, `JB3.55→JM3.56(NC)`, `JB2.6(+1V8)→JM2.5(NC)`. All
land on TE0712 no-connect pins (verified from the sheet), so harmless today. On **TE0841 the
same physical pins may be I/O or power** — grounding/1V8-driving them would then be a fault.
Cross-check against the TE0841 pin-out before claiming KU040 drop-in.

#### M7 — No fabric clock provided from the carrier
All carrier signals on the module's clock-input pins are NC: CLKIN2 (JM3.32/34), MGT_CLK1
(JM3.31/33). The design relies on the **TE0712 on-module clock generator (Si5338)** for the
FPGA/RISC-V clock. Confirm the Si5338 is populated/configured on the specific TE0712 ordering
variant to output the SoC clock (no external ref is supplied).

---

## Confirmed-correct (re-verified this pass — do NOT "fix")

- **Power delivery:** every carrier +3V3 lands on a module 3.3 V input or VCCIO16
  (JB1.2/4/6→VIN, JB1.14/16→3.3VIN, JB1.10/12→VCCIO16, JB2.1/3/5/7→VIN); every +1V8 lands on
  VCCIO13/VCCIO15 or NC; every GND lands on module GND — **the only exception is the C1 short.**
  After C1 is fixed there are **10 module 3.3 V-input pins** fed in parallel (LSHM ≈2 A/pin) →
  ample for Artix (~1 A) and KU040 (several A). GND return: 16 (JB1) + 14 (JB2) pins + shields.
- **/3v3OUT (JB2.9/11) and /VIDB (JB1.40):** float on the carrier by design — they tap the
  module's 3.3 V and 1.8 V **outputs** (JM2.10/12, JM1.39) which are intentionally unused. Harmless.
- **Two independent JTAG domains (as required):**
  - *FPGA config JTAG* — JB2.94/96/98/100 (`/FPGA_TMS/TDI/TDO/TCK`) → JM2.93/95/97/99, dedicated
    config-bank JTAG, VREF_JTAG = **3.3 V** (JM2.91 ← +3V3 via R39). Also on J1.4/5/6/8.
  - *RISC-V soft-TAP* — JB1.45/47/49/51 (`/RV_TCK/TDI/TDO/TMS`) → JM1.46/48/50/52 = **Bank 16
    user I/O @ 3.3 V**. Also on J1.12/13/9/14.
  - The two chains use **different physical pins on different connectors**, are electrically
    independent (RISC-V debug does not tunnel over config JTAG), and are **both 3.3 V**. ✓
- **Config/mode straps (all correct):** EN1 JB1.27 = 10 k↑+3V3; PGOOD JB1.29 = 10 k↑ (open-drain);
  MODE JB1.31 = 10 k↑; RESIN JB2.17 = 10 k↑ (run); NOSEQ JB1.8 = 10 k↑ (+R46 DNP, see M5);
  **JTAGEN JB1.90 = 10 k↓GND (R4) → LOW selects JTAG-to-FPGA for bitstream load** (matches the
  working hardware; on TE0841 the same pin = JTAGMODE, also "low for normal operation" — aligned).
- **Camera DVP:** D0–D7/PCLK/MCLK/FVLD/LVLD/TRIG/INT on JB3 → **Bank 13 @ 1.8 V**, matching
  HM01B0 IOVDD = +1V8 (`HM01B0.11=+1V8`). All 16 land on B13 I/O (not on the JM3 MGT pins) —
  correct. SCCB is level-shifted (PCA9306) onto the shared 3.3 V I²C, which reaches the FPGA on
  B16 — correct.
- **Motors:** /MOTOR1–4 (SI2302 gates) on JB1.32/35/99/100 → B16/B14 (3.3 V) — correct voltage.
- **Shared 3.3 V buses on B16 (3.3 V):** I²C /SDA,/SCL; ESP UART /U0TXD,/U0RXD; sensor
  interrupts /ACCEL_INT,/GYRO_INT,/INT_BARO,/TOF_Int(10 k↑); /RST,/ESP_RESET — all 3.3 V on a
  3.3 V bank. ✓
- **MGT transceivers & Ethernet:** every MGT_TX/RX/CLK pin (JM3) and every ETH_TD/RD pin (JM1)
  maps to an **unconnected** carrier JB pin — correctly unused/undriven. ✓
- **TE0842:** not a real Trenz part (only **TE0841** carries KU040). Treat as a typo for TE0841.

---

## Appendix — how to reproduce

```
kicad-cli sch export netlist --output fpga.net riskybirdv3_base/riskybirdv3.kicad_sch
python3 analyze.py   # Section A: power/GND swap scan; Section B: signal→bank + net members
python3 trace.py     # straps, series-R, SPI, PMW3901, camera
```
Scratchpad artifacts: `analyze.py`, `trace.py`, module-pinout crops
`r3_jm1_pwr.png / r3_jm2_pwr.png / r3_jm2_39.png / jm3_*.png`, TE0841 TRM text `te0841_trm.txt`.

**Sources:** carrier netlist (riskybirdv3.kicad_sch); TE0712 REV02 SCH-TE0712-02-82I36-A sheet 2;
TE0712 REV03 SCH-TE0712-03-72C36-L page 6; Trenz 4×5 SoM Integration Guide (mating swap);
TE0841 TRM v.69 §4.1 Table 2 + §6.5 Table 14; Xilinx DS181 (Artix-7 HR bank); UltraScale HP/HR
bank limits. ECO-001 §8.11 (prior JB1.13 analysis).
