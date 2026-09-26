import shlex
import shutil
import stat
import subprocess
import sys
from pathlib import Path


def run_wsl(command: str) -> str:
    result = subprocess.run(
        ["wsl.exe", "--", "bash", "-lc", command],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip() or command)
    return result.stdout.strip()


def resolve_wsl_source() -> str:
    home = run_wsl('printf "%s\\n" "$HOME"').strip()
    source = f"{home}/.vs/Winux/WinuxBuild/Linux"
    run_wsl(f"test -d {shlex.quote(source)}")
    return source


def wsl_to_windows_path(wsl_path: str) -> str:
    return run_wsl(f"wslpath -w {shlex.quote(wsl_path)}")


def remove_readonly(function, path, error_info) -> None:
    error = error_info[1]
    if not isinstance(error, PermissionError):
        raise error

    Path(path).chmod(stat.S_IREAD | stat.S_IWRITE)
    function(path)


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    target = (repo_root / "WinuxBuild" / "Linux").resolve()

    try:
        source = resolve_wsl_source()
        source_windows = wsl_to_windows_path(source)
    except (RuntimeError, FileNotFoundError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    try:
        if target.exists():
            shutil.rmtree(target, onerror=remove_readonly)
        shutil.copytree(source_windows, target)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
