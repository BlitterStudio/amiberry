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


def send_command(sock: socket.socket, command: str, *args: str) -> str:
    """Send a command and receive the response."""
    # Build message: COMMAND\tARG1\tARG2...\n
    parts = [command.upper()] + list(args)
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
Available commands:
  QUIT                      - Quit Amiberry
  PAUSE                     - Pause emulation
  RESUME                    - Resume emulation
  RESET [HARD|SOFT]         - Reset emulation (default: SOFT)
  SCREENSHOT <filename>     - Take screenshot
  SAVESTATE <state> <cfg>   - Save state and config
  LOADSTATE <statefile>     - Load state file
  DISKSWAP <disknum> <drv>  - Swap disk to drive
  QUERYDISKSWAP <drivenum>  - Query disk in drive
  INSERTFLOPPY <path> <drv> - Insert floppy in drive (0-3)
  INSERTCD <path>           - Insert CD image
  GET_STATUS                - Get emulation status
  GET_CONFIG <option>       - Get config option
  SET_CONFIG <option> <val> - Set config option
  LOAD_CONFIG <path>        - Load config file
  SEND_KEY <keycode> <state>- Send key (state: 0=up, 1=down)
  READ_MEM <addr> <width>   - Read memory (width: 1, 2, or 4)
  WRITE_MEM <addr> <w> <v>  - Write memory

Config options for GET_CONFIG/SET_CONFIG:
  chipmem_size, fastmem_size, bogomem_size, z3fastmem_size
  cpu_model, cpu_speed, cpu_compatible, cpu_24bit_addressing
  chipset, ntsc, floppy_speed, nr_floppies
  gfx_width, gfx_height, gfx_fullscreen
  sound_output, sound_stereo, sound_volume
  joyport0, joyport1, description
  turbo_emulation

Type 'help' for this message, 'exit' or 'quit' to exit.
""")


def interactive_mode(socket_path: str):
    """Run in interactive mode."""
    print(f"Amiberry IPC Client - Connected to {socket_path}")
    print("Type 'help' for available commands, 'exit' to quit.\n")

    while True:
        try:
            line = input("amiberry> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nExiting.")
            break

        if not line:
            continue

        if line.lower() in ('exit', 'quit', 'q'):
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
