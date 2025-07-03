# Руководство по обновлению иконок

## Быстрое обновление

Для обновления всех иконок в проекте выполните:

```bash
./scripts/update_all_icons.sh
```

Этот скрипт:
1. ✅ Генерирует все PNG размеры из SVG
2. ✅ Создает ICO и ICNS файлы
3. ✅ Обновляет иконку в App Bundle
4. ✅ Обновляет иконку в дистрибутивной версии
5. ✅ Очищает кэш иконок macOS

## Места, где используются иконки

### 1. App Bundle
- Основной: `CV to OSC Converter.app/Contents/Resources/AppIcon.icns`
- Дистрибутивный: `dist/macOS/CV to OSC Converter.app/Contents/Resources/AppIcon.icns`
- Прописана в: `Info.plist` как `CFBundleIconFile = AppIcon`

### 2. DMG файл
- Используется: `assets/icon.icns`
- Прописана в: `create_macos_dist.sh` как `--volicon`

### 3. Экспорты разных форматов
- PNG: `assets/icon_16.png` до `icon_512.png`
- Windows: `assets/icon.ico`
- Favicon: `assets/favicon.ico`, `favicon-16x16.png`, `favicon-32x32.png`

## Если иконки не обновляются

### macOS
```bash
# Принудительное обновление кэша
killall Dock
killall Finder

# Обновление метки времени App Bundle
touch "CV to OSC Converter.app"

# Крайняя мера - очистка кэша иконок (требует пароль)
sudo find /var/folders -name "com.apple.iconservices*" -exec rm -rf {} +
```

### Проверка целостности
```bash
# Проверить, что файл иконки существует
ls -la "CV to OSC Converter.app/Contents/Resources/AppIcon.icns"

# Проверить содержимое Info.plist
grep -A1 "CFBundleIconFile" "CV to OSC Converter.app/Contents/Info.plist"
```

## Редактирование иконок

### Исходные файлы SVG
- `assets/icon.svg` - основная иконка (64x64)
- `assets/icon_small.svg` - для средних размеров (32x32)
- `assets/icon_tray.svg` - для системного трея (16x16)
- `assets/icon_minimal.svg` - минимальный дизайн (16x16)

### После редактирования SVG
1. Отредактируйте нужный SVG файл
2. Запустите `./scripts/update_all_icons.sh`
3. Перезапустите приложение

## Технические детали

### Требования к иконкам
- **macOS ICNS**: Мульти-размерная (16x16 до 512x512)
- **Windows ICO**: Мульти-размерная (16x16 до 256x256)
- **SVG**: Векторный формат для исходников
- **PNG**: Растровые экспорты для веб и документации

### Инструменты
- **ImageMagick**: Конвертация SVG→PNG, создание ICO
- **iconutil** (macOS): Создание ICNS из iconset
- **create-dmg**: Создание DMG с иконкой

### Цветовая схема (сохраняется во всех версиях)
- CV (аналоговый): `#ff6b35` → `#3498db`
- Фон: `#1a1a1a` (темно-серый)
- Центр: радиальный градиент белый→синий→оранжевый
