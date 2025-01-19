# Variables
PYTHON_SCRIPT = generate_bitmap.py
CONFIG_FILE = generate_bitmap.yaml
OUTPUT_FILE = generated_fonts.h
GCC = gcc
OUTPUT = oled_display
C_SOURCES = oled_display.c font_*.c
C_FLAGS = -lwiringPi
HEADERS = $(wildcard *.h)

# Default target
all: $(OUTPUT)

# Generate the fonts header if CONFIG_FILE changes or OUTPUT_FILE is missing
$(OUTPUT_FILE): $(CONFIG_FILE)
	python3 $(PYTHON_SCRIPT) -S -w

# Compile the program if any source or header file changes
$(OUTPUT): $(C_SOURCES) $(HEADERS) $(OUTPUT_FILE)
	$(GCC) -o $(OUTPUT) $(C_SOURCES) $(C_FLAGS)
	strip $(OUTPUT)

clean:
	rm -f $(OUTPUT_FILE) $(OUTPUT)

.PHONY: all clean

