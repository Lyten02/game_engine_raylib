#!/bin/bash
# Emergency stop script for runaway test processes

echo "🛑 EMERGENCY STOP - Killing all test processes..."

# Kill Python test processes
pkill -f "python.*test" || true
pkill -f "pytest" || true

# Kill game engine processes
pkill -f "game_engine" || true

# Kill build processes
pkill -f "cmake" || true
pkill -f "make" || true
pkill -f "ninja" || true
pkill -f "cc1plus" || true
pkill -f "clang" || true

# Kill any FetchContent downloads
pkill -f "git.*clone" || true
pkill -f "curl" || true
pkill -f "wget" || true

# Clean up zombie processes
ps aux | grep defunct | awk '{print $2}' | xargs kill -9 2>/dev/null || true

echo "✅ All test processes stopped"
echo "💡 Run this before starting tests: ./emergency_stop.sh"