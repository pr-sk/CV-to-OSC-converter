# macOS Permissions and Security Guide

## Overview

CV to OSC Converter на macOS требует специальных разрешений для доступа к микрофону и файловой системе. Это руководство объясняет, как управлять этими разрешениями.

## Требуемые Разрешения

### 🎤 Микрофон
- **Назначение**: Чтение CV (Control Voltage) сигналов через аудио интерфейс
- **Системная категория**: Privacy & Security > Privacy > Microphone
- **Статус**: Обязательно для работы приложения

### 📁 Файлы
- **Назначение**: Чтение конфигурационных файлов и сохранение настроек
- **Системная категория**: Privacy & Security > Privacy > Files and Folders  
- **Статус**: Рекомендуется для полной функциональности

## Проверка Разрешений

### Командная строка
```bash
# Проверить текущий статус всех разрешений
./cv_to_osc_converter --check-permissions

# Запросить все необходимые разрешения
./cv_to_osc_converter --request-permissions
```

### Вывод команды проверки
```
🔐 Permission Status Report
============================
App Name: CV to OSC Converter
Bundle ID: com.example.cv-to-osc-converter
Sandboxed: No

Microphone: ✅ Granted
File Access: ✅ Granted

All Required Permissions: ✅ Granted
```

## Статусы Разрешений

- **✅ Granted**: Разрешение предоставлено
- **❌ Denied**: Разрешение отклонено пользователем
- **⚠️ Not Determined**: Разрешение ещё не запрашивалось
- **🔒 Restricted**: Разрешение ограничено системной политикой
- **❓ Unknown**: Статус неизвестен

## Ручная Настройка Разрешений

### Способ 1: Через Системные Настройки
1. Откройте **System Preferences** (Системные Настройки)
2. Перейдите в **Security & Privacy** → **Privacy**
3. Выберите **Microphone** в левом меню
4. Найдите **CV to OSC Converter** в списке и поставьте галочку
5. При необходимости повторите для **Files and Folders**

### Способ 2: Через приложение
Запустите приложение с опцией `--request-permissions`:
```bash
./cv_to_osc_converter --request-permissions
```

### Способ 3: Автоматически при первом запуске
При первом использовании аудио устройств macOS автоматически запросит разрешение.

## Решение Проблем

### Проблема: Аудио устройства показываются как [UNAVAILABLE]
**Причина**: Отсутствует разрешение на доступ к микрофону

**Решение**:
1. Проверьте статус: `./cv_to_osc_converter --check-permissions`
2. Запросите разрешения: `./cv_to_osc_converter --request-permissions`
3. Или вручную включите в System Preferences

### Проблема: Не удается загрузить конфигурацию
**Причина**: Отсутствует разрешение на доступ к файлам

**Решение**:
1. Проверьте права доступа к файлу `config.json`
2. Запустите с правами доступа к необходимым папкам
3. Используйте абсолютные пути для конфигурационных файлов

### Проблема: Приложение не запрашивает разрешения
**Причина**: Разрешения уже были отклонены ранее

**Решение**:
1. Сбросьте разрешения через Terminal:
   ```bash
   tccutil reset Microphone com.example.cv-to-osc-converter
   tccutil reset SystemPolicyAllFiles com.example.cv-to-osc-converter
   ```
2. Перезапустите приложение

## Программный API

### Основные функции
```cpp
// Проверка статуса разрешений
PermissionStatus status = MacOSPermissions::checkMicrophonePermission();
bool hasPermission = MacOSPermissions::checkAllRequiredPermissions();

// Запрос разрешений с callback
MacOSPermissions::requestMicrophonePermission([](bool granted) {
    if (granted) {
        std::cout << "Микрофон доступен!" << std::endl;
    }
});

// Генерация отчёта
std::string report = MacOSPermissions::generatePermissionReport();
```

### Утилиты
```cpp
// Информация о приложении
std::string appName = MacOSPermissions::getAppName();
std::string bundleId = MacOSPermissions::getBundleIdentifier();
bool sandboxed = MacOSPermissions::isAppSandboxed();

// Открытие системных настроек
MacOSPermissions::openSystemPreferences(PermissionType::Microphone);
```

## Интеграция с App Store

При распространении через App Store:

1. **Добавьте в Info.plist**:
   ```xml
   <key>NSMicrophoneUsageDescription</key>
   <string>This app needs microphone access to read CV signals from your audio interface</string>
   ```

2. **Включите необходимые entitlements**:
   ```xml
   <key>com.apple.security.device.audio-input</key>
   <true/>
   ```

3. **Рассмотрите использование sandboxing**:
   ```xml
   <key>com.apple.security.app-sandbox</key>
   <true/>
   ```

## Команды Разработчика

### Сброс разрешений для тестирования
```bash
# Сбросить разрешения микрофона
tccutil reset Microphone $(./cv_to_osc_converter --check-permissions | grep "Bundle ID" | cut -d: -f2 | xargs)

# Показать все разрешения TCC
tccutil dump
```

### Проверка подписи приложения
```bash
# Проверить подпись
codesign -dv --verbose=4 ./cv_to_osc_converter

# Проверить entitlements
codesign -d --entitlements :- ./cv_to_osc_converter
```

## Советы по Безопасности

1. **Минимальные разрешения**: Запрашивайте только необходимые разрешения
2. **Объяснение пользователю**: Всегда объясняйте, зачем нужно разрешение
3. **Graceful degradation**: Предоставьте альтернативы при отсутствии разрешений
4. **Регулярная проверка**: Периодически проверяйте статус разрешений

## Ссылки

- [Apple Privacy Guidelines](https://developer.apple.com/privacy/)
- [TCC Database Documentation](https://developer.apple.com/documentation/bundleresources/information_property_list/protected_resources)
- [macOS Security Framework](https://developer.apple.com/documentation/security)
