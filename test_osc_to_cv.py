#!/usr/bin/env python3
"""
Test script for OSC to CV conversion functionality
Sends test OSC messages to the CV to OSC Converter application
"""

import time
import math
import argparse
from pythonosc import udp_client

def send_test_signals(host="127.0.0.1", port=8001, duration=30):
    """Send various test signals to test OSC to CV conversion"""
    
    print(f"🎛️  OSC to CV Test Script")
    print(f"Target: {host}:{port}")
    print(f"Duration: {duration} seconds")
    print("=" * 50)
    
    # Create OSC client
    client = udp_client.SimpleUDPClient(host, port)
    
    start_time = time.time()
    
    try:
        while time.time() - start_time < duration:
            current_time = time.time() - start_time
            
            # Channel 1: Sine wave (0.1 Hz)
            sine_value = (math.sin(current_time * 0.1 * 2 * math.pi) + 1) / 2
            client.send_message("/cv/channel/1", sine_value)
            
            # Channel 2: Triangle wave (0.05 Hz)
            triangle_value = abs((current_time * 0.05) % 2 - 1)
            client.send_message("/cv/channel/2", triangle_value)
            
            # Channel 3: Sawtooth wave (0.2 Hz)
            sawtooth_value = (current_time * 0.2) % 1
            client.send_message("/cv/channel/3", sawtooth_value)
            
            # Channel 4: Square wave (0.15 Hz)
            square_value = 1.0 if (current_time * 0.15) % 1 < 0.5 else 0.0
            client.send_message("/cv/channel/4", square_value)
            
            # Channel 5: Random walk
            if not hasattr(send_test_signals, 'random_value'):
                send_test_signals.random_value = 0.5
            
            import random
            send_test_signals.random_value += (random.random() - 0.5) * 0.1
            send_test_signals.random_value = max(0.0, min(1.0, send_test_signals.random_value))
            client.send_message("/cv/channel/5", send_test_signals.random_value)
            
            # Print status every 5 seconds
            if int(current_time) % 5 == 0 and abs(current_time - int(current_time)) < 0.1:
                print(f"⏰ {int(current_time)}s - Sending: Ch1={sine_value:.3f}, Ch2={triangle_value:.3f}, Ch3={sawtooth_value:.3f}, Ch4={square_value:.3f}, Ch5={send_test_signals.random_value:.3f}")
            
            time.sleep(0.1)  # 10 Hz update rate
            
    except KeyboardInterrupt:
        print("\n🛑 Test interrupted by user")
    except Exception as e:
        print(f"❌ Error: {e}")
    
    print("✅ Test completed")

def send_single_values(host="127.0.0.1", port=8001):
    """Send single test values to specific channels"""
    
    print(f"🎯 Sending single test values to {host}:{port}")
    
    client = udp_client.SimpleUDPClient(host, port)
    
    test_values = [
        (1, 0.0, "Minimum"),
        (2, 0.25, "Quarter"),
        (3, 0.5, "Half"),
        (4, 0.75, "Three quarters"),
        (5, 1.0, "Maximum"),
        (6, 0.33, "One third"),
        (7, 0.66, "Two thirds"),
        (8, 0.1, "Low"),
    ]
    
    for channel, value, description in test_values:
        try:
            client.send_message(f"/cv/channel/{channel}", value)
            print(f"📤 Channel {channel}: {value:.3f} ({description})")
            time.sleep(0.5)
        except Exception as e:
            print(f"❌ Error sending to channel {channel}: {e}")

def main():
    parser = argparse.ArgumentParser(description="Test OSC to CV conversion")
    parser.add_argument("--host", default="127.0.0.1", help="OSC host (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8001, help="OSC port (default: 8001)")
    parser.add_argument("--duration", type=int, default=30, help="Test duration in seconds (default: 30)")
    parser.add_argument("--mode", choices=["continuous", "single"], default="continuous", 
                       help="Test mode: continuous waves or single values")
    
    args = parser.parse_args()
    
    print("🎵 CV to OSC Converter - OSC Test Script")
    print("=" * 50)
    
    if args.mode == "continuous":
        print("🌊 Running continuous wave test...")
        send_test_signals(args.host, args.port, args.duration)
    else:
        print("🎯 Sending single test values...")
        send_single_values(args.host, args.port)
    
    print("\n💡 Make sure the CV to OSC Converter GUI is running with OSC listening enabled!")
    print("   1. Open OSC Configuration window")
    print("   2. Set listen port to", args.port)
    print("   3. Click 'Start OSC Listening'")
    print("   4. Open individual channel windows to see the signals")

if __name__ == "__main__":
    try:
        main()
    except ImportError as e:
        if "pythonosc" in str(e):
            print("❌ Error: python-osc library not found")
            print("📦 Install with: pip install python-osc")
        else:
            print(f"❌ Import error: {e}")
    except Exception as e:
        print(f"❌ Error: {e}")
