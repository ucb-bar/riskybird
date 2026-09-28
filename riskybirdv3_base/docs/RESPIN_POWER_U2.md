# RESPIN — U2 (LTC3119) main-regulator power review + eFuse / high-current plan

**Project:** riskybird v3 (`riskybirdv3.kicad_sch`)
**Scope:** Evidence-backed rating/derating review of the main 3.3 V regulator **U2 = LTC3119** buck-boost and the whole battery/USB front-end, under this platform's expected loads up to a **Kintex UltraScale KU040** SoM, plus a concrete **eFuse + high-current precautions** for the respin.
**Status:** Analysis only — no `.kicad_*` edits, not committed. Companion to `ECO-001_3v3_power_redesign.md` §8–§9 (the U2 failure incident, the JB1.13 short root-cause, the PSU/USB/battery validations, and the §9.8 rework rail fault).
**Date:** 2026-08-15

---

> **⚠️ SCOPE UPDATE (2026-08-16) — U2/LTC3119 is the ARTIX-ONLY shared rail.** Following the split-rail decision, **U2 = LTC3119 is now scoped as the Artix (TE0712) main 3.3 V rail and, on all builds, the shared *housekeeping* rail** (ESP32-C6 + sensors + camera LDOs + headers, ~1 A). It is **sufficient for the Artix XC7A200T (≤3 A) but NOT for the KU040.** The **Kintex UltraScale KU040 (TE0841) gets its own DEDICATED high-current 3.3 V buck-boost** (8 A design target, ~10 A 1S input) — **specced in [`RESPIN_POWER_RAIL.md`](RESPIN_POWER_RAIL.md)** (primary regulator TPS55288RPMR / LCSC C2864583; input eFuse TPS25982 / LCSC C2155683; inductor Sunlord MWSA1204S-1R0MT). That new rail is **populated on KU040 builds and DNP on Artix builds**; U2 keeps housekeeping on every build. The KU040 sizing in §2b / §3 below is retained as the *analysis* that motivated the dedicated rail — but the KU040 5–6 A load is **no longer carried by U2**; it is carried by the RESPIN_POWER_RAIL.md regulator. Read this document for the U2/housekeeping rail; read RESPIN_POWER_RAIL.md for the KU040 rail.

---

## 0. TL;DR verdict

- **The LTC3119 silicon is correctly rated for this job.** Its **8 A typ / 7 A min average inductor-current limit** and **11 A peak** support ~5–6 A of 3.3 V output even on a sagging 1S cell, and the E-grade die stays under its 125 °C TJMAX at 6 A **if** the thermal path is good (θJA = 22 °C/W datasheet). It was **not** under-designed — it died in the field only because it was driving the day-one `JB1.13`→`JM1.14` `+3V3`→GND short on an **unfused battery** (ECO §8.11).
- **Two as-built implementation defects make the KU040 worst case marginal-to-failing:** (1) **L4 = 2.2 µH in an 0603 package** — saturates at ~1–3 A, far below U2's 8–11 A switch currents (co-primary cause of the original melt, ECO §8.4 #2); (2) **U2 on the bottom layer with a sparse EP via array** → real θJA ≈ 40–55 °C/W, which drives Tj over TJMAX at ≥5 A.
- **The ORing/switch FETs are all comfortably over-rated** — Q9 AON7534 (30 A), Q5 AON7407 (40 A / –14.5 A steady-state), LM74700 controller. The **prior worry that Q9/Q5 were under-rated (memory / ECO §6-6) is retired by the datasheets** below.
- **The one genuinely current-limited element is the USB leg (U8 LM66100, 1.5 A)** — fine as a deliberate USB ceiling (USB can't run a KU040 anyway), not a flight-path fault.
- **The platform has NO input current limit on the battery path** — the exact gap that let the JB1.13 short destroy U2. **Add an eFuse / hot-swap limiter on VSYS→VCOM** (§6).
- **Two under-rated passives-of-the-mechanical-kind:** the **Molex PicoBlade battery connectors (1.0 A/contact)** and **20 µF of VCOM input cap** are both undersized for a 7–8 A battery path (§7).
- **Part-number correction:** the KU040 4×5 SoM is the **Trenz TE0841** (KU040/KU035). **"TE0842" does not exist** in the Trenz catalog — it is a mis-recollection of TE0841. All KU040 numbers below are TE0841.

---

## 1. LTC3119 ratings (datasheet-cited)

Source: **Analog Devices LTC3119 datasheet, Rev B (3119fb)** — "18V, 5A Synchronous Buck-Boost DC/DC Converter." As-built part = **LTC3119EUFD#TRPBF**, 28-lead 4×5 mm QFN (UFD), EP pin 29 = PGND (LCSC C580693).

| Parameter | Datasheet value | Citation |
|---|---|---|
| Topology | 4-switch synchronous buck-boost, **average current-mode** control | p1, "Operation" |
| Input voltage range | **2.5 V – 18 V** (abs max VIN/PVIN/PVOUT/RUN/PGOOD −0.3 V to 19 V) | Features p1; Abs Max |
| Output voltage range | **0.8 V – 18 V** (FB ref **0.795 V** typ) | p1; block diag (795 mV) |
| Max continuous output | **5 A** (buck, VIN ≳ 6 V); **3 A at VOUT = 5 V, VIN = 3.6 V** headline | Features p1 ("5V at 3A, VIN=3.6V") |
| **Average inductor-current limit** | **8 A typ, 7 A min** (Note 3) | Elec. Char. "Inductor Current Limit … l 7 8 A" |
| **Peak inductor-current limit** | **11 A typ** → switch A off cycle-by-cycle | p ("peak level of 11A (typical), switch A is immediately turned…") |
| Current-sense transconductance | gm = **10.8 A/V**, VC range 220–960 mV | Compensation section |
| Switching frequency | **400 kHz – 2 MHz**, RT to GND; **RT = 76.8 k → 1.00 MHz** (900/1000/1100 kHz) | Features; Elec. Char. "Oscillator Frequency" |
| **RUN comparator threshold** | **1.205 V typ** (1.17 / 1.205 / 1.24), rising; 90 mV hyst — the accurate UVLO pin | Elec. Char. "RUN Pin Comparator Threshold" |
| **θJA (QFN/UFD)** | **22 °C/W** with EP soldered to PCB; TSSOP/FE = 21 °C/W | "TJMAX=125°C, θJA=22°C/W" (UFD) |
| **TJMAX (E-grade)** | **125 °C** (H-grade parts extend to 150 °C) | Order info / Notes 2–3 |
| Shutdown / quiescent | < 3 µA shutdown; overtemp + output short-circuit protection built in | Features; Note 6 |
| Features present | PGOOD, Burst Mode, MPPC (max-power-point / input-UVLO regulation), SYNC | p1 |

**How the "max output current" derates with VIN (the curve, quantified).** In the boost / 4-switch region (VIN ≤ VOUT = 3.3 V) the average inductor current ≈ the *input* current, so the 8 A (7 A min) average limit caps the output at:

> **I_out,max ≈ I_LIM · η · VIN / VOUT**

| VIN (1S cell) | I_out,max @ 8 A typ, η 0.88 | I_out,max @ **7 A min**, η 0.88 |
|---|---|---|
| 3.0 V (sag) | 6.4 A | **5.6 A** |
| 3.3 V (= VOUT) | 7.0 A | 6.2 A |
| 3.7 V (buck) | ~8 A (buck: I_L ≈ I_out) | ~7 A |
| 4.2 V (buck) | ~8 A | ~7 A |

**Reading:** even at a 3.0 V sagging cell the LTC3119 can source **~5.6 A (min-guaranteed) to ~6.4 A (typ)** at 3.3 V — enough for the KU040's 4 A startup and typical 4–5 A operation, but a heavily-loaded KU040 pulling **6 A on a deeply-sagged cell sits right at the 7 A min-guaranteed limit and will fold back.** Keep the KU040 3.3 V load ≤ 5 A for margin, or prevent the cell sagging below ~3.3 V under load.

### 1.1 As-built passives vs the target current

| Item | As-built (netlist) | Assessment |
|---|---|---|
| RT (R62) | **48.7 k** → f ≈ 1.00 MHz × 76.8/48.7 ≈ **1.55 MHz** | ✅ in the 0.4–2 MHz range; high f keeps the inductor small — good for a drone |
| **L4 (inductor)** | **2.2 µH in `L_0603_1608Metric`** | 🔴 **Value OK, package fatally undersized.** An 0603 power inductor saturates ~1–3 A; U2 runs **up to 8 A avg / 11 A peak** inductor current. Ripple is small (~0.2 A at 1.55 MHz), so 2.2–3.3 µH is fine electrically — but **Isat must be ≥ 11 A and IRMS ≥ 8 A**. This is the co-primary cause of the field melt (ECO §8.4 #2). |
| FB divider | R59 316 k / R60 100 k → 0.795·(1+3.16) = **3.31 V** | ✅ correct |
| RUN/UVLO | R58 100 k / R71 63 k, VCOM·63/163 = 1.205 V → **VCOM = 3.12 V** turn-on | ✅ correct 1S over-discharge cutoff (validated against the real 1.205 V RUN threshold) |
| VCOM input caps | **C62 10 µF + C67 10 µF = 20 µF** ceramic | ⚠️ light for an 8 A-input buck-boost — raise to ≥ 2×22 µF + bulk (§7) |
| +3V3 output caps | C63 150 µF + 1 µF/10 µF/100 nF spread | ✅ adequate bulk |
| MPPC (pin 12) | tied to VCC/SVCC (net `Net-(U2-MPPC)`, C65 4.7 µF) | input-voltage foldback **disabled** — U2 never backs off as the cell sags (ECO §8.4 #3). Consider using MPPC for a soft input-UVLO on the respin. |
| PWM/SYNC (pin 25) | GND → **Burst Mode** | fine for idle efficiency; tie high for fixed-freq PWM if sensor noise matters |
| **PGOOD (pin 7)** | **unconnected** | ⚠️ wasted telemetry — wire to an ESP/FPGA GPIO (§7) |

---

## 2. Expected 3V3 loads

The 3.3 V rail (U2 output) supplies the **entire SoM input power** because the carrier feeds both the module's `VIN` and `3.3VIN` from 3.3 V; the module's on-board DC-DCs step 3.3 V down to core/DDR/IO rails. So **U2 output current = module 3.3 V input current + carrier housekeeping.**

**Carrier housekeeping (measured/estimated):** ESP32-C6 + BMI088 + BMP390 + VL53L1 + ADS7128 + PMW3901 + camera LDOs + PCA9306 ≈ **0.15–0.25 A** total.

### 2a. Artix XC7A200T on Trenz TE0712

- **TRM:** single 3.3 V input, **≥ 3 A available for startup** (config + on-module DC-DC inrush + bulk-cap charge).
- **Measured (ECO §8.12):** short-fixed board, module mated, **0.45 A @ 3.3 V idle** (unconfigured Artix + on-module DC-DC quiescent) — validated on PSU, USB, and battery (ECO §9.5–9.7).
- **Envelope:** idle ~0.45 A; configured light logic ~0.8–1.5 A; heavy fabric/DDR ~2–3 A. **Peak 3.3 V load ≈ 3 A** → U2 output ≈ **3.2 A** worst case.

### 2b. KU040 on Trenz **TE0841** (the worst case)

- **Part verify:** KU040 4×5 SoM = **TE0841** (also carries KU035); confirmed on the Trenz shop + **TE0841 TRM**. **No "TE0842" exists** — treat the user's "TE0842" as TE0841.
- **TE0841 TRM Table 12 ("Typical power consumption")** lists VIN and 3.3VIN as **"TBD" — it is design-dependent**; Trenz directs you to the Xilinx power estimator (XPE)/Vivado. The concrete, cited requirement is:
  > *"Single 3.3 V power supply with **minimum current capability of 4 A for system startup** is recommended."* (TRM §6.1)
- **Rails / pins:** VIN = JM1.1/3/5 + JM2.2/4/6/8 (abs max **6.0 V**, rec 3.3–5.5 V); 3.3VIN = JM1.13/15 (abs max **3.4 V**, rec 3.3–3.4 V — must be a clean 3.3 V). On-module DC-DCs (TPS82085 etc.) generate PL_1V8, DDR_2V5/1V2, PL_GT_1V42/1V2, bank VCCOs.
- **Engineering estimate of operating 3.3 V input current** (module DC-DC η ≈ 0.85–0.90, I = P/(η·3.3)):

  | KU040 design point | FPGA+DDR power | Module 3.3 V input current |
  |---|---|---|
  | Startup surge (Trenz floor) | — | **≥ 4 A** |
  | Light logic + DDR4 idle | ~5 W | ~1.7 A |
  | Moderate fabric + DDR4 active | ~10 W | ~3.4 A |
  | Heavy DSP/high-util + transceivers | ~15 W | ~5.2 A |
  | Aggressive worst case | ~18–20 W | ~6.5–7 A |

- **Design target:** **U2 output = 4 A (startup floor) up to ~5–6 A** for a heavily-loaded KU040 (matches the "≥4 A SoM / 5–6 A target" in the memory/ECO). This is the load that drives the rest of this document.

---

## 3. Load / derating analysis

### 3.1 U2 input current across the 1S range

Buck-boost input current **I_in = 3.3 · I_out / (η · V_in)**, η = 0.90 (consistent with ECO §6; real η 0.85–0.90). This is the current every battery-path element (Q9, Q5, LM74700 sense, the PicoBlade connector, VSYS/VCOM copper, and U2's own switches) must carry:

| 3V3 load (I_out) | Source/case | VIN 4.2 V | VIN 3.7 V | VIN 3.3 V | VIN 3.0 V (sag) |
|---|---|---|---|---|---|
| 0.45 A | Artix idle (measured) | 0.39 A | 0.45 A | 0.50 A | 0.55 A |
| 1.5 A | Artix active | 1.31 A | 1.49 A | 1.67 A | 1.83 A |
| 3.0 A | Artix max / light KU040 | 2.62 A | 2.97 A | 3.33 A | 3.67 A |
| **4.0 A** | **KU040 startup/typ** | 3.49 A | 3.96 A | 4.44 A | 4.89 A |
| **5.0 A** | **KU040 loaded** | 4.37 A | 4.96 A | 5.56 A | 6.11 A |
| **6.0 A** | **KU040 heavy** | 5.24 A | 5.95 A | 6.67 A | **7.33 A** |

**Worst case = battery in flight, low VIN + full load: ~7.3 A at 6 A/3.0 V.** Note the 7.33 A input exceeds the LTC3119 **7 A min** average inductor limit → at that corner U2 is at/over its current limit (see §1 derating).

### 3.2 LTC3119 thermal at the load points

Converter loss P_loss = P_out·(1/η − 1); IC dissipates ~65 % of it (rest = L4 DCR + cap ESR + copper). Tj = Tamb + P_IC·θJA. Two θJA cases: **22 °C/W** (datasheet, EP well-soldered to good copper) vs **~45 °C/W** (realistic as-built: U2 on **B.Cu**, only ~6 EP vias per ECO §8.4 #4). Tamb = 45 °C (warm enclosure).

| Case | I_out | VIN | η | P_out | P_IC (≈0.65·loss) | Tj @ θJA 22 | Tj @ θJA 45 (as-built) |
|---|---|---|---|---|---|---|---|
| Artix active | 1.5 A | 3.7 | 0.90 | 4.95 W | 0.36 W | 53 °C | 61 °C |
| KU040 startup/typ | 4.0 A | 3.5 | 0.87 | 13.2 W | 1.3 W | 74 °C | 104 °C |
| KU040 heavy | 6.0 A | 3.3 | 0.86 | 19.8 W | 2.1 W | **91 °C** ✅ | **140 °C** 🔴 |
| KU040 heavy, sag | 6.0 A | 3.0 | 0.84 | 19.8 W | 2.45 W | 99 °C ✅ | **155 °C** 🔴 |

**Reading:** with the **datasheet θJA (22 °C/W)** the die survives 6 A at Tj ≤ 99 °C — comfortably inside 125 °C. With the **as-built bottom-layer/sparse-via thermal path**, Tj crosses TJMAX at ~5 A and hits thermal shutdown / degradation at 6 A. **The LTC3119 is thermally adequate; the as-built copper is not.** This, plus the saturating L4, is why "runs hotter now" appeared after the field event and why a KU040 would push it over.

### 3.3 Front-end element audit at the 7.3 A worst case

| Element | Ref | Datasheet rating | Rds(on) @ actual gate drive | P @ 7.3 A | Verdict |
|---|---|---|---|---|---|
| Battery ORing N-FET | **Q9 AON7534** | 30 V, **30 A** (23 A @ Tc 100 °C); PQFN 3.3×3.3 | ≤ 8.5 mΩ @ Vgs 4.5 V, ≤ 5 mΩ @ 10 V (LM74700 charge-pump drives it hard) | ~0.37 W | ✅ **huge margin** (retires the "7 A" note in memory) |
| Load switch P-FET | **Q5 AON7407** | −20 V, **−40 A** (−14.5 A steady-state, 1 in²); θJA 60–75 °C/W; DFN 3×3 | ≤ 12.5 mΩ @ Vgs −2.5 V, ≤ 9.5 mΩ @ −4.5 V (gate = full VSYS, so −3 to −4 V) → ~9 mΩ | ~0.48 W → ΔT ≈ 30–35 °C | ✅ **fine** (retires the ECO §6-6 Q5 worry) |
| Battery ideal-diode ctrl | **U13 LM74700** | 3.2–65 V; charge-pump gate ~to 15 V; ~1 µA off | controller only — no main current | ~0 | ✅ |
| **USB ORing ideal diode** | **U8 LM66100** | 5.5 V, **1.5 A**, 79 mΩ, SC-70-6 | carries **0 A on battery**; on USB it hard-limits the USB leg | n/a in flight | ⚠️ **1.5 A USB ceiling** — see below |
| Regulator switches | **U2 LTC3119** | 8 A avg / 11 A peak inductor limit | — | input 7.3 A ≈ 7 A **min** avg limit | ⚠️ **at the current-limit edge at 6 A/3.0 V** (§1) |

**LM66100 (U8) — is the 1.5 A a problem?** No, by design. USB is 5 V; a 6 A/3.3 V KU040 load = ~20 W ≈ **4 A at 5 V**, which no USB-C-with-Rd source and no 1.5 A diode can deliver — the platform *flies on battery*. With USB-primary voltage-OR, the LM66100 supplies first and current-limits/sags near 1.5 A, at which point VSYS falls below the cell and **the battery ORing FET (Q9) picks up the balance** — graceful load-share. **Caveat to flag:** during that handoff the LM66100 is momentarily stressed near its 1.5 A limit if a fat load is applied on USB; treat **USB alone as a config/bring-up/idle source (≤ ~1 A of 3.3 V)**, not a KU040 flight source. (Consistent with ECO §8.8/BU-006: USB's 1.5 A wall can't start a hungry FPGA — that's the ceiling working as intended.)

---

## 4. eFuse / input current-limiter recommendation

**Why (the core lesson of the whole ECO §8–§9 saga):** the battery path has **no series current limit** (Q9 = 30 A FET + a 1S LiPo good for tens of amps + no fuse). On USB the LM66100's 1.5 A ceiling turned the JB1.13 `+3V3`→GND short into a harmless brown-out; on battery the same short fed U2 unlimited current → SW1 melt (ECO §8.11.8). An input current-limit is the single fix that (a) makes battery behave like USB for a downstream short, (b) caps the buck-boost input current at U2's rating, and (c) gives inrush control for hot-plug.

### 4.1 Where it goes

**On `VSYS`→`VCOM`, replacing / merging with the Q5 load switch** — i.e. **downstream of the ORing junction** (so it protects regardless of source) and **immediately upstream of U2's VIN/PVIN** (so it caps U2's input and catches a shorted `+3V3` by starving the battery current U2 can pass into the short). Keep **SW5** gating the eFuse EN so master-off still works (the eFuse replaces the discrete P-FET + gate network, not SW5).

```
+BATT ─Q9(AON7534)─┐                         ┌── eFuse / current-limiter ──┐
                   ├── VSYS ──(SW5 = EN) ─────┤  ILIM≈8A, fast-trip≈12A,    ├── VCOM ── U2 (LTC3119) ── +3V3
+5V ──U8(LM66100)──┘   (ideal-diode ORing)   │  soft-start, OVP, rev-block │            (150µF out)
                                             └── FAULT# → ESP/FPGA GPIO ────┘
```

### 4.2 Sizing

- **Continuous current-limit (ILIM): ~8 A** — above the KU040 flight worst case (~7.3 A at 6 A/3.0 V, §3.1) so it does not nuisance-trip, below destructive.
- **Fast-trip / short threshold: ~12 A** (≈1.5× ILIM) so a hard `+3V3`→GND short (which a 1S cell would drive to tens of amps) trips in µs, before U2's SW1 can melt.
- **Soft-start / inrush timer** long enough to ride the KU040's 4 A startup surge into 150 µF+module caps without tripping (few ms), but short enough to protect on a bolted short.
- **Response to a downstream short:** current-limit-then-latch-off (auto-retry disabled or slow) so a JB1.13-class fault is contained, not cycled.

### 4.3 Concrete parts

**Preferred (full KU040 headroom): low-voltage hot-swap/eFuse controller + external N-FET.** At 6–8 A on a 1S rail an *integrated*-FET eFuse runs hot; a controller driving an external FET (they already stock AON-class 30 A parts) gives arbitrary current headroom and a real thermal pad. Set ILIM with a sense resistor.
- **ADI LTC4231** (2.7–18 V low-voltage hot-swap, ext N-FET, adjustable current limit + inrush timer, reverse-current via the FET body, FAULT/EN) — the cleanest 1S fit.
- Alt: **TI TPS16xx / TPS2492-class** low-voltage hot-swap controllers (verify min-VIN reaches the 3.0 V cell floor before choosing).

**Simpler (integrated, if platform 3.3 V load is capped ≤ 5 A — Artix + modest KU040): single-chip eFuse.**
- **TI TPS1663** — 2.7–60 V, **6 A**, ~26 mΩ integrated FET, **adjustable current limit + fast-trip**, **reverse-current blocking**, **overvoltage clamp**, UVLO, **thermal shutdown**, dvdt soft-start, FAULT#. Covers everything on the feature list in one package; 6 A is adequate for ≤ 5 A steady with margin. Runs warm at 6 A (P ≈ 6²·0.026 ≈ 0.9 W) — needs a decent copper pour.
- **TI TPS25947** (2.7–23 V, 5 A) — same feature set, lower current; only if load stays ≤ 4 A.
- **TI TPS25985 / TPS25990** (2.9–16 V, high-current hot-swap eFuse) — if you want an integrated part that truly does ≥ 8 A; larger, no reverse-block, so pair with the existing ORing.

**Recommendation:** ship the **controller + external N-FET (LTC4231-class), ILIM ≈ 8 A** for KU040 headroom and lowest self-heating; fall back to **TPS1663 (6 A integrated)** only if the design is frozen at ≤ 5 A of 3.3 V (Artix-class). Either way, route **FAULT#/PGOOD to a GPIO** for firmware.

**Secondary (defense in depth):** a small **eFuse or PTC on the `+3V3`→B2B module feed** (a 6 A resettable/eFuse just before JB1/JB2) isolates a *module* short from the rest of the carrier, so a future SoM fault can't drag the ESP/sensors down. Optional if the input eFuse is fitted.

---

## 5. Other high-current precautions

### 5.1 PCB current capacity (battery → VSYS → VCOM → U2 → +3V3)

The whole path carries up to **~7.3–8 A**. For IPC-2221 external traces at ΔT ≈ 20 °C:
- **1 oz Cu:** 8 A needs ~5.0 mm (200 mil). **2 oz Cu:** ~2.5 mm. This board is 4-layer 1.6 mm — inner layers are likely 0.5 oz, so **carry the power path on F.Cu/B.Cu pours, not thin inner traces**, and use continuous polygons, not necked traces.
- **Vias:** a 0.3 mm via carries ~1–2 A. Existing stitching (VCOM 3–4 vias, +3V3 9 vias per ECO §8.3) is thin for 8 A — use **≥ 8–10 vias (0.3 mm+) at each layer transition** on the power path.
- **SW1/SW2 switch nodes:** ECO §8.4 #4 notes SW1 copper necked to 0.3 mm segments — widen; these carry the full 8–11 A inductor current at 1.55 MHz.
- **L4 footprint:** change from 0603 to the chosen 8–11 A power-inductor land (≥ ~5×5 mm, e.g. Coilcraft XAL50xx/60xx or equivalent) with wide copper both sides.

### 5.2 U2 thermal copper (the §3.2 fix)

- **Move U2 to the top layer** or, if it must stay on B.Cu, build a **dense EP via array (≥ 16–25 × 0.3 mm)** into a large ground pour to reach the datasheet θJA ≈ 22 °C/W. Without it, Tj > TJMAX at ≥ 5 A (§3.2).

### 5.3 Connectors / B2B pin current

| Connector | Part | Per-contact rating | Path current | Verdict |
|---|---|---|---|---|
| **Battery / main power** | **Molex PicoBlade 53048 (1.25 mm)** on J3–J6; PinHeader 2.0 mm J7 | **PicoBlade = 1.0 A/contact** | up to **7–8 A** | 🔴 **grossly under-rated** — a single PicoBlade 2-pin cannot carry the KU040 pack current. Use a proper 1S LiPo connector (e.g. XT30 ~15 A, or JST-PH 2.0 mm ~2–3 A only if paralleled), or parallel several contacts. Verify which of J3–J7 is the actual flight-battery inlet and up-size it. |
| SoM power B2B | Samtec LSHM (2.0 A/pin, 2 adj. powered) | 2.0 A/pin | +3V3 on 11 pins (JB1 2/4/6/10/12/14/16 + JB2 1/3/5/7) → ~22 A; GND similar | ✅ plenty (11 × 2 A ≫ 8 A) — keep all +3V3/GND B2B pins populated to share the current |

### 5.4 Current / rail monitoring for flight firmware

- **Wire U2 PGOOD (pin 7)** — currently **unconnected** — to an ESP/FPGA GPIO for rail-health / brown-out detection.
- **Wire the eFuse FAULT#** to a GPIO so firmware sees an over-current/short trip and can log/land.
- **Add inline current sense** on the battery input (e.g. INA226/INA219 + shunt, or a shunt into the existing **ADS7128** ADC) so firmware monitors pack current, catches a growing partial short early (the §9.8 "0.45 A → 0.8 A creep" fault would have been visible), and logs power for the KU040 thermal budget.
- Keep the ESP battery telemetry fix from ECO §3 (`/VBAT_SENSE` split off the buck FB) so cell voltage under load is trustworthy.
- Optional: TVS on VSYS/+BATT to clamp hot-plug transients (U2 abs-max 19 V; cheap insurance, and never hot-plug the SoM onto a live unfused battery again — ECO §8.1).

---

## 6. Prioritized respin action list

| # | Priority | Action | Why |
|---|---|---|---|
| 1 | 🔴 must | **Fix `JB1.13` GND→+3V3** (already tracked, ECO §8.11.7) | eliminates the day-one `+3V3`→GND short that killed the first U2 |
| 2 | 🔴 must | **Replace L4** with a **2.2–3.3 µH power inductor, Isat ≥ 11 A, IRMS ≥ 8 A, DCR ≤ 10 mΩ**, ≥5×5 mm | co-primary cause of the melt; 0603 saturates at ~1–3 A vs U2's 8–11 A |
| 3 | 🔴 must | **Add the input eFuse / current-limiter** on VSYS→VCOM, ILIM ≈ 8 A, fast-trip ≈ 12 A, soft-start, rev-block, OVP, thermal SD, FAULT→GPIO (LTC4231+FET, or TPS1663 for ≤5 A) | protects U2 + FETs from any downstream short; makes battery behave like USB; the missing element behind the whole incident |
| 4 | 🟠 high | **U2 thermal copper:** top-layer or dense EP via array (≥16–25 vias) + large pour; widen SW1/SW2 | keeps Tj < TJMAX at 5–6 A (KU040); as-built θJA fails at ≥5 A |
| 5 | 🟠 high | **Up-size the flight-battery connector** off PicoBlade (1 A) to a 1S LiPo connector rated ≥ 10 A | 7–8 A pack current melts a PicoBlade |
| 6 | 🟡 med | **Widen the battery→VSYS→VCOM→+3V3 power path** to pours + ≥8–10 stitching vias per transition | 8 A copper capacity |
| 7 | 🟡 med | **Raise VCOM input cap** to ≥ 2×22 µF + bulk | 20 µF is light for 8 A input ripple |
| 8 | 🟡 med | **Wire PGOOD + eFuse FAULT + add inline current sense** (INA226 or ADS7128 shunt) | rail-health & over-current telemetry for flight firmware |
| 9 | 🟢 low | Consider **MPPC-based input UVLO** instead of tying MPPC to VCC; tie PWM/SYNC high if sensor noise matters | soft foldback as the cell sags; predictable switching spectrum |
| 10 | 🟢 low | Keep the **USB leg as config/idle only** (LM66100 1.5 A) — document it; up-size only if USB-powered heavy operation is ever needed | it is a deliberate ceiling, not a fault |

**Net:** items 1–3 are the mandatory trio (short fix + real inductor + input eFuse). With those plus the U2 thermal-copper fix (4) and the battery-connector fix (5), the LTC3119 front-end is sound for the KU040/TE0841 worst case; the silicon was never the problem.

---

## 7. References

- **Analog Devices LTC3119 datasheet, Rev B (3119fb)** — 18 V/5 A synchronous buck-boost: 2.5–18 V VIN, 0.8–18 V VOUT, FB 0.795 V, **avg inductor limit 7 A min/8 A typ, peak 11 A**, RUN threshold 1.205 V, RT 76.8 k→1 MHz, **θJA 22 °C/W (UFD QFN), TJMAX 125 °C (E)**, "5 V at 3 A, VIN 3.6 V." https://www.analog.com/media/en/technical-documentation/data-sheets/3119fb.pdf
- **Trenz TE0841 TRM (REV02, v.69)** — KU035/KU040 4×5 SoM: single 3.3 V supply, **"minimum current capability of 4 A for system startup,"** typical power "TBD/design-dependent (use Xilinx XPE)"; VIN abs-max 6.0 V, 3.3VIN abs-max 3.4 V; pins JM1.1/3/5+JM2.2/4/6/8 (VIN), JM1.13/15 (3.3VIN). https://www.trenz-electronic.de/trenzdownloads/Trenz_Electronic/Modules_and_Module_Carriers/4x5/TE0841/REV02/Documents/TRM-TE0841-02.pdf — **"TE0842" is not a Trenz part; KU040 = TE0841.**
- **Trenz TE0712 TRM** — Artix-7 XC7A200T 4×5 SoM: single 3.3 V, ≥ 3 A startup; measured 0.45 A idle (ECO §8.12, §9.5–9.7).
- **Alpha & Omega AON7534** — 30 V N-ch, 30 A (23 A @ Tc 100 °C), Rds(on) ≤ 8.5 mΩ @ Vgs 4.5 V / ≤ 5 mΩ @ 10 V; PQFN 3.3×3.3 (Q9, battery ORing). https://www.aosmd.com/sites/default/files/res/datasheets/AON7534.pdf
- **Alpha & Omega AON7407** — −20 V P-ch, −40 A (−14.5 A steady-state), Rds(on) ≤ 12.5 mΩ @ Vgs −2.5 V / ≤ 9.5 mΩ @ −4.5 V, θJA 60–75 °C/W, θJC 3.5–4.2 °C/W (Q5, load switch). https://www.aosmd.com/res/datasheets/AON7407.pdf
- **TI LM74700-Q1** — ideal-diode controller, 3.2–65 V, charge-pump gate drive to ~15 V, 20 mV forward, ~1 µA off (U13, drives Q9). https://www.ti.com/lit/ds/symlink/lm74700-q1.pdf
- **TI LM66100** — 5.5 V, 1.5 A, 79 mΩ ideal diode, SC-70-6 (U8, USB leg ceiling).
- **TI TPS1663 / TPS25947 / TPS25985** and **ADI LTC4231** — eFuse / low-voltage hot-swap candidates (§4.3).
- **Molex PicoBlade 53048** — 1.25 mm, **1.0 A/contact** (the under-rated battery connector, §5.3).
- Companion: `ECO-001_3v3_power_redesign.md` §8 (U2 failure + JB1.13 root cause), §9.5–9.8 (PSU/USB/battery validation + the §9.8 growing rail fault).
