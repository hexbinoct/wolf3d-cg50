"""Draw the add-in's menu icons (92x64): icon/icon-uns.png and icon/icon-sel.png.

usage: python3 tools/mkicon.py   (needs Pillow and the Impact font of macOS)
"""
from PIL import Image, ImageDraw, ImageFont

FONT = '/System/Library/Fonts/Supplemental/Impact.ttf'

def icon(bg, frame, path):
    im = Image.new('RGB', (92, 64), bg)
    d = ImageDraw.Draw(im)
    d.rectangle([0, 0, 91, 63], outline=frame, width=2)
    big = ImageFont.truetype(FONT, 30)
    small = ImageFont.truetype(FONT, 18)
    for text, font, y, fill in (('WOLF', big, 4, (200, 30, 30)), ('3D', small, 38, (240, 240, 240))):
        w = d.textlength(text, font=font)
        d.text(((92 - w) / 2 + 1, y + 1), text, font=font, fill=(0, 0, 0))
        d.text(((92 - w) / 2, y), text, font=font, fill=fill)
    im.save(path)

icon((40, 40, 48), (90, 90, 100), 'icon/icon-uns.png')
icon((60, 70, 110), (255, 255, 255), 'icon/icon-sel.png')
