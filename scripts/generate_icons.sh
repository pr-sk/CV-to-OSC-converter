#!/bin/bash

# Скрипт для генерации иконок в разных размерах и форматах
# Требует установленный ImageMagick

# Проверяем наличие ImageMagick
if ! command -v magick &> /dev/null; then
    echo "Ошибка: ImageMagick не установлен"
    echo "Установите с помощью: brew install imagemagick"
    exit 1
fi

# Переходим в директорию проекта
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
ASSETS_DIR="$PROJECT_DIR/assets"

cd "$PROJECT_DIR"

echo "Генерируем иконки из SVG файлов..."

# Создаем PNG из основной иконки в разных размерах
echo "📱 Генерируем PNG из icon.svg..."
magick "$ASSETS_DIR/icon.svg" -background transparent -colorspace sRGB -type TrueColorAlpha -resize 16x16 "$ASSETS_DIR/icon_16.png"
magick "$ASSETS_DIR/icon.svg" -background transparent -colorspace sRGB -type TrueColorAlpha -resize 32x32 "$ASSETS_DIR/icon_32.png"
magick "$ASSETS_DIR/icon.svg" -background transparent -colorspace sRGB -type TrueColorAlpha -resize 48x48 "$ASSETS_DIR/icon_48.png"
magick "$ASSETS_DIR/icon.svg" -background transparent -colorspace sRGB -type TrueColorAlpha -resize 64x64 "$ASSETS_DIR/icon_64.png"
magick "$ASSETS_DIR/icon.svg" -background transparent -colorspace sRGB -type TrueColorAlpha -resize 128x128 "$ASSETS_DIR/icon_128.png"
magick "$ASSETS_DIR/icon.svg" -background transparent -colorspace sRGB -type TrueColorAlpha -resize 256x256 "$ASSETS_DIR/icon_256.png"
magick "$ASSETS_DIR/icon.svg" -background transparent -colorspace sRGB -type TrueColorAlpha -resize 512x512 "$ASSETS_DIR/icon_512.png"

# Генерируем специализированные версии
echo "🔧 Генерируем специализированные версии..."
magick "$ASSETS_DIR/icon_small.svg" -background transparent -colorspace sRGB -type TrueColorAlpha -resize 16x16 "$ASSETS_DIR/icon_small_16.png"
magick "$ASSETS_DIR/icon_small.svg" -background transparent -colorspace sRGB -type TrueColorAlpha -resize 32x32 "$ASSETS_DIR/icon_small_32.png"

magick "$ASSETS_DIR/icon_tray.svg" -background transparent -colorspace sRGB -type TrueColorAlpha "$ASSETS_DIR/icon_tray_16.png"
magick "$ASSETS_DIR/icon_minimal.svg" -background transparent -colorspace sRGB -type TrueColorAlpha "$ASSETS_DIR/icon_minimal_16.png"

# Создаем ICO файл для Windows (многоуровневый)
echo "🪟 Создаем ICO файл для Windows..."
magick "$ASSETS_DIR/icon_16.png" "$ASSETS_DIR/icon_32.png" "$ASSETS_DIR/icon_48.png" "$ASSETS_DIR/icon_64.png" "$ASSETS_DIR/icon_128.png" "$ASSETS_DIR/icon_256.png" "$ASSETS_DIR/icon.ico"

# Создаем favicon
echo "🌐 Создаем favicon..."
magick "$ASSETS_DIR/icon_minimal.svg" -resize 16x16 "$ASSETS_DIR/favicon-16x16.png"
magick "$ASSETS_DIR/icon_small.svg" -resize 32x32 "$ASSETS_DIR/favicon-32x32.png"
magick "$ASSETS_DIR/favicon-16x16.png" "$ASSETS_DIR/favicon-32x32.png" "$ASSETS_DIR/favicon.ico"

# Создаем ICNS для macOS (если доступен iconutil)
if command -v iconutil &> /dev/null; then
    echo "🍎 Создаем ICNS для macOS..."
    
    # Создаем временную папку iconset
    ICONSET_DIR="$ASSETS_DIR/icon.iconset"
    mkdir -p "$ICONSET_DIR"
    
    # Копируем файлы в правильном формате для iconset
    cp "$ASSETS_DIR/icon_16.png" "$ICONSET_DIR/icon_16x16.png"
    cp "$ASSETS_DIR/icon_32.png" "$ICONSET_DIR/icon_16x16@2x.png"
    cp "$ASSETS_DIR/icon_32.png" "$ICONSET_DIR/icon_32x32.png"
    cp "$ASSETS_DIR/icon_64.png" "$ICONSET_DIR/icon_32x32@2x.png"
    cp "$ASSETS_DIR/icon_128.png" "$ICONSET_DIR/icon_128x128.png"
    cp "$ASSETS_DIR/icon_256.png" "$ICONSET_DIR/icon_128x128@2x.png"
    cp "$ASSETS_DIR/icon_256.png" "$ICONSET_DIR/icon_256x256.png"
    cp "$ASSETS_DIR/icon_512.png" "$ICONSET_DIR/icon_256x256@2x.png"
    cp "$ASSETS_DIR/icon_512.png" "$ICONSET_DIR/icon_512x512.png"
    
    # Генерируем ICNS
    iconutil -c icns "$ICONSET_DIR" -o "$ASSETS_DIR/icon.icns"
    
    # Удаляем временную папку
    rm -rf "$ICONSET_DIR"
    
    echo "✅ ICNS файл создан"
else
    echo "⚠️  iconutil не найден, пропускаем создание ICNS"
fi

echo ""
echo "✅ Все иконки сгенерированы!"
echo ""
echo "📋 Созданные файлы:"
echo "   PNG: 16x16, 32x32, 48x48, 64x64, 128x128, 256x256, 512x512"
echo "   ICO: Windows-совместимый многоуровневый файл"
echo "   Favicon: 16x16 и 32x32 PNG + ICO"
if command -v iconutil &> /dev/null; then
echo "   ICNS: macOS-совместимый файл"
fi
echo ""
echo "🔍 Файлы сохранены в: $ASSETS_DIR"

# Показываем размеры файлов
echo ""
echo "📊 Размеры файлов:"
ls -lh "$ASSETS_DIR"/icon*.png "$ASSETS_DIR"/icon*.ico 2>/dev/null | awk '{print "   " $9 ": " $5}'
if [ -f "$ASSETS_DIR/icon.icns" ]; then
    ls -lh "$ASSETS_DIR/icon.icns" | awk '{print "   " $9 ": " $5}'
fi
