# cmus-rpc-c

A lightweight tool written in C that integrates Discord Rich Presence with C\* Music Player (cmus). It displays the currently playing song as the user's status by leveraging the Discord Game SDK for real-time status updates.

## Building from source

```bash
git clone https://github.com/ZirixCZ/cmus-rpc-c && cd cmus-rpc-c

# On aarch64
gcc main.c -o cmus_rpc_c -I. ./lib/aarch64/discord_game_sdk.dylib -Wl,-rpath,@loader_path/lib/aarch64

# On x86_64
gcc main.c -o cmus_rpc_c -I. ./lib/x86_64/discord_game_sdk.so -Wl,-rpath,@loader_path/lib/x86_64

# Discord game SDK versions for other architectures are provided in the lib folder
```

## Usage

cmus-rpc-c doesn't require any arguments to be passed to it. Just run the executable and it will automatically connect to cmus and the Discord client and start updating the user's status.
The applications do not need to be started in a particular order.

```bash
./cmus_rpc_c
```

## Debug

If you want to see the debug messages, you can define the `DEBUG` macro before compiling the source code.

```bash
gcc main.c -o cmus_rpc_c -I. ./lib/aarch64/discord_game_sdk.dylib -Wl,-rpath,@loader_path/lib/aarch64 -DDEBUG

```

## Folder structure

### Description

- **`discord_game_sdk.h`**: The header file for integrating Discord Game SDK.
- **`lib/`**: Contains precompiled libraries for different platforms/architectures:
  - **`aarch64/`**: For ARM 64-bit architecture.
  - **`x86/`**: For 32-bit x86 architecture.
  - **`x86_64/`**: For 64-bit x86 architecture.
- **`main.c`**: The main source code file for the application.

### Notes

- The `lib/` directory contains platform-specific libraries required to build and run the application.
