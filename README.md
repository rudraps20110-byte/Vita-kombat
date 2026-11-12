# Vita Kombat: Realm of Twelve Warriors (PS Vita `.vpk`)

A Mortal Kombat-inspired 2D fighting game built specifically for PlayStation Vita (`960x544`, `vita2d`, `SceCtrl`, `SceAudio`), featuring an **MK9-style HD 2.5D visual overhaul** (micro-textured warrior armor & skin shading, real-time battle-damage wounds, persistent floor blood/oil/scorch decals, dynamic 2.5D camera zoom & parallax, steel-and-blood HUD, and layered bone-crunch audio), a **12-Chapter Manga Storybook Mode (~1 hour)** covering every warrior's origin, **12 unique playable characters** with **12 custom visual Finisher cinematics**, **X-Ray Super Art impact flashes**, **4-track procedural chiptune/arcade BGM** (`MENU`, `STORY`, `BATTLE`, `BOSS`) with Pause-menu Music/SFX toggles, organic 14-joint interpolated animations, chain combos, air juggles, **7 arenas**, **5 difficulty levels (Beginner → Legendary)**, Arcade Tower, VS CPU, and Training Dojo.

---

## 12 Playable Warriors & Special Abilities

| # | Fighter | Title | Special 1 (`R + □` / `D,F + □`) | Special 2 (`R + △` / `D,B + △`) | Finisher (`R + ○`) |
|---|---|---|---|---|---|
| 1 | **AGNI** | Inferno Ninja | **Hellfire Spear** (Burning projectile) | **Flame Uppercut** (Rising fire launcher) | Inferno Incineration |
| 2 | **HIMRAJ** | Cryo Assassin | **Ice Blast** (Freezes opponent 1.6s!) | **Glacier Slide** (Low ice sweep dash) | Frozen Oblivion |
| 3 | **VAJRA** | Thunder God | **Volt Bolt** (High-speed thunder sphere) | **Lightning Teleport** (Blinks behind foe) | Heaven's Electro-Strike |
| 4 | **KALIYA** | Shadow Wraith | **Shadow Clone** (Dark decoy lunge) | **Phantom Slash** (Invulnerable dash cut) | Abyssal Decapitation |
| 5 | **RUDRA** | Earth Colossus | **Seismic Slam** (Unblockable ground launch!) | **Titan Charge** (Armored shoulder ram) | Tectonic Crush |
| 6 | **AAKASH** | Storm Ronin | **Tempest Tornado** (Whirlwind air-juggle) | **Gale Katana Dash** (High-speed wind slice) | Cyclone Slice |
| 7 | **ZAHRA** | Venom Viper | **Acid Spit** (Poison orb + acid pool DoT) | **Viper Lunge** (Toxic leaping strike) | Corrosive Meltdown |
| 8 | **LOHA** | Cyber Enforcer | **Plasma Cannon** (Heavy laser blast) | **Rocket Gauntlet** (Armored jet punch) | Orbital Laser Barrage |
| 9 | **MAYA** | Blood Sorceress | **Crimson Drain** (Lifesteal blood orb!) | **Scarlet Spike** (Ground spike eruption) | Blood Moon Ritual |
| 10 | **KAAL** | Nether Overlord | **Soul Pillar** (Dark flame under enemy) | **Reaper Scythe** (Boomerang scythe) | Soul Harvest |
| 11 | **SURYA** | Solar Paladin | **Solar Flare** (Burning radiant sun orb) | **Radiant Dive** (Armored golden blade rush) | Solar Annihilation |
| 12 | **TARA** | Starlight Phantom | **Astral Shuriken** (Fast cosmic star blade) | **Void Blink** (Cosmic warp slash behind foe) | Celestial Obliteration |

---

## 7 Arenas & 5 Difficulty Levels
- **Arenas:** Hellfire Pit, Frozen Temple, Thunder Pagoda, Nether Throne, **Crimson Dojo**, **Cyber Foundry**, **Solar Citadel**
- **Difficulty Levels:** `Beginner` → `Amateur` → `Semi-Pro` → `Pro` → `Legendary`

---

## PS Vita Controls & Combos

- **Move / Jump / Crouch:** D-Pad or Left Analog Stick
- **Quick Punch:** `Square (□)`
- **Heavy Strike:** `Triangle (△)`
- **Kick:** `Cross (✕)`
- **Roundhouse Kick:** `Circle (○)`
- **Uppercut Launcher:** `Down + Triangle` (launches opponent for air juggles!)
- **Low Sweep:** `Down + Circle` (knockdown, must be blocked low)
- **Throw / Grab:** `Forward + Square` (close range, beats blocking)
- **Block:** Hold `L Trigger` (High Block) or `Down + L Trigger` (Low Block)
- **Dash / Backstep:** `R + Left/Right` or double-tap `Left/Right`
- **Special 1 / Special 2:** `R + Square` / `R + Triangle` (or `Down-Forward + Square` / `Down-Back + Triangle`)
- **Super Art (100% Meter):** `L + R`
- **Finisher (during `FINISH HIM!`):** `R + Circle`
- **Pause & In-Game Move List:** `START`

### Sample Combos
1. **Basic Cancel Combo:** `Square → Triangle → R + Square`
2. **Kick Chain:** `Square → Cross → Circle`
3. **Air Juggle Combo:** `Down + Triangle` (Uppercut Launch) → `Up/Forward + Cross` → `R + Triangle`

---

## How to Build `VitaKombat.vpk`

### Option 1: GitHub Actions (No local SDK needed!)
1. Push this folder to a GitHub repository.
2. Open the **Actions** tab → **Build VitaKombat VPK** runs automatically using `vitasdk/vitasdk:latest`.
3. Download the **`VitaKombat-vpk`** artifact containing `VitaKombat.vpk` (~9 MB).

### Option 2: Docker (Local)
```powershell
.\build-docker.ps1
```

### Installing on PS Vita
1. Copy `VitaKombat.vpk` to `ux0:/` via **VitaShell** (USB or FTP).
2. Highlight `VitaKombat.vpk` in VitaShell and press `Cross (✕)` to install.
3. Launch **Vita Kombat** from your LiveArea!

