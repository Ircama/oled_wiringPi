# oled_wiringPi

Drive a SSD1306/SH1106 OLED Display via I2C bitbanging using wiringPi

## Overview
A lightweight, standalone executable designed for Raspberry Pi that drives SSD1306/SH1106 OLED displays (128x64 pixels) using software I2C (bit-banging) implementation. The program focuses on simplicity, speed, and minimal dependencies. It uses embedded 5x7 font.

Tested with a Raspberry Pi Zero W 1.1 and software I2C. Compatible with the software I2C driver (conflicts should be anyway avoided).

## Key Features
- **Bare Metal I2C Implementation**: Uses bit-banging instead of kernel I2C drivers, providing direct GPIO control
- **Minimal Resource Usage**: Small executable footprint and efficient processing
- **Flexible Text Positioning**: Supports custom line and column positioning and automatically wraps on long texts
- **Configurable I2C Timing**: Adjustable delay for optimal communication

## Installation

- Download and install [WiringPi](https://github.com/WiringPi/WiringPi/releases) (tested with wiringpi_3.2-bullseye_armhf.deb).
- Compile with `gcc -o oled_display oled_display.c -lwiringPi && strip oled_display`
- Run with `.\oled_display ...`

## Command Line Options

### I2C Configuration
- `-a, --sda=NUM`: Specify the GPIO pin number for I2C SDA (Data line); default is 22 (BCM GPIO22)
- `-k, --scl=NUM`: Specify the GPIO pin number for I2C SCL (Clock line); default is 23 (BCM GPIO23)
- `-d, --delay=NUM`: Set the I2C timing delay for bit-banging operations; default is 5; le lower, the faster. Avoid 1 (too small) or numbers bigger than 10. 2 or 3 look still good.

Notice that the hw or sw I2C configuration of the device (set in config.ini) can be different from the usage of these GPIO ports, because this tool embeds an own software I2C stack.

### Display Control
- `-n, --no-clear`: Prevent screen clearing before writing new content
- `-s, --spacing=NUM`: Configure spacing between characters or elements (default is 1; it can also be 0)

### Text Positioning
- `-l, --line=NUM`: Select the vertical line position (row) for text; default is vertical center of the screen
- `-c, --column=NUM`: Select the horizontal column position for text; default is horizontal center of the screen
- `-t, --text=TEXT`: Specify the text content to be displayed; default is "Please wait...".

### Program Information
- `-?, --help`: Display comprehensive help information
- `--usage`: Show brief usage instructions
- `-V, --version`: Display program version information

### Special characters

- `^n`: New line (wrap line and reset to column 0)
- `^r`: Carriage return (reset to column 0 without wrapping line)
- `^b`: Backspace (cursor left)
- `^u`: Up one line
- `^d`: Down one line
- `^^`: Character `^`

## Usage Examples

1.  Basic text display:

    ```bash
    ./oled_display -t "Hello World" -l 0 -c 0
    ```

2.  Positioned text with custom I2C pins:

    ```bash
    ./oled_display --sda=2 --scl=3 --text="Status: OK" --line=2 --column=10
    ```

3.  Advanced usage to draw all lines of the screen:

    ```bash
    ./oled_display -l 0 -c 0 -d 2 -t "Lorem ipsum dolor sit"
    ./oled_display -l 1 -c 0 -d 2 -n -t "amet, consectetur"
    ./oled_display -l 2 -c 0 -d 2 -n -t "adipiscing elit. Sed"
    ./oled_display -l 3 -c 0 -d 2 -n -t "imperdiet sodales leo"
    ./oled_display -l 4 -c 0 -d 2 -n -t "at faucibus turpis"
    ./oled_display -l 5 -c 0 -d 2 -n -t "ullamcorper ut. "
    ./oled_display -l 6 -c 0 -d 2 -n -t "Maecenas et rutrum"
    ./oled_display -l 7 -c 0 -d 2 -n -t "tellus, eu pulvinar.."
    ```

    ```bash
    ./oled_display -l 0 -c 0 -d 2 -t "Lorem ipsum dolor sitamet, consectetur^nadipiscing elit. Sed^nimperdiet sodales leoat faucibus turpis^nullamcorper ut.^nMaecenas et rutrum^ntellus, eu pulvinar.."
    ```

## Using the tool to show boot and shutdown messages

### on boot at early stage

```bash
sudo vi /etc/systemd/system/boot_message.service
```

Content:

```
[Unit]
Description=Display boot message to OLED
DefaultDependencies=no

[Service]
Type=simple
ExecStart=/home/pi/oled/oled_display -c 0 -d 2 -s 2 -t " Going on..."

[Install]
WantedBy=sysinit.target
```

### on boot (later), reboot, shutdown

```bash
sudo vi /etc/systemd/system/reboot_message.service
```

Content:

```
[Unit]
Description=Display message to OLED on boot (later), reboot, shutdown

[Service]
Type=oneshot
RemainAfterExit=true
ExecStart=/home/pi/oled/oled_display -l 3 -c 0 -d 2 -t " Almost ready..."
ExecStop=/home/pi/oled/oled_display -l 2 -c 0 -d 2 -t " Going down..."
TimeoutSec=infinity

[Install]
WantedBy=multi-user.target
```

### Common systemd configuration

```bash
sudo systemctl enable boot.target
sudo systemctl enable reboot.target
sudo systemctl daemon-reload
```
