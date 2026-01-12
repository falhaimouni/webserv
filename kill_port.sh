#!/bin/bash
# Helper script to kill any process using port 8080

PORT=8080

echo "Checking for processes using port $PORT..."

# Try to find process using the port
PID=$(lsof -ti :$PORT 2>/dev/null)

if [ -z "$PID" ]; then
    # Try alternative method
    PID=$(fuser $PORT/tcp 2>/dev/null | awk '{print $1}')
fi

if [ -z "$PID" ]; then
    echo "No process found using port $PORT"
    exit 0
fi

echo "Found process(es) using port $PORT:"
ps -p $PID -o pid,cmd

read -p "Kill these processes? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    kill $PID 2>/dev/null
    sleep 1
    
    # Force kill if still running
    if kill -0 $PID 2>/dev/null; then
        echo "Force killing..."
        kill -9 $PID 2>/dev/null
    fi
    
    echo "Process killed. Port $PORT is now free."
else
    echo "Cancelled."
fi

