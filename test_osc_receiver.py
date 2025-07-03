#!/usr/bin/env python3
"""
Simple OSC receiver for testing CV to OSC Converter
"""

import argparse
import time
from pythonosc import dispatcher
from pythonosc import osc_server

def print_cv_message(unused_addr, *args):
    """Print received CV data"""
    timestamp = time.strftime("%H:%M:%S")
    print(f"[{timestamp}] CV Data: {args}")

def print_handler(unused_addr, *args):
    """Print any OSC message"""
    timestamp = time.strftime("%H:%M:%S")
    print(f"[{timestamp}] OSC: {unused_addr} -> {args}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", default="127.0.0.1", help="The ip to listen on")
    parser.add_argument("--port", type=int, default=9000, help="The port to listen on")
    args = parser.parse_args()

    # Set up dispatcher for OSC messages
    disp = dispatcher.Dispatcher()
    disp.map("/cv/channel/*", print_cv_message)  # Specific CV channel messages
    disp.map("/cv/*", print_cv_message)          # General CV messages
    disp.set_default_handler(print_handler)      # Catch all other messages

    # Create and start server
    server = osc_server.ThreadingOSCUDPServer((args.ip, args.port), disp)
    print(f"OSC Server listening on {args.ip}:{args.port}")
    print("Waiting for CV to OSC messages...")
    print("Press Ctrl+C to stop")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping OSC server...")
        server.server_close()
