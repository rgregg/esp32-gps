"""Converts images into RGB888 format"""

import os
import sys
from PIL import Image

def is_image_file(filename):
    """Indicates of a file is a supported image extension"""
    return filename.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp'))

def convert_image_to_rgb(infile, outfile):
    """Converts an infile into an RGB888 outfile"""
    try:
        img = Image.open(infile)
        # If the image has an alpha channel, composite it over black
        if img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info):
            background = Image.new('RGB', img.size, (0, 0, 0))  # Black background
            img = Image.alpha_composite(background.convert('RGBA'),
                                        img.convert('RGBA')).convert('RGB')
        else:
            img = img.convert('RGB')  # No alpha, convert directly
        with open(outfile, 'wb') as f:
            f.write(img.tobytes())
        print(f"Success: {infile} -> {outfile} ({img.width}x{img.height}, " +
              f"{img.width * img.height * 3} bytes)")
    except (OSError, IOError, ValueError) as e:
        print(f"Error processing {infile}: {e}")

def main():
    """main entry point for the app"""
    if len(sys.argv) != 3:
        print("Usage: python convert_image.py <input_directory> <output_directory>")
        sys.exit(1)

    input_dir = sys.argv[1]
    output_dir = sys.argv[2]

    if not os.path.isdir(input_dir):
        print(f"Error: Input directory '{input_dir}' does not exist.")
        sys.exit(1)

    os.makedirs(output_dir, exist_ok=True)

    files = os.listdir(input_dir)
    image_files = [f for f in files if is_image_file(f)]

    if not image_files:
        print("No image files found in the input directory.")
        return

    for infile in image_files:
        inpath = os.path.join(input_dir, infile)
        base, _ = os.path.splitext(infile)
        outfile = os.path.join(output_dir, f"{base}.rgb")
        convert_image_to_rgb(inpath, outfile)

if __name__ == '__main__':
    main()
