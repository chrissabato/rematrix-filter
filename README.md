# rematrix-filter

Audio matrix mixer plugin for OBS Studio 32+.

[![Build](https://github.com/chrissabato/rematrix-filter/actions/workflows/build.yml/badge.svg)](https://github.com/chrissabato/rematrix-filter/actions/workflows/build.yml)

## Download

| Platform | Link |
|---|---|
| Windows x64 | [rematrix-filter-windows-x64.zip](https://github.com/chrissabato/rematrix-filter/releases/download/latest/rematrix-filter-windows-x64.zip) |
| macOS Universal | [rematrix-filter-macos-universal.zip](https://github.com/chrissabato/rematrix-filter/releases/download/latest/rematrix-filter-macos-universal.zip) |

## What it does

Adds a **Rematrix** audio filter that lets you mix any combination of input channels into any output channel, each with its own dB gain. It works as a full gain matrix — every output channel can draw from every input channel independently.

Useful for:
- Muting a specific channel (e.g. a comms/IFB feed) without affecting others
- Mixing a multi-channel source down to stereo or dual mono
- Swapping, duplicating, or rerouting channels

## Installation

OBS must be set to a multi-channel layout (**Settings → Audio → Channels**) to match the number of channels in your source.

### Windows

1. Download `rematrix-filter-windows-x64.zip`
2. Extract and copy into your OBS installation folder (default: `C:\Program Files\obs-studio\`), preserving the folder structure:
   ```
   obs-plugins\64bit\rematrix-filter.dll
   data\obs-plugins\rematrix-filter\locale\en-US.ini
   ```
3. Restart OBS

### macOS

1. Download `rematrix-filter-macos-universal.zip`
2. Extract and copy the contents into:
   ```
   ~/Library/Application Support/obs-studio/plugins/
   ```
3. Restart OBS
4. If macOS blocks the plugin, go to **System Settings → Privacy & Security** and click **Allow Anyway**

## Usage

The filter appears as an audio filter, not a source filter. To add it:

1. In the **Audio Mixer**, click the gear icon (⚙) next to any audio source
2. Select **Filters**
3. Click **+** and choose **Rematrix**

The filter shows one collapsible group per output channel. Inside each group is a dB gain slider for every input channel. Set a slider to control how much of that input contributes to that output. The default is an identity pass-through (0 dB on the diagonal, -96 dB everywhere else).

### Example: mute comms, dual mono mixdown

For a 6-channel NDI source where channel 3 is comms and you want stereo dual mono from the rest:

- **Output Channel 1:** Input 1: 0 dB, Input 2: 0 dB, Input 3: -96 dB, Input 4: 0 dB, Input 5: 0 dB, Input 6: 0 dB
- **Output Channel 2:** same as Output Channel 1
- **Output Channels 3–6:** all inputs at -96 dB

> **Tip:** When summing multiple channels reduce each gain (e.g. -6 dB per input for 4 inputs) to avoid clipping.

## Building from source

Requires CMake 3.16+, Visual Studio 2022 (Windows) or Xcode (macOS), and OBS Studio 32 installed.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/path/to/obs-studio"
cmake --build build --config RelWithDebInfo
cmake --install build --config RelWithDebInfo --prefix release
```

GitHub Actions builds both platforms automatically on every push. See [`.github/workflows/build.yml`](.github/workflows/build.yml).

## Credits

Original plugin by [Andersama](https://github.com/Andersama/rematrix-filter). Updated for OBS 32 with a full matrix mixer replacing the original channel router.
