#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Font Bitmap Generator
"""

from PIL import Image, ImageFont, ImageDraw
import argparse
import logging
import yaml
import os
import sys
import urllib.request
from pathlib import Path


# Predefined symbols as provided
predefined_symbols = [
    '☺', '☻', '♥', '♦', '♣', '♠', '•',  # 7
    '◘', '○', '◙', '♂', '♀', '♪', '♫', '☼',  # 5
    '►', '◄', '↕', '‼', '¶', '§', '▬', '↨',  # 23

    '↑', '↓', '→', '←', '∟', '↔', '▲', '▼'
    '█', '▄', '▌', '▐', '▀', '░', '▒', '▓',
    '⌂', '■', '▀', '▄', '▌', '▐', '▒', '░',
    '▓', '█', '▒', '░', '▌', '▐', '▒', '░',
    '▓', '■', '▀', '▄', '▓', '▐', '■', '▀',
    '▲', '▼', '◄', '►', '◆', '◇', '○', '●',
    '◘', '◙', '◦', '▬', '▲', '▼', '▶', '◀',
    '☼', '☾', '☽', '✶', '✳', '✴', '✹',
    '✻', '✺', '✿', '❀', '❁', '❂', '❃', '❄',
    '❅', '❆', '❇', '❈', '❉', '❊', '❋', '★',
    '☆', '✪', '✫', '✬', '✭', '✮', '✯', '✰',
    # Lines and box-drawing symbols
    '─', '━', '│', '┃', '┄', '┅', '┆', '┇',
    '┈', '┉', '┊', '┋', '┌', '┍', '┎', '┏',
    '┐', '┑', '┒', '┓', '└', '┕', '┖', '┗',
    '┘', '┙', '┚', '┛', '├', '┝', '┞', '┟',
    '┠', '┡', '┢', '┣', '┤', '┥', '┦', '┧',
    '┨', '┩', '┪', '┫', '┬', '┭', '┮', '┯',
    '┰', '┱', '┲', '┳', '┴', '┵', '┶', '┷',
    '┸', '┹', '┺', '┻', '┼', '┽', '┾', '┿',
    '╀', '╁', '╂', '╃', '╄', '╅', '╆', '╇',
    '╈', '╉', '╊', '╋', '═', '║', '╒', '╓',
    '╔', '╕', '╖', '╗', '╘', '╙', '╚', '╛',
    '╜', '╝', '╞', '╟', '╠', '╡', '╢', '╣',
    '╤', '╥', '╦', '╧', '╨', '╩', '╪', '╫',
    '╬', '╭', '╮', '╯', '╰',                    # 218

    'ı', 'Œ', 'œ', 'Š', 'š', 'Ÿ', 'Ž', 'ž',
    '•', '…', '‐', '€', '⚡', '□', '▣', '▪',
    '▲', '△', '▴', '▵', '▶', '▷', '▸', '▹',

]

predefined_high_symbols = [
    '█', '▄', '▌', '▐', '▀', '░', '▒', '▓',
    '⌂', '■', '▀', '▄', '▌', '▐', '▒', '░',
    '▓', '█', '▒', '░', '▌', '▐', '▒', '░',
    '▓', '■', '▀', '▄', '▓', '▐', '■', '▀',
    '▒',
    # '░', '▀', '▐', '▌', '▐', '█', '▌'
]

def crop_and_center_bitmap(char, bitmap, settings):
    """
    Crop and center a bitmap, then ensure the content is centered by
    rotating blank columns.

    Returns:
        list[int], The cropped and centered bitmap
        altered: whether the crop altered the content
                 (True = altered, None = blank, False = unaltered)
    """
    def is_column_blank(column_byte):
        """Check if a column is blank."""
        return column_byte == 0
    
    def count_column_usage(column_byte):
        """Count how many pixels are HIGH in a column."""
        return sum(1 for y in range(settings["char_height"]) if column_byte & (1 << y))
    
    def find_content_bounds(bitmap_list):
        """Find the leftmost and rightmost non-blank columns."""
        left = 0
        right = len(bitmap_list) - 1
        
        while left <= right and is_column_blank(bitmap_list[left]):
            left += 1
        while right >= left and is_column_blank(bitmap_list[right]):
            right -= 1
            
        return left, right
    
    working_bitmap = bitmap.copy()
    altered = False
    
    # Step 1: Perform the requested number of crops
    for _ in range(settings["columns_to_crop"]):
        if len(working_bitmap) <= 1:
            break
            
        first_blank = is_column_blank(working_bitmap[0])
        last_blank = is_column_blank(working_bitmap[-1])
        
        if first_blank:  # crop priority to left
            working_bitmap.pop(0)
        elif last_blank:
            working_bitmap.pop()
        else:
            altered = True  # Here the crop loses information
            first_usage = count_column_usage(working_bitmap[0])
            last_usage = count_column_usage(working_bitmap[-1])
            if last_usage <= first_usage:
                working_bitmap.pop()
            else:
                working_bitmap.pop(0)
    
    # Step 2: Find the content bounds in the cropped bitmap
    left_bound, right_bound = find_content_bounds(working_bitmap)
    
    # If all columns are blank, just return centered blank columns
    if left_bound > right_bound:
        final_width = settings["char_width"] - settings["columns_to_crop"]
        return [0] * final_width, None
    
    # Calculate the content width and total blank columns
    content_width = right_bound - left_bound + 1
    total_blank_columns = len(working_bitmap) - content_width
    
    # Calculate ideal blank columns on each side for perfect centering
    ideal_left_blank = total_blank_columns // 2
    
    # Count actual blank columns on the left
    actual_left_blank = left_bound
    
    # If the content isn't centered, redistribute blank columns
    if actual_left_blank != ideal_left_blank:
        content = working_bitmap[left_bound:right_bound + 1]
        final_width = len(working_bitmap)
        left_padding = [0] * ideal_left_blank
        right_padding = [0] * (final_width - content_width - ideal_left_blank)
        working_bitmap = left_padding + content + right_padding
    
    return working_bitmap, altered


# https://stackoverflow.com/a/46220683/10598800
# https://stackoverflow.com/questions/27631736
# https://m2.material.io/design/typography/understanding-typography.html
def draw_text(char, settings):
    if settings["fixed_font_size"]:
        font_size = settings["fixed_font_size"]
        font = ImageFont.truetype(settings["font_name"], font_size)
    else:
        # Find font size for '$' that makes its height match settings["char_height"]
        max_font_size = settings["char_height"] * 2
        ref_font_size = None
        for font_size in range(max_font_size, 0, -1):
            font = ImageFont.truetype(settings["font_name"], font_size)
            (width_dollar, baseline_dollar), (offset_x_dollar, offset_y_dollar) = font.font.getsize("$")
            bbox_dollar = ImageDraw.Draw(Image.new("1", (100, 100))).textbbox((0, 0), "$", font=font)
            
            logging.debug("%s, font_size: %s, %s, %s", font_size, bbox_dollar[3], offset_y_dollar, settings["char_height"])
            if bbox_dollar[3] - offset_y_dollar <= settings["char_height"]:
                break

    # Get font metrics
    ascent, descent = font.getmetrics()
    (width_S, baseline_S), (offset_x_S, offset_y_S) = font.font.getsize("S")
    (width, baseline), (offset_x, offset_y) = font.font.getsize(char)

    image = Image.new("1", (settings["char_width"] + settings["aditional_width"], settings["char_height"]), color=1)
    draw = ImageDraw.Draw(image)
    draw.fontmode = "1"  # disable antialiasing
    bbox = draw.textbbox((0, 0), char, font=font)
    char_actual_width = bbox[2] - bbox[0]
    char_actual_height = bbox[3] - bbox[1]

    logging.debug("%s: font_size: %s, %s, %s, %s, %s, %s, %s, %s", char, font_size, offset_y, ascent - offset_y, ascent, descent, char_actual_height, bbox[3], bbox[1])
    if bbox[3] - offset_y_S > settings["char_height"]:
        font_size -= settings["reduce_font"]
        font = ImageFont.truetype(settings["font_name"], font_size)
        (width_S, baseline_S), (offset_x_S, offset_y_S) = font.font.getsize("S")

    draw.text((settings["char_x_anchor"], -offset_y_S), char, font=font, fill=0, stroke_width=settings["stroke_width"])

    return image


def generate_char_bitmap(char, settings):
    image = draw_text(char, settings)

    if settings["resize_image"]:
        image = image.resize((settings["char_width"] + settings["resize_image"], settings["char_height"]), Image.Resampling.LANCZOS)
    
    # Convert to bitmap format
    bitmap = []
    for x in range(settings["char_width"]):
        byte = 0
        for y in range(settings["char_height"]):
            pixel = image.getpixel((x, y))  # Get pixel value (0 for black, 255 for white)
            if pixel == 0:  # Set the bit if the pixel is black
                byte |= (1 << y)
        bitmap.append(byte)

    # Pad the bitmap to ensure it has the specified width
    while len(bitmap) < settings["char_width"]:
        bitmap.append(0)

    return bitmap


def find_font_size(settings):
    size = 4  # Start with the smallest size
    max_size = 40  # Set an upper limit for font size to prevent infinite loops
    last_valid_font = None
    width = 0
    highest_char_height = 0  # Track the highest character height

    while size <= max_size:
        font = ImageFont.truetype(settings["font_name"], size)
        max_height_in_current_size = 0

        for char in settings["full_size_chars"]:
            # Measure the character bounding box
            dummy_image = Image.new("1", (100, 100), color=1)  # Large dummy canvas
            draw = ImageDraw.Draw(dummy_image)
            left, top, right, bottom = draw.textbbox((0, 0), char, font=font)
            height = abs(bottom - top)
            max_height_in_current_size = max(max_height_in_current_size, height)

        if max_height_in_current_size <= settings["target_height"]:
            last_valid_font = font
            highest_char_height = max_height_in_current_size
        elif max_height_in_current_size > settings["target_height"]:
            break  # Stop when we exceed the target height

        size += 1

    if not last_valid_font:
        raise ValueError("Could not find a suitable font size for the given target height.")
    
    return last_valid_font, highest_char_height


def generate_font_bitmap(settings, show_image, save_image, write_file):
    # Test opening font
    try:
        font = ImageFont.truetype(settings["font_name"], 9)
        font, max_height = find_font_size(settings)
        logging.debug(f"Suggested minimum font size: {max_height}")
        logging.info(f'Generating {settings["char_width"] - settings["columns_to_crop"]}x{settings["char_height"]} font bitmap.')
    except OSError:
        logging.error(f'Unable to open font at {settings["font_name"]}.')
        quit(1)

    # Initialize the output list
    output_lines = [settings["array_declare_open"] % settings["font_label"]]
    char_images = []
    errors = { "font_errors" : 0}
    
    # Create an image large enough to display all characters
    rows = (settings["total_chars"] + settings["grid_columns"] - 1) // settings["grid_columns"]
    image_width = (settings["char_width"] + settings["grid_padding"]) * settings["grid_columns"]
    image_height = (settings["char_height"] + settings["grid_padding"]) * (rows + 1)
    grid_image = Image.new('RGB', (image_width, image_height), color=(255, 255, 255))

    # Draw each character with an external box (input parameters are rotated)
    def draw_char_with_external_box(char_height, char_bitmap, x, y, altered):
        # Rotate and convert bitmap to PIL format
        char_image = Image.new('L', (char_height, settings["char_width"] - settings["columns_to_crop"]), color=255)
        draw = ImageDraw.Draw(char_image)
        for row, byte in enumerate(char_bitmap):
            #print(byte, f"[{format(byte, '16b')}]", row)
            for col in range(settings["char_height"]):
                #print(f'[{format(1 << (settings["char_height"] - col - 1), "16b")}]')
                if byte & (1 << (settings["char_height"] - col - 1)):
                    draw.point((col, row), fill=0)
        # Rotate for natural orientation
        rotated_char_image = char_image.rotate(90, expand=True)

        # Create a new image with padding for the external box
        box_width, box_height = rotated_char_image.size
        padded_image = Image.new('RGB', (box_width + settings["pad_width"], box_height + settings["pad_height"]), (255, 255, 255))
        padded_image.paste(rotated_char_image, (1, 1))

        outline = settings["normal_grid_color"]
        if altered is True:
            outline = settings["altered_grid_color"]
        if altered is None:
            outline = settings["blank_grid_color"]
        # Draw the red box
        draw_box = ImageDraw.Draw(padded_image)
        draw_box.rectangle(
            [(0, 0), (padded_image.width - 1, padded_image.height - 1)],
            outline=outline,
            width=1
        )

        # Paste into the grid
        grid_image.paste(padded_image, (x, y))

    def process_character(char, x, y):
        """Process a single character and update grid position"""
        bitmap = None
        if char is False:
            # Draw a 1-pixel box
            bitmap = [0x81 for _ in range(settings["char_width"] - settings["columns_to_crop"])]
            bitmap[0] = 0xFF
            bitmap[-1] = 0xFF
            for i in range(settings["columns_to_crop"]):
                bitmap.append(0)
        if char:
            adjust = settings.get("adjust")
            if adjust:
                for i in adjust:
                    if i == char:
                        bitmap = adjust[i]
                        if not len(bitmap):
                            continue
                        if isinstance(bitmap[0], str):
                            result = []
                            for s in bitmap:
                                binary_str = s.replace('O', '1').replace('o', '1').replace('.', '0')
                                number = int(binary_str, 2)
                                result.append(number)
                            bitmap = result
                        if len(bitmap) == settings["char_width"] - settings["columns_to_crop"]:
                            for i in range(settings["columns_to_crop"]):
                                bitmap.append(0)
        if char and not bitmap:
            try:
                bitmap = generate_char_bitmap(char, settings)
            except UnicodeEncodeError as e:
                if not errors["font_errors"]:
                    logging.warning(f"Font issue (any other font error is not shown):", e)
                errors["font_errors"] += 1
        if not bitmap:
            bitmap = [0xFF for _ in range(settings["char_width"] - settings["columns_to_crop"])]
            for i in range(settings["columns_to_crop"]):
                bitmap.append(0)
            char = 'ALL 0xFF'

        # Draw character and update position
        cropped_bitmap, altered = crop_and_center_bitmap(char, bitmap, settings)
        draw_char_with_external_box(settings["char_height"], cropped_bitmap + [0xff] + [0xff], x, y, altered)

        # Add to output lines
        if altered is True:
            output_lines.append(f'    ' + ', '.join(f'0x{byte:02X}' for byte in cropped_bitmap) + f', // Character \'{char} (cropped)\'')
        if altered is None:
            output_lines.append(f'    ' + ', '.join(f'0x{byte:02X}' for byte in cropped_bitmap) + f', // Character \'Null\'')
        if altered is False:
            output_lines.append(f'    ' + ', '.join(f'0x{byte:02X}' for byte in cropped_bitmap) + f', // Character \'{char}\'')

        x += settings["char_width"] + settings["grid_padding"]
        if x + settings["char_width"] > image_width:
            x = 0
            y += settings["char_height"] + settings["grid_padding"]

        if settings.get("convert", "") == char:
            for col in bitmap:
                print(f"          - {format(col, '08b')}".replace('1', 'O').replace('0', '.'))

        return x, y

    x, y = 0, 0

    # Add special box representation as the first symbol
    char = False
    x, y = process_character(char, x, y)
    
    # Generate bitmaps for predefined symbols
    for char in predefined_symbols[settings["start_predefined_symbols"]:settings["start_predefined_symbols"] + 31]:
        x, y = process_character(char, x, y)

    # Generate bitmaps for Basic Latin characters (U+0020 to U+007E)
    for code_point in range(0x20, 0x7F):
        x, y = process_character(chr(code_point), x, y)

    for char in predefined_high_symbols[settings["start_predefined_high_symbols"]:settings["start_predefined_high_symbols"] + 33]:
        x, y = process_character(char, x, y)

    # Generate bitmaps for Latin-1 Supplement characters (U+00A0 to U+00FF)
    for code_point in range(0xA0, 0x100):
        x, y = process_character(chr(code_point), x, y)

    # Save or display the grid image
    if show_image or settings.get("show_image"):
        grid_image.show()  # Optionally opens the image in the default viewer
    if save_image or settings.get("save_image"):
        grid_image.save(f'{settings["font_label"]}{settings["png_extension"]}')  # Optionally save the grid image

    output_lines.append(settings["array_declare_close"] + "\n")
    
    if errors["font_errors"]:
        logging.warning("Total font errors:", errors["font_errors"])

    # Write the result to a C file (or print to console)
    if write_file or settings.get("write_file"):
        with open(settings["generated_font_path"], "w", encoding="utf-8") as f:
            f.write("\n".join(output_lines))


def main():
    import argparse
    import pickle

    parser = argparse.ArgumentParser(
        epilog='Font Bitmap Generator'
    )
    parser.add_argument(
        '-t',
        '--target',
        dest='target',
        default=None,
        type=str,
        action='store',
        help='Only process specific target in the configuration list'
    )
    parser.add_argument(
        '-d',
        '--debug',
        dest='debug',
        action='store_true',
        help='Print debug information'
    )
    parser.add_argument(
        '-s',
        '--show',
        dest='show',
        action='store_true',
        help='Show the graphical grid representing the character table'
    )
    parser.add_argument(
        '-S',
        '--save',
        dest='save',
        action='store_true',
        help='Save the image of the graphical grid representing the character table to file'
    )
    parser.add_argument(
        '-w',
        '--write',
        dest='write',
        action='store_true',
        help='Write to "generated_font_path" file'
    )
    parser.add_argument(
        '-c',
        '--config',
        dest='config_file',
        default="generate_bitmap.yaml",
        action='store',
        type=str,
        help='Configuration file (default is "generate_bitmap.yaml")'
    )
    parser.add_argument(
        '-C',
        '--convert',
        dest='convert',
        default=None,
        type=str,
        action='store',
        help='Convert character or array to bitmap.'
    )
    args = parser.parse_args()

    if not os.path.exists(args.config_file):
        logging.error("Missing configuration file '%s'.", args.config_file)
        quit(1)

    try:
        with open(args.config_file, 'rt', encoding='utf8') as file:
            yaml_settings = yaml.safe_load(file)
    except Exception as e:
        logging.error("Failed to load configuration file '%s': %s.", args.config_file, e)
        quit(1)

    target_fonts = [None]
    default_settings = {}
    for k, v in yaml_settings.items():
        if isinstance(v, list):
            target_fonts = v
            continue
        if v.get("disabled"):
            continue
        default_settings = {**default_settings, **v}
        if v.get("break"):
            break

    if args.debug:
        default_settings["debug"] = args.debug
    logging.getLogger().setLevel(logging.INFO)
    if default_settings["debug"]:
        logging.getLogger().setLevel(logging.DEBUG)
    if args.convert:
        if args.convert[0] == '[' and len(args.convert) > 4:
            try:
                for col in yaml.safe_load(args.convert):
                    print(f"          - {format(col, '08b')}".replace('1', 'O').replace('0', '.'))
            except Exception:
                logging.warning("Cannot convert %s", args.convert)
        else:
            default_settings["convert"] = args.convert

    header_handle = None
    for target in target_fonts:

        # Write the result to a C file and optionally to an H file
        settings = default_settings.copy()
        if not header_handle:
            header_file = settings.get("header_file")
        if header_file and not header_handle:
            header_handle = open(settings["header_file"], "w", encoding="utf-8")
        if not target:
            target = {None: None}
        for k, v in target.items():
            if args.target and k != args.target:
                continue
            if k and v:
                if v.get("disabled"):
                    continue
                settings = {**settings, **v}
                if v.get("break"):
                    break
            try:
                logging.info(f'Generating {settings["font_label"]} in {settings["generated_font_path"]} from {settings["font_name"]}.')
            except KeyError as e:
                logging.error(f"Key {e} not found in '{args.config_file}'.")
                quit(1)

            font_url = settings.get("font_url")
            if font_url:
                font_file = Path(settings.get("font_name"))
                if not font_file.is_file():
                    try:
                        urllib.request.urlretrieve(font_url, settings["font_name"])
                        logging.info("Downloaded '%s' from '%s'.", settings["font_name"], settings["font_url"])
                    except Exception as e:
                        logging.error("Cannot download '%s' to '%s': %s", settings["font_url"], settings["font_name"], e)
                        quit(1)

            generate_font_bitmap(settings, args.show, args.save, args.write)
            if header_file:
                header_handle.write((settings["font_header_declare"] % k) + "\n")
    if header_handle:
            header_handle.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        logging.warning("\nInterrupted.")
        sys.exit(0)
