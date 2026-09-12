# Third-Party Licenses

This project incorporates third-party open source software. Below are the details regarding licenses, source repositories, and linking mechanisms.

---

## 1. SoundTouch Audio Processing Library

- **Library**: SoundTouch Audio Processing Library (v2.3.3)
- **Author**: Olli Parviainen
- **License**: GNU Lesser General Public License (LGPL) version 2.1
- **Upstream Repository**: [https://codeberg.org/soundtouch/soundtouch](https://codeberg.org/soundtouch/soundtouch)
- **Local Source Path**: `ThirdParty/SoundTouch/`

### Licensing Compliance & Linking Notice

SoundTouch is distributed under the terms of the **GNU Lesser General Public License (LGPL) v2.1**.

#### Current Build Integration (Static Linkage)
In the current Windows build environment, SoundTouch is integrated as a static C++ library:
- **Release (x64)**: `ThirdParty/SoundTouch/lib/SoundTouch_x64.lib` (built with `/MD` CRT)
- **Debug (x64)**: `ThirdParty/SoundTouch/lib/SoundTouchD_x64.lib` (built with `/MDd` CRT)

Under **LGPL v2.1 Section 6**, incorporating an LGPL library into a proprietary or closed-source application requires providing end users with a mechanism to modify the library and run the application with their modified version:

1. **Option A — Object Code Distribution & Relinking (Current Static Linking)**:
   - The full source code of the exact version of SoundTouch used is preserved in `ThirdParty/SoundTouch/`.
   - Users and developers can recompile SoundTouch from source using MSVC (`ThirdParty\SoundTouch\source\SoundTouch\SoundTouch.vcxproj`).
   - If distributing commercial static builds, the developer provides intermediate object files (`VanceSpectral.lib`) and linking instructions so an end user can relink against a custom version of `SoundTouch.lib`. Detailed rebuild and relinking instructions are provided in [`ThirdParty/SoundTouch/INTEGRATION_NOTES.md`](file:///c:/Users/Ansh%20Srivastava/OneDrive/Documents/GitHub/VanceSpectral/ThirdParty/SoundTouch/INTEGRATION_NOTES.md).

2. **Option B — Dynamic Linking (Recommended for Public Distribution)**:
   - For closed-source public distribution without providing proprietary object files, SoundTouch can be compiled as a dynamic link library (`SoundTouch.dll` on Windows / `libSoundTouch.dylib` on macOS) and shipped alongside the plugin bundle.
   - This satisfies LGPL v2.1 Section 6(b) via standard dynamic library substitution (the user can swap the `.dll` or shared library file without touching the plugin binary).

---

### LGPL v2.1 Summary

SoundTouch is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

---

## 2. libsamplerate (Secret Rabbit Code)

- **Library**: libsamplerate Sample Rate Converter (v0.2.2)
- **Author**: Erik de Castro Lopo
- **License**: 2-Clause BSD License (FreeBSD)
- **Upstream Repository**: [https://github.com/libsndfile/libsamplerate](https://github.com/libsndfile/libsamplerate)
- **Local Path**: `ThirdParty/libsamplerate/`

### Licensing Notice & Static Linking

libsamplerate is distributed under the permissive **2-Clause BSD License**.

In this project, libsamplerate is integrated as a static library:
- Release build: `ThirdParty/libsamplerate/lib/samplerate.lib` (built with `/MD` CRT)
- Debug build: `ThirdParty/libsamplerate/lib/samplerate.lib` / `samplerateD.lib` (built with `/MDd` CRT)

Instructions for rebuilding the libsamplerate library independently and relinking VanceSpectral are provided in [`ThirdParty/libsamplerate/INTEGRATION_NOTES.md`](file:///c:/Users/Ansh%20Srivastava/OneDrive/Documents/GitHub/VanceSpectral/ThirdParty/libsamplerate/INTEGRATION_NOTES.md).

### 2-Clause BSD License Text

```text
Copyright (c) 2012-2016, Erik de Castro Lopo <erikd@mega-nerd.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

---

## 3. chowdsp_utils

- **Library**: chowdsp_utils
- **Author**: Jatin Chowdhury (Chowdhury DSP)
- **License**: Individual per-module licenses (BSD 3-Clause and GPLv3)
- **Upstream Repository**: [https://github.com/Chowdhury-DSP/chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils)
- **Local Path**: `ThirdParty/chowdsp_utils/`

### Licensing Notice & Module Breakdown

Per `ThirdParty/chowdsp_utils/LICENSE.md`, each JUCE module within `chowdsp_utils` possesses its own specific license:

- **Permissive Modules (BSD 3-Clause)**:
  - `chowdsp_core`
  - `chowdsp_data_structures`
  - `chowdsp_math`
  - `chowdsp_simd`
  - `chowdsp_buffers`
  - `chowdsp_json`, `chowdsp_logging`, `chowdsp_units`, `chowdsp_parameters`, `chowdsp_presets_v2`

- **Copyleft Modules (GPLv3)**:
  - `chowdsp_filters`
  - `chowdsp_dsp_utils`
  - `chowdsp_dsp_data_structures`
  - `chowdsp_sources`, `chowdsp_waveshapers`, `chowdsp_compressor`, `chowdsp_eq`, `chowdsp_reverb`

*Implementation Note*: The active VanceSpectral 5-stage wet effects engine (`EffectsEngine.cpp`) is implemented using native `juce::dsp` components (`juce::dsp::Chorus`, `juce::dsp::Phaser`, `juce::dsp::DelayLine`) and Airwindows algorithms, together with an internal sidechain pump.

---

## 4. Airwindows Audio DSP Algorithms

- **Library**: Airwindows DSP (Spiral, ChorusEnsemble)
- **Author**: Chris Johnson (airwindows.com)
- **License**: MIT License
- **Upstream Repository**: [https://github.com/airwindows/airwindows](https://github.com/airwindows/airwindows)
- **Local Path**: `Source/Airwindows/AirwindowsSpiral.h`, `Source/Airwindows/AirwindowsChorus.h`

### Licensing Notice & MIT License Text

The Airwindows algorithms utilized in VanceSpectral are licensed under the permissive **MIT License**:

```text
MIT License

Copyright (c) 2018 Chris Johnson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
