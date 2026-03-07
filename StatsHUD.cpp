name: Build StatsHUD Plugin

on:
  push:
    branches: [ main, master ]
  workflow_dispatch: # bouton "Run workflow" manuel

jobs:
  build:
    runs-on: windows-latest

    steps:
      # 1. Récupère ton code
      - name: Checkout code
        uses: actions/checkout@v4

      # 2. Installe CMake
      - name: Setup CMake
        uses: jwlawson/actions-setup-cmake@v1
        with:
          cmake-version: '3.26'

      # 3. Télécharge le BakkesModSDK
      - name: Download BakkesModSDK
        run: |
          git clone https://github.com/bakkesmodorg/BakkesModSDK.git C:/BakkesModSDK

      # 4. Configure le projet
      - name: Configure CMake
        run: |
          mkdir build
          cmake -S . -B build -DBAKKESMOD_SDK_PATH="C:/BakkesModSDK" -A x64

      # 5. Compile en Release
      - name: Build
        run: |
          cmake --build build --config Release

      # 6. Rend le .dll téléchargeable
      - name: Upload DLL artifact
        uses: actions/upload-artifact@v4
        with:
          name: StatsHUD-plugin
          path: build/Release/StatsHUD.dll
          if-no-files-found: error
