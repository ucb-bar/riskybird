# riskybird v3 — MASTER RESPIN CHECKLIST

**Project:** riskybird v3 (`riskybirdv3_base/` drone board + `riskybirdv3_connector/` programmer board)
**Purpose:** Top-level, deduped, prioritized planning doc for the next board spin. Aggregates every actionable item from the whole bring-up record (ECO-001 §1–§9, BRINGUP_DEBUG BU-001…BU-010, OFF-BOM, HIERARCHY_PLAN, and the redesign/FPGA-bringup memories).
**Status:** All required fixes proven on hardware where noted — this is the go-signal spin. **The FPGA (XC7A200T) is JTAG-alive on all 3 power sources (PSU/USB/battery)**; the outstanding work is folding the hardware bodges back into the source and closing the leftover decisions/GUI tasks.
**Author:** aggregated for Dima Nikiforov · **Date:** 2026-08-14

> **Convention:** each item = **[PRIORITY]** one-line action — one-line rationale *(source)*.
> Priorities: **CRITICAL** = board won't work / destroys parts without it; **IMPORTANT** = correctness, robustness, or a decision that gates layout/BOM; **NICE** = optional hardening / cleanup.

---

## 0. Covered in detail by (the 4 parallel deep-dive reports)

Reference these for depth; this checklist only points at their topics with one line each.

- **`RESPIN_FPGA_PINS.md`** — FPGA SoM B2B pin/bank audit (JB1.13 fix re-verify, VCCIO/bank-voltage audit, power-good/3-state handshake, config/JTAG straps, full collision re-scan).
- **`RESPIN_PROGRAMMER.md`** — programmer board (`riskybirdv3_connector`): FT4232H symbol/BOM correction, VPLL/VPHY filtering, USB series-R, un-bodged channels, dual-JTAG.
- **`RESPIN_POWER_U2.md`** — U2/LTC3119 ratings & load budget, L4 inductor sizing, the housekeeping-rail eFuse/input current-limit, real 3V3 current reconciliation. **(Scope: U2 = Artix-only + shared housekeeping rail; see the 2026-08-16 note at its top.)**
- **`RESPIN_POWER_RAIL.md`** — **NEW dedicated high-current 3.3 V rail for the KU040 (TE0841)**: split-rail decision, KU040 8 A budget (~10 A 1S input), regulator TPS55288 (LCSC C2864583) + eFuse TPS25982 (C2155683) + inductor MWSA1204S-1R0MT, thermal & layout tables, DNP/assembly-variant strategy.
- **`RESPIN_POWER_BROWNOUT.md`** — off-state (switch-OFF) back-feed / brownout paths (BU-004) and true-zero-shutdown options.

---

## 1. CRITICAL PATH — the must-do list at a glance

| # | Subsystem | Action | Source |
|---|---|---|---|
| C1 | FPGA/SoM | Reassign carrier **`JB1` pin 13: GND → +3V3** (one net label) | ECO §8.11.7 |
| C2 | Power | Add **input current-limit / eFuse** on VSYS/VCOM ahead of U2 (housekeeping rail) | ECO §8.5-A6c, §9.8 |
| C2b | Power | **KU040 builds only:** add the **dedicated high-current 3.3 V rail** (TPS55288 + TPS25982 eFuse + MWSA1204S inductor), split from U2, DNP on Artix | RESPIN_POWER_RAIL.md |
| C3 | Programmer | Fix FTDI symbol/BOM **FT2232HL → FT4232H** + re-verify non-ch-A nets | ECO §9.5 |
| C4 | Programmer | **VPLL** (pin 9): 1 kΩ+no-cap → 600R ferrite + 4.7 µF/100 nF | ECO §9.2–9.3 |
| C5 | Programmer | **D+/D− series 22 Ω (R5/R8) → 0 Ω** | ECO §9.2–9.3 |
| C6 | Sensors/BOM | **IC1 PMW3901**: clear PCB DNP + pin `LCSC C43496881`/MPN so JLC places it | BU-001 / E1 |
| C7 | Camera | Refresh **HM01B0 receptacle footprint** in PCB (geometry + re-route + pin-1 mate check) | redesign mem |
| C8 | BOM | Pin `LCSC Part` on all fixed-Vout **TLV700 LDOs** (U7/U10/U11) | BU-002 / E2 |

---

## 2. Power

> **⚡ TOPOLOGY DECISION (2026-08-16) — SPLIT the 3.3 V into two rails.** **U2 (LTC3119) = the Artix-only main + shared *housekeeping* rail** (ESP + sensors + camera LDOs + headers, ~1 A, all builds). The **KU040 (TE0841) gets its OWN dedicated high-current buck-boost** (8 A design target, ~10 A 1S input) that feeds **only** the module `VIN`/`3.3VIN`, **populated on KU040 builds, DNP on Artix builds.** This protects the proven Artix build, isolates a module-rail fault from the ESP/telemetry, and lets the ESP (booted on the housekeeping rail) I²C-configure the big rail before FPGA power-up. Full spec = **`RESPIN_POWER_RAIL.md`**. The items below with "(housekeeping/U2)" apply to U2; the KU040 rail is C2b.

- **[CRITICAL]** **KU040 dedicated 3.3 V rail (KU040 builds only, DNP on Artix)** — add **TI TPS55288RPMR** buck-boost (LCSC **C2864583**, 16 A integrated switches, resistor ILIM ~12 A + resistor UVLO ~3.05 V, ESP-configured VOUT/enable) + **TI TPS25982** input eFuse (LCSC **C2155683**, ILIM ~12 A / trip ~15 A, soft-start, FAULT→GPIO) on VSYS→buck-boost VIN + **Sunlord MWSA1204S-1R0MT** inductor (1 µH, Isat 28 A, DCR 7.5 mΩ). New net `MOD_3V3` + `R_HK`/`R_KU` build-time source-select (U2 on Artix / new rail on KU040 — never both). Firmware-free fallback = LTC3785 + AON FETs. Thermal: top layer, ≥16–25 EP vias, θJA ≤ 25 °C/W. *(RESPIN_POWER_RAIL.md — full budget, parts, thermal, layout, BOM/variant)*
- **[CRITICAL]** Add an **input inrush/current-limit (eFuse, programmable ILIM + slew, e.g. TPS259x/TPS1663) on VSYS/VCOM** ahead of U2 (housekeeping/U2) — the battery path has *no* fuse/PTC/limit while USB is capped at 1.5 A (LM66100); this asymmetry is what let the JB1.13 short kill U2 on battery but not USB. The recurring lesson of the whole §8–§9 incident. *(ECO §8.5-A6c, §8.9.7, §8.11.8, §9.8; RESPIN_POWER_U2.md §4)*
- **[IMPORTANT]** **Verify/upsize L4** (housekeeping/U2) — as-built is **2.2 µH in an 0603 footprint**; an 0603 power inductor saturates ~1–2 A, but the buck-boost needs 4–8 A peak for ≥3 A out on a sagging cell. Target **2.2–3.3 µH, Isat ≥ 11 A, ≥1210/SMD power package**. (NB: L4 was *not* the U2 death cause — overturned by §8.11 — but it caps real flight-load current.) *(ECO §8.3, §8.4-#2, §8.5-A6a; see `RESPIN_POWER_U2.md`)*
- **[RESOLVED]** **Real 3V3 current budget reconciled.** Artix (TE0712) idle 0.45 A, peak ≤ 3 A → stays on U2. **KU040 (TE0841): startup ≥4 A, heavy 5–6 A, aggressive ~7 A → 8 A design target on the dedicated rail (~10 A 1S input).** This is what drove the split-rail decision and the RESPIN_POWER_RAIL.md sizing. *(ECO §6-3; RESPIN_POWER_U2.md §2; RESPIN_POWER_RAIL.md §1)*
- **[IMPORTANT]** **Verify the `/VBAT_SENSE` fix is actually in the source** — the old `/VSENSE` net shorted the (now-removed) buck FB divider R48/R49 to the +BATT monitor divider **R51 (200 k)/R52 (100 k) → ESP U14.12 (ADC1_CH0)**, breaking ESP battery telemetry. Fix = R51/R52 on a *dedicated* `/VBAT_SENSE` to U14.12 only (4.2 V→1.40 V, 3.0 V→1.00 V). No BU entry ever confirmed battery telemetry works → **confirm on hardware + in schematic**. *(ECO §3 "BUG FOUND", §6-4; HIERARCHY_PLAN)*
- **[IMPORTANT]** **Characterize BU-009: board drops off USB after charge-termination** — when the MCP73831 finishes charging, the board disappears from USB and needs a battery unplug/replug. Suspected source-ORing hand-off glitch (LM66100/U8 ↔ LM74700/U13+Q9) or ESP brownout latch at the transition. **Not yet root-caused** — scope VSYS/3V3 across charge-complete; candidate next-spin ECO (VSYS hold-up / ideal-diode tuning). *(BU-009)*
- **[IMPORTANT]** **Decide the USB FPGA-power policy** — USB→VSYS is capped ~1.5 A by U8 (LM66100), so the SoM browns out on USB (BU-006). Either **document "battery-required for the SoM"** or upsize the USB ideal-diode path. (Design intent today: USB = charge+program, battery = flight.) *(BU-006 / E5)*
- **[NICE]** **~100 kΩ bleed 1V8 → GND** — an unloaded 1V8 floats to ~2.3 V from the PCA9306 (U12) VREF1 bias current (LDO can't sink); a bleed pins the rail at 1.8 V regardless of load. 2.3 V is over abs-max for HM01B0 IOVDD / FPGA 1.8 V bank. *(BU-002, BU-004 / E4)*
- **[NICE]** **Off-state back-feed (BU-004)** — 3V3/1V8 float to ~0.5 V with the switch OFF via USB ESD (U6→ESP) + batt-sense (R30→ADS7128 ESD) sneak paths; sub-threshold µA, accepted. Only act if hard-zero off is required (VBUS-gated isolation / series switch on the batt-sense tap). *(BU-004; see `RESPIN_POWER_BROWNOUT.md`)*
- **[NOTE / not a respin change]** U2's design is **validated** (ran the FPGA on PSU, USB, and battery — the exact mode that smoked the original). The original board's U2 died driving the JB1.13 day-one short; the §9.8 "growing rail fault" on the mule is a **board-specific rework artifact**, not a design defect. Respin fits a fresh U2. *(ECO §9.6–§9.8)*

---

## 3. FPGA / SoM

- **[CRITICAL]** **Reassign carrier `JB1` pin 13 from GND → +3V3** (one net-label change on the JB1 symbol) — under the LSHM hermaphroditic even/odd swap, `JB1.13`(GND) mates module `JM1.14`(3.3VIN) = a hard `+3V3`→GND dead short on every insertion, on every board. **Root cause of both the mated-only short and the U2 death.** After the change, re-run ERC + the §8.11.4 collision scan → expect **zero** collisions. *(ECO §8.11.7, §8.11.8; FPGA-bringup memory; see `RESPIN_FPGA_PINS.md`)*
- **[IMPORTANT]** **VCCIO / bank-voltage audit** — build the XDC bank-by-bank against the TE0712/TE0841 TRM: every carrier signal must land in a bank whose carrier-supplied VCCO matches its drive voltage (3.3 V). **Known latent item:** the carrier drives **`+1V8` onto module VCCIO13/VCCIO15** (`JB2.4/8/10`→`JM2.3/7/9`), setting banks 13/15 to 1.8 V — harmless only while those bank pins stay NC; overstress if 3.3 V logic is ever driven there. *(ECO §8.5-B3, §8.11.6; see `RESPIN_FPGA_PINS.md`)*
- **[IMPORTANT]** **Honor the module power-good / I/O-3-state handshake** — keep carrier I/O 3-stated until module power-good (3.3 V on JM2 pins 10/12) so carrier drivers don't back-drive module I/O; verify EN1/PGOOD/MODE + PROGRAM_B/INIT_B/DONE/M[2:0]/clocks are not externally driven before power-good. *(ECO §8.5-B2/B4; see `RESPIN_FPGA_PINS.md`)*
- **[IMPORTANT]** **Reconcile the B2B stack-height BOM contradiction** — symbols carry **LSHM-…-03.0** (7.00 mm stack, the chosen shortest-practical), but the rework spares ordered are **LSHM-150-04.0 / LSHM-130-04.0** (8.00 mm fallback). All 3 carrier connectors (JB1/JB2/JB3) MUST share one height or the module tilts → **pick one height for the respin BOM and make JB1/JB2/JB3 consistent.** *(redesign memory; ECO §8.12)*
- **[NICE]** **Cosmetic net/symbol cleanups:** `/FLVD` FPGA-side label is a typo for camera FVLD (net is fine); JB1/JB2 lib_id misnames as `Conn_02x40` but is really 2×50 (netlist authoritative); `/3v3OUT` (JB2.9/11) and `/VIDB` (JB1.40) are harmless floats. *(redesign memory; ECO §8.11.2)*
- **[NICE]** **Strap re-check for TE0841** — SC on-module pulls (SC1/SC4 "currently_not_used") aren't in the Trenz wiki; re-verify if SC firmware changes. Straps are otherwise VERIFIED for both TE0712/TE0841 (R4/R6/R7/R8/R9/R10, R46 0Ω DNP). *(redesign memory)*

---

## 4. Programmer board (`riskybirdv3_connector`)

- **[CRITICAL]** **Fix the FTDI symbol/BOM: FT2232HL → FT4232H** — the die is genuinely FT4232H (proven by `bcdDevice 8.00` + 4 USB interfaces + no EEPROM; package is *remarked*). The H-family shares power/USB/OSC/REF pins (why the bodge worked) but **multi-channel I/O pinouts differ** → re-verify all non-JTAG-ch-A nets: **UART TXD/RXD, the U9 + 3rd-chip JTAG channels, status LEDs.** Correct cable = `openFPGALoader -c ft4232`. *(ECO §9.5; FPGA-bringup memory; see `RESPIN_PROGRAMMER.md`)*
- **[CRITICAL]** **VPLL (pin 9): 1 kΩ + no cap → 600R@100MHz ferrite + 4.7 µF/100 nF** — the PHY PLL starved behind 1 kΩ with no decoupling → no USB clock → device never enumerates. **This was the primary "won't enumerate" cause.** *(ECO §9.2–§9.3)*
- **[CRITICAL]** **D+/D− series 22 Ω (R5/R8) → 0 Ω** — FT4232H has internal HS termination (working v2 wires D± direct); the 22 Ω breaks HS signal integrity. *(ECO §9.2–§9.3)*
- **[IMPORTANT]** **VPHY (pin 4): add 600R ferrite + 4.7 µF/100 nF LC filter** — lost its LC filter vs the v2 block (datasheet-recommended); milder than VPLL but restore it. Net effect: **copy the v2 (`riskybirdv2_sensor` U6) FTDI power/USB block verbatim.** *(ECO §9.3)*
- **[IMPORTANT]** **Apply the same VPLL/VPHY/USB fix to U9 + the 3rd FTDI** if the RISC-V-debug JTAG and UART-console channels are needed — only U15 (bitstream ch) was bodged/proven. *(ECO §9.3, §9.4 status; see `RESPIN_PROGRAMMER.md`)*
- **[IMPORTANT / sourcing]** **Buy correctly-marked FT4232H from an authorized distributor (Digikey/Mouser)** for production — the current reel is mismarked "FT2232HL" (a supply red flag). *(ECO §9.5)*
- **[NICE]** Confirm the debug topology supports the required **two separate JTAG buses** (FPGA fabric/bitstream + RISC-V SoC debug — they cannot tunnel over one JTAG) + SoC UART, per the working Trenz shell config. *(redesign memory; see `RESPIN_PROGRAMMER.md`)*

---

## 5. Sensors

- **[CRITICAL]** **IC1 (PMW3901): clear the PCB DNP flag + add `LCSC C43496881` / MPN `PMW3901MB-TXQT`** in the schematic — as fabbed it was DNP-on-PCB-only (BOM had it, CPL didn't) so JLC silently skipped it; fixing both re-enters it into BOM+CPL and removes the hot-air hand-solder step. *(BU-001 / E1)*
- **[IMPORTANT]** **Pin `LCSC Part` on all fixed-Vout TLV700 LDOs** — U7 `C398921` (2V8), U10 `C133796` (1V8), U11 `C2878901` (1V5) — U10 shipping without a pinned LCSC is exactly the kind of gap that lets JLC substitute a sibling Vout variant (the 2.3 V TLV70023 scare). *(BU-002 / E2)*
- **[IMPORTANT / decision]** **Resolve SPI bus ownership (FPGA vs ESP)** — PMW3901 (IC1) SPI is single-master; when shared between ESP and FPGA it needs an owner or a mux. A `/SPI\I2C_SEL` line ("Low=FPGA") exists — confirm the arbitration scheme. *(redesign memory, workstream 2)* — cross-cutting; not owned by the 4 deep-dives.
- **[IMPORTANT / decision]** **Confirm I²C multi-master plan** — I²C (BMI088/BMP390/VL53L1/ADS7128/ESP) is to stay on both ESP and FPGA (SW-muxed); extend SDA/SCL to the JB connectors and confirm pull-up/loading. *(redesign memory, workstream 2)*
- **[MED]** **Add external 10 kΩ pull-up GPIO9 → 3V3** — boot-strap robustness; today relies only on the ESP32-C6 internal ~45 kΩ, which a flux blob/bridge can drag low (BU-010 chased exactly this). *(BU-003, BU-007, BU-010 / E3)*
- **[NICE]** **~100 kΩ bleed on 1V8** (also listed under Power) — defines the rail vs the PCA9306 no-load float; relevant to HM01B0 IOVDD sequencing. *(BU-002 / E4)*

---

## 6. Camera (HM01B0)

- **[CRITICAL / GUI leftover]** **Refresh the HM01B0 receptacle footprint in the PCB** — schematic already repointed to `riskybird_lcsc:CONN-SMD_24P-P0.40_5055502420-HM01B0` (Molex **505550-2420** B8 receptacle, ring/vertical-numbered, mates the module's 503776-2410 plug), but the embedded PCB copy is stale (same-name F8 won't refresh geometry). **Do in GUI:** reopen KiCAD → Tools → Update Footprints from Library (or F8 "Replace footprints") → re-route the connector (receptacle land is wider, 5.90 vs 4.40 mm) → **verify pin-1 mating orientation in the 3D viewer** (plug↔receptacle mate face-to-face — easy to mirror) → DRC → regen gerbers. *(redesign memory)*
- **[NOTE]** Camera electrical is validated: gender fixed (plug→receptacle), pad ring-numbering corrected to vertical (pin-1 top-right, 1–12 down right col, 13–24 up left col), pinout pin-for-pin correct, IOVDD=1.8 V with PCA9306 (U12) level-shifting SCCB onto the shared 3.3 V bus. Only the PCB-geometry refresh above remains. *(redesign memory)*

---

## 7. Layout / DRC / gerbers

- **[IMPORTANT]** **U2 thermal improvement** — U2 sits on B.Cu with only ~6 GND EP vias and SW1 copper necking to 0.3 mm; ADI wants a denser via array under a 4×5 QFN at 5 A. Add EP thermal vias / thermal copper, consider **moving U2 to the top layer**. *(ECO §8.4-#4, §8.5-A6b)*
- **[IMPORTANT]** **Re-place + re-route the power front-end** for the respin (LTC3119 + ideal-diode ORing + eFuse + upsized L4 **+ the new KU040 buck-boost/eFuse/inductor**); verify current-carrying copper: **VSYS/ORing ~12 A, new-rail input ~10 A, new-rail output 8 A, VCOM/+3V3 housekeeping** — carry power on wide 2 oz pours (input/VSYS ≥2.4–3.0 mm, output ≥1.7 mm at 2 oz), ≥8–10 stitching vias per transition on the 10–12 A path, no neck-downs. *(ECO §5-5; RESPIN_POWER_RAIL.md §6–§7; redesign memory)*
- **[IMPORTANT / process]** **Before every JLC order, diff BOM designators vs CPL designators** — any part in one file but not the other silently fails to assemble (the exact IC1 DNP-desync failure; same class as the v2 connector-board LED desync). *(BU-001)*
- **[IMPORTANT]** **Run ERC + DRC clean after all net changes** (JB1.13, `/VBAT_SENSE` split, FTDI edits, camera footprint) and **regenerate gerbers/CPL/BOM**; expect new-net / power-flag ERC items to resolve. *(ECO §5, §8.11.7)*
- **[NICE]** **Optional: apply the HIERARCHY_PLAN sheet-split** (flat 179-part schematic → Power/FPGA/Sensors/Camera/ESP/USB-Debug/Motors sheets via global labels) as a behavior-preserving refactor before the power edits, verified by netlist node-membership diff. Organizational, non-blocking. *(HIERARCHY_PLAN.md)*

---

## 8. BOM / sourcing (hand-populate + off-BOM)

- **[CRITICAL / off-BOM]** **IC1 PMW3901MB-TXQT** — `LCSC C43496881` (includes the loose LN03-ZSZ lens); not stocked at Mouser/Digikey. Order **12–15** (5 boards + hot-air rework spares). If the DNP fix (C6/§5) lands, JLC assembles it instead. *(OFF-BOM; BU-001)*
- **[IMPORTANT / off-BOM]** **ESP32-C6 antenna** — 2.4 GHz **U.FL/IPEX MHF1**, `LCSC C1693673` (80 mm pigtail); module ships with none. **Verify connector is MHF1 not MHF4** and 80 mm reaches the mount. Order 6. *(OFF-BOM)*
- **[IMPORTANT]** **B2B connectors** — JB1/JB2 `LSHM-150-03.0-L-DV-A-S-K-TR` (LCSC C6980926), JB3 `LSHM-130-03.0-…` (LCSC C6890994) **OR** the 04.0 8 mm variants — **pick one height (§3 contradiction)**. Order JB1 spares (**≥2 LSHM-150-04.0**) regardless — rework attrition is high on the 0.5 mm razor-beam part. *(redesign memory; ECO §8.12)*
- **[IMPORTANT / sourcing]** **FT4232H (programmer)** — buy correctly-marked from an authorized distributor; fix the symbol/BOM off the mismarked reel. *(ECO §9.5)*
- **[NICE]** **External modules to source separately:** HM01B0 camera module (503776-2410 plug side), 4× VL53L5CX side-ToF breakouts. `R46 = 0R` is DNP by design (config jumper) — do **not** order. *(OFF-BOM)*

---

## 9. Flagged — unresolved / contradictory across the docs

1. **B2B stack height 03.0 vs 04.0** — symbols say 7.00 mm (03.0); rework spares ordered are 8.00 mm (04.0). Must be reconciled to ONE height across JB1/JB2/JB3 for the respin. *(§3, §8)*
2. **`/VBAT_SENSE` battery-telemetry fix — build status unconfirmed** — specified in ECO §3 but no BU entry validates ESP battery-voltage reads on hardware. Verify in schematic + on board. *(§2)*
3. **L4 inductor adequacy** — flagged undersized (2.2 µH/0603) for full FPGA load, but the board ran fine at 0.45 A idle on all 3 sources and the inductor was cleared as *not* the U2 death cause (§8.11 overturned §8.4). Open until the real flight-load current is measured and L4 is sized to it. *(§2)*
4. **BU-009 charge-termination USB drop** — symptom logged, not root-caused; may or may not need a next-spin ORing/hold-up change. *(§2)*
5. **eFuse vs "design already validated"** — U2 is proven on all 3 sources *without* an input current-limit, yet the eFuse is the headline lesson of the whole incident. It is **defense-in-depth against a future short/inrush**, not a fix for a present failure — keep it CRITICAL for the respin but frame it as protection, not a bug fix. *(§2)*
6. **Camera PCB footprint** — schematic is correct and the footprint file is rebuilt, but the change is only real once the GUI refresh + re-route + 3D pin-1 check + DRC + gerber regen are actually done. Until then the fab data still carries the old (scrambled) land. *(§6)*
