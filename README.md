# MIGRATED

This repository was moved to the main [binaryninja api](https://github.com/Vector35/binaryninja-api/tree/dev/plugins/workflow_objc) repository. 

# Objective-C Workflow

This is the Objective-C plugin that ships with Binary Ninja. It provides
additional support for analyzing Objective-C binaries.

The primary functionality offered by this plugin is:

- **Function Call Cleanup.** When using the Objective-C workflow, calls to
  `objc_msgSend` can be replaced with direct calls to the relevant function's
  implementation.
  
For more details and usage instructions, see the [user guide](https://dev-docs.binary.ninja/guide/objectivec.html).

## Issues

Issues for this repository have been disabled. Please file an issue for this repository at https://github.com/Vector35/binaryninja-api/issues. All previously existing issues for this repository have been transferred there as well.

## Building

This plugin can be built and installed separately from Binary Ninja via the
following commands:

```sh
git clone https://github.com/Vector35/workflow_objc.git && cd workflow_objc
git submodule update --init --recursive
cmake -S . -B build -GNinja
cmake --build build -t install
```

## Credits

This plugin is a continuation of [Objective Ninja](https://github.com/jonpalmisc/ObjectiveNinja), originally made
by [@jonpalmisc](https://twitter.com/jonpalmisc). The full terms of the
Objective Ninja license are as follows:

```
Copyright (c) 2022-2023 Jon Palmisciano

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
