# Static library merging

`StaticLibMerge.cmake` collects static-library targets from the requested targets' transitive link interfaces, then merges their archives into Winux's output archive after Winux is built. Windows uses `lib.exe`; Linux uses an `ar` MRI script and rebuilds the archive index.

Non-static link requirements from those interfaces remain on the Winux CMake target. Platform-provided libraries are not copied into Winux's archive.