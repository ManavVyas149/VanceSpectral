# Third-Party Licenses

This project incorporates third-party open source software. Below are the details regarding licenses, source repositories, and linking mechanisms.

---

## SoundTouch

- **Library**: SoundTouch Audio Processing Library
- **Author**: Olli Parviainen
- **License**: GNU Lesser General Public License (LGPL) version 2.1
- **Upstream Repository**: [https://codeberg.org/soundtouch/soundtouch](https://codeberg.org/soundtouch/soundtouch)
- **Local Path**: `ThirdParty/SoundTouch/`

### Licensing Compliance & Static Linking Notice

SoundTouch is distributed under the terms of the GNU Lesser General Public License (LGPL) v2.1. 

In this project, SoundTouch is integrated as a static library:
- Release build: `ThirdParty/SoundTouch/lib/SoundTouch_x64.lib`
- Debug build: `ThirdParty/SoundTouch/lib/SoundTouchD_x64.lib`

In compliance with LGPL v2.1 Section 6:
1. The full source code of the exact version of SoundTouch used is included in the `ThirdParty/SoundTouch/` directory of this repository (and upstream source is available at the repository link above).
2. Users and developers are permitted to modify the SoundTouch library and relink the application.
3. Instructions for rebuilding the SoundTouch library independently and relinking VanceSpectral are provided in [`ThirdParty/SoundTouch/INTEGRATION_NOTES.md`](file:///c:/Users/Ansh%20Srivastava/OneDrive/Documents/GitHub/VanceSpectral/ThirdParty/SoundTouch/INTEGRATION_NOTES.md).

---

### LGPL v2.1 Summary

SoundTouch is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

---

## libsamplerate

- **Library**: libsamplerate (Secret Rabbit Code) Sample Rate Converter
- **Author**: Erik de Castro Lopo
- **License**: 2-Clause BSD License (FreeBSD)
- **Upstream Repository**: [https://github.com/libsndfile/libsamplerate](https://github.com/libsndfile/libsamplerate)
- **Local Path**: `ThirdParty/libsamplerate/`

### Licensing Notice & Static Linking

libsamplerate is distributed under the 2-Clause BSD License.

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

## chowdsp_utils

- **Library**: chowdsp_utils
- **Author**: Jatin Chowdhury (Chowdhury DSP)
- **License**: GPLv3 / BSD 3-Clause (per-module licenses)
- **Upstream Repository**: [https://github.com/Chowdhury-DSP/chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils)
- **Local Path**: `ThirdParty/chowdsp_utils/`

### Licensing Notice

`chowdsp_utils` is integrated as JUCE modules located in `ThirdParty/chowdsp_utils/modules/`. Non-module and core components follow GPLv3; math and SIMD utilities follow 3-Clause BSD.

---

## Airwindows

- **Library**: Airwindows Audio DSP Algorithms (Spiral, ChorusEnsemble)
- **Author**: Chris Johnson (airwindows.com)
- **License**: MIT License
- **Upstream Repository**: [https://github.com/airwindows/airwindows](https://github.com/airwindows/airwindows)
- **Local Path**: `ThirdParty/Airwindows/`, `Source/Airwindows/`

### MIT License Text

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
