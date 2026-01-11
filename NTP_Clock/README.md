# NTP Clock

A WiFi-enabled digital clock that automatically syncs time from the internet and displays it on a 7-segment LED display.

## Quick Start - Web Installer

The easiest way to install the firmware is using the web-based installer:

1. **Open the Web Installer**
   - Go to [https://mcyork.github.io/ntp_clock/installer/](https://mcyork.github.io/ntp_clock/installer/) in Chrome or Edge browser
   - The installer uses Web Serial API, which requires a Chromium-based browser

2. **Connect Your ESP32**
   - Connect your ESP32-S3 board to your computer via USB
   - Make sure the board is powered and detected by your system

3. **Install Firmware**
   - Click the "Install" button on the web page
   - Select your ESP32 device when prompted
   - Grant serial port access when your browser asks
   - Wait for the installation to complete (the device will reboot automatically)

4. **First Boot**
   - After installation, the device will display its version number (2.15) for 5 seconds
   - If WiFi credentials aren't configured, it will show "AP" and create a WiFi access point
   - Connect to the access point (name will be like "NTP_Clock_A1B2C3") and configure your WiFi

## Using Your NTP Clock

### Display Sequence

When your clock boots up, you'll see this sequence:

1. **Version Display**: Shows "2.15" (or current version) for 5 seconds
2. **Connection Status**: 
   - "Conn" appears briefly when connecting to WiFi
   - "AP" appears if WiFi isn't configured (see Configuration section below)
3. **IP Address**: 
   - If connected to WiFi: The assigned IP address scrolls across the display twice (e.g., "192.168.1.100")
   - If in AP mode: The AP IP address scrolls across the display (e.g., "192.168.4.1")
4. **Time Display**: Once connected and synced, shows current time as HHMM (e.g., "12.34" for 12:34, with the decimal point flashing like a colon)

### Button Controls

Your clock has three buttons:

- **MODE Button**: Toggles between 12-hour and 24-hour time format
  - Press once: Switch formats (you'll hear a beep)
  - Settings are saved automatically

- **UP Button**: Increases display brightness
  - Press to make the display brighter (0-15 levels)
  - You'll hear a beep when brightness changes
  - Settings are saved automatically

- **DOWN Button**: Decreases display brightness
  - Press to make the display dimmer
  - You'll hear a beep when brightness changes
  - Settings are saved automatically

### Beep Sounds

The clock makes different beep sounds to indicate status:

- **Short high beep**: WiFi connected successfully
- **Two-tone beep**: Time synchronized with internet
- **Low beep**: Button pressed (MODE, UP, or DOWN)
- **Long low beep**: Error occurred (WiFi connection failed)

### Normal Operation

When everything is working correctly:

- The display shows the current time in HHMM format (e.g., "12.34" for 12:34)
- The decimal point on the hundreds digit flashes like a colon, indicating seconds
- In 12-hour mode, leading zeros are hidden (e.g., " 123" instead of "0123")
- The time updates every second
- All settings persist across power cycles

## Configuration via Web Interface

If your clock shows "AP" on the display, it means WiFi isn't configured or the connection failed. Follow these steps:

1. **Connect to Access Point**
   - Look for a WiFi network named "NTP_Clock_" followed by 6 characters (e.g., "NTP_Clock_A1B2C3")
   - Connect to this network (no password required)
   - The display will scroll the IP address (usually "192.168.4.1")

2. **Open Configuration Page**
   - Open a web browser and go to `192.168.4.1`
   - You'll see the configuration page

3. **Enter Settings**
   - **WiFi SSID**: Your WiFi network name
   - **WiFi Password**: Your WiFi password
   - **Timezone**: Select your timezone from the dropdown
   - **Daylight Saving**: Enter offset in seconds (usually 3600 for 1 hour, or 0 if not applicable)
   - **Brightness**: Set display brightness (0-15)
   - **Time Format**: Choose 12-hour or 24-hour format

4. **Save and Reboot**
   - Click "Save Configuration"
   - The device will reboot and attempt to connect to your WiFi
   - If successful, it will show "Conn" then display the time
   - If it fails, it will return to AP mode - check your WiFi credentials

## Troubleshooting

### Device Shows "AP" on Display

- **Cause**: WiFi credentials not configured or connection failed
- **Solution**: Connect to the "NTP_Clock_XXXXXX" WiFi network and configure at `192.168.4.1`

### Device Shows "Err" on Display

- **Cause**: WiFi connection failed or time sync lost
- **Solution**: The device will automatically enter AP mode. Reconfigure WiFi settings via the web interface

### Display Not Showing Time

- **Check WiFi**: Make sure the device is connected to WiFi (should show "Conn" briefly, not "AP")
- **Check Internet**: Ensure your WiFi network has internet access (needed for time sync)
- **Wait**: It may take a few seconds after WiFi connects for time to sync

### Beeps Not Working

- **Check Hardware**: Verify the buzzer is connected to GPIO 4
- **Normal**: Beeps are quiet - you may need to listen closely

### Web Installer Not Working

- **Browser**: Make sure you're using Chrome or Edge (Web Serial API requirement)
- **USB Connection**: Verify ESP32 is connected and detected
- **Permissions**: Grant serial port access when browser prompts

## For Developers

### Hardware Requirements

- **MCU**: ESP32-S3-MINI-1
- **Display**: MAX7219 7-segment LED display (4 digits)
- **Buttons**: 3 buttons (MODE, UP, DOWN)
- **Buzzer**: Piezo buzzer

### Pin Configuration

- **SPI**: GPIO 16 (SCK), GPIO 17 (MOSI), GPIO 18 (MISO)
- **Display CS**: GPIO 11
- **Buttons**: GPIO 5 (DOWN), GPIO 6 (UP), GPIO 7 (MODE)
- **Buzzer**: GPIO 4

### Building from Source

See the [GitHub repository](https://github.com/mcyork/ntp_clock) for build instructions and source code.

## Repository

- **GitHub**: [mcyork/ntp_clock](https://github.com/mcyork/ntp_clock)
- **Web Installer**: [mcyork.github.io/ntp_clock/installer/](https://mcyork.github.io/ntp_clock/installer/)
