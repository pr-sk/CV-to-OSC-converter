# CV to OSC Converter - GUI Launch Instructions

## ✅ Сборка завершена успешно!

У вас есть два исполняемых файла:
- `build/cv_to_osc_converter` - CLI версия (работает в терминале)
- `build/cv_to_osc_converter_gui` - GUI версия (требует графическое окружение)

## 🎯 Способы запуска GUI версии

### 1. Через macOS Finder (Рекомендуется)

Мы создали App Bundle для macOS:

```bash
open "CV to OSC Converter.app"
```

Или:
1. Откройте Finder
2. Перейдите к папке `/Users/prubtsov/cv_to_osc_converter`
3. Дважды кликните на `CV to OSC Converter.app`

### 2. Через Terminal.app (если Warp не работает)

1. Закройте Warp Terminal
2. Откройте стандартный **Terminal.app** (Applications → Utilities → Terminal)
3. Выполните:
```bash
cd /Users/prubtsov/cv_to_osc_converter
./launch_gui.sh
```

### 3. Через командную строку напрямую

```bash
cd /Users/prubtsov/cv_to_osc_converter/build
./cv_to_osc_converter_gui
```

## 🔧 Если GUI не запускается

### Проверьте разрешения на доступ к камере/микрофону

macOS может запросить разрешения для аудио интерфейса:
1. System Preferences → Security & Privacy → Privacy
2. Microphone → Разрешить доступ для Terminal/приложения

### Проверьте аудио устройства

```bash
# Список доступных аудио устройств
./cv_to_osc_converter --list-devices
```

### Запустите CLI версию для тестирования

```bash
# Тест базовой функциональности
./cv_to_osc_converter --help
./cv_to_osc_converter --list-devices
```

## 🎮 Использование GUI

После успешного запуска GUI вы увидите:

1. **Main Window** - основное окно с управлением
2. **Channel Visualization** - реалтайм визуализация сигналов
3. **Configuration** - настройка каналов и OSC

### Основные функции:
- **Start/Stop Conversion** - запуск/остановка конвертации
- **Real-time meters** - индикаторы уровня сигнала
- **Live waveforms** - графики сигналов в реальном времени
- **Channel configuration** - настройка диапазонов и OSC адресов
- **Audio device selection** - выбор аудио интерфейса

## 📝 Конфигурация

GUI автоматически создаст `config.json` при первом запуске.

Базовые настройки:
- **OSC Host**: 127.0.0.1 (для локального использования)
- **OSC Port**: 9000 (стандартный порт)
- **Update Rate**: 100Hz (10ms интервал)
- **CV Ranges**: 0-10V (стандартный Eurorack)

## 🆘 Troubleshooting

### "Failed to create GLFW window"
- Убедитесь, что запускаете в графическом окружении (не через SSH)
- Попробуйте Terminal.app вместо Warp
- Используйте App Bundle через Finder

### "No audio device found"
- Подключите аудио интерфейс
- Проверьте Audio MIDI Setup
- Запустите `--list-devices` для диагностики

### "OSC messages not received"
- Проверьте настройки файервола
- Тестируйте с OSC monitor приложением
- Используйте localhost (127.0.0.1) для локального тестирования

## 📞 Поддержка

Если GUI не запускается, используйте CLI версию:
```bash
cd /Users/prubtsov/cv_to_osc_converter/build
./cv_to_osc_converter --interactive
```

CLI версия предоставляет все основные функции конвертации CV в OSC.
