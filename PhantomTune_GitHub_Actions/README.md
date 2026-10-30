# Phantom Tune v0.1 — Phantom Culture

An original experimental monophonic pitch-correction VST3 for FL Studio on Windows.

## Windows prerequisites
- Visual Studio 2022 with **Desktop development with C++** and Windows SDK
- CMake 3.22 or newer
- Git and internet access for JUCE download on first configure

## Build
Double-click `build-windows.bat` from the project directory, or run:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target PhantomTune_VST3
```

Find the plugin in `build/PhantomTune_artefacts/Release/VST3/Phantom Tune.vst3`. Copy the **entire .vst3 bundle directory** into `C:\Program Files\Common Files\VST3\`, then open FL Studio > Options > Manage plugins > Find installed plugins. Insert Phantom Tune on a vocal mixer track.

## Controls
- Key: tonic (C to B)
- Scale: chromatic, major or natural minor
- Retune ms: smoothing time, smaller means faster correction
- Humanize: reduces correction depth (100% means no correction)
- Mix: aligned delayed dry / corrected wet

## Engineering notes and limitations
- This is an **experimental first prototype**, not a commercial-quality Auto-Tune replacement.
- Designed for **single-note/monophonic** vocals, not chords or full mixes.
- Uses normalized autocorrelation for pitch estimation and dual-window overlap delay-line pitch shifting. Fast retune may cause artifacts; octave errors, transient smearing and formant changes are possible.
- No formant preservation, MIDI note targeting, true automatic key detection or advanced voiced/unvoiced tracking in this version.
- Reports approximately 45 ms processing latency; actual roundtrip latency also depends on host and interface.
- Initial pitch analysis runs on the audio thread; further optimization and testing are needed for real-time performance.
- No Windows binary included. Compile and test on Windows with FL Studio before production use.
- JUCE is downloaded during CMake configuration. Review JUCE licensing before commercial distribution.


## v0.2 Harmony & Vocal Width
- Harmony: one additional chromatic interval voice (third above, fifth above, octave below, third below).
- Width: asymmetric stereo harmony balance and double level (not a full stereo widener).
- Double: independently detuned, delayed vocal layer.
- Body: soft saturation for extra density.

**Limitations:** experimental prototype, not tested in FL Studio or compiled on Windows. Harmonies are fixed semitone intervals rather than diatonic scale-aware harmonies; pitch-shifting may create artifacts and change formants. Stereo input is processed independently. Main processing adds about 45 ms latency. Test at low monitor volume.

## GitHub Actions ile Windows'ta otomatik derleme

1. GitHub'da yeni bir **public** repository oluşturun.
2. Bu `PhantomTune` klasörünün **içindeki** tüm dosyaları (gizli `.github` klasörü dahil) repository kök dizinine yükleyin.
3. GitHub > Actions > `Build Phantom Tune VST3 (Windows)` > `Run workflow`.
4. Başarılı olursa çalışmanın altındaki `Artifacts` bölümünden `PhantomTune-Windows-VST3` indirin.
5. İçindeki `.vst3` klasörünü `C:\Program Files\Common Files\VST3` içine çıkarıp FL Studio plugin taraması yapın.

Not: Bu kaynak kodun derlenmesi ve ses kalitesi henüz gerçek Windows/FL Studio ortamında doğrulanmamıştır. Workflow derleme hatası verebilir; Actions logunu inceleyin.
