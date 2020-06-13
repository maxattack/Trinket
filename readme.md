# Trinket

Yet another personal hobby game engine.

[Informal Twitter Devlog](https://twitter.com/xewlupus/status/1266125978939473921)

My goal is to make mod-able games that give players access to the level editor, which is not possible with popular commercial engines like Unity or Unreal.

Towards that goal, the architecture implements a light-weight entitiy-component system.  Different systems or 3rd-party libraries do their own natural bookkeeping, and are "glued together" with a sparseset data structure that maps ObjectIDs to linears arrays with a lookup-cost no worse than chasing two pointers.

This public repository contains my original source, but I haven't included any prebuilt dependencies or gunky build scripts because I don't have the bandwidth to support them, and didn't want to clutter up the commit history with infrastructure noise.

## Dependencies

- SDL2 - platform abstraction
- Diligent Graphics - low-level rendering-backend abstraction/validation, shader cross-compile
- GLM - math library with SIMD intrinsics
- PhysX - collision and dynamics
- Lua - runtime configuration, scripting
- EASTL - performance-conscious container templates
- Dear IMGUI - editor ui rapid development
- STB - misc utilities

## License

Copyright 2020 Max Kaufmann (max.kaufmann@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
