# oled_wiringPi

Drive a SSD1306/SH1106 OLED Display via I2C bitbanging using wiringPi

## Overview
A lightweight, standalone executable designed for Raspberry Pi that drives SSD1306/SH1106 OLED displays (128x64 pixels) using software I2C (bit-banging) implementation. The program focuses on simplicity, speed, and minimal dependencies. It uses embedded 5x7 font.

Tested with a Raspberry Pi Zero W 1.1 and software I2C. Compatible with the software I2C driver (conflicts should be anyway avoided).

## Key Features
- **Bare Metal I2C Implementation**: Uses bit-banging instead of kernel I2C drivers, providing direct GPIO control
- **Minimal Resource Usage**: Small executable footprint and efficient processing
- **Flexible Text Positioning**: Supports custom line and column positioning
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
