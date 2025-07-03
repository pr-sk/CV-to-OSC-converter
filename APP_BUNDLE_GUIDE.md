# 🍎 CV to OSC Converter - macOS App Bundle Guide

## ✅ Статус: ИСПРАВЛЕНО

App Bundle теперь полностью функционален с правильной системой разрешений macOS!

## 📦 Установка

### Автоматическая установка (рекомендуется)
```bash
# Запустить интерактивный установщик
./launch_app_bundle.sh
```

### Ручная установка
1. Скопируйте `CV to OSC Converter.app` в папку `/Applications`
2. Запустите приложение через Finder или Launchpad

## 🚀 Первый запуск

### Шаг 1: Проверка разрешений
```bash
./launch_app_bundle.sh --check
```

**Ожидаемый результат:**
```
🔐 Permission Status Report
============================
App Name: CV to OSC Converter
Bundle ID: com.cv-to-osc.converter
Sandboxed: No

Microphone: ✅ Granted
File Access: ✅ Granted

All Required Permissions: ✅ Granted
```

### Шаг 2: Запрос разрешений (если нужно)
```bash
./launch_app_bundle.sh --request
```

### Шаг 3: Проверка устройств
```bash
./launch_app_bundle.sh --list
```

**Ожидаемый результат:**
```
🎤 Audio Devices:
Available Input Devices:
  [0] Микрофон (prubtsov) - 1 channels
  [1] NDI Audio - 2 channels
  [2] Микрофон MacBook Pro (default) - 1 channels
  [5] VB-Cable - 2 channels
```

### Шаг 4: Запуск приложения
```bash
# Графический интерфейс
./launch_app_bundle.sh --gui

# Командная строка
./launch_app_bundle.sh --cli
```

## 🎛️ Режимы работы

### 🖥️ GUI Mode (графический интерфейс)
```bash
# Через launcher
./launch_app_bundle.sh --gui

# Напрямую через macOS
open "CV to OSC Converter.app"

# Через Finder
Двойной клик на CV to OSC Converter.app
```

**Особенности GUI:**
- Реал-тайм визуализация CV сигналов
- Drag & drop конфигурация
- Живое редактирование параметров
- Мониторинг производительности

### 💻 CLI Mode (командная строка)
```bash
# Базовый запуск
./launch_app_bundle.sh --cli

# С параметрами
./launch_app_bundle.sh --cli --audio-device "VB-Cable" --osc-port 8000

# Интерактивный режим
./launch_app_bundle.sh --cli --interactive
```

## 🔧 Диагностика и решение проблем

### Проблема: Устройства показываются как недоступные
```bash
# Детальная диагностика
./launch_app_bundle.sh --diagnostics

# Запрос разрешений
./launch_app_bundle.sh --request
```

### Проблема: GUI не запускается
```bash
# Проверка компонентов
ls -la "CV to OSC Converter.app/Contents/MacOS/"

# Принудительный сброс разрешений
tccutil reset Microphone com.cv-to-osc.converter
./launch_app_bundle.sh --request
```

### Проблема: Отсутствует сигнал
1. Проверьте подключение аудио интерфейса
2. Убедитесь в наличии CV сигнала на входе
3. Проверьте настройки диапазонов в конфигурации

## 📋 Технические детали

### Структура App Bundle
```
CV to OSC Converter.app/
├── Contents/
│   ├── Info.plist                          # Метаданные приложения
│   ├── MacOS/
│   │   ├── cv_to_osc_converter_gui         # GUI исполняемый файл
│   │   ├── cv_to_osc_converter_cli         # CLI исполняемый файл
│   │   └── cv_to_osc_converter_gui_wrapper # Wrapper скрипт
│   └── Resources/
│       ├── config.json                     # Конфигурация
│       ├── AppIcon.icns                    # Иконка приложения
│       └── imgui.ini                       # Настройки GUI
```

### Разрешения macOS
App Bundle корректно запрашивает следующие разрешения:

- **🎤 Microphone**: Для чтения CV сигналов
- **📁 Files**: Для конфигурационных файлов
- **🔐 Bundle ID**: `com.cv-to-osc.converter`

### Bundle Info.plist
Включает обязательные поля:
```xml
<key>NSMicrophoneUsageDescription</key>
<string>This app needs microphone access to read CV signals...</string>
```

## 🎯 Быстрые команды

```bash
# Полная проверка системы
./launch_app_bundle.sh --diagnostics

# Быстрый старт GUI
./launch_app_bundle.sh --gui

# Быстрый старт CLI
./launch_app_bundle.sh --cli --interactive

# Список устройств
./launch_app_bundle.sh --list

# Статус разрешений
./launch_app_bundle.sh --check
```

## 📱 Интеграция с macOS

### Dock и Launchpad
После установки в `/Applications` приложение появится в:
- 🚀 **Launchpad** (поиск "CV to OSC")
- 🎯 **Spotlight** (Cmd+Space, поиск "CV to OSC")
- 📁 **Finder Applications**

### System Preferences
Разрешения можно настроить в:
- **System Preferences** → **Security & Privacy** → **Privacy** → **Microphone**

### Activity Monitor
Приложение отображается как:
- GUI: `cv_to_osc_converter_gui`
- CLI: `cv_to_osc_converter_cli`

---

## ✅ Итоговое состояние

### 🎉 Что работает:
- ✅ App Bundle запускается корректно
- ✅ Разрешения запрашиваются автоматически
- ✅ Аудио устройства обнаруживаются
- ✅ GUI и CLI режимы функциональны
- ✅ OSC сообщения отправляются
- ✅ Конфигурация сохраняется

### 🛠️ Системные требования:
- **macOS**: 10.15+ (Catalina и новее)
- **Архитектура**: Intel x64 / Apple Silicon (универсальный binary)
- **Разрешения**: Микрофон (обязательно)
- **Зависимости**: Встроены в bundle

**Приложение готово к использованию!** 🚀
