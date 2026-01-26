# Contributing to Auxlee Audio Recorder

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## Code of Conduct

Be respectful, constructive, and professional in all interactions.

## How to Contribute

### Reporting Bugs

Before submitting a bug report:
1. Check if the issue already exists in the issue tracker
2. Test with the latest version
3. Collect relevant information (OS, DAW version, plugin format)

When reporting, include:
- Clear description of the issue
- Steps to reproduce
- Expected vs actual behavior
- System information (macOS version, GarageBand/Logic version, etc.)
- Backend logs if applicable
- Console.app logs if the plugin crashes

### Suggesting Features

Feature requests are welcome! Please:
- Check if it's already suggested
- Explain the use case and why it's valuable
- Consider if it fits the project's scope

### Pull Requests

1. **Fork the repository** and create a feature branch
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes**
   - Follow existing code style
   - Add comments for complex logic
   - Keep commits focused and atomic

3. **Test thoroughly**
   - Build both Debug and Release configurations
   - Test in multiple DAWs if possible
   - Verify backend API changes with curl/Postman
   - Run `auval` validation for plugin changes

4. **Submit the PR**
   - Provide a clear description of changes
   - Reference any related issues
   - Include screenshots/videos for UI changes

## Development Setup

### Plugin (C++/JUCE)

```bash
cd plugin
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Backend (Python)

```bash
cd backend
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

## Code Style

### C++ (Plugin)
- Use camelCase for variables and methods
- Use PascalCase for classes
- Follow JUCE conventions
- Keep methods focused and concise
- Use `const` where appropriate

### Python (Backend)
- Follow PEP 8
- Use type hints where helpful
- Keep functions focused
- Add docstrings for public APIs

## Project Structure

```
auxleeai/
├── plugin/          # JUCE audio plugin (C++)
│   ├── Source/      # Plugin source code
│   └── CMakeLists.txt
├── backend/         # FastAPI server (Python)
│   ├── main.py      # API endpoints
│   └── requirements.txt
└── README.md
```

## Testing

- **Plugin**: Test in GarageBand, Logic Pro, or other DAWs
- **Backend**: Use curl to test API endpoints
- **Integration**: Test recording → upload → download → playback flow

## Questions?

Feel free to open an issue for questions or discussion!
