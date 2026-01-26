#!/bin/bash

# Auxlee Audio Plugin Build Script

echo "Building Auxlee Audio Plugin..."

# Check if JUCE exists
if [ ! -d "plugin/JUCE" ]; then
    echo "JUCE not found. Cloning JUCE framework..."
    cd plugin
    git clone https://github.com/juce-framework/JUCE.git
    cd ..
fi

# Create build directory
echo "Creating build directory..."
mkdir -p plugin/build
cd plugin/build

# Configure with CMake
echo "Configuring CMake..."
cmake ..

# Build
echo "Building plugin..."
cmake --build . --config Release

echo "Build complete! Plugin should be installed in your system plugin folder."
echo "Rescan plugins in your DAW to see 'Auxlee Audio Recorder'."
