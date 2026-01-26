# Quick Start Guide

## Fast Setup (5 minutes)

### 1. Backend
```bash
cd backend
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python main.py
```

### 2. Plugin (in new terminal)
```bash
cd plugin
git clone https://github.com/juce-framework/JUCE.git
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### 3. Use in DAW
1. Rescan plugins in your DAW
2. Load "Auxlee Audio Recorder" on a track
3. Connect to `http://localhost:8000`
4. Username: `admin`, Password: `password123`
5. Start recording!

## Default Credentials
- Username: `admin`
- Password: `password123`

**⚠️ Change these in `backend/config.py` before production use!**
