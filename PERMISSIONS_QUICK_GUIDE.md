# 🔐 macOS Permissions - Быстрый Старт

## Проблема: Устройства показываются как [UNAVAILABLE]

Если при запуске `./cv_to_osc_converter --list-devices` вы видите:
```
Available Input Devices:
  [0] Микрофон MacBook Pro - 1 channels [UNAVAILABLE]
  [1] My Audio Interface - 8 channels [UNAVAILABLE]
```

## Решение: Предоставьте разрешения

### Автоматический способ (рекомендуется)
```bash
# Запросить все необходимые разрешения
./cv_to_osc_converter --request-permissions
```

### Ручной способ
1. Откройте **System Preferences** (Системные Настройки)
2. Перейдите в **Security & Privacy** → **Privacy**
3. Выберите **Microphone** в левом меню
4. Найдите **cv_to_osc_converter** в списке и поставьте галочку ✅

## Проверка результата
```bash
# Проверить статус разрешений
./cv_to_osc_converter --check-permissions

# Список устройств должен показать [AVAILABLE]
./cv_to_osc_converter --list-devices
```

## Ожидаемый результат ✅
```
🔐 Permission Status Report
============================
Microphone: ✅ Granted

Available Input Devices:
  [0] Микрофон MacBook Pro (default) - 1 channels
  [1] NDI Audio - 2 channels  
  [2] VB-Cable - 2 channels
```

## Запуск приложения
После успешного получения разрешений:
```bash
# Простой запуск
./cv_to_osc_converter

# Интерактивный режим
./cv_to_osc_converter --interactive

# С выбором устройства
./cv_to_osc_converter --audio-device "VB-Cable"
```

## Если проблема не решена
```bash
# Сбросить разрешения (для разработчиков)
tccutil reset Microphone com.unknown.cv-to-osc-converter

# Перезапустить приложение
./cv_to_osc_converter --request-permissions
```

---
**Важно**: Без разрешения на микрофон приложение не сможет читать CV сигналы!
