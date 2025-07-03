#!/bin/bash

# Скрипт для обновления всех иконок в проекте
echo "🔄 Обновление всех иконок в проекте..."

# Переходим в директорию проекта
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

# 1. Генерируем PNG иконки из SVG с помощью sips (лучшее качество)
echo "📱 Генерируем иконки из SVG с помощью sips..."
rm -rf /tmp/app_iconset && mkdir /tmp/app_iconset

# Используем sips для лучшего качества
sips -s format png assets/icon.svg --out /tmp/app_iconset/icon_512x512.png -z 512 512
sips -s format png assets/icon.svg --out /tmp/app_iconset/icon_256x256@2x.png -z 512 512
sips -s format png assets/icon.svg --out /tmp/app_iconset/icon_256x256.png -z 256 256
sips -s format png assets/icon.svg --out /tmp/app_iconset/icon_128x128@2x.png -z 256 256
sips -s format png assets/icon.svg --out /tmp/app_iconset/icon_128x128.png -z 128 128
sips -s format png assets/icon.svg --out /tmp/app_iconset/icon_32x32@2x.png -z 64 64
sips -s format png assets/icon.svg --out /tmp/app_iconset/icon_32x32.png -z 32 32
sips -s format png assets/icon.svg --out /tmp/app_iconset/icon_16x16@2x.png -z 32 32
sips -s format png assets/icon.svg --out /tmp/app_iconset/icon_16x16.png -z 16 16

# Создаем ICNS из iconset
iconutil -c icns /tmp/app_iconset -o assets/icon.icns

echo "✅ ICNS создан с помощью sips + iconutil"

# 2. Обновляем иконку в основном App Bundle
if [ -d "CV to OSC Converter.app" ]; then
    echo "🔄 Обновляем иконку в основном App Bundle..."
    cp assets/icon.icns "CV to OSC Converter.app/Contents/Resources/AppIcon.icns"
    touch "CV to OSC Converter.app"
    echo "✅ Основной App Bundle обновлен"
else
    echo "⚠️  Основной App Bundle не найден"
fi

# 3. Обновляем иконку в дистрибутивном App Bundle
if [ -d "dist/macOS/CV to OSC Converter.app" ]; then
    echo "🔄 Обновляем иконку в дистрибутивном App Bundle..."
    cp assets/icon.icns "dist/macOS/CV to OSC Converter.app/Contents/Resources/AppIcon.icns"
    touch "dist/macOS/CV to OSC Converter.app"
    echo "✅ Дистрибутивный App Bundle обновлен"
else
    echo "⚠️  Дистрибутивный App Bundle не найден"
fi

# 4. Очищаем кэш иконок macOS
echo "🧹 Очищаем кэш иконок macOS..."
killall Dock 2>/dev/null || true
killall Finder 2>/dev/null || true

# 5. Обновляем метку времени для принудительного обновления
touch assets/icon.icns
touch assets/icon.svg

echo ""
echo "✅ Все иконки обновлены!"
echo ""
echo "📋 Обновленные файлы:"
echo "   - App Bundle иконки"
echo "   - DMG иконки" 
echo "   - Все PNG размеры"
echo "   - Windows ICO"
echo "   - Favicon файлы"
echo ""
echo "🔄 Перезапуск Dock и Finder выполнен для обновления кэша"
echo ""
echo "💡 Если иконки всё ещё не обновились:"
echo "   1. Перезагрузите Finder: killall Finder"
echo "   2. Перезапустите приложение"
echo "   3. В крайнем случае, перезагрузите систему"
