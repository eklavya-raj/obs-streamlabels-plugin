# OBS Streamlabels Plugin

A high-performance OBS Studio plugin for displaying Streamlabs stream labels with full Mac, Windows, and Linux compatibility.

## Features

- Real-time stream label updates via WebSocket
- Support for all Streamlabs label types (followers, donations, subscriptions, etc.)
- Train counter logic for cumulative event tracking
- Cross-platform compatibility (Mac, Windows, Linux)
- High-performance C++ implementation
- Configurable via OBS settings

## Building

### Prerequisites

- OBS Studio development files
- CMake 3.10 or higher
- C++17 compatible compiler
- libcurl
- cJSON
- libwebsockets

### Linux

```bash
# Install dependencies
sudo apt-get install build-essential cmake libobs-dev libcurl4-openssl-dev libcjson-dev libwebsockets-dev

# Build
mkdir build && cd build
cmake ..
make -j4

# Install
sudo make install
```

### macOS

```bash
# Install dependencies (using Homebrew)
brew install cmake libcurl cjson libwebsockets

# Build
mkdir build && cd build
cmake ..
make -j4

# Install
sudo make install
```

### Windows

```bash
# Install dependencies using vcpkg or manually
# Build
mkdir build && cd build
cmake ..
cmake --build . --config Release

# Install
cmake --install .
```

## Configuration

1. Install the plugin in your OBS Studio plugins directory
2. Restart OBS Studio
3. Go to Tools > Streamlabels Settings
4. Enter your Streamlabs API token
5. Select your platform (twitch, youtube, facebook, trovo)
6. Enable auto-connect if desired

## Usage

1. Add a new source in OBS
2. Select "Stream Label" from the source list
3. Choose the label type (e.g., "Recent Follower", "Top Donator")
4. The label will update automatically based on stream events

## API Token

To get your Streamlabs API token:
1. Log in to Streamlabs
2. Go to Account Settings > API Settings
3. Generate a new API token
4. Copy the token and paste it in the plugin settings

## Supported Label Types

- Recent Follower
- Recent Donation
- Recent Subscriber
- Top Donator
- Follower Goal
- Subscriber Goal
- Donation Goal
- Bit Goal
- Viewer Count
- Stream Title
- Stream Game
- And many more (fetched from Streamlabs API)

## Architecture

The plugin consists of several components:

- **API Client**: Handles HTTP requests to Streamlabs API
- **WebSocket Client**: Maintains real-time connection for event updates
- **Stream Label Source**: OBS source that displays label text
- **Settings Manager**: Manages plugin configuration

## Performance

- Minimal CPU usage with efficient event handling
- Non-blocking I/O for network operations
- Thread-safe operations with mutex protection
- Optimized for real-time streaming scenarios

## License

This plugin is provided as-is for educational purposes.

## Credits

Based on the Streamlabs Desktop stream labels implementation.
