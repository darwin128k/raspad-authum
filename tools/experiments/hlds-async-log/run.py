"""Build/test the Linux i386 prototype; optionally probe an isolated HLDS copy."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time


def server_probe(source, library, output, asynchronous):
    root = output / ("server-async" if asynchronous else "server-native")
    root.mkdir()
    for entry in source.iterdir():
        if entry.name == "hlds_linux":
            shutil.copy2(entry, root / entry.name)
        elif entry.suffix == ".so" or entry.name == "linux64":
            (root / entry.name).symlink_to(entry, target_is_directory=entry.is_dir())
    for game in ("valve", "cstrike"):
        target = root / game
        target.mkdir()
        for entry in (source / game).iterdir():
            if entry.name in ("addons", "logs", "custom.hpk"):
                continue
            if entry.name == "dlls":
                shutil.copytree(entry, target / entry.name)
            elif entry.is_dir():
                (target / entry.name).symlink_to(entry, target_is_directory=True)
            elif entry.suffix in (".cfg", ".rc"):
                (target / entry.name).write_text("// isolated probe\n")
            else:
                shutil.copy2(entry, target / entry.name)
        (target / "logs").mkdir()
    # Use a minimal direct game DLL configuration, never load source add-ons.
    (root / "cstrike/liblist.gam").write_text(
        'game "Counter-Strike"\ngamedll_linux "dlls/cs.so"\n')
    env = os.environ.copy()
    env.pop("LD_PRELOAD", None)
    env.pop("AUTHUM_ASYNC_LOG_PROBE_STALL_MS", None)
    env["LD_LIBRARY_PATH"] = str(root) + ":" + env.get("LD_LIBRARY_PATH", "")
    if asynchronous:
        env["LD_PRELOAD"] = str(library)
        env["AUTHUM_ASYNC_LOG_PROBE_DIR"] = str(root / "cstrike/logs")
    with (root / "console.txt").open("wb") as console:
        process = subprocess.Popen(
            [str(root / "hlds_linux"), "-game", "cstrike", "-console", "-insecure",
             "-nomaster", "-port", "27991", "+maxplayers", "2", "+sv_lan", "1",
             "+log", "on", "+map", "authum_probe_no_such_map"], cwd=root, env=env, stdin=subprocess.PIPE,
            stdout=console, stderr=subprocess.STDOUT)
        try:
            process.stdin.write(b"log on\n")
            process.stdin.flush()
            deadline = time.monotonic() + 30
            while time.monotonic() < deadline:
                if process.poll() is not None:
                    raise RuntimeError((root / "console.txt").read_text(errors="replace"))
                logs = list((root / "cstrike/logs").glob("L*.log"))
                if logs:
                    break
                time.sleep(0.1)
            else:
                raise RuntimeError("No engine log; inspect " + str(root / "console.txt"))
            time.sleep(0.2)
            active = logs[0].read_bytes()
            assert (b"Log file started" in active) if asynchronous else not active
            process.stdin.write(b"log off\n")
            process.stdin.flush()
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                closed = logs[0].read_bytes()
                if b"Log file closed" in closed:
                    break
                time.sleep(0.1)
            else:
                raise RuntimeError("No close record: " + str(logs[0]))
            assert closed.count(b"Log file started") == 1
            assert closed.count(b"Log file closed") == 1
            if asynchronous:
                assert closed.startswith(active)
            result = (f"HLDS {'async' if asynchronous else 'native'}: "
                      f"active={len(active)} closed={len(closed)} bytes")
            print(result, flush=True)
        finally:
            if process.poll() is None:
                process.stdin.write(b"quit\n")
                process.stdin.flush()
                try:
                    process.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    process.terminate()
                    try:
                        process.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, help="Original Linux CS HLDS installation (read-only)")
    parser.add_argument("--report-dir", type=Path, help="New persistent directory for results and server logs")
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    output = Path(tempfile.mkdtemp(prefix="authum-async-log-research-"))
    print("Research artifacts:", output, flush=True)
    results = []
    library, driver = output / "preload.so", output / "driver"
    common = ["gcc", "-m32", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror"]
    subprocess.run(common + ["-shared", "-fPIC", str(source / "preload.c"),
                             "-ldl", "-pthread", "-o", str(library)], check=True, timeout=60)
    lh_test = source.parents[2] / "lib/lh/test"
    googletest = lh_test / "lib/googletest/googletest"
    if not (googletest / "src/gtest-all.cc").is_file():
        raise RuntimeError("Initialize the pinned GoogleTest submodule before running tests")
    subprocess.run(["g++", "-m32", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-I" + str(googletest / "include"), "-I" + str(googletest),
                    str(source / "driver.cpp"), str(lh_test / "src/main.cpp"),
                    str(googletest / "src/gtest-all.cc"), "-pthread", "-o", str(driver)],
                   check=True, timeout=120)
    for mode in ("native", "async", "overflow"):
        target = output / mode
        target.mkdir()
        env = os.environ.copy()
        for key in ("LD_PRELOAD", "AUTHUM_ASYNC_LOG_PROBE_DIR", "AUTHUM_ASYNC_LOG_PROBE_STALL_MS"):
            env.pop(key, None)
        if mode != "native":
            env.update(LD_PRELOAD=str(library), AUTHUM_ASYNC_LOG_PROBE_DIR=str(target))
        if mode == "overflow":
            env["AUTHUM_ASYNC_LOG_PROBE_STALL_MS"] = "1000"
        env.update(AUTHUM_ASYNC_LOG_TEST_DIR=str(target), AUTHUM_ASYNC_LOG_TEST_MODE=mode)
        case_filter = ("hlds_async_log.overflow_retains_fifo_prefix_without_blocking_producer"
                       if mode == "overflow" else "-hlds_async_log.overflow_*")
        result = subprocess.run([str(driver), "--gtest_filter=" + case_filter], env=env,
                                text=True, capture_output=True, check=True, timeout=10)
        print(result.stdout + result.stderr, end="", flush=True)
        results.append(result.stdout + result.stderr)
        if mode == "overflow":
            assert "dropped=1024 io_errors=0" in result.stderr
        elif mode == "async":
            assert "dropped=0 io_errors=0" in result.stderr
    if args.server:
        for asynchronous in (False, True):
            results.append(server_probe(args.server.resolve(), library, output, asynchronous) + "\n")
    if args.report_dir:
        report = args.report_dir.resolve()
        report.mkdir(parents=True, exist_ok=False)
        (report / "results.txt").write_text("".join(results))
        if args.server:
            for mode in ("server-native", "server-async"):
                target = report / mode
                target.mkdir()
                shutil.copy2(output / mode / "console.txt", target / "console.txt")
                for log in (output / mode / "cstrike/logs").glob("L*.log"):
                    shutil.copy2(log, target / log.name)
        print("Persistent report:", report, flush=True)


if __name__ == "__main__":
    main()
