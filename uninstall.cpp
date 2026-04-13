name: Build & Release

on:
  push:
    branches: [ "main" ]
    tags:
      - 'v*'
  pull_request:
    branches: [ "main" ]
  workflow_dispatch:

jobs:
  build:
    runs-on: windows-latest

    permissions:
      contents: write

    steps:
      - uses: actions/checkout@v4

      - name: Set up MSVC
        uses: ilammy/msvc-dev-cmd@v1

      - name: Install Qt 6
        uses: jurplel/install-qt-action@v3
        with:
          version: '6.5.3'
          arch: 'win64_msvc2019_64'
          setup-python: 'false'

      - name: Verify uninstaller source exists
        shell: pwsh
        run: |
          if (-not (Test-Path "shared/uninstall.cpp")) {
            Write-Error "shared/uninstall.cpp was not found."
            Get-ChildItem -Force
            if (Test-Path "shared") { Get-ChildItem shared -Force }
            exit 1
          }

      - name: Configure CMake
        shell: pwsh
        run: |
          New-Item -ItemType Directory -Force -Path build | Out-Null
          Set-Location build
          cmake .. -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64

      - name: Build BetterAngle
        shell: pwsh
        run: |
          Set-Location build
          cmake --build . --config Release

      - name: Build uninstaller.exe
        shell: pwsh
        run: |
          New-Item -ItemType Directory -Force -Path build/Release | Out-Null
          cl /std:c++20 /EHsc /O2 /W4 /Fe:build/Release/uninstaller.exe shared/uninstall.cpp ole32.lib shell32.lib

      - name: Deploy Qt Dependencies
        shell: pwsh
        run: |
          Set-Location build
          windeployqt Release/BetterAngle.exe --qmldir ../src/gui --quick --no-translations

      - name: Download VC++ Redistributable
        shell: pwsh
        run: |
          Invoke-WebRequest -Uri "https://aka.ms/vs/17/release/vc_redist.x64.exe" -OutFile "build/Release/vc_redist.x64.exe"

      - name: Archive Build
        shell: pwsh
        run: |
          New-Item -ItemType Directory -Force -Path bin | Out-Null
          $ver = (Get-Content VERSION).Trim()
          & "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" /dAppVer=$ver installer.iss

      - name: Upload uninstaller artifact
        uses: actions/upload-artifact@v4
        with:
          name: uninstaller
          path: build/Release/uninstaller.exe

      - name: Get Version
        if: startsWith(github.ref, 'refs/tags/v')
        shell: pwsh
        run: |
          $ver = (Get-Content VERSION).Trim()
          echo "VERSION=$ver" >> $env:GITHUB_ENV
          echo "TAG=v$ver" >> $env:GITHUB_ENV

      - name: Extract Latest Release Notes
        if: startsWith(github.ref, 'refs/tags/v')
        shell: pwsh
        run: |
          $lines = Get-Content RELEASE_NOTES.md
          $outLines = @()
          $seenFirst = $false
          foreach ($line in $lines) {
              if ($line.StartsWith("### ")) {
                  if ($seenFirst) { break }
                  $seenFirst = $true
              }
              if ($seenFirst) { $outLines += $line }
          }
          $outLines | Out-File CURRENT_RELEASE_NOTES.md -Encoding utf8

      - name: Create GitHub Release
        if: startsWith(github.ref, 'refs/tags/v')
        uses: softprops/action-gh-release@v1
        with:
          files: |
            bin/BetterAngle_Setup.exe
            build/Release/uninstaller.exe
          tag_name: ${{ env.TAG }}
          name: BetterAngle Pro ${{ env.TAG }}
          body_path: CURRENT_RELEASE_NOTES.md
          draft: false
          prerelease: false
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}