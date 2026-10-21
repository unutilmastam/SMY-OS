#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 The SMY-OS Authors
"""Inject keystrokes into a QEMU guest through its monitor socket once the
guest asks for them (tests/integration/ktest.sh, KTEST_SENDKEYS).

Language: Python 3, standard library only (ADR 0002).

Usage: sendkeys.py <monitor.sock> <serial.log> <trigger-text> <timeout-s> <key>...
  Waits until <serial.log> contains <trigger-text>, then sends
  "sendkey <key>" for each key (QEMU key names: a, s, up, ret, shift-a ...),
  150 ms apart. Exits 0 when all keys were sent, 1 on timeout.
"""

import os
import socket
import sys
import time


def main(argv):
    if len(argv) < 5:
        print(__doc__, file=sys.stderr)
        return 2
    sock_path, log_path, trigger, timeout = argv[0], argv[1], argv[2], float(argv[3])
    keys = argv[4:]
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with open(log_path, "rb") as fh:
                if trigger.encode() in fh.read():
                    break
        except FileNotFoundError:
            pass
        time.sleep(0.1)
    else:
        print("sendkeys: trigger never appeared", file=sys.stderr)
        return 1
    if not os.path.exists(sock_path):
        print("sendkeys: no monitor socket", file=sys.stderr)
        return 1
    with socket.socket(socket.AF_UNIX) as s:
        s.connect(sock_path)
        for key in keys:
            s.sendall(f"sendkey {key}\n".encode())
            time.sleep(0.15)
        time.sleep(0.2)
    print(f"sendkeys: sent {' '.join(keys)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
