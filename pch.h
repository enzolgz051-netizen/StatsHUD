# 🎮 StatsHUD — BakkesMod Plugin

Affiche en bas à droite de l'écran :
- **Boost restant** avec barre colorée (vert / jaune / rouge)
- **Win/Loss streak** avec couleur dorée ou rouge

---

## 📋 Prérequis

| Outil | Lien |
|---|---|
| BakkesMod | https://bakkesmod.com |
| Visual Studio 2022 | https://visualstudio.microsoft.com (avec workload **C++ Desktop**) |
| CMake 3.15+ | https://cmake.org |
| BakkesModSDK | https://github.com/bakkesmodorg/BakkesModSDK |

---

## 🛠️ Installation pas à pas

### 1. Télécharger le SDK BakkesMod

```
git clone https://github.com/bakkesmodorg/BakkesModSDK.git
```

Place le dossier quelque part (ex: `C:\BakkesModSDK`)

---

### 2. Configurer avec CMake

Ouvre un terminal dans le dossier `StatsHUD` et tape :

```bash
mkdir build
cd build
cmake .. -DBAKKESMOD_SDK_PATH="C:/BakkesModSDK"
```

---

### 3. Compiler avec Visual Studio

```bash
cmake --build . --config Release
```

Ou ouvre `build/StatsHUD.sln` dans Visual Studio et compile en **Release x64**.

---

### 4. Installer le plugin

Le `.dll` est copié automatiquement dans :
```
C:\Users\TON_NOM\AppData\Roaming\bakkesmod\bakkesmod\plugins\
```

Sinon copie manuellement `StatsHUD.dll` dans ce dossier.

---

### 5. Charger dans BakkesMod

Dans Rocket League, ouvre la console BakkesMod (`F2`) et tape :
```
plugin load StatsHUD
```

Pour charger automatiquement au démarrage, va dans :
`BakkesMod → Plugins → Plugin Manager → StatsHUD → ✅ Load on startup`

---

## ⚙️ Commandes Console

| Commande | Description |
|---|---|
| `statshud_enabled 1` | Activer le HUD |
| `statshud_enabled 0` | Désactiver le HUD |
| `statshud_scale 1.5` | Agrandir le HUD (défaut: 1.0) |
| `statshud_scale 0.8` | Réduire le HUD |

---

## 🎨 Aperçu du design

```
┌─────────────────────────┐  ← barre accent bleue
│  BOOST                  │
│  87%                    │  ← vert/jaune/rouge selon niveau
│  ████████████░░░░░      │  ← barre de boost
│  ─────────────────────  │
│  WIN STREAK        3W   │  ← doré si win, rouge si loss
└─────────────────────────┘
```

---

## 🐛 Problèmes fréquents

**Le plugin ne charge pas ?**
→ Vérifie que le `.dll` est bien dans le dossier `plugins/`

**La streak ne se met pas à jour ?**
→ La streak ne fonctionne qu'en **matchs en ligne** (Ranked / Casual)

**Le HUD ne s'affiche pas ?**
→ Tape `statshud_enabled 1` dans la console BakkesMod

---

## 📁 Structure des fichiers

```
StatsHUD/
├── StatsHUD.h          ← Déclaration de la classe
├── StatsHUD.cpp        ← Logique principale + rendu
├── pch.h               ← Headers précompilés
├── CMakeLists.txt      ← Configuration de compilation
└── README.md           ← Ce fichier
```
