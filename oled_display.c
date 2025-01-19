// SH1106 or SSD1306 controller for monochrome 128x64 OLED display via I2C

#define USE_GENERATED_FONTS

#include <stdbool.h>
#include <argp.h>
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "font_others.h"
#ifdef USE_GENERATED_FONTS
#include "generated_fonts.h"
#endif

#define SDA_PIN 22  // Data pin
#define SCL_PIN 23  // Clock pin
#define M_DELAY 5   // I2C default microdelay
#define I2C_START_MODE TRUE

#define COL_OFFSET 2 // Correct column offset: move by 2 pixels to align properly

/* While the SSD1306 controller drives a 128 x 64 display matrix, the SH1106 one
   drives a 132 X 64 Dot Matrix OLED. Some hw modules uses the SH1106 controller
   with a 128 x 64 display matrix, where the first two and the last two columns
   are not connected to the OLED display. In this case, the display starts at
   the third column and you need to set COL_OFFSET to 2.
*/

#define TOTAL_PAGES 8 // 8 pages (8 pixels per page) for 64 pixels; pages 0 to 7. Set to 4 for the 128x32 version.
// Each page is 8 pixels high, and has 128 vertical strips 1 pixel wide
#define DEFAULT_TEXT "Please wait..."
#define DEFAULT_FONT_SEL "5x7"
#define DEFAULT_FONT_BM font5x7
#define DEFAULT_FONT_WIDE 5
#define DEFAULT_FONT_SIZE 255

#define OLED_WIDTH 128    // OLED width in pixels
#define OLED_HEIGHT 64    // OLED height in pixels
#define STRETCH_TIMEOUT 10000  // microseconds

// OLED I2C address (change if needed)
#define OLED_I2C_ADDR 0x3C

typedef struct {
    const char *name;          // Font label (e.g., "5x7")
    bool is_an_alias;          // Indicates if this is an alias (true/false)
    const unsigned char *font; // Font bitmap data
    int width;                 // Character width in pixels
    int size;                  // Total size of the font data
} FontInfo;

FontInfo fonts[] = {
    {"default",         FALSE,  DEFAULT_FONT_BM, DEFAULT_FONT_WIDE, DEFAULT_FONT_SIZE * DEFAULT_FONT_WIDE},
    {"5x7",             TRUE,   DEFAULT_FONT_BM, DEFAULT_FONT_WIDE, DEFAULT_FONT_SIZE * DEFAULT_FONT_WIDE},

    {"vga8x8",          FALSE,  font_vga8x8,      8, 256 * 8},
    {"8x8",             TRUE,   font_vga8x8,      8, 256 * 8},

    {"vga7x8",          FALSE,  font_vga7x8,      7, 256 * 7},

    {"7x8",             FALSE,  font7x8,          7, 127 * 7},

    {"3x5",             FALSE,  font7x8,          3, 127 * 3},
#ifdef USE_GENERATED_FONTS
    {"ProFont5x7",      FALSE,  ProFont5x7,       5, 256 * 5},
    {"5x8",             TRUE,   ProFont5x7,       5, 256 * 5},

    {"ProFont6x8",      FALSE,  ProFont6x8,       6, 256 * 6},
    {"6x8",             TRUE,   ProFont6x8,       6, 256 * 6},

    {"ProFont8x8",      FALSE,  ProFont8x8,       8, 256 * 8},

    {"ProFont10x8",     FALSE,  ProFont10x8,     10, 256 * 10},
    {"10x8",            TRUE,   ProFont10x8,     10, 256 * 10},

    {"Hermit_light4x8", FALSE,  Hermit_light4x8,  4, 256 * 4},
    {"4x8",             TRUE,   Hermit_light4x8,  4, 256 * 4},
#endif
};

char * font_selection = DEFAULT_FONT_SEL;
int brightness = -1;
int contrast = 0x7f;
const unsigned char * font = DEFAULT_FONT_BM;
int font_wide = DEFAULT_FONT_WIDE;
int font_size = DEFAULT_FONT_SIZE * DEFAULT_FONT_WIDE;

int oled_i2c_address = OLED_I2C_ADDR;
int stretch_timeout = STRETCH_TIMEOUT;
int sda_pin = SDA_PIN;
int scl_pin = SCL_PIN;
int m_delay = M_DELAY;
int nack = FALSE;
int stretching = FALSE;
int enable_special_char = TRUE;
int return_code = EXIT_SUCCESS;
int max_stretch_loop = 0;
int arbitration_lost = 0;
int upscale = 1;
int pause_us = 500000;
int terminate_on_errors = TRUE;


static inline void handle_clock_stretching(void) {
    if (!stretching) return;

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pinMode(scl_pin, INPUT);
    while (digitalRead(scl_pin) == LOW) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        if ((now.tv_sec - start.tv_sec) * 1000000 + (now.tv_nsec - start.tv_nsec) / 1000 > stretch_timeout) {
            fprintf(stderr, "I2C clock stretching timeout\n");
            return_code = EXIT_FAILURE;
            pinMode(scl_pin, OUTPUT);
            return;
        }
    }
    pinMode(scl_pin, OUTPUT);
}

static inline void check_arbitration(void) {
    pinMode(sda_pin, INPUT);
    if (digitalRead(sda_pin) == LOW) {
        return_code = EXIT_FAILURE;
        if (terminate_on_errors) {
            fprintf(stderr, "I2C arbitration lost. Device possibly disconnected.\n");
            exit(return_code);
        }
        fprintf(stderr, "I2C arbitration lost\n");
        arbitration_lost++;
    }
    pinMode(sda_pin, OUTPUT);
}

void i2c_start(int restart) {
    pinMode(sda_pin, OUTPUT);
    pinMode(scl_pin, OUTPUT);

    if (restart) {
        digitalWrite(sda_pin, HIGH);
        delayMicroseconds(m_delay);
        digitalWrite(scl_pin, HIGH);
        handle_clock_stretching();
        delayMicroseconds(m_delay);
    }
    check_arbitration(); // SCL is high

    digitalWrite(sda_pin, LOW);
    delayMicroseconds(m_delay);
    digitalWrite(scl_pin, LOW);
}

void i2c_stop(void) {
    pinMode(sda_pin, OUTPUT);
    pinMode(scl_pin, OUTPUT);

    digitalWrite(sda_pin, LOW);
    delayMicroseconds(m_delay);
    digitalWrite(scl_pin, HIGH);
    handle_clock_stretching();
    delayMicroseconds(m_delay);
    digitalWrite(sda_pin, HIGH);
    check_arbitration();
}

static inline void i2c_write_bit(int bit) {
    digitalWrite(sda_pin, bit);
    delayMicroseconds(m_delay); // SDA change propagation delay - First delay
    digitalWrite(scl_pin, HIGH); // Set SCL high to indicate a new valid SDA value is available
    delayMicroseconds(m_delay); // Wait for SDA value to be read by target - Second delay
    handle_clock_stretching();
    if (bit) // If SDA is high, check that nobody else is driving SDA
        check_arbitration();
    digitalWrite(scl_pin, LOW); // Clear the SCL to low in preparation for next change
}

static inline int i2c_read_bit(void) {
    int bit;

    pinMode(sda_pin, INPUT); // Let the target drive data
    delayMicroseconds(m_delay); // Wait for SDA value to be written by target
    digitalWrite(scl_pin, HIGH); // Set SCL high to indicate a new valid SDA value is available
    handle_clock_stretching();
    delayMicroseconds(m_delay); // Wait for SDA value to be written by target
    bit = digitalRead(sda_pin);
    digitalWrite(scl_pin, LOW); // Set SCL low in preparation for next operation
    pinMode(sda_pin, OUTPUT);
    return bit;
}

unsigned char i2c_read_byte(int nack) {
    unsigned char byte = 0;
    unsigned char bit;

    for (bit = 0; bit < 8; ++bit) {
    byte = (byte << 1) | i2c_read_bit();
    }

    i2c_write_bit(nack);
    return byte;
}

void i2c_write_byte(unsigned char byte) {
    pinMode(sda_pin, OUTPUT);
    pinMode(scl_pin, OUTPUT);

    for (int i = 0; i < 8; i++) {
        i2c_write_bit((byte & 0x80) != 0);
        byte <<= 1;
    }

    // Acknowledge bit
    if ((i2c_read_bit() != LOW) && nack) {
        return_code = EXIT_FAILURE;
        if (terminate_on_errors) {
            fprintf(stderr, "I2C NACK received. Terminating.\n");
            exit(return_code);
        }
        fprintf(stderr, "I2C NACK received\n");
    }
}

void oled_send_command(unsigned char command) {
    i2c_start(I2C_START_MODE);
    i2c_write_byte(oled_i2c_address << 1); // Write mode (I2C address left-shifted by 1)
    i2c_write_byte(0x00); // Control byte for command mode
    i2c_write_byte(command); // Send the command byte
    i2c_stop();
}

void oled_send_column(unsigned char data) {  // Send a one-bit column
    i2c_start(I2C_START_MODE);
    i2c_write_byte(oled_i2c_address << 1); // Write mode (I2C address left-shifted by 1)
    i2c_write_byte(0x40); // Control byte for data mode
    i2c_write_byte(data); // Send the data byte
    i2c_stop();
}

static inline void oled_send_char(
    const unsigned char *text,    // Character to display (e.g., 'A')
    int upscale,                  // Horizontal scaling factor
    int spacing                   // Spacing after the character
) {
    i2c_start(I2C_START_MODE);
    i2c_write_byte(oled_i2c_address << 1); // Write mode (I2C address left-shifted by 1)
    i2c_write_byte(0x40); // Control byte for data mode
    for (int i = 0; i < font_wide; i++) {  // loop for all columns of the font
        int displ = (*text) * font_wide + i;
        if (displ < font_size)
            for (int j = 0; j<upscale; j++)  // multiply column by upscale
                i2c_write_byte(font[displ]); // Send character column by column
    }
    for (int i = 0; i < spacing; i++) {  // Spacing are blank columns
        i2c_write_byte(0x00); // blank column
    }
    i2c_stop();
}

static inline void oled_send_pattern(int pattern, int length) {
    i2c_start(I2C_START_MODE);
    i2c_write_byte(oled_i2c_address << 1); // Write mode (I2C address left-shifted by 1)
    i2c_write_byte(0x40); // Control byte for data mode
    for (int i = 0; i < length; i++) {
        i2c_write_byte(pattern); // Send pattern
    }
    i2c_stop();
}

// https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
// https://www.pololu.com/file/0J1813/SH1106.pdf
// https://www.lcd-module.de/eng/pdf/zubehoer/comparsion_SSD1305-SH1106.pdf
void oled_init(int reversed, int contrast, int h_flip, int v_flip) {
    oled_send_command(0xAE); // Display OFF

    oled_send_command(0xD5); // Set display clock divide ratio/oscillator frequency
    oled_send_command(0x80); // Default suggested value (0x80)

    oled_send_command(0xA8); // Set multiplex ratio
    oled_send_command(0x3F); // 1/64 duty (for 128x64 OLED). Set to 0x1f for the 128x32 version

    oled_send_command(0xD3); // Set display offset (vertical)   
    oled_send_command(0x00); // No offset
    oled_send_command(0x40); // Set display start line (0x40 | 0x00 for line 0)

    oled_send_command(0x8D); // Enable charge pump regulator
    oled_send_command(0x14); // Enable charge pump

    oled_send_command(0xA0 | (h_flip ? 0x00 : 0x01)); // Set segment re-map (A1 for 128x64 OLED)
    oled_send_command(v_flip ? 0xC0 : 0xC8); // Set COM output scan direction (C8 for remapped)

    oled_send_command(0xDA); // Set COM pins hardware configuration
    oled_send_command(0x12); // Alternative COM configuration for 128x64. Set to 0x02 for the 128x32 version

    oled_send_command(0x81); // Set contrast control
    oled_send_command(contrast); // Contrast value (0x7F is recommended)

    oled_send_command(0xD9); // Set pre-charge period
    oled_send_command(0x22); // Default value (0x22 for SSD1306)

    oled_send_command(0xDB); // (additionally needed to lower the contrast)
    oled_send_command(0x40); // //0x40 default, to lower the contrast, put 0


    oled_send_command(0xA4); // Entire display ON (resume to RAM content)

    if (reversed) {
        oled_send_command(0xA7); // Inverted display
    } else {
        oled_send_command(0xA6); // Normal display mode (not inverted)
    }

    oled_send_command(0x20); // Set memory addressing mode
    oled_send_command(0x00); // Horizontal addressing mode

    oled_send_command(0xAF); // Display ON
}

/*
The size of the RAM is 128 x 64 bits and the RAM is divided into eight pages,
from PAGE0 to PAGE7, which are used for monochrome 128x64 dot matrix display.
*/

void move_to(int line, int column) {
    oled_send_command(0xB0 + line);          // Set page start address (page number) by command B0h to B7h. Notice that line and page correspond.
    column += COL_OFFSET; // Correct column offset
    oled_send_command(column & 0x0F);       // Set lower start column address of pointer by command 00h~0Fh.
    oled_send_command(0x10 | (column >> 4)); // Set higher start column address of pointer by command 10h~1Fh.
}

void oled_screen_pattern(char pattern) {
    oled_send_command(0xAE); // Display OFF (avoid showing previous garbage while clearing the screen)
    for (int page = 0; page < TOTAL_PAGES; page++) {
        move_to(page, 0);
        oled_send_pattern(pattern, OLED_WIDTH);
    }
    oled_send_command(0xAF); // Display ON
}

void calculate_line_and_column(
    const char *text,
    int spacing,
    int *line,
    int *column
) {
    if (text == NULL || line == NULL || column == NULL) {
        return; // Invalid input
    }

    int text_p = 0;
    int deletion = 0;

    // Calculate text length and handle '^' escape sequences
    while (text[text_p] != '\0') {
        if (text[text_p] == '^' && text[text_p + 1] != '\0' && text[text_p + 1] != '^') {
            break;
        }
        if (text[text_p] == '^' && text[text_p + 1] == '^') {
            deletion++;
            text_p += 2; // Skip both '^'
            continue;
        }
        text_p++;
    }

    // Calculate default positions if not provided
    if (*column < 0) {
        *column = (OLED_WIDTH - ((text_p - deletion) * (font_wide + spacing))) / 2; // Center horizontally
    }
    if (*line < 0) {
        *line = (OLED_HEIGHT / 8) / 2; // Center vertically (page-based)
    }
}


void oled_display_text(const char *text, int line, int column, int spacing) {

    calculate_line_and_column(text, spacing, &line, &column);

    // Ensure column and line are within valid ranges
    if (column < 0 || column >= OLED_WIDTH) return;
    if (line < 0 || line >= (OLED_HEIGHT / 8)) return;

    move_to(line, column);

    while (*text) {
        // New line (wrap line and column 0) if ^n
        if ((*text == '^') && (*(text + 1) == 'n')) {
            text += 2;
            column = 0;
            line++;
            move_to(line, column);
            continue;
        }

        if (enable_special_char) {
            // Carriage return (reset to column 0 without wrapping line) if ^r
            if ((*text == '^') && (*(text + 1) == 'r')) {
                text += 2;
                column = 0;
                move_to(line, column);
                continue;
            }

            // Up one line if ^u
            if ((*text == '^') && (*(text + 1) == 'u')) {
                text += 2;
                line--;
                move_to(line, column);
                continue;
            }

            // Down one line if ^d
            if ((*text == '^') && (*(text + 1) == 'd')) {
                text += 2;
                line++;
                move_to(line, column);
                continue;
            }

            // Pause ^p
            if ((*text == '^') && (*(text + 1) == 'p')) {
                text += 2;
                usleep(pause_us);;
                continue;
            }

            // Home if ^h
            if ((*text == '^') && (*(text + 1) == 'h')) {
                text += 2;
                line = 0;
                column = 0;
                move_to(line, column);
                continue;
            }

            // Clear screen if ^c
            if ((*text == '^') && (*(text + 1) == 'c')) {
                text += 2;
                oled_screen_pattern(0x00);  // Clear screen
                line = -1;
                column = -1;
                calculate_line_and_column(text, spacing, &line, &column);
                move_to(line, column);
                continue;
            }

            // Backspace if ^b
            if ((*text == '^') && (*(text + 1) == 'b')) {
                text += 2;
                column -= (font_wide + spacing);
                move_to(line, column);
                continue;
            }

            // Font scale-up ^+
            if ((*text == '^') && (*(text + 1) == '+')) {
                text += 2;
                upscale++;
                continue;
                if (upscale > 3)
                    upscale = 1;
            }

            // Font reset scale ^-
            if ((*text == '^') && (*(text + 1) == '-')) {
                text += 2;
                upscale--;
                if (upscale < 1)
                    upscale = 1;
                continue;
            }

            if ((*text == '^') && (*(text + 1) == '^')) {
                text += 1;
            }
        }

        oled_send_char(text, upscale, spacing);

        text++;  // Move to the next character
        column += (font_wide + spacing);  // font_wide pixels for character + spacing
        move_to(line, column);

        // Check for horizontal overflow (wrap text if necessary)
        if (column + font_wide >= OLED_WIDTH) {
            // Move to the next line (page)
            column = 0;
            line++;
            if (line >= OLED_HEIGHT / 8) {
                // Prevent exceeding vertical limit
                line = OLED_HEIGHT / 8 - 1;
                return;
            }
            move_to(line, column);
        }
    }
}

// Define options
const char *argp_program_version = "oled_display 1.0";
const char *argp_program_bug_address = NULL;
static char doc[] = "Send text data to SSD1306/SH1106 OLED Display via I2C";
static char args_doc[] = ""; // No positional arguments

// Options structure
static struct argp_option options[] = {
    {"font",       'f', "TEXT", 0, "Select font name; use ? to get the list of the available fonts"},
    {"text",       't', "TEXT", 0, "Set the text (string)"},
    {"brightness", 'b', "NUM",  0, "Set rightness (0 to 255)"},
    {"contrast",   'B', "NUM",  0, "Set contrast (0 to 255); 127 is the default."},
    {"address",    'A', "NUM",  0, "Set I2C address (7 bit numeric)"},
    {"timeout",    'T', "NUM",  0, "Clock stretching timeout (default 1000)"},
    {"delay",      'd', "NUM",  0, "Set I2C delay (numeric)"},
    {"sda",        'a', "NUM",  0, "Set I2C SDA (numeric)"},
    {"scl",        'k', "NUM",  0, "Set I2C SCL (numeric)"},
    {"line",       'l', "NUM",  0, "Set the line number (numeric)"},
    {"column",     'c', "NUM",  0, "Set the column number (numeric)"},
    {"spacing",    's', "NUM",  0, "Set the spacing value (numeric)"},
    {"pause",      'p', "NUM",  0, "Set the used pause metric in microseconds"},
    {"no-clear",   'n', 0, 0, "Do not initialize/clear the screen", 0},
    {"reversed",   'r', 0, 0, "Reverse screen", 0},
    {"nack",       'N', 0, 0, "Print NACK", 0},
    {"stretching", 'S', 0, 0, "Activate clock stretching", 0},
    {"char",       'C', 0, 0, "Disable special char management", 0},
    {"off",        'o', 0, 0, "Display off", 0},
    {"debug",      'D', 0, 0, "Enable debug", 0},
    {"no-errors",  'e', 0, 0, "Continue on errors", 0},
    {"h-flip",     'h', 0, 0, "Flip screen horizontally", 0},
    {"v-flip",     'v', 0, 0, "Flip screen vertically", 0},
    {0}
};

// Program arguments
struct arguments {
    int brightness;
    int contrast;
    char * font_selection;
    int oled_i2c_address;
    int stretch_timeout;
    int m_delay;
    int sda;
    int scl;
    int line;
    int column;
    int spacing;
    int pause_us;
    char * text;
    int clear_screen;
    int reversed;
    int nack;
    int stretching;
    int enable_special_char;
    int display_off;
    int debug;
    int h_flip;
    int v_flip;
    int terminate_on_errors;
};

// Parse a single option
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case 'b':
            arguments->brightness = atoi(arg);
            if (arguments->brightness < 0 || arguments->brightness > 255) {
                fprintf(stderr, "Error: Brightness must be between 0 and 255.\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'B':
            arguments->contrast = atoi(arg);
            if (arguments->contrast < 0 || arguments->contrast > 255) {
                fprintf(stderr, "Error: Contrast must be between 0 and 255.\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'f':
            arguments->font_selection = arg;
            break;
        case 'A':
            arguments->oled_i2c_address = atoi(arg);
            if ((arguments->oled_i2c_address < 0) || (arguments->oled_i2c_address > 127)) {
                fprintf(stderr, "Only 7-bit addressing is supported\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'T':
            arguments->stretch_timeout = atoi(arg);
            if (arguments->stretch_timeout < 0) {
                fprintf(stderr, "Error: stretch_timeout must be positive.\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'd':
            arguments->m_delay = atoi(arg);
            if (arguments->m_delay < 0) {
                fprintf(stderr, "Error: m_delay must be positive.\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'a':
            arguments->sda = atoi(arg);
            if ((arguments->sda < 0) || (arguments->sda > 127)) {
                fprintf(stderr, "Invalid value for SDA\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'k':
            arguments->scl = atoi(arg);
            if ((arguments->scl < 0) || (arguments->scl > 127)) {
                fprintf(stderr, "Invalid value for SCL\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'l':
            arguments->line = atoi(arg);
            if ((arguments->line < 0) || (arguments->line > 7)) {
                fprintf(stderr, "Error: line must be between 0 and 7\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'N':
            arguments->nack = TRUE;
            break;
        case 'o':
            arguments->display_off = TRUE;
            break;
        case 'D':
            arguments->debug = TRUE;
            break;
        case 'h':
            arguments->h_flip = TRUE;
            break;
        case 'v':
            arguments->v_flip = TRUE;
            break;
        case 'e':
            arguments->terminate_on_errors = FALSE;
            break;
        case 'C':
            arguments->enable_special_char = FALSE;
            break;
        case 'S':
            arguments->stretching = TRUE;
            break;
        case 'r':
            arguments->reversed = TRUE;
            break;
        case 'n':
            arguments->clear_screen = FALSE;
            break;
        case 'c':
            arguments->column = atoi(arg);
            if ((arguments->column < 0) || (arguments->column > 127)) {
                fprintf(stderr, "Error: column must be between 0 and 127\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 's':
            arguments->spacing = atoi(arg);
            if ((arguments->spacing < 0) || (arguments->spacing > 63)) {
                fprintf(stderr, "Error: spacing must be between 0 and 63\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'p':
            arguments->pause_us = atoi(arg);
            if (arguments->pause_us < 0) {
                fprintf(stderr, "Error: pause_us must be positive.\n");
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 't':
            arguments->text = arg;
            break;
        case ARGP_KEY_ARG:
            // Handle non-option arguments (if any)
            fprintf(stderr, "Error: positional arguments are not allowed.\n");
            return ARGP_ERR_UNKNOWN;
        case ARGP_KEY_END:
            // End of parsing
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

// Argp parser
static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char *argv[]) {
    struct arguments arguments;
    FontInfo * selected_font = NULL;

    // Default values
    arguments.font_selection = DEFAULT_FONT_SEL;
    arguments.brightness = -1;
    arguments.contrast = 0x7f;
    arguments.oled_i2c_address = OLED_I2C_ADDR;
    arguments.stretch_timeout = STRETCH_TIMEOUT;
    arguments.m_delay = M_DELAY;
    arguments.sda = SDA_PIN;
    arguments.scl = SCL_PIN;
    arguments.line = -1;
    arguments.column = -1;
    arguments.spacing = 1;
    arguments.pause_us = 500000;
    arguments.clear_screen = TRUE;
    arguments.reversed = FALSE;
    arguments.text = DEFAULT_TEXT;
    arguments.nack = FALSE;
    arguments.stretching = FALSE;
    arguments.enable_special_char = TRUE;
    arguments.debug = FALSE;
    arguments.h_flip = FALSE;
    arguments.v_flip = FALSE;
    arguments.terminate_on_errors = TRUE;
    arguments.display_off = FALSE;

    // Parse command-line arguments
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    sda_pin = arguments.sda;
    scl_pin = arguments.scl;
    oled_i2c_address = arguments.oled_i2c_address;
    stretch_timeout = arguments.stretch_timeout;
    m_delay = arguments.m_delay;
    nack = arguments.nack;
    stretching = arguments.stretching;
    enable_special_char = arguments.enable_special_char;
    pause_us = arguments.pause_us;
    terminate_on_errors = arguments.terminate_on_errors;

    // Find the font by name
    for (int i = 0; i < sizeof(fonts)/sizeof(fonts[0]); i++) {
        if (strcmp(fonts[i].name, arguments.font_selection) == 0) {
            selected_font = &fonts[i];
            break;
        }
    }

    if (!selected_font) {
        int n = 1, num = -1;
        char *end;
        long len_str = strlen(arguments.font_selection);
        for (int i = 0; i < sizeof(fonts)/sizeof(fonts[0]); i++) {
            if (!fonts[i].is_an_alias) {
                if ((int) strtol(arguments.font_selection, &end, 10) == n && end - arguments.font_selection == len_str) {
                    selected_font = &fonts[i];
                    break;
                }
            n++;
            }
        }
    }

    if (!selected_font) {
        int n = 1;
        fprintf(stderr, "Font '%s' not found. Available fonts:", arguments.font_selection);
        for (int i = 0; i < sizeof(fonts)/sizeof(fonts[0]); i++) {
            if (fonts[i].is_an_alias)
                fprintf(stderr, ", %s", fonts[i].name);
            else
                fprintf(stderr, "\n  %d. %s", n++, fonts[i].name);
        }
        fprintf(stderr, "\n\n");
        return 1;
    }

    font = selected_font->font;
    font_wide = selected_font->width;
    font_size = selected_font->size;

    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "Failed to initialize wiringPi\n");
        return 2;
    }

    pinMode(sda_pin, OUTPUT);
    pinMode(scl_pin, OUTPUT);

    if ((arguments.contrast < 0) || (arguments.contrast > 255)) {
        fprintf(stderr, "Error. Contrast must be between 0 and 255 (127 is the default).\n");
        return 2;
    }

    if (arguments.display_off) {
        oled_send_command(0xAE); // Display OFF
        return 0;
    }

    if (arguments.clear_screen)
        oled_init(arguments.reversed, arguments.contrast, arguments.h_flip, arguments.v_flip);
    if (arguments.brightness != -1) {
        if ((arguments.brightness < 0) || (arguments.brightness > 255)) {
            fprintf(stderr, "Error. Brightness must be between 0 and 255.\n");
            return 2;
        }
        oled_send_command(0x82);
        oled_send_command(arguments.brightness);
    }
    if (arguments.clear_screen)
        oled_screen_pattern(0x00);  // Clear screen
    oled_display_text(arguments.text, arguments.line, arguments.column, arguments.spacing);
    if (arguments.debug) {
        // Print debug of all options
        printf("Text: %s\n", arguments.text ? arguments.text : "(none)");
        printf("Line position: %d\n", arguments.line);
        printf("Column position: %d\n", arguments.column);
        printf("Number of spacing pixels: %d\n", arguments.spacing);
        printf("Font: %s; width: %d; size: %d\n", selected_font->name, selected_font->width, selected_font->size / selected_font->width);
        printf("I2C address: %x\n", arguments.oled_i2c_address);
        printf("I2C microdelay (microseconds): %d\n", arguments.m_delay);
        printf("SDA GPIO BCM port: %d\n", arguments.sda);
        printf("SCL GPIO BCM port: %d\n", arguments.scl);
        printf("Do clear screen: %d\n", arguments.clear_screen);
        printf("Reversed (light mode): %d\n", arguments.reversed);
        printf("Check I2C NACK: %d\n", arguments.nack);
        printf("Do clock stretching: %d\n", arguments.stretching);
        printf("stretch_timeout: %d\n", arguments.stretch_timeout);
        printf("max_stretch_loop: %d\n", max_stretch_loop);
        printf("number of I2C arbitration_lost: %d\n", arbitration_lost);
        printf("enable_special_char: %d\n", arguments.enable_special_char);
        printf("brightness: %d\n", arguments.brightness);
        printf("contrast: %d\n", arguments.contrast);
        printf("display off: %d\n", arguments.display_off);
        printf("debug enabled: %d\n", arguments.debug);
        printf("return_code: %d\n", return_code);
    }
    return return_code;
}
