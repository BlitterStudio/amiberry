#!/usr/bin/env python3
"""
Simple test client for Amiberry Unix socket IPC.

Usage:
    python ipc_client.py                    # Interactive mode
    python ipc_client.py PAUSE              # Send single command
    python ipc_client.py GET_STATUS         # Query status
    python ipc_client.py SET_CONFIG floppy_speed 100

Socket path: /tmp/amiberry.sock (or $XDG_RUNTIME_DIR/amiberry.sock)
"""

import os
import socket
import sys
import readline  # For command history in interactive mode


def get_socket_path() -> str:
    """Determine the socket path based on environment."""
    xdg_runtime = os.environ.get('XDG_RUNTIME_DIR')
    if xdg_runtime:
        path = os.path.join(xdg_runtime, 'amiberry.sock')
        if os.path.exists(path):
            return path
    return '/tmp/amiberry.sock'


def expand_path(path: str) -> str:
    """Expand ~ and environment variables in a path."""
    return os.path.expanduser(os.path.expandvars(path))


def send_command(sock: socket.socket, command: str, *args: str) -> str:
    """Send a command and receive the response."""
    # Expand paths in arguments (for commands that take file paths)
    expanded_args = [expand_path(arg) for arg in args]

    # Build message: COMMAND\tARG1\tARG2...\n
    parts = [command.upper()] + expanded_args
    message = '\t'.join(parts) + '\n'

    sock.sendall(message.encode('utf-8'))

    # Receive response
    response = b''
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            break
        response += chunk
        if b'\n' in response:
            break

    return response.decode('utf-8').strip()


def parse_response(response: str) -> tuple[bool, list[str]]:
    """Parse response into (success, data_parts)."""
    parts = response.split('\t')
    if not parts:
        return False, ['Empty response']

    success = parts[0] == 'OK'
    data = parts[1:] if len(parts) > 1 else []
    return success, data


def print_help():
    """Print available commands."""
    print("""
=== Emulation Control ===
  QUIT                      - Quit Amiberry
  PAUSE                     - Pause emulation
  RESUME                    - Resume emulation
  RESET [HARD|SOFT]         - Reset emulation (default: SOFT)
  FRAME_ADVANCE [n]         - Advance n frames (when paused, default: 1)

=== Floppy Disk ===
  INSERTFLOPPY <path> <drv> - Insert floppy in drive (0-3)
  EJECT_FLOPPY <drivenum>   - Eject floppy from drive (0-3)
  LIST_FLOPPIES             - List all floppy drives and contents
  DISKSWAP <disknum> <drv>  - Swap disk from swapper to drive
  QUERYDISKSWAP <drivenum>  - Query which swapper disk is in drive
  SET_FLOPPY_SPEED <speed>  - Set speed (0=turbo, 100=1x, 200=2x, 400=4x, 800=8x)
  GET_FLOPPY_SPEED          - Get current floppy speed
  DISK_WRITE_PROTECT <d> <p>- Set write protection (drive 0-3, p: 0|1)
  GET_DISK_WRITE_PROTECT <d>- Get write protection status

=== CD-ROM ===
  INSERTCD <path>           - Insert CD image
  EJECT_CD                  - Eject CD

=== Hard Drives ===
  LIST_HARDDRIVES           - List mounted hard drives/directories

=== WHDLoad ===
  INSERT_WHDLOAD <path>     - Load WHDLoad game/demo from path
  EJECT_WHDLOAD             - Eject current WHDLoad content
  GET_WHDLOAD               - Get current WHDLoad status

=== State Management ===
  SCREENSHOT <filename>     - Take screenshot (supports ~/path)
  SAVESTATE <state> <cfg>   - Save state and config files
  LOADSTATE <statefile>     - Load state file
  QUICKSAVE [slot]          - Quick save to slot (0-9, default: 0)
  QUICKLOAD [slot]          - Quick load from slot (0-9, default: 0)

=== Audio ===
  SET_VOLUME <0-100>        - Set master volume (0-100)
  GET_VOLUME                - Get current volume
  MUTE                      - Mute audio
  UNMUTE                    - Unmute audio

=== Display ===
  TOGGLE_FULLSCREEN         - Toggle fullscreen/windowed mode
  SET_WARP <0|1>            - Enable/disable warp mode
  GET_WARP                  - Get warp mode status
  SET_DISPLAY_MODE <mode>   - Set mode (0=window, 1=fullscreen, 2=fullwindow)
  GET_DISPLAY_MODE          - Get current display mode
  SET_NTSC <0|1>            - Set video mode (0=PAL, 1=NTSC)
  GET_NTSC                  - Get current video mode (PAL/NTSC)
  TOGGLE_RTG [monid]        - Toggle between RTG and chipset display
  TOGGLE_STATUS_LINE        - Cycle status line (off/chipset/rtg/both)
  GET_FPS                   - Get current frame rate and idle percentage
  SET_WINDOW_SIZE <w> <h>   - Set window size (width, height)
  GET_WINDOW_SIZE           - Get current window size
  SET_SCALING <mode>        - Set scaling mode
  GET_SCALING               - Get current scaling mode
  SET_LINE_MODE <mode>      - Set line mode (scanlines, etc.)
  GET_LINE_MODE             - Get current line mode
  SET_RESOLUTION <w> <h>    - Set emulated screen resolution
  GET_RESOLUTION            - Get current resolution
  SET_AUTOCROP <0|1>        - Enable/disable automatic display cropping
  GET_AUTOCROP              - Get autocrop status

=== Sound ===
  SET_SOUND_MODE <mode>     - Set mode (0=off, 1=normal, 2=stereo, 3=best)
  GET_SOUND_MODE            - Get current sound mode

=== Configuration ===
  GET_STATUS                - Get emulation status
  GET_CONFIG <option>       - Get config option value
  SET_CONFIG <option> <val> - Set config option
  LOAD_CONFIG <path>        - Load config file
  LIST_CONFIGS              - List available config files

=== Input ===
  SEND_KEY <keycode> <state>- Send key (state: 0=release, 1=press)
  SEND_MOUSE <dx> <dy> <btn>- Send mouse input (btn: bit0=L, bit1=R, bit2=M)
  SET_MOUSE_SPEED <10-200>  - Set mouse sensitivity
  GET_MOUSE_SPEED           - Get current mouse speed
  TOGGLE_MOUSE_GRAB         - Toggle mouse capture

=== Joystick/Gamepad ===
  GET_JOYPORT_MODE <port>   - Get port mode (port: 0-3)
  SET_JOYPORT_MODE <p> <m>  - Set port mode (modes: 0=default, 2=mouse, 3=joy, 7=cd32)
  GET_AUTOFIRE <port>       - Get autofire mode for port
  SET_AUTOFIRE <port> <m>   - Set autofire (0=off, 1=normal, 2=toggle, 3=always)

=== Status ===
  GET_LED_STATUS            - Get all LED states (power, floppy, HD, CD)

=== Hardware ===
  SET_CHIPSET <chipset>     - Set chipset (OCS, ECS_AGNUS, ECS_DENISE, ECS, AGA)
  GET_CHIPSET               - Get current chipset
  SET_CPU_MODEL <model>     - Set CPU model (68000, 68010, 68020, 68030, 68040, 68060)
  GET_CPU_MODEL             - Get current CPU model
  SET_CPU_SPEED <speed>     - Set CPU speed (-1=max, 0=cycle-exact, >0=%)
  GET_CPU_SPEED             - Get current CPU speed setting
  GET_MEMORY_CONFIG         - Get all memory sizes (chip, fast, bogo, z3, rtg)
  SET_CHIP_MEM <size>       - Set chip memory size (in KB)
  SET_FAST_MEM <size>       - Set fast memory size (in KB)
  SET_SLOW_MEM <size>       - Set slow (bogo) memory size (in KB)
  SET_Z3_MEM <size>         - Set Zorro III memory size (in KB)

=== Memory Access ===
  READ_MEM <addr> <width>   - Read memory (width: 1, 2, or 4 bytes)
  WRITE_MEM <addr> <w> <v>  - Write value to memory

=== Debugging ===
  DEBUG_ACTIVATE            - Activate the debugger
  DEBUG_DEACTIVATE          - Deactivate the debugger
  DEBUG_STATUS              - Get debugger status
  DEBUG_STEP                - Step one instruction
  DEBUG_STEP_OVER           - Step over (skip subroutine calls)
  DEBUG_CONTINUE            - Continue execution until next breakpoint
  GET_CPU_REGS              - Get all CPU register values
  GET_CUSTOM_REGS           - Get Amiga custom chip register values
  DISASSEMBLE <addr> [n]    - Disassemble n instructions at address
  SET_BREAKPOINT <addr>     - Set breakpoint at address
  CLEAR_BREAKPOINT <addr>   - Clear breakpoint at address
  LIST_BREAKPOINTS          - List all active breakpoints
  GET_COPPER_STATE          - Get Copper co-processor state
  GET_BLITTER_STATE         - Get Blitter state
  GET_DRIVE_STATE           - Get floppy drive state (track, motor, etc.)
  GET_AUDIO_STATE           - Get audio channel states
  GET_DMA_STATE             - Get DMA channel states

=== Utility ===
  GET_VERSION               - Get Amiberry version info
  PING                      - Test connection (returns PONG)
  HELP                      - Show this help (also works via IPC)

Config options for GET_CONFIG/SET_CONFIG:
  chipmem_size, fastmem_size, bogomem_size, z3fastmem_size
  cpu_model, cpu_speed, cpu_compatible, cpu_24bit_addressing
  chipset, ntsc, floppy_speed, nr_floppies
  gfx_width, gfx_height, gfx_fullscreen
  sound_output, sound_stereo, sound_volume
  joyport0, joyport1, description, turbo_emulation

Type 'help' for this message, 'exit' or 'q' to quit the client.
Note: Use 'QUIT' to quit Amiberry (the emulator).
""")


def interactive_mode(socket_path: str):
    """Run in interactive mode."""
    print(f"Amiberry IPC Client - Connected to {socket_path}")
    print("Type 'help' for available commands, 'exit' or 'q' to quit the client.\n")

    while True:
        try:
            line = input("amiberry> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nExiting.")
            break

        if not line:
            continue

        if line.lower() in ('exit', 'q'):
            break

        if line.lower() == 'help':
            print_help()
            continue

        # Parse command and args
        parts = line.split()
        command = parts[0]
        args = parts[1:]

        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.connect(socket_path)
                sock.settimeout(5.0)
                response = send_command(sock, command, *args)

            success, data = parse_response(response)

            if success:
                print(f"OK: {' | '.join(data) if data else 'Success'}")
            else:
                print(f"ERROR: {' | '.join(data) if data else 'Unknown error'}")

        except socket.error as e:
            print(f"Socket error: {e}")
        except Exception as e:
            print(f"Error: {e}")


def single_command_mode(socket_path: str, command: str, args: list[str]):
    """Send a single command and exit."""
    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
            sock.connect(socket_path)
            sock.settimeout(5.0)
            response = send_command(sock, command, *args)

        success, data = parse_response(response)

        if success:
            if data:
                for item in data:
                    print(item)
            else:
                print("OK")
            sys.exit(0)
        else:
            print(f"ERROR: {' '.join(data) if data else 'Unknown error'}", file=sys.stderr)
            sys.exit(1)

    except FileNotFoundError:
        print(f"Error: Socket not found at {socket_path}", file=sys.stderr)
        print("Is Amiberry running with USE_IPC_SOCKET enabled?", file=sys.stderr)
        sys.exit(1)
    except ConnectionRefusedError:
        print(f"Error: Connection refused to {socket_path}", file=sys.stderr)
        sys.exit(1)
    except socket.timeout:
        print("Error: Connection timed out", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


def main():
    socket_path = get_socket_path()

    if len(sys.argv) > 1:
        # Command-line mode
        command = sys.argv[1]
        args = sys.argv[2:]
        single_command_mode(socket_path, command, args)
    else:
        # Interactive mode
        if not os.path.exists(socket_path):
            print(f"Warning: Socket not found at {socket_path}")
            print("Is Amiberry running with USE_IPC_SOCKET enabled?\n")
        interactive_mode(socket_path)


if __name__ == '__main__':
    main()
