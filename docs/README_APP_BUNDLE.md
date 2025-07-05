# 🎵 CV to OSC Converter - App Bundle Edition

**Конвертер CV сигналов в OSC сообщения для macOS**

---

## ⚡ Быстрый Старт

### 1️⃣ Первый запуск
```bash
./launch_app_bundle.sh
```

Выберите:
- **2** - Запросить разрешения
- **3** - Проверить устройства  
- **5** - Запустить GUI

### 2️⃣ Ежедневное использование
```bash
# Быстрый старт GUI
./launch_app_bundle.sh --gui

# Быстрый старт CLI
./launch_app_bundle.sh --cli --interactive
```

---

## ✅ Проверка Работоспособности

### Разрешения
```bash
$ ./launch_app_bundle.sh --check
🔐 Permission Status Report
============================
Microphone: ✅ Granted
All Required Permissions: ✅ Granted
```

### Устройства
```bash
$ ./launch_app_bundle.sh --list
🎤 Audio Devices:
Available Input Devices:
  [0] Микрофон MacBook Pro (default) - 1 channels
  [1] VB-Cable - 2 channels
```

### Запуск
```bash
$ ./launch_app_bundle.sh --cli
CV to OSC Converter v1.0.0
==========================
Using input device: Микрофон MacBook Pro
CV Reader initialized successfully with 1 channels
OSC sender initialized - target: 127.0.0.1:9000
Press Enter to stop...
```

---

## 🎛️ Режимы Работы

| Режим | Команда | Описание |
|-------|---------|----------|
| 🖥️ **GUI** | `--gui` | Графический интерфейс с визуализацией |
| 💻 **CLI** | `--cli` | Командная строка |
| 🔍 **Диагностика** | `--diagnostics` | Подробная информация о системе |
| 📱 **Интерактивный** | без параметров | Меню выбора действий |

---

## 🔧 Решение Проблем

### ❌ Устройства [UNAVAILABLE]
```bash
./launch_app_bundle.sh --request
```

### ❌ GUI не запускается
```bash
# Сброс разрешений
tccutil reset Microphone com.cv-to-osc.converter
./launch_app_bundle.sh --request
```

### ❌ Нет сигнала
1. Проверьте подключение аудио интерфейса
2. Убедитесь в наличии CV сигнала на входе
3. Проверьте настройки в config.json

---

## 📋 Быстрая Справка

```bash
# Все команды launcher'а
./launch_app_bundle.sh --help

# Проверка всей системы
./launch_app_bundle.sh --diagnostics

# Прямой запуск через macOS
open "CV to OSC Converter.app"
```

---

## ✨ Особенности

- ✅ **Полная поддержка macOS** - разрешения, bundle, интеграция
- ✅ **Графический интерфейс** - реал-тайм визуализация CV сигналов
- ✅ **Командная строка** - автоматизация и скриптинг
- ✅ **Автоматическая диагностика** - решение проблем
- ✅ **Горячие клавиши** - быстрое управление
- ✅ **Универсальная совместимость** - Intel + Apple Silicon

---

## 🎯 Системные Требования

- **macOS**: 10.15+ (Catalina и новее)
- **Разрешения**: Микрофон (запрашивается автоматически)
- **Аудио**: Любой совместимый интерфейс или встроенный микрофон

---

**Готово к использованию!** 🚀

Для подробной документации см. `APP_BUNDLE_GUIDE.md`
