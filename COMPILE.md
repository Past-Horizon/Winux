Install manifest dependencies:

CMake uses the sibling `../Cudev` checkout by default
(`WINUX_USE_LOCAL_CUDEV=ON`). To fetch Cudev from GitHub instead, set
`-DWINUX_USE_LOCAL_CUDEV=OFF` when configuring.

Windows (`x64-windows-static-md`):

```powershell
vcpkg install --triplet x64-windows-static-md
```

Linux (`x64-linux`):

```sh
sudo vcpkg install --triplet x64-linux
```
