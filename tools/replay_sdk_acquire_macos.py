#!/usr/bin/env python3
"""Run the shared pinned HTTPS SDK acquisition cases under native macOS Seatbelt."""
from replay_macos_sandbox import MacSandbox
from replay_sdk_acquire_linux import main


if __name__ == '__main__':
    raise SystemExit(main(MacSandbox()))
