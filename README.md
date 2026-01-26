# Auxlee Audio Plugin & API

A JUCE audio plugin with Python backend API for streaming and storing audio recordings from your DAW.

## Project Structure

```
auxleeai/
├── plugin/              # JUCE audio plugin (C++)
│   ├── Source/         # Plugin source files
│   └── CMakeLists.txt  # CMake build configuration
└── backend/            # Python FastAPI backend
    ├── main.py         # API server
    ├── config.py       # Configuration
    └── requirements.txt
```

## Features

### Audio Plugin
- **Real-time audio streaming**: Captures audio from DAW channels and streams to backend
- **Non-destructive**: Audio passes through unchanged
- **Chunk-based streaming**: Sends audio in manageable chunks
- **Authentication**: Secure HTTP Basic Auth
- **Intuitive UI**: Simple controls for connection and recording

### Backend API
- **Audio chunk reception**: Receives and stores audio chunks
- **Session management**: Organizes chunks into recording sessions
- **Auto-assembly**: Combines chunks into complete WAV files
- **Track management**: List, download, and delete recorded tracks
- **User authentication**: HTTP Basic Auth for secure access

## Prerequisites

### For the Plugin
- CMake 3.15 or higher
- C++17 compatible compiler
  - macOS: Xcode Command Line Tools
  - Windows: Visual Studio 2019+
  - Linux: GCC 7+ or Clang 6+
- JUCE Framework (v7.0+)

### For the Backend
- Python 3.8 or higher
- pip package manager

## Setup Instructions

### 1. Backend Setup

Navigate to the backend directory and install dependencies:

```bash
cd backend
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
```

### 2. Configure Authentication

Edit `backend/config.py` or create a `.env` file to set credentials:

```python
# Default credentials (CHANGE THESE!)
valid_users = {
    "admin": "your_secure_password"
}
```

### 3. Start the Backend Server

```bash
cd backend
python main.py
```

The API will be available at `http://localhost:8000`

### 4. Plugin Setup

First, clone the JUCE framework into the plugin directory:

```bash
cd plugin
git clone https://github.com/juce-framework/JUCE.git
```

### 5. Build the Plugin

```bash
cd plugin
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 6. Install the Plugin

After building, the plugin will be copied to your system's plugin folder:
- **macOS**: `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/`
- **Windows**: `C:\Program Files\Common Files\VST3\`
- **Linux**: `~/.vst3/`

## Usage

### Starting a Recording Session

1. Open your DAW and load the **Auxlee Audio Recorder** plugin on a track
2. In the plugin UI, enter:
   - API URL: `http://localhost:8000`
   - Username: `admin`
   - Password: (your configured password)
3. Click **Connect**
4. Click **Start Recording** to begin streaming audio
5. Click **Stop Recording** when done

### Managing Recorded Tracks

You can interact with the API directly:

```bash
# List all tracks
curl -u admin:password http://localhost:8000/api/tracks

# Download a track
curl -u admin:password http://localhost:8000/api/download/{track_id} -o recording.wav

# Delete a track
curl -X DELETE -u admin:password http://localhost:8000/api/tracks/{track_id}
```

## API Endpoints

- `POST /api/start-session` - Start a new recording session
- `POST /api/upload-chunk` - Upload audio chunk
- `POST /api/finalize-session/{session_id}` - Finalize session and create track
- `GET /api/tracks` - List all tracks for authenticated user
- `GET /api/download/{track_id}` - Download track
- `DELETE /api/tracks/{track_id}` - Delete track

## Architecture

### Audio Flow
1. Plugin captures audio from DAW in real-time
2. Audio is buffered into 2-second chunks
3. Chunks are encoded as WAV and sent via HTTP POST
4. Backend receives chunks and stores them
5. When recording stops, chunks are assembled into complete file

### Authentication Flow
1. Plugin sends credentials via HTTP Basic Auth header
2. Backend verifies credentials before accepting requests
3. All API endpoints require authentication

## Development

### Plugin Development
- Main processor: [plugin/Source/PluginProcessor.cpp](plugin/Source/PluginProcessor.cpp)
- UI: [plugin/Source/PluginEditor.cpp](plugin/Source/PluginEditor.cpp)
- Audio streaming: [plugin/Source/AudioStreamer.cpp](plugin/Source/AudioStreamer.cpp)
- Network client: [plugin/Source/NetworkClient.cpp](plugin/Source/NetworkClient.cpp)

### Backend Development
- Main API: [backend/main.py](backend/main.py)
- Configuration: [backend/config.py](backend/config.py)

To run backend in development mode with auto-reload:
```bash
uvicorn main:app --reload --host 0.0.0.0 --port 8000
```

## Security Notes

⚠️ **Important**: This implementation uses HTTP Basic Authentication with plain text passwords. For production use:

1. Use HTTPS/TLS encryption
2. Implement proper password hashing (bcrypt)
3. Consider OAuth2 or JWT tokens
4. Store credentials in environment variables
5. Add rate limiting and request validation
6. Implement proper session management

## Troubleshooting

### Plugin doesn't appear in DAW
- Verify plugin was built successfully
- Check it was installed to correct plugin folder
- Rescan plugins in your DAW

### Connection errors
- Ensure backend server is running
- Check firewall settings
- Verify API URL is correct (include http://)
- Confirm credentials match backend configuration

### Audio quality issues
- Check sample rate matches between DAW and plugin
- Verify network bandwidth is sufficient
- Monitor backend server logs for errors

## License

This project is provided as-is for educational and development purposes.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.
