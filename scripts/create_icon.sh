#!/bin/bash

# Create app icon for CV to OSC Converter
# This script creates a simple app icon using built-in macOS tools

ICON_DIR="resources"
ICON_NAME="AppIcon"

echo "Creating app icon..."

# Create resources directory
mkdir -p "$ICON_DIR"

# Create a simple icon using sips and built-in assets
# For now, we'll create a basic colored icon
# In production, you'd use a proper 1024x1024 icon design

# Create temporary PNG (simplified approach)
cat > /tmp/create_icon.py << 'EOF'
import os
from PIL import Image, ImageDraw, ImageFont

# Create a 1024x1024 icon
size = 1024
image = Image.new('RGBA', (size, size), (0, 0, 0, 0))
draw = ImageDraw.Draw(image)

# Background gradient (blue to purple)
for y in range(size):
    color = (int(50 + y * 0.1), int(100 + y * 0.05), int(200 - y * 0.05), 255)
    draw.line([(0, y), (size, y)], fill=color)

# Draw CV waveform
draw.rectangle([100, 400, 924, 450], fill=(255, 255, 255, 200))
for x in range(100, 900, 20):
    y = 425 + 15 * (1 if (x // 20) % 2 else -1)
    draw.line([(x, 425), (x + 10, y)], fill=(0, 255, 100, 255), width=3)

# Draw OSC representation
for i, y in enumerate(range(550, 650, 20)):
    alpha = 255 - i * 30
    draw.ellipse([200 + i * 40, y, 240 + i * 40, y + 15], 
                fill=(255, 200, 0, alpha))

# Add text
try:
    # Try to use a system font
    font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 60)
except:
    font = ImageFont.load_default()

text = "CV→OSC"
bbox = draw.textbbox((0, 0), text, font=font)
text_width = bbox[2] - bbox[0]
text_height = bbox[3] - bbox[1]
text_x = (size - text_width) // 2
text_y = 300
draw.text((text_x, text_y), text, fill=(255, 255, 255, 255), font=font)

# Save the icon
image.save('/tmp/icon_1024.png')
EOF

# Check if PIL is available, if not use a simpler approach
if python3 -c "import PIL" 2>/dev/null; then
    echo "Creating icon with Python/PIL..."
    python3 /tmp/create_icon.py
    mv /tmp/icon_1024.png "$ICON_DIR/icon_1024.png"
else
    echo "PIL not available, creating simple icon..."
    # Create a simple solid color icon as fallback
    sips -c 1024 1024 --setProperty format png /System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/BNDL.icns --out "$ICON_DIR/icon_1024.png" 2>/dev/null || {
        echo "Creating minimal icon..."
        # Ultimate fallback - create with imagemagick if available, or skip
        if command -v convert >/dev/null 2>&1; then
            convert -size 1024x1024 gradient:blue-purple "$ICON_DIR/icon_1024.png"
        else
            echo "⚠ No icon creation tools available. Skipping icon creation."
            exit 0
        fi
    }
fi

if [ -f "$ICON_DIR/icon_1024.png" ]; then
    echo "Creating iconset..."
    ICONSET="$ICON_DIR/$ICON_NAME.iconset"
    mkdir -p "$ICONSET"
    
    # Generate different icon sizes
    sips -z 16 16 "$ICON_DIR/icon_1024.png" --out "$ICONSET/icon_16x16.png"
    sips -z 32 32 "$ICON_DIR/icon_1024.png" --out "$ICONSET/icon_16x16@2x.png"
    sips -z 32 32 "$ICON_DIR/icon_1024.png" --out "$ICONSET/icon_32x32.png"
    sips -z 64 64 "$ICON_DIR/icon_1024.png" --out "$ICONSET/icon_32x32@2x.png"
    sips -z 128 128 "$ICON_DIR/icon_1024.png" --out "$ICONSET/icon_128x128.png"
    sips -z 256 256 "$ICON_DIR/icon_1024.png" --out "$ICONSET/icon_128x128@2x.png"
    sips -z 256 256 "$ICON_DIR/icon_1024.png" --out "$ICONSET/icon_256x256.png"
    sips -z 512 512 "$ICON_DIR/icon_1024.png" --out "$ICONSET/icon_256x256@2x.png"
    sips -z 512 512 "$ICON_DIR/icon_1024.png" --out "$ICONSET/icon_512x512.png"
    cp "$ICON_DIR/icon_1024.png" "$ICONSET/icon_512x512@2x.png"
    
    # Create .icns file
    iconutil -c icns "$ICONSET" -o "$ICON_DIR/$ICON_NAME.icns"
    
    # Clean up
    rm -rf "$ICONSET"
    rm -f "$ICON_DIR/icon_1024.png"
    
    echo "✓ Icon created: $ICON_DIR/$ICON_NAME.icns"
else
    echo "⚠ Could not create icon"
fi

# Clean up temp files
rm -f /tmp/create_icon.py /tmp/icon_1024.png
