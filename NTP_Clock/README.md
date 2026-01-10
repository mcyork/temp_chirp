# NTP Clock

A WiFi-enabled NTP clock using ESP32-S3-MINI-1 with a 7-segment LED display (MAX7219).

## Features

- **NTP Time Synchronization**: Automatically syncs time from NTP servers
- **7-Segment Display**: Shows time as HHMM with flashing colon (decimal point)
- **12/24 Hour Format**: Toggle between formats using MODE button
- **Brightness Control**: Adjust display brightness using UP/DOWN buttons
- **AP Mode Configuration**: Web-based configuration interface when WiFi is not configured
- **Timezone Support**: Configurable timezone with daylight saving time support
- **Persistent Settings**: All preferences saved across power cycles

## Hardware

- **MCU**: ESP32-S3-MINI-1
- **Display**: MAX7219 7-segment LED display (4 digits)
- **Buttons**: 3 buttons (MODE, UP, DOWN)
- **Buzzer**: Piezo buzzer for audio feedback

### Pin Configuration

- **SPI**: GPIO 16 (SCK), GPIO 17 (MOSI), GPIO 18 (MISO)
- **Display CS**: GPIO 11
- **Buttons**: GPIO 5 (DOWN), GPIO 6 (UP), GPIO 7 (MODE)
- **Buzzer**: GPIO 4

## Installation

### Method 1: Web Installer (Recommended)

1. **Open the Web Installer**
   - Navigate to the [installer page](https://mcyork.github.io/ntp_clock/installer/) in Chrome or Edge
   - Or open `installer/index.html` locally

2. **Connect Your ESP32**
   - Connect your ESP32-S3 board to your computer via USB
   - Ensure the board is powered and detected by your system

3. **Configure Settings**
   - Enter your WiFi SSID and password
   - Select your timezone
   - Click the install button

4. **Flash Firmware**
   - Select your ESP32 device when prompted
   - Wait for the installation to complete
   - The device will reboot and attempt to connect to WiFi

5. **Configure via AP Mode** (if WiFi connection fails)
   - The device will create an access point "NTP-Clock-AP"
   - Connect to this network with your phone/computer
   - Navigate to `192.168.4.1` in your browser
   - Configure WiFi credentials and preferences
   - Save and reboot

### Method 2: Manual Installation

1. **Clone the Repository**
   ```bash
   git clone https://github.com/mcyork/ntp_clock.git
   cd ntp_clock
   ```

2. **Install Dependencies**
   - Install [Arduino IDE](https://www.arduino.cc/en/software) or [PlatformIO](https://platformio.org/)
   - Install ESP32 board support:
     - Arduino IDE: Add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` to Additional Board Manager URLs
     - Install "esp32" by Espressif Systems
   - Select board: **ESP32S3 Dev Module** (or ESP32-S3-MINI-1)

3. **Compile and Upload**
   - Open `NTP_Clock.ino` in Arduino IDE
   - Select the correct board and port
   - Click Upload

4. **Configure via AP Mode**
   - After first boot, connect to "NTP-Clock-AP" WiFi network
   - Navigate to `192.168.4.1`
   - Enter WiFi credentials and timezone
   - Save and reboot

## Configuration

### AP Mode Web Interface

When WiFi is not configured or connection fails, the device enters AP mode:

1. **Connect to AP**: Look for "NTP-Clock-AP" in your WiFi networks
2. **Open Browser**: Navigate to `192.168.4.1`
3. **Configure**:
   - WiFi SSID and password
   - Timezone offset (seconds from UTC)
   - Daylight saving offset (usually 3600 seconds)
   - Display brightness (0-15)
   - Time format (12/24 hour)
4. **Save**: Click "Save Configuration" - device will reboot

### Button Controls

- **MODE Button**: Toggle between 12-hour and 24-hour time format
- **UP Button**: Increase display brightness
- **DOWN Button**: Decrease display brightness

All button settings are saved automatically and persist across reboots.

## Timezone Configuration

The device supports configurable timezones. Common timezone offsets:

- **PST**: -28800 seconds (UTC-8)
- **PDT**: -25200 seconds (UTC-7)
- **EST**: -18000 seconds (UTC-5)
- **EDT**: -14400 seconds (UTC-4)
- **UTC**: 0 seconds
- **CET**: 3600 seconds (UTC+1)
- **JST**: 32400 seconds (UTC+9)

Daylight saving time offset is typically 3600 seconds (1 hour) or 0 if not applicable.

## Display Indicators

- **"AP"**: Device is in AP mode (WiFi not configured)
- **"Conn"**: Connecting to WiFi
- **"Err"**: Error (WiFi connection failed)
- **Time Display**: Shows current time in HHMM format with flashing colon

## Building from Source

### Using Arduino CLI

```bash
# Install ESP32 platform
arduino-cli core install esp32:esp32

# Install libraries
arduino-cli lib install "WiFi" "SPI" "Preferences" "WebServer"

# Compile
arduino-cli compile --fqbn esp32:esp32:esp32s3 NTP_Clock.ino
```

### GitHub Actions

The repository includes GitHub Actions workflow that automatically builds firmware on push and creates releases on tag push:

```bash
# Create a release
git tag v1.0.0
git push origin v1.0.0
```

The workflow will:
1. Build the firmware
2. Create a GitHub release
3. Upload the firmware binary
4. The web installer will automatically use the latest release

## Troubleshooting

### Device Shows "AP" on Display

- **Cause**: WiFi credentials not configured or connection failed
- **Solution**: Connect to "NTP-Clock-AP" network and configure at `192.168.4.1`

### Device Shows "Err" on Display

- **Cause**: WiFi connection failed
- **Solution**: Check WiFi credentials, ensure network is in range, device will enter AP mode

### Time Not Syncing

- **Cause**: WiFi connected but NTP sync failed
- **Solution**: Check internet connection, verify NTP server is accessible

### Web Installer Not Working

- **Browser**: Ensure you're using Chrome, Edge, or another Chromium-based browser
- **USB**: Verify ESP32 is connected and detected by system
- **Permissions**: Grant serial port access when prompted

### Display Not Working

- **Wiring**: Verify SPI connections (SCK, MOSI, MISO, CS)
- **Power**: Ensure ESP32 has adequate power supply
- **CS Pin**: Verify display CS is connected to GPIO 11

## Repository

- **GitHub**: [mcyork/ntp_clock](https://github.com/mcyork/ntp_clock)
- **Web Installer**: [mcyork.github.io/ntp_clock/installer/](https://mcyork.github.io/ntp_clock/installer/)

## License

[Add your license here]

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
