#!/usr/bin/env python3
"""Check the assignment's behavior after building the programs."""
import datetime
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent
os.environ["LC_ALL"] = "C"
checks = 0


def check(condition, message):
    global checks
    if not condition:
        raise AssertionError(message)
    checks += 1


def run(number, args=(), data=b"", ok=True, env=None):
    result = subprocess.run(
        [str(ROOT / ("z%d" % number)), *map(str, args)],
        input=data, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        timeout=10, env=env,
    )
    check((result.returncode == 0) == ok,
          "z%d: status=%d stdout=%r stderr=%r" %
          (number, result.returncode, result.stdout, result.stderr))
    return result.stdout, result.stderr


_, err = run(1)
check(b"Usage:" in err, "No arguments")
for args in [("-x",), ("-U",), ("-i-p",), ("-i", "-", "-p"),
             ("-Uabc",), ("-U-1",), ("-U12abc",)]:
    run(1, args, ok=False)
out, _ = run(1, ("-i", "-p", "-i"))
check(out.startswith(b"UID:") and out.count(b"EUID:") == 2 and
      out.index(b"PID:") < out.rindex(b"UID:"), "Reverse repeated options")
out, _ = run(1, ("-ip",))
check(out.startswith(b"PID:"), "Grouped options")
out, _ = run(1, ("-p", "-s", "-p"))
pids = re.findall(rb"PID: (\d+)\nPPID: \d+\nPGRP: (\d+)", out)
check(len(pids) == 2 and pids[0][0] == pids[1][1], "setpgid")
out, _ = run(1, ("-u", "-U1", "-u", "-U2"))
check(out.splitlines() == [b"ulimit: 2 (512-byte blocks)",
                           b"ulimit: 1 (512-byte blocks)"],
      "Repeated -U arguments and units")
out, _ = run(1, ("-c", "-C0"))
check(b"core limit: 0 bytes" in out, "Core limit")
out, _ = run(1, ("-d",))
check(out.strip() == os.fsencode(os.getcwd()), "Working directory")
environment = dict(os.environ)
environment.pop("OS_LAB_TEST", None)
out, _ = run(1, ("-v", "-VOS_LAB_TEST=first",
                 "-v", "-VOS_LAB_TEST=second"), env=environment)
check(re.findall(rb"^OS_LAB_TEST=(.*)$", out, re.M) == [b"second", b"first"],
      "Environment changes in reverse order")

before = datetime.datetime.now(datetime.timezone.utc)
out, _ = run(2, env=dict(os.environ, TZ="UTC-12"))
after = datetime.datetime.now(datetime.timezone.utc)
shown = datetime.datetime.strptime(out.decode()[:19], "%Y-%m-%d %H:%M:%S")
shown = shown.replace(tzinfo=datetime.timezone.utc) + datetime.timedelta(hours=8)
check(before - datetime.timedelta(seconds=1) <= shown <= after,
      "PST = UTC-8 regardless of host timezone")

for data, expected in [(b"one\ntwo\n.stop\nignored\n", b"one\ntwo\n"),
                       (b".\n", b""), (b"\nhello\n.\n", b"\nhello\n"),
                       (b"last", b"last")]:
    out, _ = run(4, data=data)
    check(out.split(b"Saved lines:\n", 1)[1] == expected, "Linked list")

with tempfile.TemporaryDirectory(prefix="os-lab-") as directory:
    folder = Path(directory)
    path = folder / "text.txt"
    for number in (5, 6, 7):
        for content, requests, expected in [
            (b"first\n\nlast\n", b"3\n2\n1\n0\n", [b": last\n", b": first\n"]),
            (b"", b"1\n0\n", [b"No such line."]),
            (b"x\n", b"2\n1\n0\n", [b"No such line.", b": x\n"]),
            (b"last", b"1\n0\n", [b": last"]),
            (b"\n\n", b"3\n2\n0\n", [b"No such line.", b": \n"]),
        ]:
            path.write_bytes(content)
            out, _ = run(number, (path,), requests)
            check(all(item in out for item in expected),
                  "z%d: line indexing %r" % (number, content))
        run(number, (folder / "missing",), ok=False)

    secret = folder / "data.txt"
    secret.write_text("owner-only data\n")
    secret.chmod(0o600)
    out, _ = run(3, (secret,))
    check(out.count(b"fopen(r+): success") == 2, "Owner opens twice")
    out, err = run(3, (folder / "missing",), ok=False)
    check(out.count(b"EUID:") == 2 and err.count(b"No such file") == 2,
          "First failure does not skip the second attempt")

    data = b"first\n\nlast"
    path.write_bytes(data)
    processes = []
    started = time.monotonic()
    for number in (6, 7):
        process = subprocess.Popen([str(ROOT / ("z%d" % number)), str(path)],
                                   stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.stdin.write(b"2")
        process.stdin.flush()
        processes.append(process)
    for process in processes:
        process.wait(timeout=8)
        process.stdin.close()
        output = process.stdout.read()
        errors = process.stderr.read()
        check(process.returncode == 0 and not errors and
              output.endswith(b"Time is up. Entire file:\n" + data),
              "Partial input times out and the entire file is printed")
    check(4.5 <= time.monotonic() - started < 9, "Five-second alarm")

    processes = []
    for number in (6, 7):
        processes.append(subprocess.Popen(
            [str(ROOT / ("z%d" % number)), str(path)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE))
    time.sleep(3)
    for process in processes:
        process.stdin.write(b"1\n")
        process.stdin.flush()
    time.sleep(3)
    for process in processes:
        check(process.poll() is None, "Fresh alarm after a successful request")
        output, errors = process.communicate(b"0\n", timeout=3)
        check(process.returncode == 0 and not errors and
              b"first\n" in output and b"Time is up" not in output,
              "Timely number and zero exit")

print("PASS: %d checks" % checks)
