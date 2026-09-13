"""
commands:
    --all                 pack every platform
    --all --release       pack release for every platform
    --all --debug         pack debug for every platform
    --all --both          pack debug + release for every platform

    --win32               pack Windows
    --win32 --release     pack Windows release
    --win32 --debug       pack Windows debug
    --win32 --both        pack Windows debug + release

    --unix                pack Linux/Unix
    --unix --release      pack Linux release
    --unix --debug        pack Linux debug
    --unix --both        pack Linux debug + release

you can mix the platform + config args however u want.
if you don't give a config, it means --both.
--all means all platforms unless you pair it with --win32 or --unix.
"""

from __future__ import annotations

import argparse
import sys
import zipfile
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
ROOT = SCRIPT_DIR.parent
BUILD_ROOT = ROOT / "WinuxBuild"
VERSION_FILE = ROOT / "WinuxVersion.txt"
LICENSE_FILE = ROOT / "LICENSE"
INCLUDE_ROOT = ROOT / "include"
PACKAGE_ROOT = BUILD_ROOT / "Packed"

# --win32 -> Windows, --unix -> Linux in the current Winux build layout.
PLATFORMS = {
    "win32": {
        "folder": "Windows",
        "file_platform": "Windows",
        "package_platform": "windows",
        "extension": ".lib",
    },
    "unix": {
        "folder": "Linux",
        "file_platform": "Linux",
        "package_platform": "unix",
        "extension": ".a",
    },
}

CONFIGS = ("Release", "Debug")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Pack Winux libraries for the platforms/configurations."
    )

    parser.add_argument("--all", action="store_true", help="pack all platforms")
    parser.add_argument("--win32", action="store_true", help="pack Windows")
    parser.add_argument("--unix", action="store_true", help="pack Linux/Unix")

    parser.add_argument("--release", action="store_true", help="pack Release")
    parser.add_argument("--debug", action="store_true", help="pack Debug")
    parser.add_argument("--both", action="store_true", help="pack Release + Debug")

    args = parser.parse_args()

    if not (args.all or args.win32 or args.unix):
        parser.error("you need a platform arg: --all, --win32, or --unix")

    config_args = sum((args.release, args.debug, args.both))
    if config_args > 1:
        parser.error("use only one of --release, --debug, or --both")

    return args


def read_version() -> str:
    if not VERSION_FILE.is_file():
        raise FileNotFoundError(f"couldn't find version file: {VERSION_FILE}")

    version = VERSION_FILE.read_text(encoding="utf-8").strip()
    if not version:
        raise ValueError(f"{VERSION_FILE} is empty")

    return version


def selected_platforms(args: argparse.Namespace) -> list[str]:
    # --all by itself means all platforms.
    # --all --win32 / --all --unix narrows --all to that platform.
    if args.win32 or args.unix:
        platforms = []
        if args.win32:
            platforms.append("win32")
        if args.unix:
            platforms.append("unix")
        return platforms

    return list(PLATFORMS)


def selected_configs(args: argparse.Namespace) -> list[str]:
    if args.release:
        return ["Release"]
    if args.debug:
        return ["Debug"]

    # No config OR --both = all configs.
    return list(CONFIGS)


def find_library(platform_key: str, config: str, version: str) -> Path | None:
    info = PLATFORMS[platform_key]
    build_dir = BUILD_ROOT / info["folder"] / config
    filenames = ("Winux.lib",) if info["extension"] == ".lib" else ("libWinux.a",)
    candidates = [build_dir / filename for filename in filenames]
    return next((candidate for candidate in candidates if candidate.is_file()), None)


def validate_package_files() -> None:
    if not LICENSE_FILE.is_file():
        raise FileNotFoundError(f"couldn't find LICENSE: {LICENSE_FILE}")

    include_dir = INCLUDE_ROOT / "Winux"
    if not include_dir.is_dir():
        raise FileNotFoundError(f"couldn't find include folder: {include_dir}")


def add_directory_to_zip(
    archive: zipfile.ZipFile,
    source_dir: Path,
    archive_root: str,
) -> None:
    files = list(source_dir.rglob("*"))

    if not files:
        archive.writestr(f"{archive_root}/", "")
        return

    for item in files:
        relative = item.relative_to(source_dir).as_posix()
        archive_name = f"{archive_root}/{relative}"

        if item.is_dir():
            archive.writestr(f"{archive_name}/", "")
        elif item.is_file():
            archive.write(item, archive_name)


def package_platform(
    platform_key: str,
    sources: list[tuple[str, Path]],
    version: str,
) -> Path:
    info = PLATFORMS[platform_key]

    PACKAGE_ROOT.mkdir(parents=True, exist_ok=True)

    package_name = f"Winux-{version}-x64-{info['package_platform']}.zip"
    package_path = PACKAGE_ROOT / package_name

    with zipfile.ZipFile(
        package_path,
        mode="w",
        compression=zipfile.ZIP_DEFLATED,
        compresslevel=9,
    ) as archive:
        # Package layout: LICENSE, Winux headers, and libraries by config.
        archive.write(LICENSE_FILE, "LICENSE")

        add_directory_to_zip(
            archive,
            INCLUDE_ROOT / "Winux",
            "Winux",
        )

        for config, source in sources:
            archive.write(source, (Path(config) / source.name).as_posix())

    return package_path


def main() -> int:
    args = parse_args()

    try:
        version = read_version()
        validate_package_files()
    except (FileNotFoundError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    platforms = selected_platforms(args)
    configs = selected_configs(args)

    print(f"Winux {version} pack")
    print(
        f"platforms: {', '.join(PLATFORMS[p]['file_platform'] for p in platforms)}"
    )
    print(f"configs:   {', '.join(configs)}")
    print()

    missing: list[tuple[str, str]] = []
    packed: list[Path] = []

    for platform_key in platforms:
        platform_name = PLATFORMS[platform_key]["file_platform"]
        sources: list[tuple[str, Path]] = []
        platform_missing = False

        for config in configs:
            source = find_library(platform_key, config, version)

            if source is None:
                missing.append((platform_name, config))
                platform_missing = True
                print(f"[missing] {platform_name} {config}")
                continue

            sources.append((config, source))
            print(f"[found]   {source}")

        if platform_missing:
            print(f"[skipped] {platform_name} archive (missing requested build)")
            continue

        package = package_platform(platform_key, sources, version)
        packed.append(package)
        print(f"[packed]  {package}")
        print()

    if missing:
        print("some requested builds were missing:")
        for platform_name, config in missing:
            print(f"  - {platform_name} {config}")

    if not packed:
        print("nothing got packed.", file=sys.stderr)
        return 1

    if missing:
        print("packing finished with missing builds.", file=sys.stderr)
        return 1

    print(f"done. packed {len(packed)} archive(s) into {PACKAGE_ROOT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
