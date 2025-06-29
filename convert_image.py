from PIL import Image
import sys

def usage():
    print("Usage: python3 image2rawrgb.py <input_image> <output_file>")
    print("Converts any image to raw RGB (R,G,B per pixel, no header).")
    sys.exit(1)

if len(sys.argv) != 3:
    usage()

infile = sys.argv[1]
outfile = sys.argv[2]

try:
    img = Image.open(infile).convert('RGB')
    with open(outfile, 'wb') as f:
        f.write(img.tobytes())
    print(f"Success: {infile} -> {outfile} ({img.width}x{img.height}, {img.width*img.height*3} bytes)")
except Exception as e:
    print("Error:", e)
    usage()

