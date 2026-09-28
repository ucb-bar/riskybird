# riskybird v3 — Off-BOM / Hand-Populate Parts to Order

Components **not** on the JLC assembly order (`bom_riskybirdv3_order_1.xls`) that
must be hand-added to finish the boards. Quantities assume **5 boards** built.
Preferred vendors: Mouser / Digikey (exceptions noted).

## Order list

| Ref | Part | Qty (5 bd + spares) | Vendor / P/N | Notes |
|---|---|---|---|---|
| **IC1** | PMW3901MB-TXQT optical-flow sensor **(chip + lens)** | **~12–15** (5 + rework spares) | **NOT at Mouser/Digikey** → **LCSC `C43496881`** (~822 stk) | 28-pin COB/LGA, bottom-terminated → **hot-air rework** (see below). Was DNP on the PCB (BU-001), so JLC didn't place it. **The LCSC SKU includes the loose `LN03-ZSZ` lens** (its product image shows chip + lens); on the prior JLC run, ordering only this part got the lens assembled too. Since IC1 is now hand-populated, **you glue the lens on yourself after soldering the chip** (no focusing needed). No separate lens order required. |
| **U14 antenna** | 2.4 GHz antenna, **U.FL / IPEX MHF1** | **6** (5 + 1) | **LCSC `C1693673`** (HJ-2.4G-**IPEX1**-L=80mm, ~121 stk) — same-order as the PMW3901; alt LCSC `C1693678` (2.4G IPEX PCB ant). Digikey alts: Taoglas `FXP75.07.0045B` (flat-patch) / TE `2344656-1` (FPC). | ESP32-C6-MINI-**1U** has a U.FL/MHF1 RF connector; module ships with **no** antenna. `C1693673` is IPEX-1 (= MHF1/U.FL) with an 80 mm pigtail. **Verify the module connector is MHF1 (not MHF4) and that 80 mm reaches your mount; check the LCSC image for form factor (flex/FPC vs external).** |

### Not needed / already covered
- **R46 (0R):** DNP **by design** (config jumper) — do not order.
- **Camera (HM01B0 module), 4× VL53L5CX ToF breakouts:** external modules that
  mate to on-board connectors (HM01B0 conn, J3–J6). Source separately if you don't
  already have them — tell me and I'll add concrete P/Ns.

### PMW3901 sourcing note
The raw PMW3901MB-TXQT is a PixArt part **not stocked by Mouser or Digikey**
(Octopart shows no distributor stock). A breakout (Pimoroni PIM453, Pesky Products)
is **not** a substitute — IC1 is a raw footprint with a **downward optical
aperture**, so a module can't replace it without breaking the optical geometry.

**Sourcing ~10 pcs to the US (short turnaround), ranked:**
1. **LCSC `C43496881` (primary)** — genuine PixArt, **~822 in stock, ~$3.78 ea**,
   loose cut-tape (no MOQ). Ships Shenzhen → US via DHL/FedEx **express ~4–7 days**
   (pick express, ~$20–30 ship). At this price, order **12–15** for rework spares.
2. **WPG Americas** (`eshop.wpgam.com`) — **authorized PixArt distributor in the
   Americas**; potentially fastest *domestic* + guaranteed genuine, but B2B (expect
   an RFQ/account, possible MOQ). Good if you want authorized/US.
3. **Utsource** (or netcomponents/DigiPart brokers) — in stock, ~2–5 day US
   delivery, but **broker = counterfeit/re-mark risk** (PixArt sensors are faked).
   Only if LCSC is too slow and prototype-grade authenticity is acceptable.

PixArt-direct is MOQ/lead-time unfriendly for small qty.

## Soldering the PMW3901 (IC1) — assessment

**Package:** 28 perimeter pads **0.35 × 0.70 mm at 0.65 mm pitch** (≈5.3 mm ring)
**+ one 1.9 × 2.4 mm center ground pad**, all on the **underside** (LGA / QFN-style,
no leads, no castellations), with an integrated lens on top. It sits on the
**bottom** side of the board.

**Verdict: hand-iron soldering is NOT feasible — plan on hot-air rework.**
- The pads are *under* the body: an iron tip can't reach the perimeter pads, and
  the center ground pad is completely inaccessible. There are no exposed
  leads/edges to drag-solder.

Two-stage assembly (same as JLC did, but by hand): **(A) solder the bare chip,
then (B) glue the lens on.**

**A) Solder the chip:**
1. **Paste:** apply solder paste to all 29 pads — ideally a **small stencil for
   IC1** (or a bottom-side board stencil); manual dab-per-pad works but is fussy at
   0.65 mm pitch. Don't forget the center pad (thermal/GND).
2. **Place** the bare chip with correct **pin-1 orientation**; leave the factory
   **protective film over the optical window ON** through reflow so the die stays
   clean, and avoid flux on the optical area.
3. **Preheat** the board (hot plate / preheater) to cut thermal shock, then
   **hot-air reflow** to a lead-free profile (~245–250 °C peak). Localize heat and
   shield neighboring bottom-side parts (kapton).
4. Bottom-side complication: support the board on its tall top-side parts (B2B
   connectors, ESP module) or work it lens-down on a fixture; a hot-plate-from-top
   + hot-air approach also works. Clean flux residue afterward.

**B) Fit the lens (the loose `LN03-ZSZ` from the same LCSC pack):**
5. After soldering, **peel the protective Kapton** (use the peel groove) and
   **glue the lens onto the chip's top gluing slots**, aligned to the pad-1 /
   optical-center marks. **No focusing required** (per datasheet). Keep glue out of
   the optical aperture.

**If you have no hot-air:** the chip is reflow-rated (MSL) — a hot-plate + paste
reflow of the whole bottom side is an alternative for stage A. A hand-iron alone
will not do this part. Order **2–3× spares** and consider practicing on a scrap
first. The lens (stage B) is a simple glue step regardless.
