# RESPIN — Dedicated high-current 3.3 V rail for the Kintex UltraScale KU040 (TE0841)

**Project:** riskybird v3 (`riskybirdv3.kicad_sch`)
**Scope:** Specify a NEW high-current 3.3 V power rail so the drone board can host a **Trenz TE0841 (Kintex UltraScale KU040/KU035)** SoM. Quantitative budget, topology decision, regulator + eFuse + passive selection with concrete LCSC parts, thermal design, and layout/current tables. The existing **U2 = LTC3119** buck-boost is sufficient for the Artix XC7A200T (TE0712, ≤3 A) but **not** for the KU040 — see the "given" below.
**Status:** Design spec / analysis only — **no `.kicad_*` edits, not committed.** Companion to `RESPIN_POWER_U2.md` (U2 rating/load review), `ECO-001_3v3_power_redesign.md` §8–§9 (the U2 field incident + JB1.13 root cause), and `RESPIN_CHECKLIST.md`.
**Date:** 2026-08-16
**New design freedom (this session):** the prototype came back and **the center board cutout can be reduced → board area is freed.** This spec is therefore **not space-constrained**: it optimizes for thermal robustness (larger inductor, generous copper pours, dense thermal-via arrays, wide power traces), not minimum footprint.

---

## ⚠️ REVISION (2026-08-16) — DECISION CHANGED to SINGLE RAIL + part corrected to TPS552882
**This section supersedes the split-rail / TPS55288 / "no external FETs" content in §0–§8 below on those points.** Rationale: user direction (one board for all builds, no optional/DNP parts) + two datasheet findings.

### R.1 Decision: ONE buck-boost for ALL builds, single continuous 3.3 V plane
- **Drop the split rail and drop U2/LTC3119 entirely.** A single high-current buck-boost supplies the **whole** 3.3 V (FPGA module **+** ESP/sensors/camera) on **every** build (Artix and KU040). No assembly variants, no DNP — one BOM. Running the big buck-boost at the Artix idle (0.45 A) up to KU040 (8 A) is fine.
- **Single continuous 3.3 V plane** (do NOT split it — a split plane breaks return paths for signals crossing it). Topology: regulator VOUT = star point → a **wide inner-layer pour feeding the module 3.3VIN B2B pins (sized ~8 A, heavily via-stitched)**; the same continuous plane feeds the low-current housekeeping loads. Bulk cap at the regulator output **and** local bulk + ceramic at the FPGA 3.3VIN.
- Split-rail advantages that no longer apply: **config-sequencing** → gone (TPS552882 is autonomous, see R.2); **fault isolation** → the input eFuse trips on a hard short and cuts the rail safely (losing "telemetry survives a module fault" is an accepted trade for one-BOM simplicity). Manage the one downside — module DC-DC switching ripple on the shared rail — with a local ferrite/RC at any noise-sensitive sensor (most already sit behind their own LDOs).

### R.2 Part correction: TPS55288 → **TPS552882** (the non-I²C sibling)
- The **TPS55288 is I²C-programmed** (10-bit DAC sets Vref 45 mV–1.2 V + ILIM via registers) → it will NOT come up at 3.3 V standalone without firmware writes = a config-at-boot dependency. **Use the pin-compatible `TPS552882`** — **fixed 1.2 V reference, output set by a plain external FB divider, no I²C** → autonomous power-up. Same **RPM VQFN-26 (4×3.5 mm)** package. **Standard `TPS552882RPMR` is NOT on LCSC** → use the pin/function-identical AEC-Q100 **`TPS552882QRPMRQ1` = LCSC `C2876594`** (~$4.53). **IMPORTED into `riskybird_lcsc` 2026-08-16** (easyeda2kicad: symbol `TPS552882QRPMRQ1` + footprint `VQFN-HR-26_L4.0-W3.5` + 3D). Sanity-check the imported pin map vs the datasheet before drawing.

**Part-import status (2026-08-16) — ALL 3 NEW POWER PARTS IMPORTED + JLC-verified** (audit: `RESPIN_BOM_JLC.md`):
- Buck-boost `TPS552882QRPMRQ1` = **C2876594** — JLC **Extended**, ~102 stock, ~$5.40 (AEC-Q100, single-source, **thin stock → pre-order/watch**).
- eFuse: the spec'd **`C2155683` (TPS259822) is NOT in the JLC assembly catalog** → substituted **`TPS259824ONRGER` = C2155766** — JLC Extended, ~106 stock, 2.7–24 V, 2–15 A adj ILIM. **Imported.**
- Inductor `XAL7030-152MEC` = **C5352784** (Coilcraft, 1.5 µH, 15 A, DCR 8.36 mΩ, **7.5×7.5×3.1 mm**, on LCSC/JLC) — **FINAL, imported** @ f_SW 1.0 MHz. Chosen for mechanical size (superseded the 1 µH C408497 and the too-large 12.5 mm Bourns 2.2 µH — see §R.7).
- External buck FETs `AON7534` (C115844) already on BOM; brownout-fix high-side P-FET → `Si2301` (C10487, **Basic**).
All symbols + footprints + 3D now in `riskybird_lcsc`.

### R.3 Architecture correction: it needs **2 external buck-side N-FETs** (NOT "integrated, no FETs")
- Datasheet pinout is explicit: **DR1H (pin 2) / DR1L (pin 1) are gate-driver outputs for EXTERNAL buck-side MOSFETs**, and **SW1 (pin 23) = "drain of the *external* low-side / source of *external* high-side MOSFET."** Only the **boost side (SW2, pins 21/25) is internal** (RDS(on) 7.1/7.6 mΩ). So this is a **hybrid controller** = **2 external power N-FETs on the input/buck side** (fewer than a full 4-FET controller, more than "zero"). Size them for the peak inductor current limit (~25 A) + the ~10–16 A operating current → the **AON7534 (30 A)** already on the board is a fine candidate (or a lower-Rds dual-N to save area). Verify DR1H/DR1L gate-drive strength vs the chosen FET's Qg.

### R.4 Schematic-ready connection table (TPS552882, VIN = VSYS_FUSED, VOUT = 3.3 V, ≤8 A)

**Rail BOM — parts to place (all symbols + footprints now in `riskybird_lcsc`):**
| Role | Part | LCSC | KiCad symbol | Footprint |
|---|---|---|---|---|
| Buck-boost ctrl (U) | TPS552882QRPMRQ1 | C2876594 | `TPS552882QRPMRQ1` | `VQFN-HR-26_L4.0-W3.5_TPS55288RPMR` |
| Input eFuse (U) | TPS259824ONRGER | C2155766 | `TPS259824ONRGER` | `VQFN-24_L4.0-W4.0-P0.50-TL-EP_TI_RGE` |
| Inductor L1 | **XAL7030-152MEC** (1.5 µH, 15 A, DCR 8.36 mΩ, **7.5×7.5×3.1 mm**) | C5352784 | `XAL7030-152MEC` | `IND-SMD_L7.5-W7.5_XAL7030-152MEC` |
| Buck HS FET Q_HS | AON7534 (30 A N-FET) | C115844 | `AON7534` | (reuse Q9's) |
| Buck LS FET Q_LS | AON7534 (30 A N-FET) | C115844 | `AON7534` | (reuse Q9's) |

**Passives (standard MLCC / thick-film — JLC Basic):** Cin 2×22 µF (≥10 V X7R) + 0.1 µF · Cout 2×47–100 µF + ceramics (+ local bulk at FPGA 3.3VIN) · VCC 4.7 µF · BOOT1/BOOT2 0.1 µF ea · FB Rtop **100 k** / Rbot **56.2 k** (+22 pF ff) · EN/UVLO Rtop **40.2 k** / Rbot **28.0 k** · R_ILIM **27 k** (→12.2 A) · **R_FSW 19.6 k → 1.0 MHz** · **R_MODE 27 k** (PFM + internal LDO) · **COMP (Eq 24–26, for 1.5 µH / 1 MHz / ~100 µF Cout, f_C ≈ 7 kHz): R_C 3.65 k + C_C 5.6 nF (series) + C_P 82 pF (∥)** — recompute if effective Cout differs; verify by load transient · plus the eFuse passives (§R.6).

| Pin(s) | Name | Connection / value |
|---|---|---|
| 3 | VIN | ← `VSYS_FUSED` (eFuse output). Cin ≥ 2×22 µF + 0.1 µF, close to VIN/PGND |
| 1 | DR1L | → gate of buck **LS FET Q_LS = AON7534** (`riskybird_lcsc:AON7534`, C115844) |
| 2 | DR1H | → gate of buck **HS FET Q_HS = AON7534** (C115844), bootstrapped via BOOT1 |
| 23 | SW1 | buck node: **Q_HS(AON7534) source + Q_LS(AON7534) drain + L1(XAL7030-152MEC) end A** |
| 21,25 | SW2 | boost node (internal FETs): **L1(XAL7030-152MEC) end B** |
| 11,26 | VOUT | → `+3V3` plane. Cout ≥ 100 µF bulk + ceramics; local bulk+ceramic at FPGA 3.3VIN |
| 14 | FB | divider VOUT→FB→GND: **Rtop 100 k / Rbot 56.2 k → 3.33 V** (Vref 1.2 V); ~22 pF feed-forward across Rtop |
| 17 | ILIM | R_ILIM to AGND ≈ **24–27 kΩ** → ~12–14 A avg-inductor limit (headroom over ~9.4 A). ILIM ≈ 330/R[kΩ] (approx — use the TPS552882 eq) |
| 4 | EN/UVLO | VIN →**R_top 40.2 k**→ EN →**R_bot 28.0 k**→ GND → **turn-on ≈ 3.00 V / turn-off ≈ 2.80 V** (1.23 V ref; ~0.20 V hyst = I_UVLO 5 µA × R_top) — 1S over-discharge backstop. *(Or drive EN from SW5 / eFuse-PG for pure logic-enable.)* |
| 8 | FSW | **R_FSW = 19.6 kΩ → 1.0 MHz** (Eq 3; required since L = 1.5 µH → fsw > 1.2/1.5 = 0.8 MHz) |
| 15 | MODE | R_MODE to AGND → select **PFM** (light-load eff.) + internal VCC LDO (value from TPS552882 MODE table) |
| 18 | COMP | R-C loop-comp network to AGND (design per datasheet/EVM) |
| 19 | VCC | ≥ 4.7 µF to AGND |
| 22 | BOOT1 | 0.1 µF to SW1 |
| 20 | BOOT2 | 0.1 µF to SW2 |
| 12,13 | ISP,ISN | tie **both to VOUT** = disable the extra output-current-sense loop (rely on ILIM + eFuse) |
| 16 | CDC | no cable-droop comp: per datasheet (R to AGND or floating) |
| 7 | DITH/SYNC | → GND (no dithering) or cap for spread-spectrum EMI |
| 9,24 | PGND | power ground |
| 10 | AGND | signal ground (single-point tie to PGND) |
| 5 | PG | open-drain **power-good** (hi-Z when Vout >95%); 100 k pull-up to 3V3, optional → MCU GPIO |
| 6 | CC̄ | open-drain **constant-current indication**; pull-up, optional → GPIO (NOT I²C) |

- **Inductor L1 = XAL7030-152MEC** (`C5352784`, 1.5 µH, 15 A, DCR 8.36 mΩ, **7.5×7.5×3.1 mm**) @ **f_SW = 1.0 MHz** — **FINAL (2026-08-16), imported.** Chosen for mechanical size: the 12.5×12.5×6.5 mm Bourns SRP1265A-2R2M was too large to place without board restructuring; the XAL7030 (7.5 mm, half-height) drops in. Trade: fsw 700 k→1 MHz (slightly more switching loss, accepted) — 1.5 µH needs fsw > 0.8 MHz, and the higher f_RHPZ (~39 kHz) keeps the loop healthy (f_C ~7 kHz). Also on LCSC/JLC (`C5352784`) if machine placement is ever wanted. *(Prior candidates for the record: Bourns SRP1265A-2R2M/C2831487 = 2.2 µH/37 A but 12.5 mm; MWSA1204S-4R7MT/C408501 = 4.7 µH/14.4 A marginal; MWSA1204S-2R2MT/C408499 JLC 0-stock.)*
- **eFuse `TPS259824ONRGER` (C2155766 — JLC-assemblable; the TPS259822/C2155683 is NOT in JLC's catalog)** now guards the SOLE rail: `VSYS → eFuse → TPS552882 VIN`, **replacing the Q5 load switch**; **SW5 gates the eFuse EN.** ILIM ~12 A, dVdt soft-start cap, UVLO/OVP dividers, FLT→GPIO, IMON→ADC.

### R.5 TBD status (datasheet SLVSFC5 read 2026-08-16)
**RESOLVED:** (1) **pin 5 = PG, pin 6 = C̄C̄** — open-drain status, NOT I²C, no strapping; (2) **Vref = 1.2 V** → FB divider **Rtop 100 k / Rbot 56.2 k → 3.33 V**; (5) **ILIM ≈ 330 / R_ILIM[kΩ]** → **R_ILIM ≈ 25 kΩ for ~13 A** avg (peak limit 25 A); (6) **fsw ≈ 20000 / R_FSW[kΩ]** → **R_FSW ≈ 40.2 kΩ for 500 kHz**; soft-start **internal 4 ms** (no cap); Vin 2.7–36 V (UVLO 2.9 V rising); inductor 1–10 µH (1 µH valid, low end→more ripple); BOM = **C2876594** JLC Extended (confirmed). **STILL TO LOCK:** (3) **R_MODE** for PFM + internal-VCC-LDO (R to AGND; I_MODE 10 µA; detection thresholds 0.189 / 0.351 / 0.614 V — pick band from the Detailed Description); (4) **COMP loop network** (R-C to AGND — start from the TPS552882EVM values); (7) **external buck FET P/Ns** (AON7534 candidate) + gate-drive/Qg check.

### R.6 Input eFuse — TPS259824ONRGER (C2155766) pin table
Package **RGE-24** (VQFN 4×4, EP1=IN / EP2=GND). Path: `/VSYS → eFuse → /VSYS_FUSED (= TPS552882 VIN)`. Variant = **16.9 V OVLO + circuit-breaker (latch/trip)**.

| Pin(s) | Name | Connect |
|---|---|---|
| 1,2,3,16, EP1(25) | IN | ← `/VSYS` (ORing output); solder EP1 to the input plane (thermal) |
| 17–24 | OUT | → `/VSYS_FUSED` (feeds U15 VIN + the EN/UVLO divider top R78) |
| 4,5,14, EP2(26) | GND | GND |
| 6 | EN/UVLO | ← **SW5** (master enable — move SW5 here from the old Q5 gate). Optional UVLO divider from IN |
| 8 | ILIM | R to GND sets current-limit + fast-trip (~12 A) — value per the TPS25982 ILIM eq |
| 7 | ITIMER | cap to GND = OC blanking; **leave OPEN = fastest trip** (input-short protection) |
| 15 | dVdt | cap to GND = output slew / rail soft-start (inrush into FPGA bulk caps); float = fastest |
| 9 | IMON | R to GND → V ∝ load current → ADC (optional current telemetry) |
| 13 | PG | open-drain power-good; pull-up to +3V3; **→ drive U15 EN/UVLO** for clean sequencing (or MCU) |
| 10 | RETRY_DLY | cap = restart delay; **tie GND = latch-off** (circuit-breaker stays off after a fault) |
| 11 | NRETRY | cap = retry count; GND = retry-indefinitely (pick with RETRY_DLY for the behavior you want) |
| 12 | LDSTRT | load-detect/handshake — **tie to GND** (unused) |

*OVLO note:* the 259824 = 16.9 V OVLO (no OV protection below that for a ≤5.5 V rail). If you want ~7.6 V OV trip, check whether **TPS259823ONRGE** is JLC-assemblable.

**Sized passives (2026-08-16, from the TPS25982 equations):**
- **ILIM (8):** R_ILIM = **121 Ω 1%** → I_LIM = 1460/R + 0.11 ≈ **12.2 A**, fast-trip 2.1× ≈ 25.6 A [Eq 4]. (82–1650 Ω range ✓.)
- **dVdt (15):** C_dVdt = **150 pF** → SR ≈ 31 V/ms → inrush ≈ **1.35 A** into the ~44 µF buck-boost Cin [Eq 2/3].
- **ITIMER (7):** C_ITIMER = **22 nF** → OC blanking ≈ **178 µs** (>100 µs TI rec; rides drone motor-current transients; < 30 nF max) — **NOT open** [Eq 5]. Fast-trip still catches hard shorts in ~400 ns regardless.
- **IMON (9):** R_IMON = **604 Ω** → 12.2 A → 1.81 V (≤ 1.8 V max for 3.3–5 V V_IN) [Eq 12]; add **10 kΩ series + 1 nF** RC filter to the ADC.
- **PG (13):** **100 kΩ** pull-up to +3V3 (open-drain, active-high; ≤6 V rail).
- **EN/UVLO (6):** R_VL1 = **60.4 kΩ** (top) / R_VL2 = **40.2 kΩ** (bottom) → UVLO **on 3.0 V / off ~2.75 V** [Eq 1; V_UVLO 1.2 V rising / 1.1 V falling]; **SW5 in series with R_VL1** (closed = on, ~30 µA divider).
- **RETRY_DLY (10) → GND** and **NRETRY (11) → GND** = **latch-off / no auto-retry** [Table 7-3] (circuit breaker stays off after a fault).
- **LDSTRT (12) → GND** (unused).
- **Cin** = 1 µF + 0.1 µF at IN (VSYS bulk = existing C45/C49); **Cout** = the buck-boost's 2×22 µF Cin (already placed).

### R.7 Verifications + old-rail teardown / re-wiring (single-rail transition)
**Datasheet-verified (SLVSFC5):**
- **EN/UVLO is ACTIVE-HIGH** — U15 (pin 4: "logic high enables; low = shutdown") **and** eFuse U16 (pin 6, "Active High Enable"). → the master switch must pull EN **low** to turn off.
- **MODE R77 = 27 kΩ is CORRECT** → Table 7-2 detection band (~18.9–35.1 kΩ) = **Internal VCC LDO + PFM** (light-load power-save). Canonical value is 24.9 kΩ; 27 kΩ stays in-band across I_MODE (9–11 µA) tolerance. No change needed.
- Value checks: ILIM (Eq 5, Vout=3.3 V) I_AVG = 330000/R_ILIM → **27 k → 12.2 A** ✓ · FSW (Eq 3) 28.7 k → **~690 kHz** ✓ · FB (Eq 6, Vref 1.2 V) 100 k/56.2 k → **3.34 V** ✓.

**🔴 FIX — buck FET gate drivers swapped + Q10 source floating (as-drawn bug):**
- Q8 (HS: drain→/VSYS_FUSED, source→SW1): gate → **DR1H (pin 2)** *(not DR1L)*.
- Q10 (LS: drain→SW1): gate → **DR1L (pin 1)** *(not DR1H)*, **and source → PGND** *(currently unconnected)*.

**DELETE (old LTC3119 rail):** `U2`, `L4`, `R59`/`R60` (FB), `R62` (RT), `R61` (VC), `R58`/`R71` (RUN/UVLO), `C65` (MPPC), `C60`/`C61` (BST), `R70` (0 Ω), `C62`/`C67` (VCOM caps), `TP1` (VCOM TP), `Q5` (AON7407 load switch), `R45` (Q5 gate pull-up), `R47`/`R48` (200 k on VCOM — *verify function first*).

**RE-WIRE (do NOT just delete — these used old-rail nets as signals):**
- **SW5** (master switch): move from the old Q5 gate to the **eFuse EN/UVLO (pin 6)**. Active-high → wire it in the eFuse UVLO divider: **VSYS → SW5 → R_top → eFuse.EN → R_bot → GND** (closed = enabled + UVLO set; open = EN pulled to GND = off, zero shutdown leakage; SW5 carries only µA). U15 then auto-enables via its own 40.2 k/28 k divider once /VSYS_FUSED rises.
- **Q6 + Q7** (2N7002): were gated by **VCOM as a "system-on" signal** (battery-sense gating). VCOM disappears → re-point their gate to the new system-on (**eFuse PG pin 13** or a new 3V3-good). Coordinate with `RESPIN_POWER_BROWNOUT.md` (the R30 top-gate fix).

**KEEP:** `U8` (LM66100), `U13` (LM74700), `Q9` (AON7534), `C45`/`C49` (VSYS caps), `TP2` (VSYS TP).

**Inductor L1 change (do in schematic):** re-assign L1 to **XAL7030-152MEC** (`C5352784`, symbol `XAL7030-152MEC`, footprint `riskybird_lcsc:IND-SMD_L7.5-W7.5_XAL7030-152MEC`, 1.5 µH, 7.5 mm — imported). Because L=1.5 µH @ 1 MHz, also update: **R_FSW (R76) 28.7 k → 19.6 k**, **COMP: R79 2.6 k → 3.65 k, C82 8.2 n → 5.6 n, C81 100 p → 82 p**. (R_ILIM 27 k, FB, EN/UVLO, MODE unchanged.) *(Reload KiCAD libs to see the new symbol.)*

---

## 0. TL;DR / executive summary

- **Verdict — SPLIT RAIL.** Keep **U2 (LTC3119)** as the **shared housekeeping 3.3 V** (ESP32-C6, sensors, camera LDOs, PCA9306, headers) on **every** build. Add a **NEW dedicated high-current buck-boost** that powers **only the module's `VIN`/`3.3VIN` B2B pins**, **populated on KU040 (TE0841) builds and DNP on Artix (TE0712) builds.** Rationale in §2 — the decisive points are (a) the Artix build stays byte-for-byte the already-validated design (no added cost/risk), (b) fault isolation (a module-rail short can no longer kill the ESP/telemetry), and (c) the ESP boots on the always-on housekeeping rail and can then configure the big rail — which turns the TPS55288's I²C dependency from a liability into clean, deliberate power sequencing.
- **KU040 3.3 V budget (this rail carries the module only):** startup floor **≥ 4 A** (TE0841 TRM), typical operating **3–4 A**, heavy **5–6 A**, aggressive worst case **~6.5–7 A**. **Design the rail for 8 A continuous** (margin). See §1.
- **Worst-case 1S input current for this rail:** at 8 A / 3.3 V out, from a sagging cell the **input current reaches ~9.8 A ≈ 10 A** (I_in = 3.3·I_out /(η·V_in)). The whole battery path — ORing FETs, the new eFuse, VSYS copper — must carry ~10–12 A (with housekeeping). See §1.3.
- **Primary regulator: TI TPS55288RPMR** — 2.7–36 V, four-switch buck-boost with **integrated 16 A switches**, resistor-set current limit + resistor UVLO, VQFN-26 (3.5×4 mm). **LCSC C2864583, 5 849 in stock, $2.79 @1 / $2.42 @10.** (The "monolithic tops out at 4–5 A" rule does *not* apply here — this part integrates 16 A FETs, so no external power FETs are needed.) Output voltage/output-enable are set over I²C by the ESP (off the housekeeping rail) before FPGA power-up.
- **Firmware-free fallback: ADI LTC3785 + 4× AON N-FETs** — 2.7–10 V all-N four-switch buck-boost **controller**, resistor-set, arbitrary current. Ideal 1S V_IN range but **LCSC stock is thin (C685993 out of stock)** → hand-populate from Digi-Key/Mouser. Use only if a firmware-independent rail is mandated.
- **Input eFuse: TI TPS25982 (15 A, 2.7 mΩ, 2.7–24 V smart eFuse)** on the new rail's input branch (VSYS → eFuse → TPS55288 VIN). ILIM ≈ 12 A, fast-trip ~15 A, programmable soft-start, load-current monitor, FAULT→GPIO. **TPS259822ONRGET = LCSC/JLC C2155683** (auto-retry; use the latch-off "…821" sibling if you prefer latch). NOT the 10 A TPS25981 (would sit at its edge).
- **Inductor: Sunlord MWSA1204S-1R0MT** — 1.0 µH, **Isat 28 A, I_RMS(heat) 35 A, DCR 7.5 mΩ**, 12.5×12.5 mm molded (the exact anti-pattern to the old L4 2.2 µH/0603 that saturated at 1–3 A and co-caused the U2 melt). See §4.
- **ORing FETs confirmed:** Q9 AON7534 (30 A) carries the ~10–12 A battery path with margin (P ≈ 1.2 W at 12 A). USB leg (LM66100, 1.5 A) **cannot** run a KU040 — USB is a bring-up/config/idle source, **battery is the flight source.**
- **Added KU040-build BOM cost: ≈ $5–7/board.** Artix build cost unchanged (all new parts DNP).

---

## 1. KU040 (TE0841) 3.3 V current budget

### 1.1 What this rail feeds

In the split-rail architecture (§2) the **new rail powers ONLY the module** — its `VIN` and `3.3VIN` B2B pins — which feed the module's **on-module DC-DCs** (TPS82085-class bucks that make VCCINT 0.95 V, VCCBRAM, VCCAUX 1.8 V, DDR4 1.2 V, MGT rails) **plus** the 3.3 V that goes **directly** to the FPGA's HR-bank VCCO. Carrier housekeeping (ESP + sensors + camera LDOs, ≈ 0.15–0.25 A) stays on U2/LTC3119 and is **not** on this rail.

### 1.2 Derivation (XPE methodology + TE0841 TRM)

The **TE0841 TRM (REV02) §6.1** gives the one hard cited number and directs power-vs-utilization to the Xilinx Power Estimator (XPE) / Vivado (Table 12 lists VIN/3.3VIN as "TBD — design-dependent"):

> *"Single 3.3 V power supply with **minimum current capability of 4 A for system startup** is recommended."*

Building an operating envelope from the XCKU040 power domains (Kintex UltraScale DS892 + XPE), converted to a **module 3.3 V input current** through the on-module DC-DC efficiency (η_mod ≈ 0.85–0.90, I = P/(η·3.3)). VCCINT (0.95 V core) dominates and scales with fabric utilization/clock; DDR4 (1.2 V), VCCAUX (1.8 V) and 3.3 V HR-bank I/O add the rest; the carrier NCs all MGT/GTH so transceiver power ≈ 0.

| KU040 design point | FPGA + on-module rails (approx) | **Module 3.3 V input current** |
|---|---|---|
| **Startup surge** (TRM floor: config + on-module DC-DC inrush + bulk-cap charge) | — | **≥ 4 A** |
| Light logic, DDR4 idle | ~5 W | ~1.7 A |
| Moderate fabric + DDR4 active | ~10 W | ~3.4 A |
| Heavy DSP / high utilization + DDR4 | ~15 W | ~5.2 A |
| Aggressive worst case | ~18–20 W | **~6.5–7 A** |

**Design headline numbers for this rail:**

| | 3.3 V output current |
|---|---|
| **Startup floor (must meet)** | **4 A** |
| **Typical operating** | **3–4 A** |
| **Heavy** | **5–6 A** |
| **Aggressive worst case** | **~7 A** |
| **→ Design target (continuous, with margin)** | **8 A** |

The 8 A design target sits above the ~7 A aggressive worst case with headroom, and the chosen TPS55288 (16 A integrated switch) has ~2× margin over that.

### 1.3 1S input current across the battery range (the number that sizes the front-end)

Buck-boost input current **I_in = 3.3·I_out /(η·V_in)**, η = 0.90 (0.85–0.90 real). Usable 1S range after UVLO ≈ 3.0–4.2 V.

| Rail output (I_out) | V_in 4.2 V | V_in 3.7 V | V_in 3.3 V | V_in 3.0 V (sag) |
|---|---|---|---|---|
| 4 A (startup/typ) | 3.5 A | 4.0 A | 4.4 A | 4.9 A |
| 6 A (heavy) | 5.2 A | 6.0 A | 6.7 A | 7.3 A |
| **8 A (design target)** | **7.0 A** | **7.9 A** | **8.9 A** | **9.8 A** |

**Worst case = full 8 A on a deeply-sagged cell → ~9.8 A ≈ 10 A input to this rail.** With housekeeping (~1.0–1.2 A input on U2) the **shared ORing junction / VSYS carries ~11–12 A** worst case. This straddles buck (V_in > 3.3) and boost (V_in < 3.3) across the discharge → a **buck-boost is mandatory** (a step-down cannot hold 3.3 V once the cell sags below ~3.5 V).

---

## 2. Topology decision — SPLIT RAIL (recommended) vs one bigger buck-boost

### 2.1 Option (a): SPLIT RAIL — **RECOMMENDED**

Keep **U2 (LTC3119)** as the shared housekeeping 3.3 V on all builds; add a **new dedicated buck-boost** for the module 3.3 V, **populated on KU040 builds, DNP on Artix builds.**

```
                       ┌──────────────────────────────────────────── HOUSEKEEPING 3V3 (all builds)
                       │   U2 LTC3119 (existing, validated)  ──► ESP32-C6, BMI088/BMP390/VL53L1/ADS7128/
+BATT ─Q9(AON7534)─┐   │   ~1 A, resistor-set, comes up FIRST     PMW3901, camera LDOs, PCA9306, headers
                   ├── VSYS ──┤ (via existing eFuse per RESPIN_POWER_U2, ~6 A)
+5V ──U8(LM66100)──┘   │
   (ideal-diode ORing) │   ┌── NEW eFuse TPS25982 (KU040 builds only, DNP on Artix) ──┐
                       └───┤  ILIM≈12A, SS, FAULT→GPIO, EN=SYS_ON                       ├── MOD_3V3 ─► module VIN/3.3VIN (B2B)
                           └── NEW buck-boost TPS55288 (16 A) + 1 µH/28 A L ────────────┘   8 A, ESP-configured over I²C
```

**Why split wins:**

1. **The Artix (TE0712) build is unchanged and already hardware-validated.** ECO §9.5–9.7 proved the LTC3119 front-end runs the Artix on PSU/USB/battery. Option (b) would force the bigger (I²C) regulator onto the proven Artix build, adding cost, area, firmware dependence, and re-validation risk to a design that is *done*.
2. **Fault isolation.** A module-rail fault (the exact class of event — JB1.13 short — that destroyed the first U2, ECO §8.11) now trips only the **new** eFuse. The ESP + sensors keep running on U2, so firmware **sees** the FAULT and can log/land instead of the whole board browning out. In option (b) a single rail is a single point of failure for the entire board.
3. **The I²C chicken-and-egg is solved by construction.** The TPS55288 needs I²C to set VOUT and assert output-enable (§3.2). In a split design the **ESP boots on the always-on, firmware-free LTC3119 rail**, then configures + enables the big rail *before* the FPGA is powered → clean, deliberate sequencing and a natural place to gate the FPGA on housekeeping-good. In option (b) with a single I²C rail, **nothing is alive to configure the rail that would power the configurator** — a hard bring-up problem.
4. **Thermals split into two smaller hotspots**, each easier to sink, instead of one ~3 W regulator carrying the whole board.
5. **Board area is now available** (freed cutout) → the "two regulators cost area" objection is moot.

**Costs of split:** two buck-boosts; a build-time **module-3V3 source-select** (see §8.3) so the module's `VIN`/`3.3VIN` net is fed by U2 on Artix builds and by the new rail on KU040 builds. Both are minor.

### 2.2 Option (b): one bigger buck-boost replacing U2 for all builds — **NOT recommended**

Replace U2 with a single ~8 A buck-boost feeding the whole board. Pros: one rail, one part, simplest net. Cons (decisive): imposes the bigger/I²C part + cost + firmware dependence + re-validation on the **proven Artix build**; **single point of failure** (loses fault isolation and telemetry-through-fault); the **chicken-and-egg** if the single rail is I²C-configured; one larger thermal hotspot; and it discards the already-laid-out, validated LTC3119. The only scenario where (b) is attractive is if you pick a **fully firmware-free resistor-set** single regulator (LTC3785+FETs) *and* accept re-validating the Artix build — not worth it here.

### 2.3 Artix-BOM-cost impact & DNP / assembly-variant strategy

- **Artix (TE0712) build:** new rail = **DNP** (TPS55288, its L, its I/O caps, the TPS25982, associated R/C). BOM/cost **unchanged** vs today.
- **KU040 (TE0841) build:** add ≈ **$5–7/board** (TPS55288 $2.79 + inductor ~$0.6 + TPS25982 ~$1.5 + caps/resistors ~$0.7 + the source-select 0 Ω). See §8.
- **Mechanism:** one schematic, two JLC **assembly variants** (or two BOM/CPL pairs) — KiCad "Do not populate" on the new-rail parts for the Artix variant. The **module-3V3 source-select** (§8.3) is two 0 Ω / footprint jumpers: `R_HK` (U2→MOD_3V3) fitted on Artix, `R_KU` (new-rail→MOD_3V3) fitted on KU040 — **never both** (prevents the two regulators back-driving each other). Diff BOM-vs-CPL designators before every order (the recurring IC1-DNP-desync lesson, `RESPIN_CHECKLIST.md` §7).

---

## 3. Regulator selection

### 3.1 Monolithic vs 4-switch-controller-plus-FETs, and USB vs battery

- **Most monolithic buck-boosts do top out at 4–5 A** (LTC3119 5 A, LTC3113 3 A, MP8859 2.5 A, MP28167 4 A) → individually short of an 8 A target. **Exception:** the **TPS55288 integrates two 16 A boost-leg MOSFETs** (four-switch), so it delivers the 8 A target *monolithically* — no external power FETs, no gate-drive layout. This is why a controller+FET solution is **not strictly required** at this current, contrary to the usual rule.
- **4-switch controller + external FETs** (LTC3785, LM5176) is the firmware-free route and scales to arbitrary current, but: **LM5176 min V_IN = 4.2 V** (2.5 V only with a bias supply) → **cannot hold 3.3 V from a 3.0 V sagging cell** → disqualified for the battery/flight path. **LTC3785 (2.7–10 V)** is the correct V_IN match but **LCSC stock is thin** (see §3.3).
- **USB vs battery:** heavy KU040 use on **USB** would be a trivial 5 V→3.3 V **buck** — but the LM66100 USB leg is 1.5 A (≈ 5 W ≈ 1.5 A of 3.3 V) and a KU040 needs ~4 A@5 V ≈ 20 W, which no USB-C-Rd source + 1.5 A diode can supply. **The KU040 flies on battery** (ECO §9.7), so the rail **must** be a battery-capable buck-boost down to ~3.0 V. A USB-only buck is therefore not a substitute; USB stays a bring-up/config/idle source. (If bench-only heavy USB operation is ever wanted, add a parallel 5 V→3.3 V buck fed from `+5V`, e.g. TPS54540, enabled only when USB present — optional, not in the flight BOM.)

### 3.2 PRIMARY — TI TPS55288RPMR

**Part:** TPS55288RPMR, **LCSC C2864583** — **5 849 in stock, $2.79 @1 / $2.42 @10**, VQFN-26-HR **3.5×4 mm** (SLVSF01B).

| Parameter | Value | Note |
|---|---|---|
| Topology | Synchronous **four-switch buck-boost**, average-current-mode | integrated FETs |
| V_IN | **2.7 V – 36 V** | covers the 3.0–4.2 V 1S range **and** 5 V USB |
| V_OUT | 0.8 V – 21.26 V | set to 3.3 V |
| Integrated switches | **two 16 A boost-leg MOSFETs**; output current to 16 A | ~2× margin over the 8 A target |
| Avg inductor current limit | resistor on **ILIM pin**: **20 kΩ → 16.5 A typ**; range 9.09 k–100 kΩ (larger R = lower limit, per datasheet Eq. 5) | **firmware-free** current limit |
| Switching frequency | 200 kHz – 2.4 MHz (external R) | pick ~1 MHz (see §4) |
| EN/UVLO | precision **1.23 V** threshold, 5 µA source → resistor-divider UVLO | **firmware-free** cell-protection cutoff |
| Interface | I²C: sets V_OUT (11-bit DAC, 0.8–22 V) or selects external-FB divider; sets output-enable + I²C current limit | ESP configures it |

**Configuration for this design:**
- **Output 3.3 V:** either the internal DAC (I²C) or **external FB divider** (set FB bit in reg 04h, R_FB_UP = 100 kΩ). Given the ESP is configuring anyway, the DAC is simplest.
- **ILIM pin:** target ~11–12 A average inductor-current limit (above the ~10 A worst-case input, §1.3; below the 16.5 A max) → **R_ILIM ≈ 27–30 kΩ** (finalize with datasheet Eq. 5; 20 kΩ = 16.5 A, larger = lower). This is the hard, firmware-independent ceiling.
- **EN/UVLO divider (from VSYS):** rising ≈ **3.05 V** for 1S over-discharge protection, e.g. **R_top ≈ 24 kΩ / R_bot ≈ 16 kΩ** → 1.23·(40/16) = 3.08 V rising; the 5 µA source × 24 kΩ ≈ 0.12 V hysteresis → ~2.96 V falling. Keep it at/just below the LTC3119 RUN-UVLO (~3.12 V) so housekeeping governs the final cutoff. Also gate EN with the **SYS_ON / SW5** master condition (AND the UVLO divider with master-on) so the master switch kills this rail.
- **Startup behaviour (important):** with EN/UVLO above 1.23 V the device is enabled for **I²C and switching**, but reliably reaching 3.3 V with output enabled needs the ESP to write the config (V_OUT + output-enable). Below 0.4 V on EN the registers reset to defaults. → **The ESP (on the housekeeping rail) must configure this rail before FPGA power-up.** The **ILIM and UVLO are resistor-set and protect the hardware regardless of firmware state.**
- **PGOOD / current monitor:** route the TPS55288 status + its load-current-monitor output to an ESP/FPGA GPIO / the ADS7128 ADC for rail-health and KU040 power telemetry.

### 3.3 FALLBACK (firmware-free) — ADI LTC3785 + 4× AON N-FETs

**Part:** LTC3785EUF#PBF (QFN-24 4×4). **2.7–10 V V_IN/V_OUT**, all-N-channel four-switch buck-boost **controller**, 100 kHz–1 MHz, resistor-set output, **programmable soft-start + current limit**, foldback/short/OVP. The **ideal 1S V_IN match** and **fully firmware-free** (resistor FB). **Stock caveat:** the LCSC listing (LTC3785EUF-1#TRPBF, **C685993**) is **out of stock** → **hand-populate from Digi-Key/Mouser** (LTC3785EUF#PBF), or order ahead. External FETs (4×, 20–30 V, low R_DS(on), the board already stocks AON parts): e.g. **AON6414A** (30 V N, ~2.5 mΩ) or reuse-class **AON7534** (30 V, 30 A, ≤8.5 mΩ @4.5 V, LCSC C115844). Loop comp on the ITH pin per LTpowerCAD. Use this **only if a firmware-independent module rail is a hard requirement**; otherwise the TPS55288 is simpler, cheaper, in-stock, and its firmware dependence is a feature (deliberate sequencing) in the split architecture.

### 3.4 Recommendation

**Primary: TPS55288RPMR (C2864583).** In stock, integrated 16 A, resistor-set hardware protection, ESP-sequenced — the cleanest fit for the split-rail design.
**Fallback: LTC3785 + AON FETs**, firmware-free, if mandated (accept the LCSC stock/hand-populate hit).

---

## 4. Passives

### 4.1 Inductor — the part that must NOT be under-sized (the old L4 lesson)

The as-built L4 (2.2 µH in an **0603**) saturated at ~1–3 A against U2's 8–11 A switch currents and was a co-cause of the U2 melt (`RESPIN_POWER_U2.md` §1.1). Size the new inductor **above the peak switch current**: at 8 A out / ~10 A input, the boost-region inductor current ≈ input current ≈ 10 A average, with peak = I_avg + ½·ΔI_L. Choose L for a sane ripple and rate Isat/IRMS well above the peak **and** above the ILIM ceiling (~12 A avg / higher peak).

- **Inductance:** **1.0 µH** at ~1 MHz (TPS55288-typical). Ripple ΔI_L = V_IN·(V_OUT−V_IN)/(V_OUT·L·f) in buck, or the boost equivalent; at 1 µH/1 MHz, ΔI_L ≈ 2–3 A → peak inductor current ≈ 11–13 A at the 10 A input corner. (1.5 µH is acceptable for lower ripple at slightly higher DCR.)
- **Ratings:** **Isat ≥ 20 A, I_RMS ≥ 15 A, DCR ≤ 8 mΩ** — comfortably above the ILIM ceiling and peak current.
- **Recommended part:** **Sunlord MWSA1204S-1R0MT** — 1.0 µH, **Isat 28 A, I_RMS(heat) 35 A, DCR 7.5 mΩ**, molded shielded 12.5×12.5×4 mm. (1.5 µH sibling **MWSA1204S-1R5MT**: 1.5 µH, Isat 24.4 A, DCR 9.5 mΩ.) *Confirm the LCSC C-number on order — the MWSA1204S series is in the JLC catalog.*
- **Confirmed-stock lighter alternate:** **Sunlord MWSA1004-2R2MT** (2.2 µH, DCR 6.3 mΩ, **Isat 13 A**) = **LCSC C149550** — Isat 13 A is **marginal** (below the 12 A ILIM ceiling's peak), so use it only if the 1204S is unavailable, and then lower ILIM accordingly. **Prefer the 1204S.**
- DCR loss at 10 A: 7.5 mΩ × 10² = **0.75 W** in the inductor — fine for a 35 A-rated molded part; feed it with wide copper both sides.

### 4.2 Input capacitance (VIN of TPS55288)

- **Ceramic:** **2–3 × 22 µF / 10 V X5R/X7R** (0805/1206) + **0.1 µF** HF close to VIN/PGND — the switching-current path decoupling.
- **Bulk:** add **1 × 100 µF** (low-ESR ceramic or polymer, ≥10 V) to hold VIN during the KU040 startup surge / hot transients (the input sees ~10 A). This also softens the source impedance the eFuse sees.
- Keep the loop VIN→FET→PGND tight and wide (§7).

### 4.3 Output capacitance (3.3 V → module)

- **Ceramic:** **3 × 22 µF / 10 V X7R** + a few 0.1 µF near the B2B pins. The module carries its own input bulk (TE0712 REV03 uses 6 × 22 µF/10 V on its 3.3 V input; TE0841 similar), which parallels this — good.
- **Bulk:** **1 × 100–150 µF** low-ESR for the config-time load step.
- Total effective output cap (carrier + module) ~ 200–300 µF → keeps the rail stable through the ≥4 A startup step; size the eFuse soft-start to charge it without nuisance-tripping (§5.2).

### 4.4 Compensation / config support

- **TPS55288** loop compensation is largely handled internally for the USB-PD-class part; fit the datasheet-recommended VCC (1 µF) and any bootstrap caps, and verify the loop with TI's design tool for V_IN 3.0–5 V, V_OUT 3.3 V, 8 A. Set frequency via the FREQ resistor to ~1 MHz (balance: small inductor vs. sensor-band switching noise — fixed-frequency PWM, not a burst mode, for predictable spectrum near the IMU/baro).
- **LTC3785 (fallback):** external loop comp on ITH (R+C), SS cap sets soft-start and doubles as fault timer, R_SENSE or the FET R_DS(on)-sense sets current limit — per datasheet / LTpowerCAD.

---

## 5. Input eFuse for this rail

### 5.1 Why (the recurring lesson) & where

The battery path has **no series current limit** (Q9 30 A FET + 1S LiPo good for tens of A + no fuse). On USB the LM66100's 1.5 A ceiling turned the JB1.13 `+3V3`→GND short into a harmless brown-out; on battery the same short fed U2 unlimited → SW1 melt (ECO §8.11.8). This new rail carries **more** current than U2 did → it **must** have its own input current-limit.

**Placement:** on the **new rail's input branch only** — `VSYS → TPS25982 → TPS55288 VIN` — downstream of the ORing junction (protects regardless of source) and immediately upstream of the buck-boost (caps its input and catches a shorted `MOD_3V3`/module short by starving it). The eFuse's EN replaces a discrete load switch; gate its EN with the **SYS_ON/SW5** master condition. Keep the existing/planned smaller eFuse (TPS1663-class, ~6 A, per `RESPIN_POWER_U2.md` §4) on the **U2 housekeeping branch** so the two rails are independently protected. *(Alternative: a single main TPS25982 on VSYS covering both branches at ~12 A — simpler, one part, but loses per-rail fault isolation. Per-rail is recommended.)*

### 5.2 Part & sizing — TI TPS25982

**Part:** TPS25982, **2.7–24 V, 2.7 mΩ, 15 A** smart eFuse, integrated FET, 1.5 %-accurate load-current monitor, adjustable transient-fault management, integrated hot-swap, 4×4 mm VQFN. **TPS259822ONRGET = LCSC/JLC C2155683** (auto-retry). Use the **latch-off** sibling (TPS259821…) if you want a JB1.13-class hard fault contained rather than cycled — recommended for this rail.

- **Continuous current-limit (ILIM resistor): ~12 A** — above the ~9.8 A worst-case input (§1.3) so no nuisance trip, below destructive. (2.7 mΩ × 12² ≈ **0.39 W** self-heating — trivial in a 15 A part with a copper pad.)
- **Fast-trip / short threshold: ~15 A** — a bolted `MOD_3V3`→GND short (a 1S cell would drive to tens of A) trips in µs, before the TPS55288 or the module can be damaged.
- **Soft-start (dVdt) / inrush:** size to charge the ~200–300 µF of rail + module input caps and ride the ≥4 A KU040 startup surge without tripping (a few ms), while still protecting on a bolted short. Use the TPS25982 adjustable transient-fault timer so the startup surge is allowed but a sustained overload trips.
- **OVP clamp:** the TPS25982 clamps VSYS transients (hot-plug) — cheap insurance for the buck-boost (TPS55288 abs-max 36 V, so ample headroom).
- **FAULT# + IMON → GPIO/ADC:** route to an ESP/FPGA GPIO and the load-monitor to the ADS7128 so firmware sees an over-current/short trip and logs KU040 rail current (would have caught the §9.8 "0.45→0.8 A creep").

Do **NOT** use the 10 A **TPS25981** here — at ~10 A worst-case input it sits at its rating edge with no margin.

### 5.3 ORing-FET / series-element confirmation at ~10–12 A

| Element | Ref | Rating | R_DS(on) @ drive | P @ 12 A | Verdict |
|---|---|---|---|---|---|
| Battery ORing N-FET | **Q9 AON7534** | 30 V, **30 A** (23 A @ Tc 100 °C) | ≤ 8.5 mΩ @ Vgs 4.5 V (LM74700 charge-pump drives it hard, ≤5 mΩ @10 V) | **~1.2 W** (≤ 0.7 W at 5 mΩ) | ✅ margin |
| Battery ideal-diode ctrl | U13 LM74700 | controller, no main current | — | ~0 | ✅ |
| USB ORing diode | U8 LM66100 | 5.5 V, **1.5 A** | 0 A on battery | n/a in flight | ⚠ USB = config/idle only |
| New rail eFuse | **TPS25982** | 15 A, 2.7 mΩ | — | 0.39 W @12 A | ✅ |
| New buck-boost | **TPS55288** | 16 A switch | — | see §6 | ✅ (2× margin) |

VSYS copper and the ORing junction now carry **~12 A** (new rail ~10 A + housekeeping ~1.2 A input) — see the trace/via table in §7. Q9's DFN thermal pad handles ~1.2 W; confirm its pad copper on re-layout.

---

## 6. Thermal design (exploit the freed board area)

### 6.1 TPS55288 dissipation

Loss = P_out·(1/η − 1). Worst efficiency is the deep 4-switch region (V_in ≈ V_out). For an **integrated-FET** part the IC carries most of the loss (conduction in the 4 FETs at the ~10 A inductor current + switching + control).

| Case | I_out | V_in | η | P_out | Total loss | ~IC dissipation |
|---|---|---|---|---|---|---|
| Heavy (sustained-realistic) | 6 A | 3.7 V | 0.88 | 19.8 W | 2.7 W | ~1.8 W |
| Design target | 8 A | 3.3 V | 0.86 | 26.4 W | 4.3 W | ~2.8 W |
| Aggressive + sag (transient) | 8 A | 3.0 V | 0.85 | 26.4 W | 4.7 W | ~3.0 W |

### 6.2 θJA target & junction temp

VQFN-26 (3.5×4 mm) with exposed pad: **θJA ≈ 40 °C/W** on a JEDEC 2s2p board; with a **large copper pour + dense EP via array** (now affordable with the freed area) the effective θJA drops toward **20–25 °C/W** (θJC(bottom) is only a few °C/W). Tj = Tamb + P_IC·θJA, Tamb = 45 °C (warm enclosure):

| Case | P_IC | Tj @ θJA 40 (bad) | Tj @ θJA 25 (target) | Tj @ θJA 20 (best) |
|---|---|---|---|---|
| Heavy 6 A | 1.8 W | 117 °C | **90 °C** ✅ | 81 °C ✅ |
| Target 8 A | 2.8 W | 157 °C 🔴 | **115 °C** ✅ | 101 °C ✅ |
| Aggressive+sag 8 A | 3.0 W | 165 °C 🔴 | 120 °C ⚠ | 105 °C ✅ |

**Reading:** with a proper pour (θJA ≤ 25 °C/W) the part stays under 125 °C TJMAX even at the 8 A target; the "bad copper" column is the trap that melted U2. **The 8 A / 3.0 V corner is a transient** (a cell near cutoff cannot sustain 8 A), and the resistor ILIM (~12 A) plus UVLO fold it back — so **continuous 6–7 A is comfortable, 8 A is protected headroom.** Design for **θJA ≤ 25 °C/W** and it is safe across the envelope.

### 6.3 Copper / via prescription (both regulator and eFuse)

- **TPS55288 EP:** land it on a **large ground pour** (≥ 150–200 mm² reachable copper on the layer + an internal/bottom plane), with a **thermal-via array of ≥ 16–25 × 0.3 mm vias** on ~0.8–1.0 mm pitch under the pad, tented/filled, dropping to an internal + the opposite-side plane. Prefer the **top layer** for the regulator (avoid the bottom-layer/sparse-via mistake that gave U2 a real θJA ≈ 45 °C/W).
- **TPS25982 EP:** same idea, ~12–16 × 0.3 mm vias into a plane (only ~0.4 W, so less critical, but keep it cool for the accurate current monitor).
- **Inductor:** wide copper on both terminals (it dissipates ~0.75 W); it also acts as a heat spreader.
- Keep the two hot parts spaced so their pours don't fight for the same copper.

---

## 7. Layout / current guidance (IPC-2221)

IPC-2221: I = k·ΔT^0.44·A^0.725 (A in mil², k = 0.048 external / 0.024 internal). At **ΔT = 20 °C**:

| Path | Current | 1 oz external width | **2 oz external width** | 1 oz internal width |
|---|---|---|---|---|
| **Input** (VSYS→eFuse→TPS55288 VIN) | ~10 A | ~4.7 mm (186 mil) | **~2.4 mm** | ~9 mm |
| **ORing junction / VSYS** (both rails) | ~12 A | ~6.0 mm (236 mil) | **~3.0 mm** | ~11 mm |
| **Output** (3.3 V → module B2B) | 8 A | ~3.5 mm (137 mil) | **~1.7 mm** | ~6.5 mm |

**Prescription:**
- **Carry all power on F.Cu/B.Cu polygon pours, not traces** — this is a 4-layer 1.6 mm board with likely thin (0.5 oz) inner copper. Prefer **2 oz outer copper** for the input/VSYS path (JLC offers 2 oz outer); if 1 oz, the pours must be as wide as the table's 1 oz column and continuous (no necks).
- **No neck-downs** at the switch node, eFuse, or B2B fan-out (the SW1-to-0.3 mm-segment mistake that stressed U2, ECO §8.4 #4).
- **Vias:** a filled/plated 0.3 mm via carries ~1.5 A conservatively. Use **≥ 8–10 × 0.3 mm vias** at each layer transition on the ~10 A input/VSYS path, **≥ 6–8** on the 8 A output path. Stitch the pours generously.
- **Switch node (SW pins → inductor):** short, wide, single-layer where possible; keep it a compact area (dV/dt EMI). Return the input-cap and inductor grounds to the EP/PGND with the tightest loop.
- **eFuse and buck-boost input caps:** place the input ceramics right at the pins; keep the high-di/dt loop tiny.
- Route the module `MOD_3V3` from the buck-boost output across **all** `VIN`/`3.3VIN` B2B pins (JM1.1/3/5/13/15 + JM2.2/4/6/8/10/12 via the carrier map) so the 8 A shares across many contacts (Samtec LSHM 2 A/pin × many ≫ 8 A) — keep every power/GND B2B pin populated.

---

## 8. BOM / cost & assembly-variant summary

### 8.1 New parts (KU040 build only — DNP on Artix)

| Ref (proposed) | Part | LCSC | ~Unit $ | Function |
|---|---|---|---|---|
| U_bb | **TPS55288RPMR** | **C2864583** (5 849 stk) | 2.79 | 16 A four-switch buck-boost (module 3.3 V) |
| U_ef | **TPS259822ONRGET** (or "…821" latch) | **C2155683** | ~1.5 | 15 A smart eFuse, ILIM 12 A |
| L_bb | **Sunlord MWSA1204S-1R0MT** (1 µH/28 A) | *confirm C# (MWSA1204S series in JLC cat.)*; alt MWSA1004-2R2MT = **C149550** (13 A, marginal) | ~0.6 | buck-boost inductor |
| C_in | 2–3× 22 µF/10 V X7R + 100 µF bulk + 0.1 µF | JLC basic | ~0.3 | VIN decoupling |
| C_out | 3× 22 µF/10 V X7R + 100–150 µF bulk + 0.1 µF | JLC basic | ~0.3 | 3.3 V output |
| R_ILIM, R_FREQ, R_UVLO(×2), R_FB, R_ILIM(eFuse), R_SS(eFuse), VCC/BST caps | 0402/0603 | JLC basic | ~0.4 | config network |
| R_KU | 0 Ω (new-rail → MOD_3V3 select) | JLC basic | ~0.01 | source-select (KU040) |
| **Total added / KU040 board** | | | **≈ $5–7** | |

### 8.2 Unchanged / all builds

- **U2 LTC3119** + its L4-replacement (2.2–3.3 µH, Isat ≥ 11 A power inductor per `RESPIN_POWER_U2.md`), its eFuse (TPS1663-class ~6 A), FB/RUN dividers — the **shared housekeeping rail**.
- ORing front-end: Q9 AON7534 (C115844), U13 LM74700, U8 LM66100 — carry ~12 A, confirmed OK.
- `R_HK` = 0 Ω (U2 → MOD_3V3 select) — fitted on **Artix** builds.

### 8.3 Module-3V3 source-select (build-time)

Introduce a dedicated net **`MOD_3V3`** = the module `VIN`/`3.3VIN` B2B feed. Two mutually-exclusive 0 Ω/footprint jumpers select its source:
- **Artix build:** fit **`R_HK`** (U2 `+3V3` → `MOD_3V3`); DNP the new rail + `R_KU`. Module runs off the LTC3119 (≤3 A — validated).
- **KU040 build:** fit **`R_KU`** (new-rail output → `MOD_3V3`); DNP `R_HK`. Module runs off the TPS55288 (8 A).
- **Never fit both** (prevents U2 and the new rail back-driving each other). This is the single net-topology change the split rail requires; the rest is populate/DNP.

### 8.4 Assembly-variant process

Two JLC assembly variants (or two BOM/CPL pairs) off one schematic; KiCad DNP on the new-rail group for the Artix variant. **Diff BOM designators vs CPL designators before every order** (the IC1-DNP-desync class of failure, `RESPIN_CHECKLIST.md` §7). Wire the TPS55288 status/monitor + eFuse FAULT to GPIO/ADC.

---

## 9. Prioritized action list

| # | Priority | Action | Why |
|---|---|---|---|
| 1 | 🔴 must | **Adopt the SPLIT-RAIL topology** — U2/LTC3119 = shared housekeeping (all builds); new TPS55288 rail = module-only (KU040 builds, DNP on Artix). Add net `MOD_3V3` + `R_HK`/`R_KU` source-select. | §2 — protects the proven Artix build, isolates faults, solves the I²C bring-up chicken-and-egg |
| 2 | 🔴 must | **Add the TPS55288RPMR buck-boost** (C2864583): ILIM R for ~12 A, UVLO divider ~3.05 V, ~1 MHz, 3.3 V out; ESP configures VOUT+enable off the housekeeping rail before FPGA power-up. | §3 — the 8 A KU040 rail |
| 3 | 🔴 must | **Add the TPS25982 eFuse** (C2155683) on VSYS→TPS55288 VIN: ILIM ~12 A, fast-trip ~15 A, soft-start for the ≥4 A startup, latch-off preferred, FAULT+IMON→GPIO/ADC; EN=SYS_ON/SW5. | §5 — the missing input current-limit that let JB1.13 kill U2 |
| 4 | 🔴 must | **Spec the inductor properly:** 1.0 µH, Isat ≥ 20 A, IRMS ≥ 15 A, DCR ≤ 8 mΩ — **Sunlord MWSA1204S-1R0MT** (28 A). Never an 0603. | §4 — the old L4 (0603, ~1–3 A Isat) co-caused the U2 melt |
| 5 | 🟠 high | **Thermal copper:** TPS55288 on top layer, large GND pour + **≥16–25 × 0.3 mm EP vias**; target θJA ≤ 25 °C/W (Tj < 125 °C at 8 A). Same for the eFuse EP. | §6 — bottom-layer/sparse-via was U2's thermal failure |
| 6 | 🟠 high | **Route power as wide 2 oz pours:** input/VSYS ~2.4–3.0 mm (2 oz), output ~1.7 mm; ≥8–10 stitching vias per transition on the 10–12 A path; no neck-downs; compact SW node. | §7 — ~10 A input / 8 A output / ~12 A VSYS |
| 7 | 🟠 high | **Confirm ORing/VSYS carry ~12 A:** Q9 AON7534 fine (P ≈ 1.2 W); widen its pad copper; USB leg is config/idle only (document "battery required for KU040"). | §5.3 |
| 8 | 🟡 med | **Telemetry:** wire TPS55288 status + load-monitor and eFuse FAULT to GPIO/ADS7128; add the input current-sense (INA226/shunt) already planned. | rail-health / KU040 power budget |
| 9 | 🟢 low | Keep **LTC3785 + AON FETs** as the documented firmware-free fallback (accept the LCSC-stock/hand-populate hit) if a firmware-independent rail is ever mandated. | §3.3 |
| 10 | 🟢 low | Optional bench-only 5 V→3.3 V buck off `+5V` for heavy USB operation (not in the flight BOM). | §3.1 |

**Net:** items 1–4 are the mandatory set (split topology + TPS55288 + TPS25982 + a real inductor). With the thermal-copper (5) and wide-pour (6) layout, the KU040/TE0841 rail is robust across the full 1S discharge, the proven Artix build is untouched, and a downstream short can no longer destroy the regulator.

---

## 10. References

- **TI TPS55288 datasheet (SLVSF01B), "36-V, 16-A Buck-Boost Converter with I²C Interface"** — 2.7–36 V V_IN, 0.8–21.26 V V_OUT, two integrated 16 A boost-leg MOSFETs, resistor-set ILIM (20 kΩ→16.5 A typ, range 9.09–100 kΩ), EN/UVLO 1.23 V + 5 µA, 200 kHz–2.4 MHz, VQFN-26-HR 3.5×4 mm. https://www.ti.com/lit/ds/slvsf01b/slvsf01b.pdf — LCSC **C2864583** (TPS55288RPMR), 5 849 stk, $2.79/@1. https://www.lcsc.com/product-detail/C2864583.html
- **TI TPS25982 datasheet, "2.7-V to 24-V, 2.7-mΩ, 15-A Smart eFuse"** — integrated FET, 1.5 % load-current monitor, adjustable transient-fault management, hot-swap, 4×4 VQFN. https://www.ti.com/lit/ds/symlink/tps25982.pdf — TPS259822ONRGET = LCSC/JLC **C2155683**. https://jlcpcb.com/partdetail/TexasInstruments-TPS259822ONRGET/C2155683
- **ADI LTC3785 datasheet** — 2.7–10 V, all-N four-switch buck-boost controller, resistor-set, prog. soft-start/current-limit, foldback/short/OVP, QFN-24 4×4. https://www.analog.com/en/products/ltc3785.html — LCSC **C685993** (LTC3785EUF-1#TRPBF, out of stock at time of writing).
- **TI LM5176 datasheet** — 4-switch buck-boost controller, **V_IN 4.2–55 V** (2.5 V only with bias) → **min V_IN too high for the 1S floor**; disqualified for the battery path. https://www.ti.com/lit/ds/symlink/lm5176.pdf
- **Sunlord MWSA-S series datasheet** — molded SMD power inductors; **MWSA1204S-1R0MT** 1.0 µH / DCR 7.5 mΩ / Isat 28 A / IRMS 35 A; **MWSA1204S-1R5MT** 1.5 µH / 9.5 mΩ / 24.4 A; **MWSA1004-2R2MT** 2.2 µH / 6.3 mΩ / 13 A = LCSC **C149550**. https://media.digikey.com/pdf/Data%20Sheets/Shenzhen%20Sunlord/MWSA-S_Series_DS.pdf
- **Trenz TE0841 TRM (REV02)** — KU035/KU040 4×5 SoM: single 3.3 V supply, **"minimum current capability of 4 A for system startup,"** power "TBD/design-dependent (use Xilinx XPE)"; VIN abs-max 6.0 V, 3.3VIN abs-max 3.4 V. https://www.trenz-electronic.de/trenzdownloads/Trenz_Electronic/Modules_and_Module_Carriers/4x5/TE0841/REV02/Documents/TRM-TE0841-02.pdf
- **AMD/Xilinx Kintex UltraScale Data Sheet: DC and AC Switching Characteristics (DS892)** — XCKU040 rail set (VCCINT 0.95 V, VCCAUX 1.8 V, VCCO incl. 3.3 V HR I/O), power-domain methodology behind §1.2. https://www.mouser.com/pdfDocs/Kintex-UltraScale-ds892-kintex-ultrascale-data-sheet.pdf
- **AOS AON7534** (Q9 battery ORing) — 30 V N, 30 A, ≤8.5 mΩ @Vgs 4.5 V. LCSC **C115844**.
- Companions: `RESPIN_POWER_U2.md` (U2/LTC3119 rating + load review, the L4 lesson, the housekeeping-rail eFuse), `ECO-001_3v3_power_redesign.md` §8–§9 (U2 field incident + JB1.13 root cause + PSU/USB/battery validation), `RESPIN_CHECKLIST.md` (master respin list).
