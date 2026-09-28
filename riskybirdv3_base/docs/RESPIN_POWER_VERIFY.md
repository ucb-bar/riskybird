# RESPIN_POWER_VERIFY — First-principles audit of the single-rail 3.3 V power redesign

**Project:** riskybird v3 (`riskybirdv3.kicad_sch`)
**Scope:** Independent, datasheet-driven verification of the new single-rail 3.3 V front-end that replaced the LTC3119 (U2). Every schematic connection and every passive value was checked against the exported netlist and recomputed from the primary datasheets — **not** trusted from `RESPIN_POWER_RAIL.md`.
**Method:** netlist export (`kicad-cli sch export netlist`) + s-expr parse of the `.kicad_sch` for placed values/LCSC part numbers; datasheet equations pulled from the actual PDFs (`pdftotext`). **All `.kicad_*` files inspected read-only; nothing edited.**
**Datasheets used:** TPS552882-Q1 **SLVSFQ8A** (Dec 2021); TPS25982/TPS2598x **SLVSEI3D** (rev May 2026); AOS **AON7534** (rev'd); Coilcraft **XAL7030-152MEC**.
**Date:** 2026-08-17

---

## 0. Refdes map (as actually placed — verified against the netlist)

| Role | Refdes | Placed value / part |
|---|---|---|
| Buck-boost controller | **U15** | TPS552882QRPMRQ1 (C2876594), VQFN-HR-26 |
| Input eFuse | **U16** | TPS259824ONRGER (C2155766), RGE-24 |
| Buck inductor | **L2** *(doc calls it "L1")* | XAL7030-152MEC (C5352784), 1.5 µH |
| Buck HS FET | **Q8** | AON7534 (C115844) |
| Buck LS FET | **Q10** | AON7534 (C115844) |
| Battery ORing FET / ctrl | Q9 / U13 | AON7534 / LM74700 |
| USB ORing | U8 | LM66100 |
| Master switch | SW5 | SPST slide |
| FB divider | R73 / R74 | 100 k / 56.2 k |
| ILIM (buck-boost) | R75 | 27 k |
| FSW | R76 | 19.6 k |
| MODE | R77 | 27 k |
| COMP | R79 / C82 / C81 | 3.65 k / 5.6 n / 82 p |
| EN/UVLO (buck-boost) | R78 / R80 | 40.2 k / 28 k |
| VCC decoupling | C79 | "4.7 µF" label / **LCSC C53067 = 1 µF 6.3 V** |
| BOOT1 / BOOT2 | C78 / C76 | 0.1 µF / 0.1 µF |
| Cin (VSYS_FUSED) | C86,C87,C85 | 22 µF + 22 µF + 10 µF |
| Cout (+3V3) bulk | C80 (+ C46,C14,C39…) | 150 µF (+ ceramics) |
| eFuse ILIM | R49 | 121 Ω |
| eFuse ITIMER | C84 | 22 nF |
| eFuse dVdt | C83 | 150 pF |
| eFuse IMON | R50 | 604 Ω |
| eFuse EN divider | R52 / R45 | 60.4 k (top) / 40.2 k (bot) |
| eFuse PG pull-up | R51 | 100 k → +3V3 |
| Downstream PG gating | Q6 / Q7 | 2N7002 (gates on /FUSE_PG) |
| AGND↔PGND net-tie | NT1 | NetTie_2 |

---

## 1. Executive summary

**No CRITICAL defects.** The most dangerous prior bug — the buck FET gate-driver swap (HS/LS on the wrong DR pins, Q10 source floating) — is **confirmed fixed** in the schematic. The old LTC3119 rail is **completely removed** with no orphan nets. The JB1.13 root-cause short is **fixed** (JB1.13 now on `+3V3`). All buck-boost and eFuse set-point resistors recompute to their intended targets.

Findings: **0 CRITICAL, 3 MAJOR, ~11 MINOR.** The MAJOR items are all part-selection / value issues, not topology errors, and all have trivial fixes.

| # | Item | Verdict |
|---|---|---|
| **Connectivity** | | |
| A1 | Buck FET gate-driver polarity (Q8→DR1H, Q10→DR1L, Q10.S→PGND) | **PASS** (prior swap fixed) |
| A2 | Buck node SW1 = Q8.S + Q10.D + L2.1 + BOOT1 | PASS |
| A3 | Boost node SW2 = U15.21/25 + L2.2 + BOOT2 | PASS |
| A4 | U15 VIN←VSYS_FUSED, VOUT→+3V3, PGND/AGND, ISP=ISN=VOUT | PASS |
| A5 | FB divider senses +3V3 | PASS |
| A6 | MODE/COMP/VCC set resistors return to **AGND** | PASS |
| A7 | ILIM/FSW/FB-bot/EN-bot resistors return to **GND (PGND)** not AGND | FLAG (MINOR) |
| A8 | NT1 joins AGND↔GND at exactly one point | PASS |
| A9 | eFuse IN←VSYS, OUT→VSYS_FUSED, EP1/EP2 | PASS |
| A10 | eFuse EN/UVLO divider (VSYS→R52→SW5→EN→R45→GND) | PASS |
| A11 | eFuse RETRY_DLY/NRETRY/LDSTRT → GND (latch-off) | PASS |
| A12 | /FUSE_PG → Q6/Q7 gates + 100 k pull-up (R51) | PASS |
| A13 | eFuse IMON node routed to an ADC | FLAG (MINOR — not routed) |
| A14 | U15 PG (pin 5) pull-up / MCU tap | FLAG (MINOR — floating) |
| A15 | Old LTC3119 rail (U2/L4/Q5/R59-62/C60-67/TP1…) removed, no orphans | PASS |
| A16 | JB1.13 reassigned GND→+3V3 (root-cause fix) | PASS |
| **Passives — buck-boost (TPS552882)** | | |
| B1 | FB 100 k/56.2 k → 3.335 V (Eq 6) | PASS *(but see C-Vout MAJOR)* |
| B2 | ILIM 27 k → 12.2 A avg (Eq 5) | PASS |
| B3 | FSW 19.6 k → 1.000 MHz (Eq 3) | PASS |
| B4 | MODE 27 k → Internal-LDO + PFM (Table 7-2) | PASS |
| B5 | EN/UVLO 40.2 k/28 k → on 3.00 V / off 2.77 V | PASS |
| B6 | COMP Type-II 3.65 k/5.6 n/82 p → fz 7.8 k, fp 532 k | PASS (bench-validate PM) |
| B7 | VCC cap C79 (4.7 µF intended / **1 µF 6.3 V placed**) | **FLAG (MAJOR)** |
| B8 | BOOT1/BOOT2 0.1 µF | PASS |
| **Passives — eFuse (TPS259824)** | | |
| B9 | ILIM 121 Ω → 12.2 A (Eq 4) | PASS *(UL-2367 wants ≥182 Ω — MINOR)* |
| B10 | ITIMER 22 nF → **10.3 ms** blanking (Eq 5) *(doc says 178 µs — wrong)* | FLAG (MINOR) |
| B11 | dVdt 150 pF → 30.7 V/ms, inrush 1.66 A (Eq 2/3) | PASS |
| B12 | IMON 604 Ω → 1.81 V @ 12.2 A (Eq 12) | PASS (marginal at trip) |
| B13 | EN/UVLO 60.4 k/40.2 k → on 3.00 V / off 2.75 V (Eq 1) | PASS |
| **Ratings / selection** | | |
| C1 | AON7534 Vds/Vgs/Rds(on)/Qg vs application | PASS (6× Vds margin) |
| C2 | XAL7030-152MEC Isat/Irms/ripple (Artix + KU040) | PASS (Isat 23.5 A) |
| C3 | Current limits cover KU040 worst case | PASS (tight, ~1.24×) |
| C4 | **Cap voltage ratings on VSYS / VSYS_FUSED (6.3 V on ~5 V)** | **FLAG (MAJOR)** |
| C5 | Cap voltage ratings on +BATT (6.3 V on 4.2 V) | FLAG (MINOR) |
| C6 | **Vout worst-case 3.41 V vs TE0841 3.3VIN abs-max 3.4 V** | **FLAG (MAJOR, KU040)** |
| C7 | eFuse OVP = 16.9 V (no OV protection 4–5 V) | FLAG (MINOR, accepted) |
| C8 | Voltage ratings absent from schematic (value+footprint only) | FLAG (process) |
| C9 | JLC assemblability (all parts on JLC) | PASS (buck-boost Extended/thin) |

---

## 2. Section A — Connectivity findings (with observed net members)

### A1 (CRITICAL-class check) — External buck FET gate polarity: **FIXED / PASS**
The prior "as-drawn bug" (RESPIN_POWER_RAIL §R.7: gate drivers swapped, Q10 source floating) is corrected:
- **Q8 (HS):** S1/2/3 → `Net-(U15-SW1)`; D5/6/7/8 → `VSYS_FUSED`; **G4 → `Net-(Q8-G)` → U15 pin 2 (DR1H)**. ✔ drain to VIN side, source to SW1, gate on DR1H.
- **Q10 (LS):** **S1/2/3 → `GND`** (was "unconnected"); D5/6/7/8 → `Net-(U15-SW1)`; **G4 → `Net-(Q10-G)` → U15 pin 1 (DR1L)**. ✔ source to PGND, drain to SW1, gate on DR1L.

This is the correct hybrid-controller buck side: external N-FET half-bridge between `VSYS_FUSED` and `GND`, switch node `SW1`, bootstrapped HS gate via C78 (BOOT1→SW1).

### A2/A3 — Switch nodes
- **`Net-(U15-SW1)`** = {U15.23 (SW1), Q8.S×3, Q10.D×4, L2.1, C78.2 (BOOT1 cap)} — correct buck node.
- **`Net-(C76-Pad2)` (SW2)** = {U15.21, U15.25 (both SW2 bonds), L2.2, C76.2 (BOOT2 cap)} — correct boost node. Boost FETs internal; L2 straddles SW1↔SW2.

### A4 — U15 core pins
`VIN(3)←VSYS_FUSED`; `VOUT(11,26)→+3V3`; `PGND(9,24)→GND`; `AGND(10)→AGND`; `DITH/SYNC(7)→GND` (dithering off, fixed-freq — good for sensor EMI); `ISP(12)=ISN(13)=+3V3` → output-current-sense loop **disabled** per datasheet §7.3.9 (relies on ILIM + eFuse) ✔. Floating (acceptable): PG(5), C̄C̄(6), CDC(16 — reads 0 V since ISP=ISN).

### A6/A7/A8 — Grounding
- **AGND island** = {U15.10, R77 (MODE), C81+C82 (COMP), C79 (VCC), NT1.1}. **`NT1.2 → GND`** → AGND meets PGND at exactly one point ✔.
- **MINOR (A7):** the set resistors **R74 (FB-bot), R75 (ILIM), R76 (FSW), R80 (EN-bot) all return to `GND` (PGND)**, not AGND. Datasheet §7.3.7/7.3.9 specify these resistors to **AGND**. Because AGND↔PGND single-point-tie carries only µA of analog return, the DC reference error is negligible (<<1 %), so this does not shift fSW/ILIM/Vout meaningfully — but it is a reference-hygiene deviation worth correcting on the next spin (route R74/R75/R76 to the AGND island).

### A9/A10/A11/A12 — eFuse
- IN(1,2,3,16,EP1=25) → `VSYS`; OUT(17–24) → `VSYS_FUSED`; GND(4,5,14,EP2=26) → `GND` ✔.
- **EN/UVLO(6)** = `Net-(SW5-B)` = {U16.6, R45 (40.2 k→GND), SW5.2}. `SW5.1 → Net-(SW5-A) = {R52 (60.4 k→VSYS)}`. So the full path is **VSYS → R52(60.4 k) → SW5 → EN → R45(40.2 k) → GND** — SW5 in series with the top leg (open = EN pulled to GND, zero-leak off) ✔. Matches doc R.6 exactly.
- RETRY_DLY(10)/NRETRY(11)/LDSTRT(12) → `GND`: latch-off circuit breaker, no auto-retry, load-detect unused ✔ (datasheet Table 7-1 / pin desc).
- **PG(13) → `/FUSE_PG`** = {U16.13, R51 (100 k→+3V3 pull-up), Q6.G, Q7.G}. Open-drain PG pulls the 2N7002 gates; rail-good ⇒ Q6/Q7 on. Q6.D→R31(100 k)→`/GPIO5` (fuse-PG telemetry to ESP), Q7.D→D4 (status LED). Nothing floats ✔.

### A13/A14 — Unused telemetry (MINOR)
- **IMON(9)** = {U16.9, R50 (604 Ω→GND)} only — the IMON voltage is developed but **not routed to any ADC** (ADS7128 U1 is on-board). Current-monitor feature is effectively unused. Optional per doc; note as unrealized intent.
- **U15 PG(5)** floating — no 100 k pull-up to +3V3, no MCU tap (doc R.4 intended one). Rail-good from the regulator is not observable.

### A15/A16 — Teardown & root-cause fix
- **Old LTC3119 rail fully removed:** U2, L4, R59, R60, R61, R62, R71, C60, C61, C62, C65, C67, TP1, Q5, R47, R48 all absent from the netlist. No `VCOM`, no dangling `VSENSE`, no LTC3119 refs. (R58 still exists but is repurposed as a 10 k pull-up on +3V3 — not an orphan.) ✔
- **`+3V3` now includes JB1.13 and JB1.14** (both on the +3V3 plane) — the JB1.13 GND→+3V3 root-cause fix (ECO §8.11.7) **is in the schematic** ✔.
- `/3v3OUT` (JB2.9/11) remains a pre-existing benign 2-node carrier float (joins module 3.3VIN when mated, ECO §8.11.4) — not part of this rail.

---

## 3. Section B — Passives recomputation (datasheet equations, arithmetic, verdict)

### 3.1 Buck-boost TPS552882 (SLVSFQ8A). Constants: **VREF = 1.2 V** (§6.5 REFERENCE VOLTAGE, 1.188–1.212); **GEA = 190 µA/V** (§6.5 ERROR AMPLIFIER); VCC LDO = **5.2 V** (§7.3.1).

| Param | Datasheet eq | Computed ideal | Nearest std | **Placed** | Result |
|---|---|---|---|---|---|
| **FB divider** | Eq 6: `Vout = 1.2·(1+Rtop/Rbot)` | Rbot=57.14 k → 3.30 V exactly | 56.2 k (E96) → **3.335 V** | R73=100 k, R74=**56.2 k** | Vout = 1.2·(1+100/56.2)=**3.335 V** (+1.06 %). PASS; see C6. |
| **ILIM** | Eq 5: `I_AVG = 330000/R_ILIM` (min-term=1 for Vout≥1.67 V; calib. 20 kΩ→16.5 A) | 27.05 k → 12.2 A | 27 k (E24) | R75=**27 k** | 330000/27000 = **12.22 A** avg. PASS. |
| **FSW** | Eq 3: `fSW[MHz] = 1000/(0.05·R_FSW[Ω]+20)` (calib. 100 kΩ→200 kHz) | 19.6 k → 1.000 MHz | 19.6 k (E96) | R76=**19.6 k** | 1000/(0.05·19600+20)=1000/1000 = **1.000 MHz**. PASS (range 9.09–100 kΩ ✔). |
| **MODE** | Table 7-2 + V_MODE = I_MODE(10 µA)·R; thresholds DT3 0.189 / DT2 0.351 / DT1 0.614 V | 24.9 k = Internal-LDO+PFM | 24.9 k | R77=**27 k** | V_MODE=10 µA·27 k=**0.270 V** ∈ (0.189, 0.351) ⇒ **Internal VCC LDO + PFM**. Robust across I_MODE 9–11 µA (0.243–0.297 V, both in band). PASS. |
| **EN/UVLO** | Fig 7-2: `Vin_on = 1.23·(Rtop+Rbot)/Rbot`; hyst ≈ I_UVLO(5 µA)·Rtop + VUVLO_HYS(14 mV)·ratio | on 3.00 / off ~2.77 V | 40.2 k / 28 k | R78=40.2 k, R80=**28 k** | on = 1.23·68.2/28 = **2.996 V**; off ≈ 3.00 − 5 µA·40.2 k − 0.014·2.436 = **2.77 V**. PASS (1S over-discharge backstop). |
| **COMP** | Type-II; fz=1/(2π·Rc·Cc); fp=1/(2π·Rc·Cp); GEA=190 µA/V | fz~7 k target | 3.65 k/5.6 n/82 p | R79=3.65 k, C82=**5.6 n**, C81=**82 p** | fz = 1/(2π·3650·5.6n)=**7.79 kHz**; fp = 1/(2π·3650·82p)=**532 kHz** (≈fSW/1.9). Mid-band gm·Rc=0.69. PASS — form is correct, zero near target crossover, HF pole below fSW. **Exact crossover/PM require a plant model (current-sense gain not published) → bench load-transient validation recommended** (datasheet gives no numbered comp equations for this average-current-mode part). |
| **VCC cap** | §7.3.1 internal LDO output = 5.2 V @ 60 mA | ≥4.7 µF, ≥10 V | 4.7 µF/16 V | C79 = **"4.7 µF" label but LCSC C53067 = 1 µF 6.3 V** | **FLAG (MAJOR):** value mismatch (1 µF vs 4.7 µF intended) **and** 6.3 V on a 5.2 V node (1.24× — Class-II MLCC severely derated + poor reliability margin). Use 4.7 µF / ≥16 V. |
| **BOOT1/2** | 0.1 µF BOOT→SW | 0.1 µF | 0.1 µF | C78,C76 = **0.1 µF** | PASS. |
| **Cin** | ≥2×22 µF + HF | 44 µF + 0.1 µF | — | C86,C87=22 µF + C85=10 µF = **54 µF** | Capacitance PASS; **no dedicated 0.1 µF HF cap on VIN** (MINOR) and voltage rating flagged in C4. |
| **Cout** | ≥100 µF bulk + ceramics | ~100–150 µF | — | C80=150 µF (+10+10+4.7 µF ceramics) | PASS (bulk present). Note MLCC DC-bias derating (§C4) lowers effective Cout — consistent with the ~100 µF the COMP was designed for. |

*Feed-forward:* doc R.4 mentions ~22 pF across R73 (FB Rtop). **Not placed** — the FB net has only R73/R74/U15.14 (MINOR; optional transient aid).

### 3.2 Input eFuse TPS259824ONRGE (SLVSEI3D). Variant decode: **"24" = 16.9 V OVLO, "O" = Circuit Breaker.** Constants: ILIM eq const 1460, offset 0.11 A; GIMON = **246 µA/A**; VUVLO(R)=1.2 V, VUVLO(F)=1.1 V; ΔVITIMER=0.98 V, IITIMER=2.1 µA; ISC(fast-trip)=210 %·ILIM.

| Param | Datasheet eq | Computed | Placed | Result |
|---|---|---|---|---|
| **ILIM** | Eq 4: `R_ILIM = 1460/(I_LIM−0.11)` ⇒ `I_LIM = 1460/R + 0.11` (verified vs table: 100 Ω→14.71 A, 182 Ω→8.13 A, 300 Ω→4.98 A) | 121 Ω → 12.18 A | R49 = **121 Ω** | I_LIM = 1460/121+0.11 = **12.18 A**; fast-trip = 2.1× = **25.6 A**. PASS. **MINOR:** 121 Ω < UL-2367 floor of 182 Ω (electrically valid, range 82–1650 Ω; only affects UL recognition). |
| **ITIMER** | Eq 5: `t[ms] = C[nF]·ΔVITIMER/IITIMER = C·0.98/2.1` | 22 nF → **10.27 ms** | C84 = **22 nF** | **FLAG (MINOR):** actual blanking ≈ **10.3 ms**, not the **178 µs** the doc claims (doc off by ~58×). Functionally safe: the 25.6 A fast-trip catches hard shorts in <400 ns regardless; ITIMER only governs moderate 12–25 A overloads, non-destructive for a 3.2 mΩ eFuse (≈2 W·10 ms). Startup charge (23 kΩ·22 nF≈0.5 ms τ, reaches 2.47 V within tD,ON=2.1 ms) → no false startup fault. If a shorter moderate-overload trip is wanted, use ~1–2.2 nF. |
| **dVdt** | Eq 3: `C_dVdt[pF] = 4600/SR[V/ms]`; Eq 2: `I_inrush[mA] = SR·Cout[µF]` | 150 pF → SR=30.7 V/ms; inrush=30.7·54=1.66 A | C83 = **150 pF** | PASS. SR **30.7 V/ms**, inrush into the 54 µF `VSYS_FUSED` cap = **1.66 A** (« 12.2 A ILIM ⇒ no nuisance trip). (Doc said 1.35 A into 44 µF; actual Cin is 54 µF.) |
| **IMON** | Eq 12: `V_IMON = GIMON·I_OUT·R_IMON`; Table 7-2: VIMON(MAX)=1.8 V @ Vin 3.3 V | 604 Ω → 1.486 V @10 A; 1.81 V @12.2 A | R50 = **604 Ω** | PASS. Normal (≤10 A) = **1.49 V**; at the 12.2 A limit = **1.81 V**, a hair over the 1.8 V guideline (but VIMON(MAX) rises with Vin, and our Vin≥3.7 V normally). MINOR: node not routed to an ADC (A13). |
| **EN/UVLO** | Eq 1: `Vin_uvlo = VUVLO·(RVL1+RVL2)/RVL2` | on 3.00 / off 2.75 V | R52=60.4 k, R45=40.2 k | on = 1.2·100.6/40.2 = **3.00 V**; off = 1.1·2.5025 = **2.75 V**. PASS. Divider current @3.7 V = 36.8 µA ≫ 20× the 0.1 µA EN leakage ✔. |
| **RETRY straps** | Pin desc / Table 7-1: RETRY_DLY→GND = latch-off | latch-off | →GND | PASS — circuit breaker stays off after a fault (contains a JB1.13-class short). |

---

## 4. Section C — Ratings / part-selection

### C1 — External FET AON7534 (30 V N-ch, PQFN-8): **PASS, generous margin**
- VDS=30 V (BVDSS 30 V, VSPIKE 36 V). Off-state VDS ≈ `VSYS_FUSED` ≤ ~5 V (USB) → **~6× margin.** (eFuse OVLO clamps input at 16.9 V worst case — still 1.8× under BVDSS.)
- VGS=±20 V. Gate drive = VCC LDO = 5.2 V (bootstrapped for the HS). Well inside ±20 V; Rds(on) is spec'd at VGS 4.5 V (8.5 mΩ max) so 5.2 V gives ≤8.5 mΩ (typ ~5 mΩ at 10 V). ✔
- ID = 30 A (Tc) / 20 A (Ta) vs ~10 A operating inductor current → conduction loss per FET ≈ (10 A)²·8.5 mΩ·duty ≈ <0.85 W; PQFN thermal pad handles it. ✔
- **Gate-drive strength (the doc's open question):** Qg(4.5 V)=6.8–10 nC. At 1 MHz, gate current = 10 nC·1 MHz = **10 mA/FET**; both FETs ≈ 20 mA vs the VCC LDO's 60 mA capability → **adequate.** ✔
- Verdict: appropriate buck-side FET, well matched to this controller.

### C2 — Inductor XAL7030-152MEC (1.5 µH): **PASS — not marginal**
Ratings (Coilcraft/distributor): **L = 1.5 µH ±20 %, Isat = 23.5 A (30 % drop), Irms = 15 A, DCR = 8.36 mΩ, 7.5×7.5×3.1 mm.** (Note: the doc's "15 A" is the **Irms**, not Isat — it understated the saturation headroom.)
- **Ripple @ 1 MHz:** buck (Vin 5 V): ΔI_L = (Vin−Vout)·(Vout/Vin)/(L·fSW) = 1.7·0.66/1.5 = **0.75 A**; boost (Vin 3.0 V): ΔI_L = Vin·D/(L·fSW), D=(3.3−3.0)/3.3=0.091 → **0.18 A**. Sane (<1 A pk-pk).
- **Peak inductor current:** Artix ~3 A out @3.0 V → I_in≈3.7 A (pk ~3.8 A). KU040 8 A out @3.0 V → I_in = 3.3·8/(0.9·3.0)=**9.8 A** (pk ~9.9 A). Both are **far below Isat 23.5 A** — even a transient hitting the 12.2 A avg / ~14–16 A peak current limit stays under Isat. **Not marginal for KU040.** ✔
- Irms 15 A ≥ KU040 ~10 A RMS ✔; DCR loss at 10 A = 0.84 W (fine, feed with wide copper).

### C3 — Current-limit coverage: **PASS (tight)**
- KU040 worst-case rail input ≈ **9.8 A** (8 A out, sagged 3.0 V cell). eFuse ILIM 12.2 A and buck-boost avg ILIM 12.2 A ⇒ **~1.24× headroom.** Adequate but not generous; there is no room to raise the KU040 draw beyond ~8 A without approaching the limits. Both limits are ~half of the parts' abs-max (eFuse fast-trip 25.6 A; buck-boost 16 A switch / 16.5 A max ILIM), so no abs-max risk.

### C4 — **Cap voltage ratings on VSYS / VSYS_FUSED: FLAG (MAJOR)**
Confirmed placed parts (LCSC lookups):
- `VSYS_FUSED` bulk **C86/C87 = C59461 = 22 µF 6.3 V X5R 0603**; `VSYS`/`VSYS_FUSED` 10 µF **C85/C49 = C15525 = 10 µF 6.3 V X5R 0402**.
- **These nets reach ~5 V when powered from USB** (VSYS = ORed +5 V via LM66100). A 6.3 V MLCC at ~5 V is only **1.26× derated** — well outside the ≥2× best practice, and a 6.3 V X5R at 5 V DC bias **loses ~60–70 % of its capacitance** (54 µF nominal Cin → likely ~20–25 µF effective). Voltage margin is also thin against USB hot-plug ringing on VSYS. **Recommend 16 V (min 10 V) X7R** on all VSYS / VSYS_FUSED caps.

### C5 — +BATT caps: FLAG (MINOR)
**C2/C3/C40/C41 = C140782 = 47 µF 6.3 V X5R 0603**, plus C48=C15525 (10 µF 6.3 V). +BATT max = 4.2 V (4.35 V at charge CV; hot-plug transients higher). 6.3 V = **1.5×** — the bare minimum, and Trenz themselves up-rated the equivalent module input caps 6.3 V→10 V for exactly this reason (ECO §8.9.3). **Recommend 10 V.**

### C6 — **Output voltage vs TE0841 abs-max: FLAG (MAJOR, KU040 build)**
FB = 100 k / 56.2 k sets **Vout(nom) = 3.335 V.** Worst-case tolerance stack (Vref 1.212 V, Rtop +1 %, Rbot −1 %):
`Vout(max) = 1.212·(1 + 101/55.638) = 3.412 V`.
The TE0841 (KU040) **3.3VIN abs-max is 3.4 V** (per the doc's TRM citation). **The worst-case high output (3.41 V) marginally exceeds it.** For the Artix/TE0712 build (higher 3.3VIN tolerance) this is fine; for the KU040 build it removes all margin against a stated abs-max.
**Recommend retargeting to ≤3.30 V nominal** (e.g. R74 = 57.6 k → 3.283 V nom, worst-case high ≈ 3.36 V < 3.4 V) so the whole tolerance band sits under the TE0841 limit.

### C7 — eFuse OVP: FLAG (MINOR, accepted)
TPS259824 OVLO = **16.9 V rising / 16.4 V falling** — i.e. **no overvoltage protection anywhere in the 3–5 V operating range.** A mis-inserted charger or +5 V fault up to 16.9 V passes straight through to U15 (abs-max 36 V, so U15 survives; downstream 3.3 V loads are protected only by U15's regulation). Consciously accepted because the 7.6 V-OVLO sibling (TPS259823) is not JLC-assemblable (doc R.6). Acceptable for a battery/USB drone; note the residual exposure.

### C8 — Voltage ratings not captured in the schematic (process FLAG)
Capacitor symbols store **only capacitance + footprint size** (verified on C87: Value="22u", Footprint="C_0603", no voltage field). Voltage rating is implied solely by the LCSC part number in the (hidden) "LCSC Part" property. The ratings above were recovered by looking those up — but this means **voltage adequacy is invisible on the schematic and easy to regress at BOM time.** Recommend adding an explicit voltage to each power-cap Value (e.g. "22u/16V") and diffing against LCSC before every order.

### C9 — JLC assemblability: PASS (with supply caveat)
Per the doc's 2026-08-16 audit (not re-queried live here — JLC stock is volatile): buck-boost **C2876594 = JLC Extended, ~102 stock, single-source AEC-Q100 → pre-order/watch**; eFuse **C2155766 = Extended, ~106 stock**; inductor **C5352784 = on JLC**; AON7534 **C115844** and 2N7002/LM66100/LM74700 all catalog parts. The buck-boost is the supply-risk item (Extended + thin + single-source).

---

## 5. Section D — Doc (`RESPIN_POWER_RAIL.md` R.4/R.6/R.7) vs schematic drift

| # | Doc says | Schematic has | Note |
|---|---|---|---|
| D1 | Inductor **"L1"** | placed as **L2** | Refdes drift (cosmetic). |
| D2 | eFuse ITIMER 22 nF → **"178 µs"** blanking | 22 nF → **10.3 ms** (Eq 5) | **Doc arithmetic wrong (~58×).** Placed value safe (see B10). |
| D3 | VCC cap **"4.7 µF"** | Value "4.7 µF" but **LCSC = 1 µF 6.3 V** | Value/BOM mismatch + marginal voltage (B7). |
| D4 | FB Rtop feed-forward **"~22 pF"** | **not placed** | Optional; absent. |
| D5 | U15 PG **"100 k pull-up to 3V3 → GPIO"** | **floating** | Telemetry unrealized (A14). |
| D6 | eFuse IMON **"→ ADC"** | node to R50 only, **not routed** | Telemetry unrealized (A13). |
| D7 | EN/UVLO (buck-boost) turn-off **"~2.80 V"** | computed **2.77 V** | Close; within estimate. |
| D8 | Cin **"2×22 µF + 0.1 µF"** | 22+22+10 µF, **no dedicated 0.1 µF HF** | Add an HF 0.1 µF at VIN. |
| D9 | R.7 "MODE 27 k = correct (band 18.9–35.1 kΩ)" | R77 = 27 k ✔ | Confirmed by V_MODE calc (B4). |
| D10 | R.7 "ILIM 27 k → 12.2 A, FSW 19.6 k → 1.0 MHz, FB → 3.34 V" | all confirmed ✔ | Doc R.7 values correct; only ITIMER/VCC drifted. |

*(Everything else in R.4/R.6/R.7 — connection table, eFuse pin table, gate-driver fix, teardown list — matches the schematic.)*

---

## 6. Prioritized fix list

**CRITICAL:** none.

**MAJOR**
1. **C4 — VSYS / VSYS_FUSED cap voltage.** Change C86, C87 (22 µF), C85, C49 (10 µF) and any other VSYS/VSYS_FUSED ceramics from **6.3 V → 16 V** (min 10 V) X7R. Rationale: these nets sit at ~5 V on USB; 6.3 V is 1.26× and loses most capacitance to DC bias, thinning the buck-boost Cin.
2. **C6 — Output set-point vs TE0841.** Retarget FB for the KU040 build: **R74 = 57.6 k** (with R73 = 100 k) → Vout(nom) 3.283 V, worst-case high ≈ 3.36 V, keeping the whole band under the TE0841 3.3VIN abs-max of 3.4 V. (3.335 V is acceptable for Artix-only.)
3. **B7 — VCC cap C79.** Reconcile the value (schematic labels 4.7 µF, BOM part C53067 = 1 µF) **and** raise voltage to **≥16 V** (VCC = 5.2 V; 6.3 V is 1.24×). Fit **4.7 µF / 16 V** X7R.

**MINOR** (next-spin cleanup)
4. **C5** — +BATT caps (C2/C3/C40/C41 47 µF, C48 10 µF): 6.3 V → **10 V**.
5. **A7** — route R74 (FB-bot), R75 (ILIM), R76 (FSW) returns to the **AGND** island, not PGND (reference hygiene).
6. **D8** — add a **0.1 µF HF** ceramic right at U15 VIN (and confirm one at each Cin/Cout cluster).
7. **B10 / D2** — correct the doc's ITIMER figure to **10.3 ms**; decide if that blanking is intended (reduce C84 to ~1–2.2 nF for a faster moderate-overload trip; the hard-short fast-trip is unaffected either way).
8. **A14 / D5** — add U15 PG 100 k pull-up to +3V3 (+ optional GPIO tap) if rail-good telemetry is wanted.
9. **A13 / D6** — route eFuse IMON (with the doc's 10 k + 1 nF RC) to an ADS7128 channel if current telemetry is wanted.
10. **C8** — put explicit voltage ratings in every power-cap Value field; diff BOM vs CPL before ordering.
11. **C9** — pre-order/second-source the buck-boost (C2876594, JLC Extended, thin single-source stock).
12. **B9** — note R49 = 121 Ω is below the UL-2367 floor (182 Ω); irrelevant unless UL recognition is pursued.
13. **C7** — accept the 16.9 V eFuse OVLO (no OV protection 3–5 V), or move to TPS259823 (7.6 V) if it becomes JLC-assemblable.

---

## 7. What was verified NOT to be a problem (previously-feared items now cleared)
- Buck FET gate-driver polarity + Q10 source (prior CRITICAL bug) — **fixed.**
- Old LTC3119 rail — **fully removed, no orphans.**
- AGND/PGND single-point tie (NT1) — **correct.**
- eFuse EN/UVLO divider — **complete** (R52 top is present; not a floating enable).
- Buck FET / inductor ratings — **generous** (6× Vds, 2.4× Isat).
- ILIM / FSW / MODE / FB / EN set-points — **all recompute to intent.**
- Latch-off strapping, dVdt inrush, fast-trip — **correct and safe.**
- JB1.13 root-cause short — **fixed** (now on +3V3).

**UNVERIFIED (stated honestly):** exact loop crossover/phase margin for U15 (average-current-mode plant model / current-sense gain not published — needs a bench load-transient test); live JLC stock (used the doc's 2026-08-16 snapshot); precise MLCC DC-bias derating curves (used typical X5R behavior); TE0841 3.3VIN abs-max taken from the doc's TRM citation (3.4 V).
