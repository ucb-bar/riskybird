# riskybird v3 — Board Bring-Up Debug Log

**Project:** riskybird v3 (`riskybirdv3_base/`)
**Board rev:** v3 — first assembled spin
**Author:** Dima Nikiforov
**Started:** 2026-08-05

Running log of issues found during bring-up of the assembled v3 main board. Each
entry records the symptom, root cause, the workaround applied to **these**
(already-fabricated) boards, and the preventive change queued for the **next spin**.

---

## Issue index

| ID | Date | Component | Severity | Status |
|----|------|-----------|----------|--------|
| BU-001 | 2026-08-05 | IC1 — PMW3901 optical flow | Not populated | Workaround: hand-solder + alt vendor. Next-spin fix pending. |
| BU-002 | 2026-08-05 | U10 — 1V8 LDO (rail reads 2.3 V) | Over-voltage on 1.8 V rail — likely back-feed | BOM correct (C133796). No 2.3 V variant exists in SC-70 → likely BACK-FEED on a good TLV70018. Marking should read `ODV`; confirm w/ load test. **Update 2026-08-11:** populating **IC1 (PMW3901)** pulls the 1V8 rail from 2.3 V → **~1.9 V** (still ~0.1 V high, much better). This is the load-test confirmation: an unloaded rail floats high (back-feed), a real load drags it toward nominal → the LDO is fine; over-voltage was a light-load/back-feed artifact. |
| BU-003 | 2026-08-05 | U14 — ESP32-C6 USB enumerate/disconnect loop | Not a defect | RESOLVED ✓ — flashed Zephyr hello_world; boots stably, banner captured. |
| BU-004 | 2026-08-05 | 3V3/1V8 float to ~0.5 V with system switch OFF | Incomplete power-down (benign) | Back-feed bypassing Q5 via USB (U6→ESP ESD) + batt-sense (R30→ADS7128 ESD). Switch/Q5 work correctly. |
| BU-005 | 2026-08-05 | D7 / U4 — charger LED blinks rapidly | Benign (no battery) | MCP73831 no-battery cycling. Expected on USB-only bring-up. |
| BU-006 | 2026-08-05 | FPGA SoM (Artix-7 TE0712) brownout-resets on USB power | Power budget | USB→VSYS capped ~1.5 A by U8 (LM66100); Artix exceeds it. Power the FPGA from battery (Q9/LM74700 path), not USB. |
| BU-007 | 2026-08-06 | ESP32 stuck in ROM download mode — app never runs (no console) | Reset-state artifact | RESOLVED ✓ by power-cycle (unplug/replug). Chip was stuck in ROM download after esptool/USB-JTAG resets; a clean POR boots the app. NOT a GPIO9 short (a replug wouldn't fix that). |
| BU-008 | 2026-08-06 | 4× VL53L5CX side-facing ToF | Working ✓ | All four detected (id 0xF002 @ 0x29), readdressed 0x31–0x34, init'd, streaming distance + 4×4 grid. |
| BU-009 | 2026-08-06 | Board drops off USB after battery finishes charging | Power arbitration (to investigate) | Needed a battery unplug/replug to re-enumerate. Suspected source-ORing glitch at charge-termination (USB/U8 LM66100 ↔ battery/U13 LM74700 + Q9), or ESP brownout at the transition. |
| BU-010 | 2026-08-11 | U14 — *appeared* "stuck in ROM download mode" after IC1 rework | RESOLVED ✓ — **capture artifact, not a HW fault** | **FALSE ALARM.** The board was running the app the whole time; the host's serial-port *open* asserted DTR/RTS, which map to **BOOT/EN** on the C6 USB-Serial/JTAG → reset into download **every time we tried to read it**. Opening with **DTR/RTS deasserted** (`dtr=rts=False` before open) → OG board boots normally + PMW3901 images (SQUAL ~86). ESP/GPIO9 healthy; the pin-23 rework was unnecessary. **Fix:** capture tools now open with DTR/RTS deasserted. (E3 GPIO9 pull-up still nice-to-have but was not the issue.) |

---

## Next-Spin ECO Checklist

Consolidated design changes for the next board spin, distilled from the BU entries
below. **All require KiCAD closed.** Kept here (not a separate file) so findings and
their fixes stay in one place.

| # | Change | Where | Source | Priority |
|---|--------|-------|--------|----------|
| E1 | **IC1 (PMW3901): clear the PCB DNP flag + pin `LCSC C43496881` / MPN in the schematic** so JLC assembles it (the LCSC SKU includes the lens; JLC glued it on last time). Removes the hand-solder/hot-air step. | sch + pcb | BU-001 | **High** |
| E2 | **Pin `LCSC Part` on the TLV700 LDOs** — U7 `C398921` (2V8), U10 `C133796` (1V8), U11 `C2878901` (1V5) — so JLC can't substitute a sibling Vout variant. | sch | BU-002 | **High** |
| E3 | **Add an external 10 kΩ pull-up on GPIO9 → 3V3** for boot-strap robustness (currently relies only on the chip's internal ~45 kΩ). | sch + pcb | BU-003, BU-007 | Med |
| E4 | **Add ~100 kΩ bleed 1V8 → GND** so the rail sits at ~1.8 V under no load (the PCA9306 VREF1 bias current floats an unloaded 1V8 to ~2.3 V). | sch + pcb | BU-002, BU-004 | Low (opt) |
| E5 | **USB power budget for the FPGA:** USB→VSYS is capped at ~1.5 A by U8 (LM66100). If USB-powered FPGA bring-up is ever wanted, upsize the USB ideal-diode/path; else document **battery-required** for the SoM. | consider | BU-006 | Decision |

**Explicitly NOT changing:**
- **SW5 main switch** — verified correct; the 2-pin SPST-symbol-on-SPDT-footprint works (BU-004). No change.
- **BU-004 off-state back-feed** (3V3/1V8 → ~0.5 V when switched off) — inherent to USB/ESD + batt-sense paths, sub-threshold µA, accepted. Only revisit if a hard-zero off-rail is required (VBUS-gated isolation / series switch on the batt-sense tap).

---

## BU-001 — Optical-flow sensor (IC1, PMW3901) not assembled

**Symptom.** Received assembled boards; the PMW3901 optical-flow sensor (IC1,
bottom side) is not populated.

**Root cause — two independent faults, either one sufficient to prevent assembly:**

1. **DNP desync, schematic ↔ PCB (decisive cause).** IC1 is flagged **DNP on the
   PCB** but is **not DNP in the schematic**.
   - The BOM is exported from the *schematic* → IC1 **does** appear in the BOM.
   - The CPL/placement file is exported from the *PCB* with `--exclude-dnp` →
     IC1 is **absent from the CPL**.
   - JLC populates only parts present in **both** the BOM and the CPL. With no
     placement location, IC1 was silently skipped.
2. **No LCSC part number.** IC1's BOM line had blank MPN + LCSC, so even with a
   placement JLC could not have auto-sourced the part.

**Audit.** A full schematic-DNP vs PCB-DNP comparison across every component on
the board found **IC1 is the only DNP mismatch** — no other part is silently
un-placed.

**Part details.**
- Ref **IC1**, footprint `riskybird:PMW3901MBTXQT`, **bottom side** (B.Cu),
  ≈ (73.85, 99.9) mm, 29 pads.
- MPN **PMW3901MB-TXQT**, package COB-28.
- LCSC **C43496881** (JLC stock ≈ 596, *extended* part) — not used this run.

**Workaround for these boards (already fabricated):**
- **Hand-solder IC1.** Source **PMW3901MB-TXQT from an alternate vendor** (JLC
  inventory is thin and was not used this run).
- COB-28 package, bottom-side — hot-air / reflow recommended over hand-iron.

**Preventive fix for the next spin (requires KiCAD closed):**
1. **Clear the DNP flag on IC1 in the PCB** → it re-enters the CPL.
2. Add **`LCSC Part = C43496881`** + `MPN = PMW3901MB-TXQT` to IC1 in the
   schematic → JLC auto-sources it.
3. Regenerate CPL + BOM and **verify IC1 appears in both files**.

**General lesson.** The BOM (from the schematic) and the CPL (from the PCB) can
disagree on DNP state *and* on reference designators. **Before every order, diff
the BOM designators against the CPL designators** — any part in one but not the
other silently fails to assemble. (Same failure class as the connector-board
LED reference-designator desync, fixed 2026-08.)

---

## BU-002 — 1V8 rail measures 2.3 V at the test point

**Symptom.** The 1V8 test point reads **2.3 V** instead of 1.8 V.

**Rail topology.** 1V8 is generated by **U10**, a **fixed-output** LDO
`TLV70018DCKR` (SC-70-5): pin 1/3 = +3V3 (IN / EN), pin 2 = GND, **pin 5 = +1V8
OUT**. There is **no feedback divider** — the output voltage is intrinsic to the
part. Net `+1V8` loads: HM01B0 pin 11 (IOVDD), JB2 pins 2/4/6/8/10 (FPGA
PL_1V8 bank), U12 pin 2 (PCA9306 VREF1), R68/R69 (CAM I2C pull-ups),
R25→IC1 VDD (3 Ω series), decoupling caps.

**Root cause (leading hypothesis): wrong LDO variant fitted.**
- U10 is a *fixed* LDO, so 2.3 V cannot come from a resistor/design error.
- **2.3 V is exactly the output of the TLV70023** — the same-package, same-family
  sibling (TLV700**xx**, last two digits = Vout: …018 = 1.8 V, **…023 = 2.3 V**).
- **U10 had no `LCSC Part` pinned in the schematic** → matched at JLC by value
  string → prime opportunity for a sibling-variant substitution.
- Correct part: **`TLV70018DCKR` → LCSC `C133796`** (SC-70-5, stock ~5.4k).

**Secondary hypothesis (less likely): back-feed float.** An LDO cannot *sink*
current; any injection into the net from 3V3 (e.g. U12 PCA9306 VREF2→VREF1
leakage, or R68/R69 pulling up while the camera is unpowered) would let the rail
float above 1.8 V. The *exact* 2.3 V reading argues against this (a float would
be load-dependent and messy), but the load test below settles it.

**⚠️ Do not operate downstream loads on this rail until fixed** — 2.3 V is
over-spec for HM01B0 IOVDD (abs-max ~2.0 V), the FPGA PL_1V8 bank VCCO
(abs-max ~2.0 V — risk of I/O damage), and PMW3901 VDD. **Fix 1V8 before
hand-soldering IC1 (BU-001).**

**Bench confirmation (fastest first):**
1. **Read U10's SC-70-5 top marking** under magnification. The intended TLV70018
   (1.8 V) = **`ODV`** (see table below).
2. Check **which LCSC was selected for U10 at JLC** vs `C133796`.
3. **Load test:** ~100 Ω from 1V8→GND (~18 mA). Holds 2.3 V stiffly ⇒ it's the
   LDO itself; sags ⇒ U10 is fine and something is back-feeding the net.

**TLV700 SC-70-5 (DCK) top-marking codes** (TI datasheet SLVSA00E, Package Option
Addendum, 10-Nov-2025). The R/T order suffix does not change the marking:

| Device (DCK) | Vout | Top mark |
|---|---|---|
| TLV70012 | 1.2 V | `ODT` |
| TLV70015 | 1.5 V | `ODU`  (= U11's part) |
| **TLV70018** | **1.8 V** | **`ODV`  ← intended U10** |
| TLV70025 | 2.5 V | `QTP` |
| TLV70028 | 2.8 V | `ODW`  (= U7's part) |
| TLV70030 | 3.0 V | `ODR` |

**There is NO 2.0/2.1/2.2/2.3 V option in the SC-70 (DCK) package** — the 2.2 V
part (TLV70022) exists only in SOT-23 (DDC), which won't fit U10's SC-70 land. So
a clean 2.3 V reading **cannot** come from a footprint-compatible mis-fitted
sibling. Interpretation:
- Mark = **`ODV`** ⇒ correct 1.8 V part fitted ⇒ the 2.3 V is a **back-feed**
  (confirm with the load test), not a wrong part.
- Mark = `ODW`/`QTP`/other ⇒ wrong part (but those read ~2.8/2.5 V, not 2.3).

**Fix:**
- These boards: rework U10 → genuine `TLV70018DCKR` (SC-70-5 hot-air swap).
- Next spin (requires KiCAD closed): **pin `LCSC Part = C133796` to U10** so JLC
  cannot substitute a sibling variant. Consider pinning LCSC on *all* fixed-Vout
  LDOs (U7/U10/U11 TLV700 family) to close this class of error.

**BOM validation (2026-08-05).** Parsed the JLC order BOM
(`bom_riskybirdv3_order_1.xls`): **U10 = `TLV70018DCKR` / `C133796` = the correct
1.8 V part** (U7 = TLV70028/C398921 2V8, U11 = TLV70015/C2878901 1V5 also correct).
So the order/BOM is NOT the source of the 2.3 V. Combined with the marking table
above (no 2.3 V part exists in SC-70/DCK to be mis-fitted), the **leading
explanation is now a back-feed/float on a correct TLV70018**, not a wrong part.
Confirm:
1. **Marking check** — if U10 reads `ODV`, the correct 1.8 V part is fitted, which
   rules out a wrong variant. Any other code identifies a mis-fit (but none maps
   to 2.3 V).
2. **100 Ω load test** — sags ⇒ back-feed; then find the injector on the 1V8 net
   (candidates: PL_1V8 on JB2 if the FPGA module is populated, the U12 PCA9306
   VREF1 path, or the HM01B0 camera if connected). Holds stiffly ⇒ it really is
   the LDO output.

**U10 marking confirmed `ODV` (2026-08-05)** — correct TLV70018 (1.8 V) is fitted.
Wrong-part is ruled out; this is a rail interaction.

### +1V8 net — full injector inventory (every node)

| Node | What it is | Injection risk |
|---|---|---|
| U10.5 OUT | TLV70018 output (correct, `ODV`) | source only — LDO **can't sink** |
| 1V8 TP + C42/C51/C52/C72/C73 | test point + ~12.6 µF decoupling | none |
| **HM01B0.11 (IOVDD)** | camera 1.8 V — **same chip: AVDD=2.8 V (pin 2, +2V8), DVDD=1.5 V (pin 12, +1V5)** | **HIGH** |
| JB2.2/4/6/8/10 (PL_1V8) | FPGA SoM 1.8 V bank (JB2 also carries +3V3) | only if SoM populated |
| U12.2 (VREF1) | PCA9306 ref, VREF2 = 3V3 | weak (µA internal) |
| R68.1/R69.1 (2.2k) | CAM-I²C pull-ups to 1V8 | sink; PCA9306-clamped |
| R25.2 (3 Ω) | → Net-(IC1-VDD): PMW3901 VDD (**IC1 unpopulated**) + C11/C12/C13 | none — dead-end |

### Ranked hypotheses (2026-08-05)

1. **★ Camera AVDD(2V8) → IOVDD(1V8): 2.8 − ~0.5 diode = 2.3 V.** HM01B0 has AVDD
   2.8 V and IOVDD 1.8 V on one die. With the camera plugged in and 2V8 live, an
   internal ESD/substrate diode (or AVDD-before-IOVDD sequencing) pulls the 1V8
   net to ≈2.3 V; U10 can't sink it. **2.3 V ≈ 2V8 − one diode = strong numeric
   match.** *Definitive test: unplug the camera, re-measure 1V8 (expect 1.8 V);
   also confirm 2V8 ≈ 2.8 V.* If confirmed → camera rail-sequencing issue (bring
   IOVDD up before AVDD / add bleed); note 2.3 V exceeds HM01B0 IOVDD abs-max
   (~2.0 V), so it's functionally relevant, not cosmetic.
2. **FPGA SoM PL_1V8 (JB2)** — only if the Trenz module is mounted (a populated
   module drives ~1.8 V, not 2.3, unless it has an internal 3V3→1V8 path). Likely
   not populated at this stage. Confirm whether the SoM is on the board.
3. **PCA9306 VREF2(3V3)→VREF1(1V8) internal leak** — real but µA-weak; can't reach
   2.3 V alone. Minor residual contributor.
4/5. I²C-pull-up path (PCA9306-clamped) and R25/PMW3901 (IC1 absent, dead-end):
   negligible / ruled out.

**Highest-value next step:** if the camera is connected, **unplug it and re-read
1V8** — that single test isolates hypothesis #1 from the rest.

### CONFIRMED cause (2026-08-05): PCA9306 no-load bias float

Bench state: **no camera and no FPGA populated.** That removes hypotheses #1 and
#2, leaving the **PCA9306 (U12)** as the only device on 1V8 that bridges to a
higher rail — so it is the injector, confirmed by elimination.

**U12 pin mapping validated against the TI datasheet** (PCA9306, SCPS113O, Fig 5-2
DCU / Table 5-1) — **exact match**, no swap:
`1=GND, 2=VREF1(+1V8), 3=SCL1, 4=SDA1, 5=SDA2, 6=SCL2, 7=VREF2(+3V3), 8=EN`.
Usage also correct: VREF1 (1.8 V) < VREF2 (3.3 V); EN pulled to VREF2 via R67
(200 kΩ) exactly per the datasheet app diagram; pull-ups on both I²C sides.

**Mechanism:** the PCA9306 biases its pass-gate from VREF2 → R67 (200 kΩ) → EN/gate
→ internal clamp → **VREF1 (the 1V8 net)**, sourcing a few µA *into* 1V8. Normally
the 1.8 V supply sinks it; here **U10 is an LDO (can't sink)** and the real 1V8
loads (camera IOVDD, FPGA PL_1V8) are **unpopulated**, so nothing absorbs the µA
and the rail floats to ≈2.3 V. **Correct part, correct wiring — an emergent
no-load float, not a defect.**

**Corroboration (why not 1V5/2V8):** 1V5 (U11) and 2V8 (U7) are also LDO-fed and
unloaded, yet sit at nominal — because their nets contain *no* injector (only the
LDO output, caps, and the absent camera pin, which is a load not a source). Only
1V8 carries the PCA9306 VREF1, so only 1V8 floats. If the cause were generic
(LDO-can't-sink + light load) all three would misbehave; they don't.

**Update (2026-08-05): camera plugged in → 1V8 still ~2.3 V.** The "self-resolve
with the camera" prediction did NOT hold — an *idle* HM01B0 draws negligible IOVDD
current (it only loads 1V8 once clocked/streaming, which needs the ESP/FPGA
running), so the PCA9306 µA injection still floats the lightly-loaded rail. 2.3 V
is within the HM01B0 IOVDD operating range, so likely not harming the camera. →
The **~100 kΩ bleed on 1V8 is now the recommended next-spin fix** (defines the
rail regardless of active load).

**Confirms with:** 100 Ω load test → 1V8 drops to 1.8 V; a 10–100 kΩ bleed to GND
holds ~1.8 V. Optional design insurance: add a ~100 kΩ bleed on 1V8 (~18 µA) for a
defined rail with no loads. U10's marking (`ODV`) earlier confirmed a genuine
1.8 V part, consistent with this being a rail interaction, not a bad LDO.

---

## BU-003 — USB (ESP32-C6) enumerates/disconnects in a loop ("pops in and out")

**Symptom.** On plugging in USB-C, the ESP32-C6 "USB JTAG/serial debug unit"
(`303a:1001`) repeatedly appears/disappears in `lsusb` (~every 2–3 s), with
repeated KDE "device detected" popups.

**Investigation — host-side causes ruled out:**
- Kernel log: every cycle enumerates **cleanly** (descriptor + all strings read
  OK, `ttyACM0` attaches) then a clean device-side `USB disconnect` ~2 s later.
  No `-71/-110`, no "unable to enumerate", no over-current ⇒ **not** a USB
  signal-integrity or host-power problem.
- ModemManager inactive; nothing holding `/dev/ttyACM0` open (`lsof` empty) ⇒
  **not** a host daemon toggling DTR/RTS. The ESP is resetting *itself*.

**Root cause: blank / unprogrammed flash.** The serial console (`ttyACM0`) prints
`invalid header: 0xffffffff` continuously (1849× in an 8 s capture). `0xffffffff`
= erased flash → ROM bootloader finds no valid app → RTC watchdog resets the chip
every ~2 s → the on-die USB-Serial-JTAG drops and re-enumerates. **Expected for a
never-flashed board — not a hardware defect.**

**Strapping / reset circuit verified GOOD (net trace):**
- **GPIO9** (pin 23, BOOT strap) = SW4 button + **C43 0.1 µF** + ESP32-C6 internal
  ~45 kΩ pull-up. No *external* pull-up R, but RC ≈ 4.5 ms ≪ strap-latch time ⇒
  straps **HIGH = normal SPI boot** (confirmed: chip boots and reads flash, not
  stuck in download mode). Optional robustness: add external 10 k to 3V3.
- **EN/CHIP_PU** (pin 8) = **R40 10 k** pull-up to 3V3 + **C37/C58 0.1 µF** + SW1
  reset button. Correct.

**Resolution: flash firmware.**
- Manual download mode: **hold SW4 (BOOT), tap SW1 (RESET), release SW4** → chip
  parks in download mode (USB stays enumerated, looping stops).
- Or let esptool auto-enter download mode via USB-Serial-JTAG:
  `esptool --chip esp32c6 -p /dev/ttyACM0 write_flash …` (or `idf.py flash`).
  (esptool not currently installed here: `pipx install esptool`.)
- The popping / KDE popups stop the moment valid firmware boots.

**Resolution confirmed (2026-08-05).** Built + flashed Zephyr `hello_world` for
`esp32c6_devkitc/esp32c6/hpcore` from `~/Tools/zephyr-chipyard-sw` (branch
`riskybird-bringup`) via `west build` + `esptool` over `/dev/ttyACM0`. After flash:
enumerate/disconnect loop stopped (0 events/15 s vs ~6 before), USB stable, and
console banner captured: `*** Booting Zephyr OS ... *** / Hello World!
esp32c6_devkitc/esp32c6/hpcore`. Chip = **ESP32-C6FH4, 4 MB** flash (set
`flash-size=4MB` for real builds). Toolchain notes: hpcore default console is
UART0 — add a `&usb_serial` console overlay to see output on the USB-Serial-JTAG;
and `drivers/sensor/st/Kconfig` in the Zephyr tree had been accidentally emptied
(blocks non-sensor builds) — restored from HEAD.

---

## BU-004 — 3V3/1V8 don't fully collapse (~0.5 V) when the system switch is OFF

**Symptom (switch OFF).** VSYS = 5.3 V, Net-(Q5-G) = ~5.2 V, **VCOM = 0 V** — i.e.
Q5 correctly cuts the switched rail — **but 3V3 and 1V8 still read ~0.5 V** instead
of 0. (Switch ON: Net-(Q5-G) = 0 V, board fully powered and working.)

**Switch + Q5 are CORRECT** (earlier SPST/footprint theory was wrong — retracted).
Net trace confirms the intended behavior: SW5.1 → Q5 gate, SW5.2 → GND, R45 pulls
the gate to VSYS (default off); gate = 0 V on / ~5.2 V off; VCOM tracks. Power
chain: `VSYS → Q5 (AON7407) → VCOM → U2 (LTC3119) VIN/PVIN → 3V3 → U10 LDO → 1V8`.

**Root cause: back-feed through sneak paths that bypass Q5.** With VCOM truly 0,
3V3 is held at ~0.5 V by always-on domains leaking in through ESD/sense diodes:

1. **USB (dominant, only while USB plugged):**
   `VBUS(+5V) → U6 (USBLC6, VBUS-referenced) → /USB_DP,/USB_DN → ESP32 (U14) pins
   18/17 USB ESD diodes → ESP VDD (3V3)`. Native-USB MCU on bus power.
2. **Battery-sense divider:** `+BATT → R30 → /GPIO5 → ADS7128 (U1) AIN5 ESD diode
   → 3V3`. The divider's GND leg (R31) IS correctly gated by **Q6 (gate = VCOM)**
   so it opens when off — but that only removes the R31→GND path; **R30 still
   injects +BATT into the ADC input ESD clamp**, pulling 3V3 up.
3. **1V8** just tracks the back-fed 3V3 (coupled via the U10 LDO and the U12
   PCA9306 passing the 3V3-side I²C to the 1V8 side).

Ruled out: the +BATT diodes D2/D3/D5/D6 go to the ToF breakout connectors
(J3–J6)/Q1–Q4, not the 3V3 domain.

**Assessment: expected and essentially harmless.** An "off" rail floating to
~0.5 V through ESD diodes is sub-threshold (the 3V3 logic isn't actually running)
and draws only µA. Q5 is doing its job on VCOM.

**Confirm the split (one test):** unplug USB, run on **battery only**, switch off →
path 1 disappears, so 3V3 should drop much closer to 0 (only the weaker +BATT path
remains). That is also the real field condition (USB absent).

**If true-zero shutdown is ever required (usually not):**
- Path 1 (USB): inherent to a bus-powered native-USB MCU; would need a VBUS-gated
  switch on the ESP 3V3 or on the USB data lines. Normally accepted.
- Path 2 (+BATT): raise R30 / add a series switch on the sense tap, or accept the
  µA leak. Also verify R30 limits the ADS7128 ESD-clamp current to a safe value.

---

## BU-005 — Charger LED (D7) blinks rapidly

**Symptom.** D7 blinks rapidly during USB-only bring-up.

**Circuit.** U4 = **MCP73831-2-OT** single-cell Li-ion charger. LED path: `+5V →
R3 → D7 → U4 pin 1 (STAT, open-drain)`. Runs off +5V — independent of the dead
VCOM/3V3 (BU-004).

**Root cause: no battery attached.** With no cell on +BATT, the MCP73831 cycles —
it charges the output cap to the regulation voltage, detects "charge complete"
(LED off), the voltage then sags, it restarts (LED on) — producing a rapid STAT
blink. **Expected, not a defect,** on USB-only bring-up.

**Note.** If D7 blinks rapidly with a *healthy battery* connected, that IS a fault
— check the PROG resistor on U4 pin 5 (Net-(U4-PROG)), thermal regulation, or a
damaged/over-discharged cell.

---

## BU-006 — FPGA SoM (Artix-7 TE0712) brownout-resets on USB power

**Symptom.** With the Trenz Artix-7 SoM populated and the board on **USB power**,
the board keeps resetting.

**Root cause: USB power budget.** USB path = `VBUS(+5V) → U8 (LM66100 ideal diode,
~1.5 A) → VSYS → LTC3119 → 3V3 → FPGA`. **U8 caps USB current at ~1.5 A @ 5 V ≈
7.5 W regardless of source** (a PC port is far less). An Artix-7 100T exceeds that
(config inrush + core/aux/IO), so VSYS sags → brownout → reset. USB-C CC is wired
correctly (5.1k Rd on both), so that's not the limiter — U8 is.

**By design:** USB = charge + program; the **battery** is the high-current path
(`+BATT → Q9 (AON7534) + U13 (LM74700 ideal-diode ctrl) → VSYS`, several amps).
**Fix:** power the FPGA from the 1S LiPo or a bench supply on +BATT (~3.7–4.2 V,
current-limit ~3–4 A); keep USB for data/console only.

---

## BU-007 — ESP32 won't run the flashed app; stuck in ROM download mode

**Symptom.** After flashing *any* app (hello_world included), **no console output**
on `/dev/ttyACM0`; device enumerates stably; esptool always "flashes fine."

**Evidence (not assumption):** `esptool --before no-reset … flash-id` **connects**
after both `esptool --after hard-reset` and `west flash` → the chip is in the ROM
bootloader / **download mode**, i.e. **the app is NOT running** (a running app
would make a no-reset connect fail). esptool "works" because it's talking to the
bootloader, not the app.

**Root cause: the chip got stuck in ROM download mode** — an esptool/USB-Serial-JTAG
reset-state artifact (likely left over from the FPGA brownout reset loop, BU-006,
and/or the USB port change). Every subsequent esptool/`west flash` "hard reset"
re-entered download mode instead of running the app, so no app (not even
hello_world) produced console output.

**RESOLVED (2026-08-06): a power-cycle (unplug/replug USB) cleared it.** A clean
POR booted the app, and esptool flash+reset then behaved normally (BU-008 captured
right after). My earlier "GPIO9 held low (SW4 stuck / flux short)" hypothesis was
**wrong** — a power-cycle would not clear a hardware short, and the app now runs
with no physical change to SW4/GPIO9. **Recovery if it recurs: power-cycle the
board.** (The external 10 kΩ GPIO9 pull-up from BU-003 is still worthwhile for
strap robustness, but it was NOT the cause here.)

---

## BU-009 — Board drops off USB after the battery finishes charging

**Symptom.** With a battery installed and charging over USB, once the charger
**finishes** (charge-complete), the board disappears from `lsusb`; a battery
**unplug/replug** is needed to re-enumerate.

**Suspected cause (to confirm): source-ORing glitch at charge-termination.** VSYS
is ORed from two ideal-diode paths — **USB: +5V → U8 (LM66100) → VSYS**, and
**battery: +BATT → Q9 (AON7534) + U13 (LM74700 controller) → VSYS**. The ESP
appears to be riding the battery path; when the MCP73831 (U4) terminates charge,
a brief VSYS dip or an ideal-diode hand-off latch takes the ESP down (USB drops),
and it doesn't recover until a battery replug re-establishes VSYS. (An ESP
brownout-detector latch at the transition is a related possibility.)

**Diagnose (bench):**
1. Scope **VSYS** and **3V3** across the charge-complete moment — look for a dip /
   dropout that would reset the ESP.
2. Watch the **MCP73831 STAT / charge state** and **U13 (LM74700) gate / VSYS**
   at termination — does Q9 momentarily open (ideal-diode hand-off)?
3. Check whether it happens **USB-only vs. battery-present**, and whether the
   ESP's brownout detector is tripping (reset reason on the re-enumerated boot).

**Not yet root-caused** — logged for investigation. If confirmed a hand-off
glitch, mitigations would target the LM74700/Q9 ideal-diode behavior or add VSYS
hold-up; candidate for the next-spin ECO once characterized.

## BU-010 — ESP32 stuck in ROM download mode after IC1 (PMW3901) rework

> **⚠ RESOLVED 2026-08-11 — FALSE ALARM (capture artifact). The analysis below was
> WRONG.** The board boots fine and the ESP/GPIO9 are healthy. The "silent across
> resets" was self-inflicted: the host opening the serial port **asserts DTR/RTS by
> default**, and on the ESP32-C6's native USB-Serial/JTAG those lines map to
> **BOOT/EN** — so *every capture attempt reset the chip into ROM download mode.*
> Opening the port with **`dtr=False; rts=False` set before `open()`** shows the OG
> board running the app normally, with the PMW3901 imaging (chip inv-ID 0xB6,
> SQUAL ~86). The pin-23/GPIO9 rework was unnecessary. Fix applied to the capture
> tools (`serial_capture.py`, `flightlog_dump.py`): deassert DTR/RTS on open.
> **Lesson: never assert DTR/RTS when merely reading an ESP32-C6 USB-JTAG port.**
> (Kept below for the record; the "root cause: GPIO9 held low" conclusion is retracted.)

**Symptom.** After removing + re-soldering IC1 (PMW3901) on the *original* board
(board flex noted, plus opposite-side hot air), U14 no longer boots the flashed
app — the USB serial is **silent** (no banner, no console) across clean
power-cycles and SW1 resets. `esptool` still connects fine.

**The ESP is healthy — NOT a dead/disconnected chip.** `esptool … flash_id` reads
the chip features, MAC `98:a3:16:ff:fe:b6:d5:00` (matches prior), uploads and
*runs* the RAM stub flasher, and reads the 4 MB flash. So USB D±, power, EN, and
the module's critical joints are intact. Erase + reflash both succeed.

**Root cause: GPIO9 (BOOT strap) held LOW at reset → ROM download mode.**
- A *running* app prints continuously; the one-shot ROM banner is lost on the
  native USB-Serial/JTAG (USB re-enumerates on reset). So "silent across resets" =
  app not running = parked in download mode.
- Download mode requires GPIO9 actively pulled LOW. An *open* GPIO9 joint would
  float **high** on the internal ~45 kΩ pull-up and boot the app — so this is a
  **short/bridge**, not an open: bridge on GPIO9 (module **pin 23**) to GND/neighbor,
  or SW4 (BOOT) stuck closed (SW4 ties GPIO9→GND).

**Distinct from BU-007.** BU-007 was a transient reset-state artifact cleared by a
power-cycle. BU-010 **survives** clean power-cycles (no battery; 3V3 verified
normal) → a physical GPIO9 pull-down = rework collateral, not a soft wedge.

**Fix (in progress).** Reflux + wick pin 23 (GPIO9) and neighbors to clear a
bridge; inspect/actuate SW4. Confirm with a meter: **GPIO9→GND should read ~3.3 V
idle, not 0 V.** As of this entry a pin-23 reflow has not yet restored boot —
pending a GPIO9 voltage reading to distinguish "bridge still present" from "GPIO9
fine, look elsewhere (GPIO8/GPIO15 straps)."

**Fallback.** The 2nd (fresh) board is fully working — PMW3901 alive + imaging
(SQUAL ~53), and IMU (0x18/0x68), down-ToF (0x29→0x30), baro (0x76), ADS7128
(0x17) all online on I2C. Recover-if-easy, else proceed on the fresh board.

**Reinforces E3** (external 10 kΩ GPIO9→3V3 pull-up): a stronger pull-up raises the
bar for a flux blob / bridge to drag BOOT low, which would have prevented this.
