# RESPIN — "OFF isn't OFF": residual/brownout voltage on +3V3 and the LDO rails with the master switch OFF

**Project:** riskybird v3 (`riskybirdv3.kicad_sch`)
**Status:** Root-cause analysis + prioritized respin proposal (analysis only — **no `.kicad_*` edits made**).
**Date:** 2026-08-15
**Scope of THIS document:** the *master-switch-OFF residual voltage* bug **only**. This is a **different** fault from the JB1.13↔JM1.14 B2B short and the U2 death documented in `ECO-001_3v3_power_redesign.md` §8–§9 — those are unrelated and already resolved. Do not conflate them.

All net/pin/part claims below are from a `kicad-cli` **kicadxml netlist export** of `riskybirdv3.kicad_sch`, cross-checked against the AON7407, AON7534, LM66100, LM74700, TLV700xx and ADS7128 datasheets (citations in §7).

---

## 0. TL;DR

- **The master switch is `SW5`** (the docs call it "SW3" — that refdes does not exist in the netlist; the actual part is `SW5`). It is a low-current SPST that pulls **`Q5` (AON7407 P-FET) gate to GND** to turn the high-side load switch **ON**. `SW5` gates **only** the node `VCOM` (the LTC3119 `U2` input) → `+3V3`. Everything on **`+BATT` / `VSYS` / `+5V` stays live by design** whenever a battery or USB is attached.
- **`Q5` is oriented correctly and does NOT leak.** Its body diode (anode=drain=`VCOM`, cathode=source=`VSYS`) is reverse-biased for `VSYS`>`VCOM`, so with `SW5` OFF, `VCOM` is held at GND by `R47‖R48` (100 kΩ to GND) and `U2`/`+3V3` get **no** power from the front end. The residual on `+3V3` is **not** a forward leak through the switch — it is a **back-feed INTO `+3V3` from an always-live node.**
- **PRIMARY ROOT CAUSE — the battery-sense divider back-feeds `+3V3` through an ADC input clamp.** `R30` (200 kΩ) ties **`+BATT` directly to `ADS7128` (U1) pin 4 = `AIN5/GPIO5`**. `U1`'s analog-input ESD/OVP structure clamps to its supply `AVDD = +3V3` (abs-max AINx = `AVDD + 0.3 V`). With `+3V3` off, `+BATT` (≈4.2 V) forward-biases that clamp diode and **injects ~5–18 µA into the `+3V3` rail**, floating it to a "brownout" voltage. **Present whenever the battery is connected — independent of SW5 and USB.**
- **The other LDO rails are FOLLOWERS, not independent leaks.** `U7`(+2V8), `U10`(+1V8), `U11`(+1V5) have **both IN and EN tied to `+3V3`**. TLV700 EN enables above **0.9 V**; once the `+3V3` residual crosses ~0.9 V the LDOs enable and pass their (also-residual) `+3V3` input through to `+2V8/+1V8/+1V5`. Kill the `+3V3` residual and these vanish.
- **SECONDARY path (USB only):** the ESP32-C6 (`U14`) USB D+/D− pins (17/18) back-power `+3V3` through the ESP's ESD clamps when VBUS (`+5V`) is present. Weak vs. R30; only with USB plugged.
- **Fix (cheap, targeted):** the sense divider is **gated on the wrong end** — `Q6` grounds the *bottom* (good for standby-current) but the *top* resistor `R30` still bridges `+BATT` to the ADC pin. **Gate the divider at the TOP** (high-side switch on `+BATT`, bottom permanently to GND). One small P-FET removes the leak and keeps identical telemetry. Details in §5.

---

## 1. Corrected power topology (what is live vs. dead with SW5 OFF)

From the netlist (front-end nets `+BATT`, `VSYS`, `VCOM`, `+5V`, `+3V3`):

```
                 ideal-diode ORing (USB-primary)                 master switch (SW5)
 +5V (VBUS,J8) ─► U8 LM66100 ──┐                                   gates Q5 gate net
                 (VIN1→VOUT)   ├─► VSYS ─► Q5 (AON7407 P-FET) ─► VCOM ─► U2 (LTC3119) ─► +3V3
 +BATT ─► Q9 AON7534 ──────────┘         source=VSYS  drain=VCOM        buck-boost        │
          (body diode +BATT→VSYS;        (SW5 OFF ⇒ Q5 OFF)                                ├─► ESP U14, sensors U1/U3/U5/U9/IC1
           gate = U13 LM74700)                                                            ├─► U7 IN+EN → +2V8 (camera AVDD)
                                                                                          ├─► U10 IN+EN → +1V8 (camera IOVDD + JB2 VCCIO + U12 VREF1)
 +BATT ─► U4 MCP73831 charger (VDD=+5V) — stays live to charge with SW5 OFF               ├─► U11 IN+EN → +1V5 (camera DVDD)
 +BATT ─► R30 200k ─► U1.AIN5  ◄── ⚠ THE LEAK (see §2)                                     └─► U12 PCA9306 VREF2, headers, B2B
```

| Node | State with **SW5 OFF** | Why |
|---|---|---|
| `+BATT` | **LIVE** (≈3.0–4.2 V) | battery `BATT1` is hard-wired to `+BATT`; SW5 is downstream. |
| `VSYS` | **LIVE** | `Q9` body diode conducts `+BATT`→`VSYS` unconditionally (N-FET, source=`+BATT`, drain=`VSYS`); `U13` also drives it fully on when powered. USB adds `+5V`→`VSYS` via `U8`. |
| `+5V` | **LIVE** iff USB plugged | `J8` VBUS. |
| `VCOM` | **DEAD (≈0 V)** | `Q5` OFF and its body diode blocks `VSYS`→`VCOM`; `R47‖R48` = 100 kΩ pulls `VCOM` to GND. |
| `+3V3` | **should be 0 V — but floats to a residual** | back-fed by `R30` (§2) and, with USB, by the ESP USB clamps (§3). |
| `+2V8/+1V8/+1V5` | **follow `+3V3`** | LDO IN **and** EN both on `+3V3` (§2.3). |

**Master-switch mechanics (verified):** `SW5` pin1 = `Net-(Q5-G)`, pin2 = `GND`; `R45` (100 kΩ) from `Net-(Q5-G)` to `VSYS`. SW5 OFF → gate pulled to `VSYS` → V_GS(Q5)=0 → P-FET OFF. SW5 ON → gate to GND → V_GS≈−V_SYS → ON. `AON7407` V_GS(th) = −0.3/−0.55/−0.9 V (min/typ/max).

---

## 2. PRIMARY root cause — R30 battery-sense divider injects into +3V3

### 2.1 The exact path (netlist evidence)

```
+BATT ──[ R30 200kΩ ]── /GPIO5 ──[ R31 100kΩ ]── Net-(Q6-D) ── Q6 drain (2N7002)
                          │                                     Q6 source = GND
                          └── U1 pin 4  (ADS7128 "AIN5/GPIO5")  Q6 gate  = VCOM
```

Netlist nodes:
- `R30` (200 K): pin1 = `+BATT`, pin2 = `/GPIO5`.
- Net `/GPIO5`: `R30.2`, `R31.1`, **`U1` pin 4 (`AIN5/GPIO5`)**.
- `R31` (100 K): pin1 = `/GPIO5`, pin2 = `Net-(Q6-D)`.
- `Q6` (2N7002): pin3(drain)=`Net-(Q6-D)`, pin2(source)=`GND`, pin1(gate)=**`VCOM`**.
- `U1` (ADS7128) supplies: pin7 `AVDD` = `+3V3`, pin10 `DVDD` = `+3V3`.

This is a **battery-voltage telemetry divider** read by the `ADS7128` I²C ADC (the ESP reads V_BATT over I²C). The designer cleverly gated it to draw **zero standby current** — `Q6`'s gate is on `VCOM` (the switched rail), so when the system is ON `Q6` grounds the divider bottom, and when OFF `Q6` opens the bottom so no DC flows top-to-bottom. **Good intent — but gated on the WRONG end.**

### 2.2 Why it leaks (the ADC input clamp)

With `SW5` OFF: `VCOM`=0 → `Q6` OFF → the divider *bottom* floats. But the *top* resistor `R30` still ties **`+BATT` (live) → `U1.AIN5`**. `AVDD` (=`+3V3`) is off (≈0 V). Per the **ADS7128 datasheet, Abs-Max: `AINx/GPIOx = GND−0.3 V to AVDD+0.3 V`** — i.e. every analog input has an ESD/OVP clamp diode to `AVDD`. Driving `AIN5` toward `+BATT` forward-biases the **AIN5→AVDD** clamp, so current flows:

```
+BATT → R30 (200k) → U1.AIN5 → [AIN5→AVDD ESD clamp diode] → AVDD = +3V3 rail
```

**This back-powers the entire `+3V3` rail through U1's supply pin.** The rail floats up until the injected current equals the aggregate off-state leakage of everything on `+3V3`.

Injection current: `I ≈ (V_BATT − V_clamp − V_3V3) / 200 kΩ`, `V_clamp ≈ 0.4–0.6 V`.

| Condition | I into +3V3 |
|---|---|
| V_BATT=4.0 V, V_3V3=1.0 V | ≈ **13 µA** |
| V_BATT=4.0 V, V_3V3=2.5 V | ≈ **5 µA** |
| V_BATT=4.2 V, V_3V3≈0 (ceiling) | ≈ **18 µA** |

Because the observed symptom shows the *downstream LDO rails* also lit, the `+3V3` residual is landing **above ~0.9 V** (see §2.3) — i.e. the rail's aggregate leakage is low enough that R30 floats it well up (potentially toward `V_BATT − 0.5 ≈ 3.5 V` at very light leakage). **This is the brownout.**

**This path is active whenever a battery (or the charger's VBAT output) is present — regardless of SW5 or USB.** It is the storage-drain path and the dominant residual source.

### 2.3 Why the OTHER LDO rails also show residual (they follow +3V3)

`U7`(TLV70028→+2V8), `U10`(TLV70018→+1V8), `U11`(TLV70015→+1V5) each have **IN (pin1) AND EN (pin3) tied to `+3V3`** (netlist confirmed). Per the **TLV700 datasheet, EN is active-high and enables the regulator above 0.9 V** (shutdown below 0.4 V). So:

- If the `+3V3` residual **> ~0.9 V**, the EN pins (hardwired to `+3V3`) satisfy the enable threshold → each LDO turns on and passes its (also-residual) `+3V3` input through to its output (in regulation or dropout). Result: `+2V8/+1V8/+1V5` appear at ~min(target, residual−dropout).
- Below ~0.9 V the LDOs stay in shutdown and their outputs are truly 0 (the TLV700 PMOS pass-diode is oriented OUT→IN, so a disabled LDO does **not** pass IN→OUT).

So the camera rails are **pure followers of the `+3V3` residual**, not independent leak sources. `+1V8` additionally fans this residual out to **`JB2` (FPGA VCCIO pins)** and **`U12` PCA9306 VREF1** — biasing the FPGA bank supply and the level-shifter reference too. **Fixing `+3V3` fixes all of them.**

---

## 3. SECONDARY path — USB back-power through the ESP32-C6 USB pins (USB plugged only)

Netlist: `/USB_DP` = `U14.18` (ESP `GPIO13/USB_D+`) + `U6.6`; `/USB_DN` = `U14.17` (ESP `GPIO12/USB_D−`) + `U6.4`. `U6` = `USBLC6-2P6` ESD array powered from `+5V` (VBUS, pin5); the USB-C connector `J8` is on `/CONN_DP/DN`.

When USB is plugged with `+3V3` off: VBUS energizes `U6`, and the ESP's D+/D− pins have internal ESD clamps to `VDD3P3` (=`+3V3`). Any bias on the data lines above `+3V3+0.3` injects into `+3V3`. At idle the host holds the lines low (its 15 kΩ pulldowns) and the ESP's own pull-up is unpowered, so this path is **weak/transient** compared to R30 — but it is real and it is a known ESP32-family "USB back-power" behavior. It only matters in the "off but charging over USB" scenario, where R30 is *also* active and dominates.

`U6`'s own steering diodes clamp the data lines to `+5V`/GND (not to `+3V3`), so `U6` itself does not inject into `+3V3`.

---

## 4. Leakage-path audit — every candidate the brief listed, evaluated

| # | Candidate path | Verdict | Evidence |
|---|---|---|---|
| (a) | **Q5 (AON7407) body diode** VSYS→VCOM | **BLOCKS — no leak** | P-FET, source=`VSYS`, drain=`VCOM`; body diode anode=drain(`VCOM`)/cathode=source(`VSYS`); reverse for VSYS>VCOM. `VCOM` held at GND by `R47‖R48`. |
| (a) | **Q9 (AON7534) body diode** +BATT→VSYS | conducts **by design** (upstream of SW5) | N-FET ideal-diode pass; keeps `VSYS` live — expected, not the bug. Does not reach the off `+3V3` domain (Q5 blocks). |
| (a) | Q6/Q7 (2N7002) | no leak | gates on `VCOM` (=0 when off) → both OFF. `Q7`→`D4` power LED (`+BATT`→R12→D4→Q7→GND) is dark when off; `Q6`→`R31` is the sense divider bottom (§2). |
| (b) | **Ideal-diode ORing off-state** (U8 LM66100 / U13 LM74700) | no *new* path into off domain | `U8` blocks when USB absent; `U13`/`Q9` keep `VSYS` live but `VSYS` is upstream of the master gate. Neither touches `+3V3`. |
| (c) | **USB VBUS back-feed** not gated by SW5 | **minor path** (ESP USB ESD) | §3. `+5V`→`U8`→`VSYS` and `+5V`→`U4` charger are by-design and don't reach `+3V3`. |
| (d) | **IO-pin back-powering from an always-powered driver** | **PRIMARY = R30→U1 clamp**; ESP-USB = secondary | §2, §3. Systematic netlist scan for any 2-leg passive bridging {`+BATT`,`+5V`,`VSYS`} to a `+3V3`-domain device pin returned exactly one hit: **`R30` (+BATT → U1.AIN5)**. Device-to-device scan returned only the two USB nets. |
| (e) | **LDO EN-vs-IN** (LDO input live but should be disabled) | **followers, not a source** | U7/U10/U11 IN+EN both on `+3V3`; they mirror the `+3V3` residual (§2.3). No LDO has its IN on an always-live rail. |
| (f) | **High-value dividers/pull-ups bridging a live node to an off rail** | **R30 is the one** | Every `+3V3` pull-up's far end is a signal or an open-drain/high-Z pin (e.g. `U4` STAT is open-drain = sink only). No `+3V3` pull-up is sourced by an always-live driver. `/VBAT` (`JB1.80`) is a lone floating module pin. |

**Net result: two paths energize the off `+3V3` domain — R30 (dominant, battery-present) and the ESP USB ESD (minor, USB-present). The LDO rails are downstream followers.**

---

## 5. Risk assessment (per rail)

| Rail | Residual mechanism | Magnitude | Risk class |
|---|---|---|---|
| **`+3V3`** | R30 injection (+ ESP-USB when plugged) | floats to an undefined ~0.5–3.5 V at ~5–18 µA | **Indeterminate IC bias + storage drain.** The rail partially biases the ESP, all sensors, the ADS7128, PCA9306 and the FPGA VCCIO banks. Sub-threshold rail → undefined logic states, possible I²C contention, and a non-zero starting point on the next clean power-up (soft-start/POR reliability). |
| **`+2V8/+1V8/+1V5`** | follow `+3V3` via LDO EN>0.9 V | ≈ min(target, +3V3_resid − dropout) | Consequential only. Biases camera + FPGA VCCIO(`+1V8`→JB2) + PCA9306 VREF. Resolves when `+3V3` is fixed. |
| **`U1.AIN5` pin** | continuously clamped at `AVDD+~0.5 V` by R30 | ~5–18 µA through the ESD diode | **Reliability:** continuously forward-biasing an input ESD/OVP clamp is *within* abs-max (`AVDD+0.3` momentarily) but is **not a rated steady-state operating mode** → long-term latch-up/degradation exposure on U1. |
| **`+BATT` storage drain** | R30 path, battery attached, SW5 off | **~5–18 µA continuous ≈ 0.12–0.44 mAh/day ≈ 4–13 mAh/month** | **Defeats "true off."** This is **~4–14× the documented ~1.32 µA standby target** (ECO §2/§6) and it lands on the *logic* rail. Not a fast battery-killer on a ~300 mAh cell, but it is real parasitic drain plus the correctness problem that "off" leaves the system half-biased. |
| **USB-plugged case** | ESP USB ESD into `+3V3` | small/transient | Functional/indeterminate only (USB is sourcing anyway); a known ESP USB back-power behavior. Lowest concern. |

**Bottom line on severity:** the immediate hazard is **correctness / indeterminate state** (the board is never truly off while a battery is attached, and the logic domain sits at an undefined voltage), with a **secondary slow storage-drain** and a **latent U1 reliability** concern. It is not an acute fire/latch hazard, but it must be fixed for "off" to mean off and for clean, repeatable power sequencing.

---

## 6. Prioritized respin solutions

### S1 — [PRIMARY FIX] Gate the battery-sense divider at the TOP, not the bottom
The divider's intent (zero standby current via `Q6`) is correct but `Q6` is on the wrong end: grounding the *bottom* still leaves `R30` tying `+BATT` to the ADC pin. **Move the gate to the high side:**

- **Add a P-FET (small SOT-23/DFN) high-side switch between `+BATT` and `R30`**, driven ON by a "system-on" signal (the same `VCOM`-derived condition that turns `Q6` on today — e.g. an inverter/level network off `VCOM`, or a spare master-gated enable). Make the divider bottom (`R31`, 100 kΩ) **permanent to GND**.
  - **OFF:** high-side P-FET open → `R30` top floats → `U1.AIN5` pulled to GND through `R31` → **no clamp injection, AIN5 = 0 V, zero branch drain.**
  - **ON:** identical 200 k/100 k ratio → battery telemetry unchanged.
- **Alternative (fewer new parts):** relocate the *existing* gating device from the divider bottom to the top and add the appropriate polarity device; net add ≈ 1 FET + 1 gate resistor.
- **Do NOT** try to fix this with a series diode in the AIN line (corrupts the analog reading) or by simply increasing `R30` (only scales the leak, doesn't remove the DC path to the clamp).
- **Board-level bodge for existing units:** lift `R30` and re-feed its top from a switched node (or insert a small P-FET), so `+BATT` no longer reaches `U1.AIN5` when off.

**This single change removes the dominant leak and restores the ~µA standby the architecture was designed for.**

### S2 — [ARCHITECTURAL / defense-in-depth] Enforce the "no always-live node bridges into an off domain" invariant
`SW5`→`Q5` can only ever gate the *downstream* (`VCOM`→`+3V3`) domain; `+BATT`/`VSYS`/`+5V` must stay live (the charger has to charge with the switch off; the ORing feeds `VSYS`). So "off" cannot be enforced by killing those rails — it must be enforced by the rule that **every sense tap, detect divider, pull-up, or signal that touches an always-live rail on one end and an off-domain IC pin on the other is either master-gated or reverse-blocked.** Today `R30` is the *only* violator; keep this invariant as future battery/USB/current-sense taps and B2B connections are added. Consider distributing a clean master-gated `SYS_ON` rail specifically to gate such taps.

### S3 — [ROBUSTNESS] Decouple the camera-LDO EN pins from the +3V3 input
`U7/U10/U11` EN are hard-tied to their own `+3V3` input, so they (a) cannot be independently disabled and (b) auto-enable on any `+3V3` residual > 0.9 V. Once S1 removes the residual this is moot, but for margin, drive the EN pins from a supervised enable (an ESP GPIO or the `U2` power-good, `U2` pin7 `PGOOD` is currently unconnected) so a floating/residual `+3V3` can never partially enable the camera rails or the FPGA VCCIO(`+1V8`). Low priority.

### S4 — [USB] Mitigate ESP USB back-power (only if "off while charging" must be clean)
Options, low priority: accept it (small/transient); or provide an *intentional* VBUS→ESP power path so the ESP is deliberately powered for charge/DFU rather than parasitically biased; or add a properly-sequenced load switch on the ESP `+3V3`. Not required to resolve the reported symptom (R30 dominates), but note it for the "truly dead while charging" goal.

### S5 — [SYNERGY] Master eFuse with a real shutdown pin
The ECO §8.5/§9.7 already calls for an input current-limit/eFuse on `VSYS`/`VCOM` (to protect `U2`). If that eFuse has a logic disconnect, let `SW5` drive it so the master command hard-disconnects the whole downstream domain (cleaner than gating a discrete P-FET). **Note this does NOT fix R30** — R30 taps `+BATT` upstream of any such eFuse and must be fixed by S1 regardless.

### Bench verification after the fix
1. `SW5` OFF, **battery connected, USB unplugged** → measure `+3V3`, `+2V8`, `+1V8`, `+1V5`: expect **~0 V**; measure `+BATT` draw: expect **~1–2 µA** (down from ~5–18 µA).
2. Repeat with **USB plugged (charging)** → `+3V3` domain should stay near 0 V apart from any small residual USB-ESD bias on the ESP pins; confirm the charger still terminates.
3. `SW5` ON → confirm `+3V3` = 3.3 V and battery telemetry (`U1.AIN5` = V_BATT × 100 k/300 k ≈ 1.0–1.4 V) reads correctly.

---

## 7. References (with the specific numbers used)

- **AON7407** (Q5) — AOS datasheet: **20 V P-Channel** MOSFET; V_GS(th) = −0.3 / −0.55 / −0.9 V; body-diode V_SD = −0.52 V typ (−1 V max) @ I_S=−1 A. → source=`VSYS`/drain=`VCOM` body diode blocks `VSYS`→`VCOM`. (aosmd.com AON7407.pdf)
- **AON7534** (Q9) — N-Channel MOSFET used as the battery ideal-diode pass; body diode conducts source(`+BATT`)→drain(`VSYS`).
- **LM66100** (U8) — TI 1.5 A ideal-diode/load switch; USB(`+5V`)→`VSYS`, reverse-blocks when USB absent. ST (pin5) NC.
- **LM74700** (U13) — TI ideal-diode controller; ANODE=`+BATT`, CATHODE=`VSYS`, drives `Q9` gate.
- **TLV70018 / TLV70028 / TLV70015** (U10/U7/U11) — TI TLV700 datasheet: **EN active-high, device ON above 0.9 V, OFF below 0.4 V**; V_EN abs-max = V_IN+0.3 V. IN and EN both on `+3V3` in this design. (mikroe/TI TLV700_datasheet.pdf)
- **ADS7128** (U1) — TI datasheet: AVDD 2.35–5.5 V; **Abs-Max analog input `AINx/GPIOx = GND−0.3 V to AVDD+0.3 V`** (pins 1,2,3,4,5,6,15,16) ⇒ input ESD/OVP clamp to AVDD(=`+3V3`); `AIN5/GPIO5` = **pin 4**. (ti.com/lit/ds/symlink/ads7128.pdf)
- **ESP32-C6-MINI-1** (U14) — USB D+/D− = `GPIO13`/`GPIO12` (pins 18/17); internal ESD clamps to VDD3P3(=`+3V3`).
- Netlist source: `kicad-cli sch export netlist --format kicadxml riskybirdv3.kicad_sch` (2026-08-15).

---

### Appendix — corrected refdes/terminology vs. the ECO/memory notes
- **Master switch = `SW5`** (not "SW3"; no `SW3` in the netlist). `SW5` pin1→`Net-(Q5-G)`, pin2→GND.
- Master P-FET load switch = **`Q5` (AON7407)**, gate pull-up `R45` (100 kΩ) to `VSYS`.
- Battery ideal-diode = **`Q9` (AON7534)** + **`U13` (LM74700)**; USB ideal-diode = **`U8` (LM66100)**.
- Battery-sense divider = **`R30` (200 kΩ) / `R31` (100 kΩ)**, gated by **`Q6` (2N7002)** at the bottom, read by **`U1` (ADS7128) AIN5/pin4** — *this is the leak; the memory's "R51 200k/R52 100k → ESP ADC" refers to the pre-redesign net names.*
