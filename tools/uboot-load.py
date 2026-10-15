#!/usr/bin/env python3
"""
Load a raw binary into the board's RAM through the U-Boot prompt, using only "mw.l".

This U-Boot has no loady/loadx/loadb, but it can write memory one 32-bit word at a time.
We type one "mw.l <addr> <value>" per word and wait for the "=> " prompt after each line,
so we never send faster than U-Boot can read (the console has no flow control).
Then we read everything back with "md.l" to check it, and optionally "go".

If the board is not at the "=> " prompt we ask you to replug it and catch U-Boot. Nothing else (picocom, screen)
may have the serial port open.

  tools/uboot-load.py build/luckfox/kernel.bin --addr 0x00200000 --go
"""
import argparse
import re
import struct
import sys
import time

import serial

PROMPT = b"=> "


def wait_prompt(port, timeout=3.0):
    """Read until U-Boot prints its prompt. Returns everything read before it."""
    buf = b""
    deadline = time.monotonic() + timeout
    while not buf.endswith(PROMPT):
        chunk = port.read(port.in_waiting or 1)
        if chunk:
            buf += chunk
        elif time.monotonic() > deadline:
            raise TimeoutError(f"no U-Boot prompt; last output: {buf[-200:]!r}")
    return buf


def catch_uboot(port, timeout=60.0):
    """Spam Ctrl+C until U-Boot's prompt shows up, while the user power-cycles the board.
    This U-Boot's autoboot delay is 0, so Ctrl+C must already be arriving when it checks."""
    print("no '=> ' prompt: unplug and replug the Pico's USB-C now (waiting up to 60 s)...")
    buf = b""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        port.write(b"\x03")
        time.sleep(0.02)
        buf = (buf + port.read(port.in_waiting))[-64:]
        if PROMPT in buf:
            time.sleep(0.3)                  # let the queued Ctrl+Cs drain
            port.reset_input_buffer()
            command(port, "")
            print("caught U-Boot")
            return
    sys.exit("gave up waiting for U-Boot")


def command(port, line, timeout=3.0):
    port.write(line.encode() + b"\n")
    return wait_prompt(port, timeout)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("binary")
    ap.add_argument("--addr", type=lambda s: int(s, 0), default=0x00200000)
    ap.add_argument("--port", default="/dev/cu.usbserial-210")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--go", action="store_true", help="jump to the image after loading")
    ap.add_argument("--no-verify", action="store_true", help="skip the md.l read-back check")
    args = ap.parse_args()

    data = open(args.binary, "rb").read()
    data += b"\0" * (-len(data) % 4)                    # pad to whole words
    words = struct.unpack(f"<{len(data) // 4}I", data)  # ARM is little-endian

    try:
        port = serial.Serial(args.port, args.baud, timeout=0.1)
    except serial.SerialException as e:
        sys.exit(f"cannot open {args.port}: {e}\n(close picocom/screen first)")

    with port:
        port.reset_input_buffer()
        try:
            command(port, "", timeout=1)                 # already at "=> "?
        except TimeoutError:
            catch_uboot(port)

        start = time.monotonic()
        for i, w in enumerate(words):
            command(port, f"mw.l {args.addr + 4 * i:08x} {w:08x}")
            if i % 64 == 0 or i == len(words) - 1:
                print(f"\rwriting {4 * (i + 1)}/{len(data)} bytes", end="", flush=True)
        print(f"  ({time.monotonic() - start:.1f}s)")

        if not args.no_verify:
            out = command(port, f"md.l {args.addr:08x} {len(words):x}", timeout=10).decode(errors="replace")
            # md.l lines look like: "00200000: f10c00c0 e1a0400d e1a0500e e1a06000    .....@...P...`.."
            got = []
            for m in re.finditer(r"^([0-9a-f]{8}):((?: [0-9a-f]{8}){1,4})", out, re.M):
                got += [int(x, 16) for x in m.group(2).split()]
            bad = [i for i, (a, b) in enumerate(zip(words, got)) if a != b]
            if len(got) < len(words) or bad:
                first = bad[0] if bad else len(got)
                sys.exit(f"verify FAILED at {args.addr + 4 * first:#010x}")
            print(f"verified {len(data)} bytes at {args.addr:#010x}")

        if args.go:
            # U-Boot runs with the D-cache on, so our mw.l data may still be sitting in the
            # cache, not in RAM, and instruction fetches read RAM. This U-Boot has no
            # "dcache flush" command, so push our lines out by copying 1 MB of unused RAM
            # (16-21 MB, below U-Boot's relocated copy at the top of RAM).
            command(port, "cp.l 0x01000000 0x01400000 0x40000", timeout=10)
            port.write(f"go {args.addr:08x}\n".encode())
            # print the kernel's output until it returns to the prompt (or 5 s pass)
            out = b""
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline and not out.endswith(PROMPT):
                out += port.read(port.in_waiting or 1)
            sys.stdout.write(out.decode(errors="replace"))
            print()


if __name__ == "__main__":
    main()
