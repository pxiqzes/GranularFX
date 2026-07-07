# GranularFX — starter VST3 plugin

A working granular pitch/time-manipulation effect, built with JUCE. It writes
incoming audio into a rolling history buffer, spawns overlapping "grains"
from it at randomised offsets, and plays each grain back at an adjustable
pitch ratio — the classic granular texture/pitch-shift sound.

Knobs: **Grain Size**, **Density** (grains/sec), **Pitch** (±24 semitones),
**Scatter** (randomised read-position jitter, ms), **Mix**.

## What you need installed

- **CMake** 3.15+ — https://cmake.org/download/
- **Git**
- A C++ compiler:
  - Windows: **Visual Studio 2022 Community** (free) with the "Desktop
    development with C++" workload
  - Mac: **Xcode** (from the App Store) + command line tools
    (`xcode-select --install`)

You do **not** need to manually download JUCE — the CMakeLists.txt fetches
it automatically the first time you configure the project (needs internet
access for that one step).

## Build on Windows

```bash
cd GranularFX
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

The VST3 will be built and automatically copied to your system's VST3
folder (usually `C:\Program Files\Common Files\VST3`). Rescan plugins in
your DAW and look for "GranularFX".

## Build on Mac

```bash
cd GranularFX
mkdir build
cd build
cmake .. -G Xcode
cmake --build . --config Release
```

This copies the VST3 to `~/Library/Audio/Plug-Ins/VST3`. Rescan plugins in
your DAW.

(Tip: you can also open the generated Xcode project directly — useful if
you want to use Xcode's debugger.)

## Testing it

A free host like **Reaper** (fully-functional unlimited trial) is a good
place to test: create a track, load "GranularFX" as an effect, play audio
through it, and turn the Pitch and Scatter knobs.

## Where to go from here

- `Source/GranularEngine.h` is the entire DSP — heavily commented, read it
  top to bottom. It's the only file you need to touch to change the sound.
- Ideas to try next, roughly in order of difficulty:
  1. Add a **stereo spread** parameter (randomise each grain's pan)
  2. Add a **reverse grain** probability
  3. Smooth parameter changes (currently they can zipper/click if moved
     fast — look into `juce::SmoothedValue`)
  4. Replace the fixed Hann window with a selectable window shape
  5. Add an independent **time-stretch** control (decouple grain spawn
     rate from playback rate, so pitch and duration can be controlled
     separately — this is the "textbook" phase vocoder alternative)
- Once you're happy with VST3 on both platforms, adding **AU** support for
  Mac-native hosts (Logic, GarageBand) is a one-line change in
  `CMakeLists.txt`: `FORMATS VST3 AU`.
