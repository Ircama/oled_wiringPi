# oled_wiringPi

__Driving a SSD1306/SH1106 OLED Display via I2C bitbanging using wiringPi with Font Bitmap Generator tool__

Raspberry Pi standalone executable writing to a monochrome 128x64 OLED display driven by a SH1106 or SSD1306 controller via directly bit-banged I2C. The package includes a Font Bitmap Generator for converting TTF and ODF fonts to target bitmaps that can be easily embedded into the program.

## Overview
A lightweight, standalone executable designed for Raspberry Pi that drives SSD1306/SH1106 controllers for OLED displays (128x64 pixels) using a software I2C (bit-banging) implementation with optional clock-stretching and optional print of NACK. It uses selectable fonts and can be configured for SH1106 (`#define COL_OFFSET 2` for 128x64 pixels) or SSD1306 (`#define COL_OFFSET 0`).

Tested with a Raspberry Pi Zero W 1.1.

Tested with a SH1106 controller (132x64 pixel) driving a 128x64 pixel OLED display that needs a horizontal offset (of two pixels) to fit the centered 128x64 display window (most low cost OLED modules have this setup).

The Font Bitmap Generator tool is described in a specific chapter.

## Key Features
- Uses bit-banging instead of kernel I2C drivers or hw I2c, providing direct GPIO control
- Small executable footprint
- Supports command-line options with different settings
- Allows vertical and horizontal centering and includes wrapping long texts

## Installation

- Python is needed to build the fonts
- Download and install [WiringPi](https://github.com/WiringPi/WiringPi/releases) (tested with wiringpi_3.2-bullseye_armhf.deb).
- Compile with `make`.
- Run with `./oled_display ...`

## Command Line Options

```
Usage: oled_display [OPTION...]
Send text data to SSD1306/SH1106 OLED Display via I2C

  -a, --sda=NUM              Set I2C SDA (numeric)
  -A, --address=NUM          Set I2C address (7 bit numeric)
  -b, --brightness=NUM       Set rightness (0 to 255)
  -B, --contrast=NUM         Set contrast (0 to 255); 127 is the default.
  -c, --column=NUM           Set the column number (numeric)
  -C, --char                 Disable special char management
  -d, --delay=NUM            Set I2C delay (numeric)
  -D, --debug                Enable debug
  -e, --no-errors            Continue on errors
  -f, --font=TEXT            Select font name; use ? to get the list of the
                             available fonts
  -h, --h-flip               Flip screen horizontally
  -k, --scl=NUM              Set I2C SCL (numeric)
  -l, --line=NUM             Set the line number (numeric)
  -n, --no-clear             Do not initialize/clear the screen
  -N, --nack                 Print NACK
  -o, --off                  Display off
  -p, --pause=NUM            Set the used pause metric in microseconds
  -r, --reversed             Reverse screen
  -s, --spacing=NUM          Set the spacing value (numeric)
  -S, --stretching           Activate clock stretching
  -t, --text=TEXT            Set the text (string)
  -T, --timeout=NUM          Clock stretching timeout (default 1000)
  -v, --v-flip               Flip screen vertically
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version
```

Exit return code is 0 for success, 1 if NACK is received, or 2 if there was an error to initialize *wiringPi*.

### I2C Configuration
- `-A, --address=NUM`: Set I2C address (7 bit numeric); default is 0x3C.
- `-a, --sda=NUM`: Specify the GPIO pin number for I2C SDA (Data line); default is 22 (BCM GPIO22).
- `-k, --scl=NUM`: Specify the GPIO pin number for I2C SCL (Clock line); default is 23 (BCM GPIO23).
- `-d, --delay=NUM`: Set the I2C timing delay for bit-banging operations; default is 5; the lower, the faster. See below.
- `-N, --nack`: Enable printing of NACK (Not Acknowledge) messages if received from the I2C device. If NACK, the exit return code will be 1.
- `-S, --stretching`: Enable clock stretching.
- `-T, --timeout`: Clock stretching timeout in microseconds (default 10000 microseconds).
- `-e, --no-errors`: Continue on I2C errors (I2C NACK received and I2C arbitration lost).

Notice that the Raspberry Pi hw or sw I2C configuration of the device (set in *config.ini*) can be different (or totally missing), because this tool embeds an own software I2C stack which directly drives the GPIO ports.

### Display Control
- `-t, --text=TEXT`: Specify the text content to be displayed; default is "Please wait...".
- `-f, --font=TEXT`: Select font name; use ? to get the list of the available fonts
- `-l, --line=NUM`: Select the vertical line position (row) for text; default is vertical center of the screen.
- `-c, --column=NUM`: Select the horizontal column position for text; default is horizontal center of the screen.
- `-n, --no-clear`: Prevent screen initialization and clearing before writing new content. A previous command is expected (not using `-n`) to inizialize and clear the screen.
- `-s, --spacing=NUM`: Configure spacing between characters or elements (default is 1; it can also be 0).
- `-r, --reversed`: Reverse the whole screen from dark to light.
- `-C, --char`: Disable the special char management.
- `-p, --pause`: Set the used pause metric in microseconds. Numbers greater than 200000 are visible. Default is 500000.
- `-v, --v-flip`: Flip screen vertically.
- `-h, --h-flip`: Flip screen horizontally.

### Program Information
- `-D, --debug`: Enable the print-out of all options.
- `-?, --help`: Display comprehensive help information.
- `--usage`: Show brief usage instructions.
- `-V, --version`: Display program version information.

### Fonts

Use `-f label` or `--font label` to select a font.

Label can be a string or the corresponding font number.

All fonts use a 8-bit height monospaced matrix corresponding to the controller pages, regardless the height of the character format.

n |Label (optional alias)  | Description
--|------------------------|-----------------
1 | default, 5x7            | 5x7 pixel format (popular font for small displays, widely available in GitHub)
2 | vga8x8, 8x8             | 8x8 pixel format (good quality, VGA font)
3 | vga7x8                  | 7x8 pixel format (good quality, VGA font elaborated for 7 columns with minimal loss)
4 | 7x8                     | 7x8 pixel format (limited set of 127 chars with the first 32 ones null)
5 | 3x5                     | 3x5 pixel format (imported from [oled-font-3x5](https://github.com/fabiopiratininga/oled-font-3x5))
6 | ProFont5x7, 5x8         | 5x7 pixel format (good quality, generated by generate_bitmap.py from [ProFont](https://github.com/ryanoasis/nerd-fonts/tree/master/patched-fonts/ProFont/ProFontWinTweaked))
7 | ProFont6x8, 6x8         | 6x8 pixel format (generated by generate_bitmap.py from ProFont)
8 | ProFont8x8              | 8x8 pixel format (generated by generate_bitmap.py from ProFont)
9 | ProFont10x8, 10x8       | 10x8 pixel format (generated by generate_bitmap.py from ProFont)
10 | Hermit_light4x8, 4x8    | 4x8 pixel format (generated by generate_bitmap.py from [Hermit_light](https://github.com/Swordfish90/cool-retro-term/blob/master/app/qml/fonts/modern-hermit/Hermit-light.otf))

Besides, option `^+` upscales fonts horizontally.

### Special characters

The text can include special characters:

- `^n`: New line (wrap line and reset to column 0)
- `^r`: Carriage return (reset to column 0 without wrapping line)
- `^b`: Backspace (cursor left)
- `^u`: Up one line
- `^d`: Down one line
- `^c`: Clear the screen
- `^h`: Home
- `^^`: Character `^`
- `^p`: Pause of 0.5 seconds (period can be changed with `-p` flag)
- `^+`: Upscale font horizontally
- `^-`: Return to normal font

Using the `-C` option disables special char management.

## Usage Examples

1.  Basic text display:

    ```bash
    ./oled_display -t "Hello World" -l 0 -c 0
    ```

2.  Positioned text with custom I2C pins:

    ```bash
    ./oled_display --sda=2 --scl=3 --text="The quick brown fox jumps over the lazy dog." --line=2 --column=10 -f 8x8
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

4. Drawing a spinner:

    ```bash
    /home/pi/oled/oled_display -l 7 -t "|^p^b/^p^b-^p^b\^p^b|^p^b/^p^b-^p^b\^p^b|^p^b/^p^b-^p^b\^p^b|^p^b/^p^b-^p^b\^p^b|^p^b/^p^b-^p^b\^p^bCompleted" -S
    ```

5. Blink "Attention!" (centered label):

    ```bash
    ./oled_display --text='Attention!^p^cAttention!^p^cAttention!^p^c'
    ```

## Running the tool in the PINN rescue shell

The *oled_display* program can be run into the [PINN Rescue Shell](https://github.com/procount/pinn/blob/master/README_PINN.md#how-to-access-the-shell-or-ssh-into-pinn) through the following command:

```bash
umount /mnt
mount /dev/mmcblk0p7 /mnt

/mnt/usr/lib/arm-linux-gnueabihf/ld-2.31.so --library-path /mnt/usr/lib/arm-linux-gnueabihf:/mnt/usr/lib /mnt/home/pi/oled_display -t "Rescue Shell Active" -S -d 0 -l 7
```

Use the appropriate loader for your system and set the I2C SDA/SCL switches to the related GPIO pins.

## I2C Tuning

The `-d` option represents the I2C timing in microseconds, set to 5 microseconds by default. This control should be enough precise as wiringPi uses `nanosleep` for delays > 100uS and `delayMicrosecondsHard` for under 100uS delays. The value can be adjusted to optimize the timing for your specific hardware setup, considering the I2C clock speed you want to achieve. The standard I2C clock speeds are 100 kHz (standard mode) and 400 kHz (fast mode).

The generic formula to compute the frequency is: `frequency = 1 / (delay * 2) ≈ 100 kHz`. The value is multiplied by 2 because there are two delays per clock cycle since the clock signal needs to be high and low for equal amounts of time.

You can set it to any value between 0 and 10.

For instance, if you want to achieve a 400 kHz clock speed, the period of the clock is 2.5 microseconds (1 / 400000). Therefore, the delay for each half of the clock period should be 1.25 microseconds.

Here are the relations between the delay values of the `d` option and the approximate I2C frequency.

Value|Frequency
-----|----------
10   |  50 kHz
 9   |  55 kHz
 8   |  60 kHz
 7   |  70 kHz
 6   |  80 kHz
 5   | 100 kHz
 4   | 125 kHz
 3   | 160 kHz
 2   | 250 kHz
 1   | 400 kHz
 0   | > 400 kHz

Reporting of NACK messages (Not Acknowledge) is disabled by default. The `-N` option enables printing them if received from the I2C device. This is useful to test the I2C quality, which should never produce NACK; if you see NACK displayed, lower the frequency with the `-d` option, or enable I2C Clock Stretching.

I2C clock stretching can always be used and is strongly suggested at high I2C frequencies; it is disabled by default and related controlling options are `-S` and `-T`; the former activates the feature and the latter (optional) allows modifying its timeout.

The `-D` option is useful to dump internal counters and check the communication quality.

## Using the tool to show boot and shutdown messages

This light standalone executable can be used for displaying system boot and shutdown messages early in the OS boot stage (even before I2C modules load). It can also be used to show messages while the system initiates a shutdown or a reboot.

### Message at early stage boot phase

```bash
sudo vi /etc/systemd/system/boot_message.service
```

The message is displayed during the first 7 - 8 seconds of the boot phase after the bootloader handoff (about 14 seconds from power-on).

Content:

```
[Unit]
Description=Display boot message to OLED
DefaultDependencies=no

[Service]
Type=simple
ExecStart=/home/pi/oled/oled_display -s 2 -d 2 -t "Going on..."

[Install]
WantedBy=sysinit.target
```

### Messages during boot (later), reboot, or shutdown phases

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
ExecStart=/home/pi/oled/oled_display -s 2 -d 2 -t "Almost ready..."
ExecStop=/home/pi/oled/oled_display -s 2 -d 2 -t "Going down..."
TimeoutSec=infinity

[Install]
WantedBy=multi-user.target
```

### Common systemd configuration

Common systemd activation for the above systemd examples.

```bash
sudo systemctl enable boot.target
sudo systemctl enable reboot.target
sudo systemctl daemon-reload
```

# Font Bitmap Generator

## Overview
A Python tool for converting TTF and ODF fonts into compact bitmap representations suitable for embedded systems and low-resolution displays.

The **Font Bitmap Generation** is a Python-based utility for creating custom bitmap representations of font characters by converting OTF (OpenType) and TTF (TrueType) fonts. It generates bitmaps for predefined symbols in a flexible and customizable manner. The tool is designed for developers working on embedded systems, graphical displays, or similar environments requiring pixel-perfect font rendering.

Its goal is to simplify font rendering processes and ensures high-quality output suitable for various display applications.

## Key Features
- Generates bitmap fonts for customizable 256 character set
- Higly configurable (Specify input TTF or ODF fonts and may other parameters, supporting variable character width and fixed 8-bit character height)
- Manual adjustments (Specify adjustments for specific characters and define a number of customizable converter behaviour)
- Intelligent bitmap scaling, centering and cropping (Dynamically crop unnecessary blank columns of character bitmaps while ensuring optimal centering of the used portions; automatically scale characters that do not fit the target height)
- Visual preview of generated font in the default viewer (Generate a graphical grid of characters as an image for easy visual inspection of bitmap quality, highlighting horizontally cropped characters in blue and blank characters in green)
- YAML based automation configuration, to create multiple fonts in sequence, with automatic font downloading and other control flags.
- Character bitmaps are generated rotated clockwise of 90 degrees

## Workflow
- select appropriate TTF/ODF fonts for low-resolution displays (ideal ones are squared Monospaced Sans Serif typefaces that typically mimic the [look and feel of the old cathode tube screens](https://github.com/Swordfish90/cool-retro-term/tree/master/app/qml/fonts) based on [Fixedsys](https://en.wikipedia.org/wiki/Fixedsys) or [Monaco](https://en.wikipedia.org/wiki/Monaco_(typeface))
- configure `font_url` (optional), `generated_font_path`, `font_label`, `font_name`
- start with `char_width: 8`, `columns_to_crop: 0`, `reduce_font: 0`, `start_predefined_symbols: 0`
- manually find optimal font size `fixed_font_size` through several attempts (the automatic font size detection is not enough optimized)
- define vertical cropping with `columns_to_crop`; through the visual grid preview, reduce the unused external vertical blank columns to the minimum, while verifying absence of blue boxes, showing left-side or right-side cropped characters (and check green ones, related to blank characters)
- Ensure consistent 8-row representation
- chech how letter "g" is rendered. If not totally shown, correct the related font scaling `reduce_font`
- define the 7-bit and 8-bit symbol character sets supported by the fonts with `start_predefined_symbols`
- define manual adjustments with `adjust`

## Switches
- `disabled` (skip target)
- `break` (interrupt processing)

`convert` allows converting a single character of a `[...]` list to the appropriated sequence for `adjust`.

## Usage

```
generate_bitmap.py [-h] [-t TARGET] [-d] [-s] [-S] [-w] [-c CONFIG_FILE] [-C CONVERT]

optional arguments:
  -h, --help            show this help message and exit
  -t TARGET, --target TARGET
                        Only process specific target in the configuration list
  -d, --debug           Print debug information
  -s, --show            Show the graphical grid representing the character table
  -S, --save            Save the image of the graphical grid representing the character table to file
  -w, --write           Write to "generated_font_path" file
  -c CONFIG_FILE, --config CONFIG_FILE
                        Configuration file (default is "generate_bitmap.yaml")
  -C CONVERT, --convert CONVERT
                        Convert character or array to bitmap.

Font Bitmap Generator
```

Example usage:

```
python generate_bitmap.py -s -S -w
```

## Output Formats
- C-style byte array (`const unsigned char[]`): both `.c` and `.h`
- PNG grid preview of all characters

## Use Cases
- Embedded system font generation
- Low-resolution display typography
- Microcontroller graphics
- Resource-constrained interfaces
