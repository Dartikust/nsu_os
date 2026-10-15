#!/usr/bin/env python3
"""Check the assignment's behavior after building the programs."""
import datetime
import os
from pathlib import Path
import re
import pty
import select
import termios
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
out, err = run(1, ("-i17389",), ok=False)
check(b"Usage:" in err and b"UID:" not in out,
      "Malformed option prints usage before executing anything")
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

controls = b"\x1b[A \x1b[2J \r\b\t\v\x7f\n"
out, _ = run(4, data=controls + ".\n".encode())
check(out.split(b"Saved lines:\n", 1)[1] ==
      b"^[[A ^[[2J ^M^H^I^K^?\n",
      "Control sequences are stored as visible text")
check(b"\x1b" not in out, "No active escape characters in output")
out, _ = run(4, data=b"\x1b[A\x1b[B\x1b[C\x1b[D \x1bOA\x1bOB\x1bOC\x1bOD\n.\n")
check(out.split(b"Saved lines:\n", 1)[1] == b"^[[A^[[B^[[C^[[D ^[OA^[OB^[OC^[OD\n",
      "Arrow keys use the same caret notation as terminal echo")
out, _ = run(4, data=b"\x1b\n\x1b[\n.\n")
check(out.split(b"Saved lines:\n", 1)[1] == b"^[\n^[[\n",
      "Incomplete escape sequences stay visible")
russian = "строка ↑\n".encode()
out, _ = run(4, data=russian + b".\n")
check(out.split(b"Saved lines:\n", 1)[1] == russian,
      "UTF-8 text is preserved")

# Check the real terminal path too, with unsafe echo initially enabled.
master, slave = pty.openpty()
original = termios.tcgetattr(slave)
original[3] &= ~termios.ECHOCTL
termios.tcsetattr(slave, termios.TCSANOW, original)
process = subprocess.Popen([str(ROOT / "z4")], stdin=slave, stdout=slave,
                           stderr=slave, close_fds=True)
terminal_output = b""
try:
    deadline = time.monotonic() + 3
    while b"finishes input:" not in terminal_output:
        ready, _, _ = select.select([master], [], [], max(0, deadline - time.monotonic()))
        if not ready:
            raise AssertionError("z4 terminal prompt did not appear")
        terminal_output += os.read(master, 4096)
    os.write(master, b"\x1b[A\n.\n")
    process.wait(timeout=3)
    while select.select([master], [], [], 0.1)[0]:
        terminal_output += os.read(master, 4096)
    check(process.returncode == 0 and terminal_output.count(b"^[[A") == 2 and
          b"\x1b" not in terminal_output, "Arrow key stays text during terminal input/output")
    check(termios.tcgetattr(slave) == original, "Original terminal settings restored")
finally:
    if process.poll() is None:
        process.kill()
        process.wait()
    os.close(master)
    os.close(slave)

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
        path.write_bytes(b"first\nsecond\n")
        for wrong in (b"abc", b"0abc", b"00", b"+0", b"-0", b"0 ",
                      b" 0", b"0 1", b"", b"-1", b"1abc",
                      b"0\x00junk", b"9" * 200 + b"0"):
            out, _ = run(number, (path,), wrong + b"\n1\n0\n2\n")
            check(out.count(b": first\n") == 1 and b": second\n" not in out and
                  b"exactly 0 to quit." in out,
                  "z%d: only an exact zero quits; invalid input %r" % (number, wrong))
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
        for first in (b"1\n", b"abc\n"):
            process = subprocess.Popen(
                [str(ROOT / ("z%d" % number)), str(path)],
                stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            process.stdin.write(first)
            process.stdin.flush()
            processes.append(process)
    time.sleep(6)
    for process in processes:
        check(process.poll() is None, "No timer after the first input, even if invalid")
        output, errors = process.communicate(b"0x\n2\n0\n", timeout=3)
        check(process.returncode == 0 and not errors and
              b": \n" in output and b"Time is up" not in output and
              output.count(b"5 seconds:") == 1,
              "Subsequent requests behave like task 5 without a deadline")

print("PASS: %d checks" % checks)
