# RESPIN — JLCPCB Assembly BOM audit + substitution list

**Project:** riskybird v3 base board (`riskybirdv3_base/riskybirdv3.kicad_sch`)
**Goal:** a BOM JLC can use to **directly SMT-assemble** the respin board — every part verified against JLC's *assembly parts library* (Basic / Preferred Extended / Extended), not merely "listed on LCSC."
**Status:** Audit only — **no `.kicad_*` edits, nothing committed.** Companion to `RESPIN_POWER_RAIL.md`, `RESPIN_POWER_U2.md`, `RESPIN_POWER_BROWNOUT.md`, `RESPIN_CHECKLIST.md`, `OFF-BOM_PARTS_ORDER.md`.
**Date:** 2026-08-16

---

## 0. Method, data source, reliability

- **BOM extracted** from the live schematic with `kicad-cli sch export bom` (grouped by Value+Footprint). The schematic carries LCSC C-numbers in a property named **`LCSC Part`** (126 of 419 symbols populated) — the bennymeg *Fabrication Toolkit* plugin (this project's `fabrication-toolkit-options.json`) reads that field, so the flow is compatible. There is **no** bare `LCSC` property; parts without a `LCSC Part` value are the "no-LCSC" gaps below.
- **JLC assembly status** (in-catalog? class? stock? price?) queried from **`jlcsearch.tscircuit.com/api`**, which is a hosted mirror of the **community `jlcparts` SQLite database** (github.com/yaqwsx/jlcparts) — i.e. JLC's *assembly* catalog, exactly the source the brief recommended. The GitHub-Pages raw DB path (`yaqwsx.github.io/jlcparts/data/cache.json`) now 404s, so the tscircuit mirror was used instead. Fields returned per part: `is_basic`, `is_preferred`, `stock`, `price`, `package`, `mfr`, `description`.
- **Reliability:**
  - **In-catalog (assemblable y/n) — HIGH.** A part returned by the mirror is in the jlcparts assembly DB; a part with **0 results is not in the assembly catalog** (may be LCSC-order-only or discontinued). This is how the BMP390 and the eFuse gaps were found.
  - **Class (Basic/Preferred/Extended) — HIGH.** Stable field.
  - **Stock — MEDIUM (point-in-time snapshot; some noise).** The jlcparts DB is refreshed periodically, so absolute stock is approximate. Observed noise: one jellybean 4.7 kΩ 0201 briefly read "9" then "1.3 M" on re-query. **Treat all stock counts below as order-time-verify**, especially the low ones. Re-check any part at `jlcpcb.com/partdetail/<C#>` before the order.
  - **Not verifiable from this source:** exact assembly-line placement rules for odd packages, and JLC's per-order "economic vs standard" surcharge for Extended parts (every non-Basic part on an SMT order incurs a per-unique-part feeder fee — noted where it matters).

**Headline:** The board is **NOT yet fully JLC-assemblable as drawn.** The silicon is nearly all in the catalog, but **~26 line-items have no `LCSC Part` field** (JLC would skip them), **1 part is genuinely not in the assembly catalog (U5 BMP390)**, and the **3 new power-rail parts** need their C-numbers pinned (buck-boost OK, eFuse + inductor need the corrected C#s below). All gaps have a concrete JLC-assemblable fix listed. Count: **1 not-assemblable, ~26 no-LCSC-field, ~6 low-stock watch items.**

## 0a. Applied changes (2026-08-16) — U5 swap + IC1 DNP/LCSC

Two fixes were **applied to `riskybirdv3.kicad_sch` AND `riskybirdv3.kicad_pcb`** (backups `*.bak_20260816_142627`; **not committed**; validated: BOM re-export ✓, ERC unchanged at 182 before/after, PCB loads via pcbnew, IC1 now present in the `--exclude-dnp` position file):

- **U5 BMP390 → BMP388** (the not-assemblable part). Value `BMP390→BMP388`, MPN `BMP390L→BMP388`, `LCSC Part C2684802→C779278`, in both schematic instance and PCB footprint fields. **Footprint land pattern `riskybird:BMP390` kept** (BMP388 = same LGA-10 2×2 package/pinout). *Note:* the `lib_id`/footprint name still reads "BMP390" (cosmetic; the BOM Value/MPN/LCSC are now BMP388/C779278). Firmware baro driver must cover BMP388.
- **IC1 PMW3901 un-DNP'd + LCSC added.** Cleared `(attr smd dnp)` → `(attr smd)` on the PCB footprint (the actual "BOM-yes/CPL-no" desync cause — the schematic symbol was already `dnp=no`), and added `LCSC Part = C43496881` to both the schematic symbol and PCB footprint. IC1 now exports into the position/CPL file.

**Still a GUI follow-up (recommended):** open KiCAD and run **Update PCB from Schematic (F8)** once to confirm sch↔pcb parity (fields were edited on both sides to match, so no geometry change is expected). And when the *other* §2 no-LCSC parts get their C#s, pin them in the schematic and re-run F8.

**B2B connectors actually ordered (from the real order BOM `production/riskybirdv3-bom.csv`, 2026-07-26):** the **LSHM-04.0 / 8.00 mm stack** — JB1+JB2 = `LSHM-150-04.0-L-DV-A-S-K-TR` (**C5367357**, qty 2), JB3 = `LSHM-130-04.0-L-DV-A-S-K-TR` (**C3645597**, qty 1). **Not** the 03.0/7.00 mm option. So the current schematic already matches what was ordered; the 03.0-vs-04.0 "contradiction" resolves to **04.0** for the respin (and 04.0 has more JB1/JB2 stock than 03.0 — 11 vs 1).

---

## 1. Full BOM — JLC assembly status

Legend for **STATUS**: **OK** = has C#, in catalog, adequate stock · **NO-LCSC** = in catalog but no `LCSC Part` field in schematic (JLC will skip until assigned) · **NOT-ASM** = not in JLC assembly catalog · **LOW-STK** = assemblable but thin stock (order-time verify) · **THT** = through-hole (JLC THT-assembly upcharge, else hand-solder) · **N/A** = not a placed part.

Class = Basic / Pref (Preferred Extended) / Ext (Extended). Stock/price are the 2026-08-16 snapshot.

| Ref(s) | Value | Footprint | MPN | LCSC (in sch) | Class | Stock | Price | STATUS |
|---|---|---|---|---|---|---|---|---|
| 1V5,1V8,2V8,3V3,BATT1,TP1,TP2 | TestPoint | Pad_D1.0mm | — | — | — | — | — | **N/A** (bare pads, not placed) |
| C1 | 4.7µ | 0201 | GRM035R60J475ME15D | C335103 | Ext | 684k | $0.20 | OK |
| C2,C3,C40,C41 | 47µF | 0603 | — | — | — | — | — | **NO-LCSC** → C140782 |
| C4,C5,C6,C7,C10,C13,C15,C38 | 100nF | 0201 | GRM033R60J104KE19D | C76928 | Ext | 2.56M | $0.001 | OK |
| C8,C42,C59,C66 | 1µF | 0201 | CL03A105MQ3CSNH | C53067 | Ext | 43k | $0.004 | OK |
| C9,C39 | 4.7µF | 0201 | GRM035R60J475ME15D | C335103 | Ext | 684k | $0.20 | OK |
| C11,C12 | 22µF | 0603 | — | — | — | — | — | **NO-LCSC** → C59461 (Basic) |
| C14 | 10µF | 0402 | — | — | — | — | — | **NO-LCSC** → C15525 (Basic) |
| C16,C17 | 100n | 0201 | GRM033R60J104KE19D | C76928 | Ext | 2.56M | $0.001 | OK |
| C37,C43,C44,C45,C50,C51,C54,C56–C58,C60,C61,C77 | 0.1µ | 0201 | GRM033R60J104KE19D | C76928 | Ext | 2.56M | $0.001 | OK |
| C46,C47,C48,C49,C62,C67,C73,C74,C75 | 10µ | 0402 | — | — | — | — | — | **NO-LCSC** → C15525 (Basic) |
| C52,C53,C55,C68,C69,C70 | 1µ | 0201 | CL03A105MQ3CSNH | C53067 | Ext | 43k | $0.004 | OK |
| C63 | 150µ | 1206 | — | — | — | — | — | **NO-LCSC** → C528968 |
| C64 | 470p | 0201 | GRM033R71H471KA12D | C161493 | Ext | 12k | $0.003 | OK |
| C65 | 4.7µ | 0402 | — | — | — | — | — | **NO-LCSC** → C23733 (Basic) |
| C71,C72 | 0.1µ | 0402 | — | — | — | — | — | **NO-LCSC** → C1525 (Basic) |
| D1,D4 | LED | 0402 | — | — | — | — | — | **NO-LCSC** → C965793 (grn) / C20613596 (wht) — pick color |
| D2,D3,D5,D6 | BAT54JFILM | SOD-323 | — | — | — | — | — | **NO-LCSC** → C86299 |
| D7 | LED_RED | 0402 | — | — | — | — | — | **NO-LCSC** → C25503345 |
| HM01B0 | camera recept. | 5055502420 | 505550-2420 | C3642262 | Ext | 5112 | $0.37 | OK (mates HM01B0 module plug) |
| IC1 | PMW3901MB-TXQT | COB/LGA-28 | — | **C43496881** ✅ | Ext | 596 | $4.96 | **FIXED 2026-08-16** — LCSC added + PCB `dnp` cleared (§0a) |
| J1 | FTSH-110-01-L-DV-K-TR | 2×10 1.27mm | FTSH-110-01-L-DV-K-TR | C20728453 | Ext | **31** | $2.85 | **LOW-STK** (debug hdr) |
| J2,J9,J10,J11 | Conn_01x05 | 1×5 2.54mm hdr | — | — | Ext | 251k | $0.03 | **NO-LCSC + THT** → C492404 |
| J3,J4,J5,J6 | Conn_01x02 | Molex PicoBlade 53048-0210 | 53048-0210 | C505099 | Ext | 30.8k | $0.14 | OK as drawn — **respin: upsize** (1A/contact, see §5) |
| J8 | USB-C recept. | TYPE-C-31-M-12 | TYPE-C-31-M-12 | C165948 | Ext | 336k | $0.16 | OK |
| JB1,JB2 | LSHM-150-04.0-…-S-K-TR | 2×50 0.5mm | LSHM-150-04.0-L-DV-A-S-K-TR | C5367357 | Ext | **11** | $13.40 | **LOW-STK** (B2B) — see §5 |
| JB3 | LSHM-130-04.0-…-S-K-TR | 2×30 0.5mm | LSHM-130-04.0-L-DV-A-S-K-TR | C3645597 | Ext | **23** | $8.27 | **LOW-STK** (B2B) — see §5 |
| L4 | 2.2µ | 0603 | — | — | — | — | — | **NO-LCSC + SUPERSEDED** by the new power inductor C408497 (see §4/§5) |
| Q1,Q2,Q3,Q4 | SI2302 | SOT-23 | — | — | Ext | 215k | $0.017 | **NO-LCSC** → C2891732 |
| Q5 | AON7407 | DFN-8 3×3 | AON7407 | C176756 | Ext | 19.4k | $0.26 | OK |
| Q6,Q7 | 2N7002 | SOT-23 | — | — | **Basic** | 1.84M | $0.013 | **NO-LCSC** → C8545 (Basic) |
| Q9 | AON7534 | PQFN-8 3.3×3.3 | AON7534 | C115844 | Ext | 33k | $0.21 | OK (also = TPS552882 buck-side FET, §4) |
| R1,R3,R12 | 150 | 0201 | RC0201FR-07150RL | C295773 | Ext | 30k | $0.001 | OK |
| R2 | 2k | 0201 | 0201WMF2001TEE | C270358 | Ext | 123k | $0.001 | OK |
| R4,R6–R10,R15,R16,R19,R20,R40–R42,R55,R72 | 10k | 0201 | 0201WMF1002TEE | C473048 | Ext | 1.14M | $0.001 | OK |
| R5,R11 | 5.1k | 0201 | 0201WMF5101TEE | C270344 | Ext | 354k | $0.001 | OK |
| R13,R14,R17,R18,R21,R24,R26–R29,R34,R38 | 47R | 0201 | RC0201FR-0747RL | C273279 | Ext | 25.9k | $0.001 | OK |
| R22,R23,R43,R44 | 4.7K | 0201 | 0201WMF4701TEE | C270346 | Ext | 1.30M | $0.001 | OK |
| R25 | 3 | 0603 | — | — | — | — | — | **NO-LCSC** → C22356394 (3Ω 0603 1%) |
| R30 | 200K | 0201 | RC0201FR-07200KL | C138163 | Ext | 29.5k | $0.001 | OK (respin re-gates this net, §5) |
| R31 | 100K | 0201 | 0201WMF1003TEE | C270364 | Ext | 1.45M | $0.001 | OK |
| R32 | 10K | 0201 | 0201WMF1002TEE | C473048 | Ext | 1.14M | $0.001 | OK |
| R33,R63,R64,R65,R66 | 100 | 0201 | RC0201FR-07100RL | C77623 | Ext | 44k | $0.001 | OK |
| R37 | 47k | 0201 | 0201WMF4702TEE | C270345 | Ext | 285k | $0.002 | OK |
| R39,R46 | 0R | 0201 | RC0201FR-070RL | C106227 | Ext | 28k | $0.001 | OK (R46 DNP by design) |
| R45,R58,R60 | 100k | 0201 | 0201WMF1003TEE | C270364 | Ext | 1.45M | $0.001 | OK |
| R47,R48,R67 | 200k | 0201 | RC0201FR-07200KL | C138163 | Ext | 29.5k | $0.001 | OK |
| R56,R57 | 4.7k | 0201 | 0201WMF4701TEE | C270346 | Ext | 1.30M | $0.001 | OK |
| R59 | 316k | 0201 | GR0201F316KTAG00 | C49654581 | Ext | 8.5k | $0.001 | OK |
| R61 | 52.3k | 0201 | 0201WMF5232TEE | C473632 | Ext | 4.3k | $0.001 | OK |
| R62 | 48.7k | 0201 | 0201WMF4872TEE | C473629 | Ext | 15k | $0.001 | OK |
| R68,R69 | 2.2k | 0201 | 0201WMF2201TEE | C473508 | Ext | 12.8k | $0.002 | OK |
| R70 | 0R | 1206 | — | — | — | — | — | **NO-LCSC** → assign any JLC 0Ω 1206 jumper (low priority) |
| R71 | 63k | 0201 | RC0201DR-0763K4L | C851774 | Ext | 9.9k | $0.004 | OK |
| SW1,SW2,SW4 | SW_SPST | B3U-1000P | B3U-1000P | C231329 | Ext | 85.6k | $0.14 | OK |
| SW5 | slide (master) | MSK12C02 | MSK12C02 | C2681570 | Ext | 85.6k | $0.17 | OK |
| U1 | ADS7128IRTER | WQFN-16 3×3 | ADS7128IRTER | — | Ext | 2045 | $2.70 | **NO-LCSC** → C2867992 |
| U2 | LTC3119EUFD | QFN-28 4×5 | LTC3119EUFD#TRPBF | C580693 | Ext | **11** | $14.41 | **LOW-STK** — but **REMOVED in single-rail respin** (§4) |
| U3 | BMI088 | LGA-16 | BMI088 | — | Ext | 2636 | $12.66 | **NO-LCSC** → C194919 |
| U4 | MCP73831 | SOT-23-5 | MCP73831T-2ACI/OT | C424093 | Ext | 11k | $0.79 | OK |
| U5 | ~~BMP390~~ **BMP388** | LGA-10 2×2 | BMP388 | **C779278** ✅ | Ext | 1166 | $1.14 | **FIXED 2026-08-16** — swapped BMP390→BMP388 in sch+pcb (§0a) |
| U6 | USBLC6-2P6 | SOT-666 | USBLC6-2P6 | — | Ext | 27k | $0.10 | **NO-LCSC** → C2827693 |
| U7 | TLV70028DCKR | SC-70-5 | TLV70028DCKR | C398921 | Ext | 988 | $0.08 | OK |
| U8 | LM66100DCK | SC-70-6 | LM66100DCK | — | Ext | 548 | $0.26 | **NO-LCSC** → C5219211 |
| U9 | VL53L1CXV0FY1 | LGA-12 2.5×4.9 | VL53L1CXV0FY1 | — | Ext | 5732 | $4.92 | **NO-LCSC** → C190004 |
| U10 | TLV70018DCKR | SC-70-5 | TLV70018DCKR | — | Ext | 5425 | $0.04 | **NO-LCSC** → C133796 |
| U11 | TLV70015DCKT | SC-70-5 | TLV70015DCKT | C2878901 | Ext | 479 | $0.21 | OK |
| U12 | PCA9306DC | VSSOP-8 | PCA9306DC | — | Ext | 27.5k | $0.18 | **NO-LCSC** → C33196 |
| U13 | LM74700 | SOT-23-6 | LM74700 | — | Ext | 10.4k | $0.49 | **NO-LCSC** → C2941042 |
| U14 | ESP32-C6-MINI-1U | module | ESP32-C6-MINI-1U-H4 | C20627095 | Ext | 787 | $4.55 | OK (U.FL antenna = off-BOM, §5) |

---

## 2. MUST RESOLVE before respin (prioritized)

### 🔴 P1 — Blocks assembly of a placed part

| Item | Problem | Fix (JLC-assemblable) |
|---|---|---|
| **U5 BMP390** (`C2684802`) | **Not in JLC assembly catalog** (LCSC-order-only). Any BMP390 query = 0 hits. JLC cannot place it. | **Substitute BMP388 = `C779278`** (Bosch, LGA-10 **2×2**, Ext, ~1166 stk, $1.14). Same package/pinout as BMP390 and same BMP38x register family → footprint- and driver-compatible (BMP390 = temp-improved BMP388). *Alt:* keep BMP390 as a **hand-populate** off-BOM part. Confirm the `riskybird:BMP390` footprint = LGA-10 2×2 before swapping. |
| **~26 no-LCSC line-items** | No `LCSC Part` field → Fabrication-Toolkit BOM leaves them blank → JLC silently skips them. | Assign the C-numbers in §1 / the table below. All are **in the catalog** (assemblable). |
| **IC1 PMW3901** | No `LCSC Part` **and** DNP-on-PCB (BU-001) → JLC skipped it last run. | Add `C43496881` (Ext, ~596 stk, $4.96) **and clear the PCB DNP** (RESPIN_CHECKLIST C6). If you keep it DNP, it stays a hot-air hand-populate (`OFF-BOM_PARTS_ORDER.md`). |

**No-LCSC-field parts → assign these (all verified in-catalog, 2026-08-16):**

| Ref(s) | Value/Part | Assign LCSC | Class | Note |
|---|---|---|---|---|
| C2,C3,C40,C41 | 47µF 0603 | **C140782** (GRM188R60J476ME15D, 6.3V X5R) | Ext | no 47µF-0603 Basic exists; 0805 Basic = C16780 if footprint allows |
| C11,C12 | 22µF 0603 | **C59461** (CL10A226MQ8NRNC) | **Basic** | |
| C14, C46–C49,C62,C67,C73–C75 | 10µF 0402 | **C15525** (CL05A106MQ5NUNC 6.3V) | **Basic** | |
| C63 | 150µF 1206 | **C528968** (GRM31CR60J157ME11L 6.3V) | Ext | |
| C65 | 4.7µF 0402 | **C23733** (CL05A475MP5NRNC) | **Basic** | |
| C71,C72 | 0.1µF 0402 | **C1525** (CL05B104KO5NNNC 50V X7R) | **Basic** | |
| D1,D4 | LED 0402 | **C965793** (green) or **C20613596** (white) | Ext | choose by intended indicator color |
| D2,D3,D5,D6 | BAT54JFILM SOD-323 | **C86299** (exact MPN) | Ext | exact BAT54JFILM; alt C358488 (BAT54J) |
| D7 | red LED 0402 | **C25503345** (XL-1005SURC) | Ext | |
| J2,J9,J10,J11 | 1×5 2.54mm header | **C492404** (PZ254V-11-05P) | Ext | **THT** — JLC THT-assembly upcharge, else hand-solder |
| Q1–Q4 | SI2302 SOT-23 N-FET | **C2891732** (SI2302) | Ext | many equivalents; also C20628872 (SI2302S) |
| Q6,Q7 | 2N7002 SOT-23 | **C8545** (2N7002) | **Basic** | |
| R25 | 3Ω 0603 | **C22356394** (RCA033RFLF 1%) | Ext | |
| R70 | 0Ω 1206 jumper | assign any JLC 0Ω 1206 | — | low priority; couldn't pull a clean 1206-0Ω hit — verify at order |
| U1 | ADS7128IRTER | **C2867992** | Ext | WQFN-16-EP 3×3 ✓ |
| U3 | BMI088 | **C194919** | Ext | LGA-16 ✓ ($12.66) |
| U6 | USBLC6-2P6 | **C2827693** (SOT-666) | Ext | match footprint SOT-666 (not the SOT-23-6 …2SC6) |
| U8 | LM66100DCK | **C5219211** (LM66100QDCKRQ1, SC-70-6) | Ext | Q1 variant, best stock; alt C2832141 |
| U9 | VL53L1CXV0FY1 | **C190004** (VL53L1CXV0FY/1, LGA-12) | Ext | in catalog — the exact MPN just has a "/1" in JLC's name |
| U10 | TLV70018DCKR | **C133796** | Ext | matches RESPIN_CHECKLIST C8; **1.8V-fixed — pin it so JLC can't sub a wrong-Vout sibling** |
| U12 | PCA9306DC | **C33196** (PCA9306DCUR, VSSOP-8) | Ext | |
| U13 | LM74700 | **C2941042** (LM74700QDBVRQ1, SOT-23-6) | Ext | |

### 🟠 P2 — Low stock / order-early (assemblable, but thin)

| Ref | Part | C# | Stock | Action |
|---|---|---|---|---|
| JB1,JB2 | LSHM-150-04.0 | C5367357 | **11** | Samtec 0.5mm B2B is the tightest supply on the board (need 2/board = 10 for 5 boards). The 03.0 height alt **C6980926 is worse (stk 1)**. **Lock the stack-height decision (03.0 vs 04.0 — RESPIN_CHECKLIST §3/§9) and pre-order + spares.** |
| JB3 | LSHM-130-04.0 | C3645597 | 23 | order-early; 03.0 alt C6890994 (stk 24) |
| J1 | FTSH-110-01-L-DV-K-TR | C20728453 | 31 | debug header; order-early |
| U7 | TLV70028DCKR | C398921 | 988 | fine for 5 boards; note for larger runs |
| U11 | TLV70015DCKT | C2878901 | 479 | fine for 5 boards |
| U14 | ESP32-C6-MINI-1U | C20627095 | 787 | fine; antenna off-BOM |
| **buck-boost** | TPS552882QRPMRQ1 | **C2876594** | **102** | §4 — enough for a proto run, thin for volume; single-source AEC-Q100 |
| **eFuse** | TPS259824ONRGER | **C2155766** | ~106 | §4 |
| **inductor** | MWSA1204S-1R0MT | **C408497** | ~343 | §4 |

*(U2 LTC3119 `C580693` also reads stk 11, but it is **deleted** in the single-rail respin — see §4.)*

---

## 3. The 3 new power-rail parts — confirmed JLC-assemblable C-numbers

Per `RESPIN_POWER_RAIL.md` **REVISION (2026-08-16)**: single continuous 3.3 V rail, **drop U2/LTC3119**, one non-I²C buck-boost for all builds.

| Function | Part | **LCSC C#** | Class | Stock | Price | Verified spec |
|---|---|---|---|---|---|---|
| **Buck-boost** (2.7–36 Vin → 3.3 V, ≤8 A) | **TPS552882QRPMRQ1** | **`C2876594`** | **Extended** | **~102** | $5.40 | VQFN-26-HR 3.5×4; 2.7–36 V in, 200 kHz–2.2 MHz, 16.5 A ILIM, adj. current limit, PFM light-load, built-in LDO, spread-spectrum. **Non-I²C** (fixed 1.2 V ref + external FB divider) → autonomous power-up. AEC-Q100. **ASSEMBLABLE.** |
| **Input eFuse** (2.7–24 V, ~12–15 A, adj ILIM, SS, FAULT) | **TPS259824ONRGER** | **`C2155766`** | **Extended** | **~106** | $3.88 | QFN-24 4×4; **2.7–24 V, 2.7 mΩ, 2–15 A adjustable ILIM**, integrated FET, OCP/SCP/UVLO/thermal. *Alt:* **TPS259827ONRGET `C2155765`** (VQFN-24, 4.5 mΩ, ~29–226 stk). **These replace the brief's `C2155683` (TPS259822), which is NOT in the JLC catalog** (0 hits — confirmed the memory's "failed CAD fetch"). |
| **Power inductor** (1–2.2 µH, Isat ≥ 20–25 A) | **MWSA1204S-1R0MT** | **`C408497`** | **Extended** | **~343** | $0.71 | 13.5×12.8 mm molded shielded; **1 µH, Isat 24 A, Irms 35 A, DCR 7.5 mΩ** (JLC desc). Exactly the Sunlord part the spec names. *Same-family alts:* MWSA1204S-R68MT `C408495` (0.68 µH, Isat 28 A), MWSA1204S-4R7MT `C408501` (4.7 µH, Isat 18 A). **Avoid** C149550 (MWSA1004-2R2MT) — not in catalog. |

**Notes:**
- **Buck-boost:** the standard non-automotive **TPS552882RPMR is NOT on LCSC/JLC** (confirmed — query returns nothing) so `C2876594` (the Q1 automotive twin) is the only JLC path; it *is* assemblable but **Extended + ~102 stock + single-source** → order early, keep spares. The I²C sibling **TPS55288 `C2864583`** (Ext, ~6000 stk, $2.68) is deeper-stock **but I²C-programmed → will not reach 3.3 V standalone at boot** — do not substitute it into the single-rail design without the firmware config path.
- **eFuse reverse-blocking caveat:** the TPS25982 family does **not** provide reverse-current blocking; in this design the upstream ideal-diode ORing (U13 LM74700 + Q9, and U8 LM66100) already handles reverse — so the eFuse only needs OCP/SCP/soft-start/FAULT, which C2155766 has. Pick auto-retry vs latch-off per the TI datasheet ordering suffix (…24/…27 vs …21/…23) — verify the exact behavior against the datasheet before schematic entry.
- **External buck-side FETs (2×):** the TPS552882 is a *hybrid* controller needing two external buck-side N-FETs (DR1H/DR1L, SW1). **AON7534 = `C115844`** (already on this BOM, Ext, ~33k stk, 30 V/30 A) is a fine candidate — verify gate-drive Qg per datasheet.

---

## 4. Respin new-part cross-check (parts introduced by the fixes)

| Change (doc) | New part | JLC-assemblable option | Status |
|---|---|---|---|
| Single-rail power (`RESPIN_POWER_RAIL.md`) | buck-boost / eFuse / inductor | C2876594 / C2155766 / C408497 | ✅ see §3 |
| Single-rail power | **remove U2 LTC3119** (`C580693`) + old L4 (2.2µ/0603, no-LCSC) | n/a (deletions) | ✅ removing them clears two weak spots (U2 stk 11; L4 was the saturating 0603) |
| eFuse replaces the Q5 load switch (`RESPIN_POWER_RAIL.md` R.4) | — | Q5 AON7407 `C176756` may be freed | note |
| Brownout fix (`RESPIN_POWER_BROWNOUT.md` S1) — high-side P-FET gating the R30 batt-sense divider on `+BATT` | small SOT-23 P-FET + gate R | **Si2301CDS = `C10487`** (SOT-23, **Basic**, ~299k stk, 20 V, 3.1 A) — plenty for a µA sense tap; *alt* DMG2305UX `C150470` | ✅ assemblable (Basic) |
| Battery connector upsize (`RESPIN_POWER_U2.md` §5.3) — PicoBlade → XT30/JST-PH | 1S LiPo connector ≥10 A | XT30/XT60 are **THT, not SMT** → JLC won't SMT-place → **hand-solder / off-BOM**; a JST-PH-2.0 SMD (e.g. C160404-class) could be JLC-placed but is only ~2–3 A | ⚠ likely off-BOM |
| B2B stack-height reconcile (`RESPIN_CHECKLIST.md` §3) | LSHM 03.0 **or** 04.0 | 04.0 C5367357/C3645597 (stk 11/23) **vs** 03.0 C6980926/C6890994 (stk **1**/24) | ⚠ **04.0 currently in BOM has more JB1/JB2 stock than 03.0 (1 pc); both thin — decide + pre-order** |
| Programmer FT4232H redraw (`RESPIN_PROGRAMMER.md`) | FT4232H etc. | **separate board `riskybirdv3_connector` — OUT OF SCOPE** for this base-board BOM | ⚠ flagged only |
| ESP32-C6 U.FL antenna (`OFF-BOM`) | 2.4 GHz IPEX MHF1 pigtail | `C1693673` — **off-BOM hand-add** (not SMT-placed) | note |

---

## 5. Things to hand off / verify at order time

- **Pin `LCSC Part` on all the §2 assignments** in the schematic (not just the ICs) so the Fabrication-Toolkit BOM is complete, then **diff BOM designators vs CPL designators** before ordering (the IC1 DNP-desync lesson, `RESPIN_CHECKLIST.md` §7).
- **Re-verify every LOW-STK C# at `jlcpcb.com/partdetail/<C#>`** on the day of order — stock here is a point-in-time mirror snapshot and was observed to be noisy on jellybeans.
- **Extended-part feeder fees:** almost the entire silicon set is *Extended* (only 2N7002/22µF/10µF/4.7µF/0.1µF resolve to Basic). Each unique Extended part carries JLC's per-part loading fee — expected for this design, just budget for it.
- **U5 substitution needs a firmware check** — BMP388 vs BMP390 register/driver differences are small but non-zero; confirm the flight FW baro driver covers BMP388, or hand-populate BMP390.
- **BMP390/PMW3901/VL53L1** are all Bosch/PixArt/ST sensors that can be genuine-supply-sensitive on LCSC — for flight units prefer verifying lot/authenticity (PixArt fakes are common, per `OFF-BOM_PARTS_ORDER.md`).
- **Could not fully verify:** the exact 0Ω-1206 jumper C# for R70 (trivial), and the auto-retry-vs-latch behavioral suffix of the eFuse (needs the TI datasheet). Everything else in §1–§4 was confirmed against the jlcparts mirror on 2026-08-16.
