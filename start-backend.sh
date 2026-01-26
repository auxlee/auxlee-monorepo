#!/bin/bash

# Auxlee Audio Backend Startup Script

echo "Starting Auxlee Audio Backend API..."

# Check if virtual environment exists
if [ ! -d "backend/venv" ]; then
    echo "Creating virtual environment..."
    cd backend
    python3 -m venv venv
    cd ..
fi

# Activate virtual environment
echo "Activating virtual environment..."
source backend/venv/bin/activate

# Install dependencies if needed
if ! python -c "import fastapi" 2>/dev/null; then
    echo "Installing dependencies..."
    pip install -r backend/requirements.txt
fi

# Start the server
echo "Starting FastAPI server on http://localhost:8000"
cd backend
python main.py
