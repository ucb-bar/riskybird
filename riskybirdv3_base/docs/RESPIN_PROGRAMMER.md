# RESPIN — Programmer / Debug Board (`riskybirdv3_connector`)

Audit of the v3 programmer/debug board against the **known-working v2 FTDI block**
(`riskybirdv2_sensor`, ref **U6**) and the **real FT4232H silicon** that the boards are
actually populated with. Produced 2026-08-15.

**Evidence base**
- Netlists exported read-only to scratchpad: `conn.net` (this board), `v2.net` (`riskybirdv2_sensor`).
- FT4232H pinout: **official FTDI datasheet FT_000060 v2.2** (Tables 3.1/3.2/3.3/3.5), fetched this session
  (`…/tool-results/webfetch-1786777420840-lipif2.pdf`, pages 9–12).
- FT2232H pinout: the schematic symbol's own `pinfunction` labels (they match the FT2232H datasheet 1:1).
- Living log: `riskybirdv3_base/docs/ECO-001_3v3_power_redesign.md` §9.1–§9.8.
- Memory: `riskybird-v3-redesign.md` (FT4232H plan), `riskybird-v3-fpga-bringup.md` (this session's FTDI findings).

**KiCAD files are READ-ONLY** — nothing below was edited into the schematic; this is the change list for the respin.

---

## 0. TL;DR — what must change for the respin

| # | Sev | Item |
|---|-----|------|
| 1 | **CRITICAL** | Symbol/BOM say **FT2232HL** but the silicon is **FT4232H** (quad). Ch-A JTAG works by luck (shared pins); everything on the other channels is on the **wrong FT4232H channel**. Pick the chip for real and redraw the symbol. |
| 2 | **CRITICAL** | Board is **two complete FT2232H programmer blocks** (U9+U1+J1+Y3, U15+U2+J2+Y2) = literal ×2 copies of the v2 U6 block. As FT4232H that is **8 channels for a 2-JTAG-+-2-UART job**. Consolidate to **one FT4232H** (the documented plan) or commit to **2× genuine FT2232H**. (There are **only two** FTDIs, not three.) |
| 3 | IMPORTANT | Three power/USB regressions vs v2 (VPLL, VPHY, USB series-R) — already bench-bodged; bake the v2 topology into the respin. |
| 4 | IMPORTANT | **`+3V3` LED-supply net is orphaned (undriven)** → all six status LEDs are dead. And the FTDI can't drive them anyway (no EEPROM; FT4232H pins have no LED function). |
| 5 | IMPORTANT | **No EEPROM** anywhere → default VID/PID, `iSerial 0` (two chips + multiple boards are USB-indistinguishable by serial), no CBUS/LED config. |
| 6 | IMPORTANT | The two `/FTDI_3v3_x` domains are **hard-tied to the drone's 3V3 through J3** → 3 regulators in parallel when mated, and the "two independent VTRef domains" are actually shorted together. Move to a VTRef-sense scheme. |
| 7 | MINOR | Reset asymmetry (U9 exposes nTRST, U15 exposes nSRST; each floats the other), RESET# hard-tie vs v2's pull-up, cross-labeled crystals, an empty `/` net, thin VBUS bulk cap. |

---

## 0a. DECISION (2026-08-17) — keep FT2232HL this spin; defer the FT4232H rework

**Staying on FT2232HL (`C27882`) for this respin.** The received chips are FT4232H silicon remarked into
FT2232HL packages (§1), but they **work for the FPGA-config JTAG path** (channel A) — the design intent —
so we accept the current parts rather than gamble on another remarked reel. Confirmed C27882 = genuine
FT2232HL and the *different* genuine FT4232H part is **`C56742`** (FT4232HL-Reel).

**⇒ §1 and §2 (redraw as true FT4232H + consolidate to one chip) are DEFERRED** to a later full FT4232HL
rework. At that rework: switch the LCSC to **`C56742`**, set value/MPN to `FT4232HL`, draw the real
FT4232H symbol, and re-map nTRST/nSRST onto the JTAG's own channel.

**In scope for THIS spin (all FT2232HL-valid):**
- **§3** — VPLL 1 kΩ → **600R ferrite (GZ1608D601TF / `C1002`)** + 4.7 µF/100 nF; VPHY ferrite + caps; **USB D± 22 Ω → 0 Ω.** *(the enumeration/SI fixes — the important ones.)*
- **§5** — fix the orphaned `+3V3` LED-supply net.
- **§6** — optional EEPROM (wiring guide §6.1) for unique serials.

**Accepted limitation on the remarked silicon:** nTRST/nSRST land on a separate USB interface (§1.3), so
the adapter can't toggle target resets directly — tolerable for FPGA config (TAP reset via TMS); revisit
at the FT4232HL rework.

---

## 1. CRITICAL — FT2232H → FT4232H symbol reconciliation + channel→function map

### 1.1 The mechanism
Package is LQFP-64 for **both** FT2232H and FT4232H, and the **power / USB / clock / config**
pins are identical between them — that is why the core works and why **Channel A JTAG works unchanged**.
But the **data-bus pins are re-partitioned**: FT2232H is 2 channels each 16-bit (`ADBUS`+`ACBUS`,
`BDBUS`+`BCBUS`); FT4232H is 4 channels each 8-bit (`ADBUS`,`BDBUS`,`CDBUS`,`DDBUS`). Same pin numbers,
different silicon behind them.

**Authoritative FT4232H data-pin map (datasheet Table 3.1):**
- Channel A = pins **16,17,18,19,21,22,23,24** — MPSSE-capable
- Channel B = pins **26,27,28,29,30,32,33,34** — MPSSE-capable
- Channel C = pins **38,39,40,41,43,44,45,46** — UART / bit-bang only (no MPSSE)
- Channel D = pins **48,52,53,54,55,57,58,59** — UART / bit-bang only (no MPSSE)

On **FT2232H** those very pins are, respectively, Channel-A-low (`ADBUS`), **Channel-A-high (`ACBUS`)**,
Channel-B-low (`BDBUS`), **Channel-B-high (`BCBUS`)**.

### 1.2 What the schematic actually wired vs where it lands on FT4232H

U9 and U15 are wired **identically** (nets differ only by `_1`/`_2`). Table for one chip (U9):

| Pin | Net (→ J3) | FT2232H symbol pin | **Real FT4232H pin** | Real ch | Verdict |
|----:|------------|--------------------|----------------------|:------:|--------|
| 16 | `/TCK_1` (J3.12) | ADBUS0 | **ADBUS0 = TCK** | A (MPSSE) | ✅ correct — JTAG works |
| 17 | `/TDI_1` (J3.13) | ADBUS1 | ADBUS1 = TDI | A | ✅ |
| 18 | `/TDO_1` (J3.9) | ADBUS2 | ADBUS2 = TDO | A | ✅ |
| 19 | `/TMS_1` (J3.14) | ADBUS3 | ADBUS3 = TMS | A | ✅ |
| 26 | `/NTRST_1` (J3.16) | ACBUS0 | **BDBUS0 (ch B)** | B | ⚠️ lands on a *different USB interface* than the JTAG |
| 27 | `/NSRST_1` (floating) | ACBUS1 | **BDBUS1 (ch B)** | B | ⚠️ + net dangles (not on J3) |
| 29 | `/JTAG_LED1` | ACBUS3 | **BDBUS3 (ch B)** | B | ⚠️ LED on a ch-B data pin |
| 38 | `/TXD_1` (J3.18) | BDBUS0 | **CDBUS0 = TXD (ch C)** | C | △ works as UART, but ch **C**, not B |
| 39 | `/RXD_1` (J3.15) | BDBUS1 | **CDBUS1 = RXD (ch C)** | C | △ |
| 54 | `/RX_LED1` | BCBUS3 | **DDBUS3 (ch D)** | D | ⚠️ LED on a ch-D data pin |
| 55 | `/TX_LED1` | BCBUS4 | **DDBUS4 (ch D)** | D | ⚠️ LED on a ch-D data pin |

**Net effect on the real FT4232H, per chip:**
- **Interface A** = TCK/TDI/TDO/TMS = **JTAG (MPSSE)** → *works* (this is the proven path, IDCODE `0x03636093`).
- **Interface B** = nTRST(BDBUS0) + nSRST(BDBUS1) + JTAG_LED(BDBUS3) → a whole MPSSE-capable channel
  spent on two resets + an LED, **on a separate USB interface from the JTAG**.
- **Interface C** = TXD/RXD = **UART console** → *works*, but it is interface #2 (UART-only channel), not "channel B".
- **Interface D** = RX_LED + TX_LED → two LEDs on a UART channel.

### 1.3 The concrete functional damage (beyond "it enumerated")
1. **nTRST / nSRST cannot be driven from the JTAG adapter interface.** On a real FT2232H, `ACBUS0/1`
   are the *high byte of the same Channel A* as the JTAG, so OpenOCD `ftdi layout_signal nTRST/nSRST`
   toggles them on the JTAG MPSSE engine. On FT4232H there is **no ACBUS** — Channel A is only 8 pins,
   all consumed by TCK/TDI/TDO/TMS. The reset nets sit on **Channel B**, a different interface the JTAG
   driver does not own. To assert them you must open interface B separately and bit-bang. Default
   openFPGALoader/OpenOCD configs won't touch them. Tolerable for FPGA config-JTAG (TAP reset via TMS),
   **potentially blocking for RISC-V SoC debug** where system reset (nSRST) is often wanted.
2. **The natural home for the resets is unused.** Pins **21–24 (ADBUS4-7 = ch-A GPIOL0-3)** are left
   `unconnected`. On FT4232H those are exactly where nTRST/nSRST belong so they ride the same MPSSE
   interface as JTAG. The design put resets on ch B instead.
3. **Reset lines sit on UART-default output pins.** At power-up FT4232H ch B defaults to UART; `BDBUS0`
   idles as TXD (driven). So `/NTRST_1` (→ target TRST) is a driven output at reset — benign while ch B is
   closed, but any accidental open-as-UART of ch B would glitch the target's TRST.

### 1.4 Recommended respin — consolidate to ONE FT4232H (matches the documented plan)
The plan (`riskybird-v3-redesign.md`): *"one FT4232H … ch A/B = MPSSE JTAG, C/D = UART; replaces two
FT2232H."* The received silicon already **is** FT4232H, and a single one covers the whole requirement:

| Channel | Pins | Function | Notes |
|---|---|---|---|
| **A** (MPSSE) | ADBUS0-3 = 16-19; **nTRST→ADBUS4(21), nSRST→ADBUS5(22)** | **FPGA config JTAG** | resets now on the *same* interface, OpenOCD-drivable |
| **B** (MPSSE) | BDBUS0-3 = 26-29; resets on BDBUS4/5 = 30/32 | **RISC-V SoC JTAG** | the "separate JTAG bus" the Chipyard shell needs |
| **C** (UART) | CDBUS0-1 = 38-39 | **console UART #1** (e.g. RISC-V) | |
| **D** (UART) | DDBUS0-1 = 48-52 | **console UART #2 / aux** | or bit-bang the status LEDs here |

Draw a **true FT4232H symbol** (LQFP-64; a QFN-56 variant also exists — pick the package you'll buy) and
**re-verify every non-power net against Table 3.1** (do not reuse the FT2232H symbol). One USB-C, one
chip, half the BOM. Buy **correctly-marked FT4232H from an authorized distributor** (Digikey/Mouser) —
the current reel is remarked "FT2232HL" (genuine FT4232H die, but a sourcing red flag; ECO §9.5).

**Low-risk alternative** — if you'd rather not redraw: commit to **2× genuine FT2232H** (correctly
marked) and keep the schematic verbatim. It is exactly the proven v2 U6 block ×2, so ch-A JTAG + ch-B
UART + ACBUS resets all behave as they did on v2. Cost: two chips, two USB cables, an older/pricier part,
and it walks away from the documented FT4232H direction.

---

## 2. CRITICAL — architecture: two programmer blocks (only two, not three)

The netlist contains **exactly two** FTDI instances — **U9** and **U15** (the "third chip" in the brief
does not exist; the "6× FT2232HL-REEL" grep hits were symbol-library sub-units). Each FTDI is the centre
of a **complete, independent programmer**:

| Block | FTDI | LDO | USB-C | Resonator | Rail | To drone |
|---|---|---|---|---|---|---|
| #1 | **U9** | U1 (AP2112K) | **J1** + F1 fuse | **Y3** | `/FTDI_3v3_1` | J3.10 |
| #2 | **U15** | U2 (AP2112K) | **J2** + F2 fuse | **Y2** | `/FTDI_3v3_2` | J3.2 |

Both blocks feed the single **J3** ribbon to the drone (see §4). This is **two verbatim copies of the v2
U6 block**. The design intent is coherent *for FT2232H*: 2 chips × 2 channels = the 4 channels the job
needs (FPGA-JTAG + RISC-V-JTAG + 2 consoles). But populated with **FT4232H it is 8 channels**, of which
only ~4 are used (ch A + ch C per chip). This is the same requirement a **single FT4232H** satisfies — so
§1.4's consolidation both fixes the symbol scramble and removes the redundant second chip, LDO, USB-C,
fuse, resonator, and LED cluster.

---

## 3. IMPORTANT — power/USB block vs the v2 golden reference

Full netlist diff of the FTDI support components, U9/U15 vs **v2 U6**:

| FT2232H pin | v2 U6 (WORKING) | v3 U9/U15 | Status |
|---|---|---|---|
| **VPLL (9)** | 3V3 → **FB1 600R@100 MHz ferrite** → VPLL, **C28 4.7 µF + C27 100 nF** | 3V3 → **R66/R67 1 kΩ** → VPLL, **no cap** | ❌ enumeration-killer (ECO §9.2). Bench-bodged: R66/R67 shorted. |
| **VPHY (4)** | 3V3 → **FB2 600R ferrite** → own net, **C30 4.7 µF + C29 100 nF** | tied **direct** to shared `/FTDI_3v3_x`, no ferrite, no local cap | ❌ lost LC filter |
| **D+/D− (8/7)** | **direct** to USB (J8) | **22 Ω series** R8/R5 (U9), R9/R6 (U15) | ❌ breaks HS termination. Bench-bodged: shorted. |
| **REF (6)** | R? **12 kΩ** → GND | R64/R65 **12 kΩ** → GND | ✅ identical |
| **VREGOUT (49)** | C32 3.3 µF + C33 100 nF (own net) | C50/C51 3.3 µF + 100 nF, tied to VCORE | ✅ cap present. Topology differs (v3 ties VREGOUT→VCORE = standard bus-powered; v2 ran VCORE off an external +1V8). Both valid. |
| **VREGIN (50)** | +3V3 | `/FTDI_3v3_x` | ✅ |
| **RESET# (14)** | **R26 1 kΩ pull-up** to +3V3 | **hard-tied** to `/FTDI_3v3_x` | △ works (held high); v2's pull-up is the nicer practice — see §6 |
| **OSC (2/3)** | Y1 ceramic resonator (CSTNE), no ext caps | Y3 (U9) / Y2 (U15), same resonator style | ✅ consistent |

**Respin power/USB fixes (copy v2 verbatim):**
- VPLL: `1 kΩ → 600R@100 MHz ferrite`, add **4.7 µF + 100 nF** to GND at VPLL.
- VPHY: add a **600R@100 MHz ferrite** from 3V3 + **4.7 µF + 100 nF** at VPHY (give it its own filtered net, not the shared rail).
- USB D+/D−: **22 Ω → 0 Ω / DNP-bridge** (FT4232H has internal HS termination; v2 wires direct).

**Concrete parts (ready to place) — the v2 U6 filter, one copy per rail.**
The v2 schematic specifies the ferrite *generically* (`600R@100MHz/0.5A`, 0603, no MPN — refs FB1=VPLL, FB2=VPHY, each with 4.7 µF + 100 nF). Pin it to a real JLC-assemblable part:

| Component | Value / spec | Part | LCSC | Footprint | JLC |
|---|---|---|---|---|---|
| Ferrite (VPLL + VPHY) | 600 Ω@100 MHz, ≥0.5 A, DCR ~0.45 Ω | Sunlord **GZ1608D601TF** (1.2 A rated) | **C1002** | 0603 (`L_0603_1608Metric`) | **Basic** |
| Local bulk cap | 4.7 µF X5R/X7R ≥6.3 V | (any Basic 0402/0603) | — | 0402/0603 | Basic |
| Local HF cap | 100 nF X7R ≥6.3 V | (any Basic 0402) | — | 0402 | Basic |

Topology per rail (copy from v2 U6): `/FTDI_3v3 → ferrite → {VPLL or VPHY pin} `, and at the pin place **4.7 µF ∥ 100 nF to GND**. Give VPHY its **own filtered node** (do not share the raw rail).

**Per-chip FTDI-power BOM delta** (×2 chips U9, U15 — or ×1 if consolidating to a single FT4232H per §1.4):
- 2× ferrite **C1002** (one VPLL, one VPHY)  ← *replaces* the 1 kΩ R66/R67
- 2× 4.7 µF + 2× 100 nF (VPLL + VPHY local decoupling)  ← VPLL currently has none
- USB D± series R (R5/R6 or R8/R9): **22 Ω → 0 Ω** (2 parts/chip)

USB-C front end is otherwise correct: VBUS (A4/B4/A9/B9 all tied) → F1/F2 (6 V 2 A) → `/5V_x` → AP2112K
(EN tied to VIN = always on); **CC1/CC2 each have their own 5.1 kΩ Rd** (R1/R3, R2/R4) = correct UFP
config; **D+/D− tied across both flips** (A6=B6, A7=B7) and **not swapped** (DP→D+, DM→D−).

---

## 4. J3 drone-interface connector

`J3` = `Conn_02x10_Odd_Even` (20-pin ribbon header). Full map (from `conn.net`):

| J3 | Net | Source | | J3 | Net | Source |
|---:|-----|--------|---|---:|-----|--------|
| 1 | GND | | | 2 | **`/FTDI_3v3_2`** | U15 rail |
| 3 | GND | | | 4 | `/TCK_2` | U15 JTAG |
| 5 | `/TDO_2` | U15 | | 6 | `/TMS_2` | U15 |
| 7 | `/NSRST_2` | U15 | | 8 | `/TDI_2` | U15 |
| 9 | `/TDO_1` | U9 | | 10 | **`/FTDI_3v3_1`** | U9 rail |
| 11 | GND | | | 12 | `/TCK_1` | U9 JTAG |
| 13 | `/TDI_1` | U9 | | 14 | `/TMS_1` | U9 |
| 15 | `/RXD_1` | U9 UART | | 16 | `/NTRST_1` | U9 |
| 17 | `/RXD_2` | U15 UART | | 18 | `/TXD_1` | U9 UART |
| 19 | GND | | | 20 | `/TXD_2` | U15 UART |

So J3 carries **two JTAG domains + two UARTs + two 3V3 rails + 4 GND**. All 20 pins used.

### 4.1 The two 3V3 domains are hard-tied to the drone (IMPORTANT)
`/FTDI_3v3_1` (U9's whole rail: VCCIO ×4, VPHY, VREGIN, RESET#, fed by U1 LDO off J1-VBUS) goes out on
**J3.10**; `/FTDI_3v3_2` likewise on **J3.2**. On the drone both land on the drone's **+3V3** (ECO §9.4/§9.6).
Consequences:
- **Regulators in parallel.** With everything plugged in, **U1 + U2 + the drone's LTC3119** all drive one
  3V3 node. ECO §9.6 saw them "coexist" only because all sit at ~3.3 V — a fragile equilibrium, not a
  design intent. A small mismatch back-drives an LDO.
- **The two VTRef domains are actually one.** `/FTDI_3v3_1` and `/FTDI_3v3_2` are shorted together *through*
  the drone's 3V3 (J3.10 ↔ drone +3V3 ↔ J3.2). Any intent to run the FPGA-config JTAG and the RISC-V JTAG
  at **different** target voltages is defeated — they are forced equal. (Moot today: the plan runs
  everything at 3.3 V, so a single VTRef suffices — but then two rails buy nothing.)
- **Back-powering hazard.** Plug the programmer's USB with the drone attached-but-unpowered and the 600 mA
  AP2112K tries to energize the drone's entire 3V3 rail through J3 → brownout/over-current.

**Respin:** make the FTDI **VCCIO reference the target VTRef** (sense J3.2/J3.10 as a high-impedance
reference / feed VCCIO from the target), and power only the FTDI **core/PLL/PHY** (VREGIN/VPLL/VPHY) from
the local USB LDO so the chip still enumerates with the target unplugged (ECO §9.4). If two genuinely
independent target voltages are ever needed, one FT4232H can still do it — feed **ch-A VCCIO (pin 20)** and
**ch-B VCCIO (pin 31)** from the two separate target VTRefs. Do **not** drive target 3V3 from the
programmer LDO.

### 4.2 JTAG signal integrity / connector
- **No series or pull resistors** on any JTAG line (`/TCK*,/TMS*,/TDI*,/TDO*` are point-to-point FTDI↔J3);
  the design relies on the target for TAP pull-ups. Acceptable for short ribbon at MPSSE speeds; if SI is
  marginal, add small (22–33 Ω) series at the FTDI and/or TMS/TCK pull-ups referenced to VTRef.
- **Connector type vs the plan.** The plan wants a fine-pitch, high-mating-cycle debug connector
  (Samtec **FTSH 2×10 1.27 mm** or **Tag-Connect**). `J3` here is a generic 2×10 header driven over a
  ribbon — fine for the programmer↔drone ribbon, but confirm the **drone-side** debug land matches the
  high-cycle plan, and consider swapping J3 to latching FTSH-2×10 (pin count already matches: 20).

---

## 5. IMPORTANT — status LEDs are dead two ways

1. **Orphaned supply net (real bug).** All six LED anode resistors (R41–R44, R49, R63, 150 Ω) tie to a
   net literally named **`+3V3`** — which has **no driver**: its only nodes are those six resistors. It
   never bonds to `/FTDI_3v3_1`, `/FTDI_3v3_2`, or `/5V_x`. (A stray global `+3V3` label was copied from
   the drone schematic, where `+3V3` is the main rail; here the rails are `/FTDI_3v3_x`, so the label
   floats.) **The LEDs D14–D19 have no anode supply and cannot light.** Fix: connect `+3V3` to the local
   3V3 rail (`/FTDI_3v3_x`).
2. **Even if powered, the FTDI can't drive them.** They are wired as FT2232H CBUS LED-driver outputs
   (`ACBUS3`, `BCBUS3/4`) — which require **EEPROM** configuration (see §6) to become TX/RX/JTAG activity
   indicators. There's no EEPROM. And on the **FT4232H** silicon those pins are plain ch-B/ch-D data pins
   with **no LED-indicator function at all**. To keep activity LEDs on an FT4232H design, bit-bang them
   from a spare UART channel (e.g. ch D) or drop them.

---

## 6. IMPORTANT — no EEPROM

Neither v2 nor v3 fits a config EEPROM: `EECS(63)/EECLK(62)/EEDATA(61)` are tied to 3V3 on both U9 and
U15 (ECO §9.5 confirms the running chip reports `iSerial 0`, silicon defaults). Consequences on this
**two-chip FT2232H** board:
- **No unique serial** → the two on-board FTDIs, and any two boards on the same bench, are
  **indistinguishable by serial** to the host; you must select by USB bus/port path. Annoying for a
  multi-target JTAG rig.
- **No custom VID/PID or product strings.** A genuine FT2232H enumerates as `0403:6010` "Dual RS232-HS" —
  but note the **remarked silicon on these boards actually reports as FT4232H `0403:6011` "Quad RS232-HS"**,
  which is what `ftdi_eeprom`/FT_PROG will detect when you program (see the programming steps below).
- **No CBUS/LED configuration** (§5.2).

**Respin (recommended for a polished programmer):** add a **93‑series Microwire ×16 EEPROM** (in‑stock
choice: **93LC66C**, MSOP‑8 — see Part below) on EECS/EECLK/EEDATA so you can FT_PROG a unique serial,
product string, and — on a real FT2232H path only — LED functions. Optional if you're happy addressing by
port and dropping the LEDs. **One EEPROM per FTDI.**

**Part (in stock — chosen for this build):** Microchip **93LC66C‑I/MS = LCSC `C1350907`** (MSOP‑8, 4 Kbit).
- ⚠️ The "**C**" variant is **org‑selectable** → **ORG (pin 6) must be tied to VCC** for ×16 (see the table). The fixed‑×16 "B" MSOP parts (C1351729 etc.) were out of stock.
- Fixed‑×16 "B" alternative (ORG = NC, no strap): SOIC‑8 `93LC56B‑I/SN` = **C183620** — requires a SOIC‑8 footprint.

### 6.1 Wiring guide (instantiate once per FTDI — one on U9, one on U15)

**FT2232H EEPROM pins** (LQFP-64): **EECS = pin 63, EECLK = pin 62, EEDATA = pin 61.** (These config pins are
identical on the FT4232H, so this wiring carries over unchanged if you do the FT4232HL rework later.)
On the current schematic all three are **strapped to 3V3** on both chips (§6 intro) — **remove those straps first**, then wire the 93LC56B per the table.

| 93LC56B pin | Name | Connect to | Notes |
|---:|---|---|---|
| 1 | CS | FT2232H **EECS (pin 63)** | chip select |
| 2 | CLK | FT2232H **EECLK (pin 62)** | serial clock |
| 3 | DI | FT2232H **EEDATA (pin 61)** | direct |
| 4 | DO | FT2232H **EEDATA (pin 61)** via **2.2 kΩ** series R | DO shares the EEDATA line through the resistor (FTDI standard) |
| 5 | VSS | GND | |
| 6 | ORG | **VCC** (`/FTDI_3v3_x`) | 93LC66C is org‑selectable → **tie ORG high = ×16** (a fixed‑×16 "B" part would be NC) |
| 7 | NC / PE | **NC** | |
| 8 | VCC | filtered **3V3** (`/FTDI_3v3_x`) + **100 nF** to GND | put the 100 nF right at pin 8 |

```
   FT2232H (U9 / U15)              93LC56B  (one per FTDI)
    EECS   (63) ──────────────────► CS  (1)
    EECLK  (62) ──────────────────► CLK (2)
    EEDATA (61) ───────┬──────────► DI  (3)
                       └──[2.2k]──◄─ DO  (4)
    /FTDI_3v3 ──┬───────────────────► VCC (8)
                └──[100nF]── GND
    GND ─────────────────────────── VSS (5)
                                     ORG(6) → VCC   PE(7) = NC
```

Only **EEDATA carries both directions**: it drives DI directly and reads DO back through the 2.2 kΩ (so the two never contend). **Instantiate this block twice** — one 93LC56B per FTDI — so each chip (hence each board and each JTAG channel) gets its own unique USB serial.

**Per-chip EEPROM BOM delta** (×2 chips, or ×1 if consolidated):
- 1× 93LC66C‑I/MS (**C1350907**, MSOP‑8; org‑selectable → ORG→VCC) [or fixed‑×16 93LC56B **C183620** SOIC‑8]
- 1× 2.2 kΩ 0402 (DO→EEDATA)
- 1× 100 nF 0402 (VCC decoupling)

**Programming it (in-circuit over USB — no extra HW).** Blank EEPROM → chip falls back to `0403:6011`
(the remarked-FT4232H default; a genuine FT2232H would be `0403:6010`), `iSerial 0` (today's collision). Program per chip:
- Linux (`ftdi_eeprom` from libftdi): write a `.conf` with `vendor_id/product_id/manufacturer/product` +
  `use_serial=true`, `serial="RB3-U15-0001"`; then `ftdi_eeprom --read-eeprom <conf>` (verify seen) →
  `ftdi_eeprom --flash-eeprom <conf>`; replug.
- Windows: **FT_PROG** GUI — auto-detect, set/auto-increment serial, Program.
- **Two-chip gotcha:** both-blank → both serial `"0"`, so target by USB path the first time
  (`ftdi_eeprom --device d:<bus>/<addr>`, get it from `lsusb -t`), give each a distinct serial, then
  `openFPGALoader --ftdi-serial <serial>` (or udev rules) can address them independently.

---

## 7. MINOR / cleanup

- **Reset asymmetry.** U9 routes **nTRST** to J3.16 and leaves **nSRST (pin 27)** floating; U15 routes
  **nSRST** to J3.7 and leaves **nTRST (pin 26)** floating. The two programmers expose *different* reset
  signals to the target, and each dangles the other. Make the reset scheme deliberate and symmetric
  (and put both on the JTAG interface's own channel per §1.4).
- **RESET# (pin 14)** is hard-tied to the 3V3 rail on v3 vs a **1 kΩ pull-up** on v2. Both hold RESET#
  high; the pull-up is marginally better (allows an RC/button and limits inrush). Restore the pull-up.
- **Crystal designators cross-labeled** (U9→Y3, U15→Y2). Electrically fine — each chip has its own
  resonator, no cross-wiring — cosmetic only.
- **Empty net `/`** (0 nodes) — a stray unnamed-label artifact; clean it up so ERC is quiet.
- **VBUS bulk cap is thin:** only **10 nF (C1/C2)** at the connector before the fuse; the 10 µF (C3/C5)
  sits *after* the fuse on `/5V_x`. Fine for a ~100 mA FTDI load, but add a small bulk cap at the USB-C
  VBUS if you want margin.
- **USB ESD:** no TVS on VBUS/CC/D± (dev board — low priority; add if field-handled).

---

## 8. Consolidated respin checklist

**Chip & symbol**
- [ ] Decide chip: **1× FT4232H** (recommended, matches plan) or **2× genuine FT2232H** (verbatim-v2 fallback).
- [ ] If FT4232H: **draw a true FT4232H symbol** and re-verify all data nets against datasheet Table 3.1.
- [ ] Buy correctly-marked parts from an authorized distributor (current reel is remarked).

**Channel assignment (FT4232H path)**
- [ ] Ch A MPSSE = FPGA config JTAG; **nTRST→ADBUS4(21), nSRST→ADBUS5(22)**.
- [ ] Ch B MPSSE = RISC-V SoC JTAG; resets on BDBUS4/5(30/32).
- [ ] Ch C UART = console; Ch D UART = aux console / LED bit-bang.
- [ ] Collapse block #2 (U15/U2/J2/F2/Y2 + its LED cluster) into the single FT4232H unless a 2nd USB host is truly required.

**Power / USB (copy v2)**
- [ ] VPLL: 600R ferrite (**GZ1608D601TF / C1002**) + 4.7 µF + 100 nF (drop the 1 kΩ). See §3.
- [ ] VPHY: same ferrite (**C1002**) + 4.7 µF + 100 nF on its own filtered net.
- [ ] USB D+/D−: 22 Ω → 0 Ω/DNP-bridge.
- [ ] RESET#: restore ~1 kΩ pull-up.

**Power domains / connector**
- [ ] Stop driving target 3V3 from the programmer LDO; make FTDI **VCCIO = target VTRef** (sense, not source).
- [ ] Power FTDI core/PLL/PHY from USB LDO so it enumerates with the target unplugged.
- [ ] Consider FTSH-2×10 latching for J3 / confirm drone-side high-cycle land.

**Loose ends**
- [ ] Fix the orphaned **`+3V3`** LED-supply net (bond to local 3V3) — or drop the LEDs.
- [ ] Add **93LC56B EEPROM** (**C183620** SOIC-8 / **C1351729** MSOP-8) per chip for unique serial + strings (+ LED functions if FT2232H). Wiring + programming in §6.
- [ ] Symmetric, deliberate nTRST/nSRST scheme; no floating reset pins.
- [ ] Delete stray `/` net; tidy crystal designators; optional VBUS bulk cap + ESD.

---

### Appendix — sources
- FT4232H pinout: FTDI **FT_000060 v2.2**, Tables 3.1 (pin config), 3.2 (power), 3.3 (common), 3.5
  (UART channels) — local `…/tool-results/webfetch-1786777420840-lipif2.pdf` pp. 9–12.
- FT2232H pin functions: the schematic symbol `pinfunction` labels (verified 1:1 against the FT2232H datasheet).
- Netlists: `…/scratchpad/conn.net`, `…/scratchpad/v2.net` (kicad-cli export, read-only).
- Prior findings: `ECO-001_3v3_power_redesign.md` §9.1–§9.8; memories `riskybird-v3-redesign.md`, `riskybird-v3-fpga-bringup.md`.
