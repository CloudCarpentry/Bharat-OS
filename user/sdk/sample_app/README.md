# Sample Application

This directory provides a minimal example of a user-space application built against the Bharat-OS SDK.

## Purpose
Demonstrates how to invoke core system calls (like `write` and `exit`) and ensures the SDK builds and links correctly without external libc dependencies.

## Structure
- `main.c`: Contains the entry point `_start`, prints a message via the SDK's `bharat_write`, and exits via `bharat_exit`.

## Building
This application is automatically built when running the top-level SDK build script:

```bash
cd user/sdk/
./build.sh build --target x86_64
```
The resulting executable will be located in `user/sdk/build/x86_64/sample_app`.
