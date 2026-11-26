Build Linux Debug and Release:

```sh
cmake --preset linux-debug
cmake --build WinuxBuild/Linux/Debug --parallel

cmake --preset linux-release
cmake --build WinuxBuild/Linux/Release --parallel
```

Cross-compile Windows x64 with MinGW-w64 (requires the `x86_64-w64-mingw32`
compiler tools and vcpkg):

```sh
cmake --preset mingw-x64-debug
cmake --build WinuxBuild/Windows/Debug --parallel

cmake --preset mingw-x64-release
cmake --build WinuxBuild/Windows/Release --parallel
```

Windows builds with MSVC use `x64-windows-static-md`:

```powershell
vcpkg install --triplet x64-windows-static-md
```
