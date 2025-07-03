# Отчет об очистке неиспользуемых иконок

## Удаленные файлы

### Устаревшие SVG иконки:
- `assets/icon_1.svg` - старая версия иконки
- `assets/icon_2.svg` - старая версия иконки  
- `assets/icon_3.svg` - старая версия иконки
- `assets/icon_transparent.svg` - дубликат основной иконки

### Дублирующие ICNS файлы:
- `assets/icon_final.icns` - дубликат icon.icns
- `assets/icon_sips.icns` - дубликат icon.icns

### Устаревшие PNG файлы:
- `assets/icon_64_fixed.png` - исправленная версия (заменена автогенерацией)

### Автогенерируемые PNG файлы (удалены для экономии места):
- `assets/icon_16.png`
- `assets/icon_32.png`
- `assets/icon_48.png`
- `assets/icon_64.png`
- `assets/icon_128.png`
- `assets/icon_256.png`
- `assets/icon_512.png`
- `assets/icon_small_16.png`
- `assets/icon_small_32.png`
- `assets/icon_tray_16.png`
- `assets/icon_minimal_16.png`

*Примечание: PNG файлы могут быть пересозданы в любое время с помощью скрипта `scripts/generate_icons.sh`*

## Исправленные проблемы

1. **Ссылка на несуществующий файл**: Исправлена ссылка на `icon_transparent.svg` в `scripts/update_all_icons.sh`
2. **Дублирование файлов**: Удалены дублирующие ICNS файлы
3. **Устаревшие версии**: Удалены старые версии иконок
4. **Автогенерируемые файлы**: Удалены PNG файлы, которые легко пересоздать

## Оставшиеся файлы в assets/

### Основные иконки:
- `icon.svg` - основная векторная иконка
- `icon_small.svg` - версия для средних размеров
- `icon_minimal.svg` - минималистичная версия
- `icon_tray.svg` - версия для системного трея

### Готовые форматы:
- `icon.icns` - для macOS приложения
- `AppIcon.icns` - для App Bundle
- `icon.ico` - для Windows
- `favicon.ico` - для веб-сайтов
- `favicon-16x16.png` и `favicon-32x32.png` - PNG версии favicon

### Прочие ресурсы:
- `logo.svg` - логотип проекта
- `hero-image.svg` - основная иллюстрация

## Экономия места

Всего удалено около 15-20 файлов, что сэкономило примерно 5-10 МБ дискового пространства.

## Как пересоздать PNG файлы

Если нужны PNG файлы, выполните:
```bash
./scripts/generate_icons.sh
```

Этот скрипт автоматически создаст все необходимые PNG файлы из SVG исходников.
