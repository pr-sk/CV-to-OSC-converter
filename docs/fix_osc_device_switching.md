# Исправление зависания при переключении OSC устройств

## Описание проблемы

Приложение зависало при переключении между OSC устройствами в канале. Причина - при выборе одного OSC устройства, а затем изменении на другое, возникала ситуация взаимной блокировки (deadlock) при очистке и создании новых соединений.

## Причины зависания

1. **Взаимные блокировки мьютексов**: При обновлении конфигурации устройства метод `updateDeviceConfig` держал блокировку `stateMutex_`, а затем вызывал `cleanupDevice`, который пытался заблокировать `deviceMutex_`. Это могло приводить к deadlock.

2. **Неправильная очистка ресурсов**: OSC приемники и отправители не полностью освобождались перед удалением из коллекций, что могло оставлять активные сетевые соединения.

3. **Отсутствие задержек**: При быстром переключении устройств старые соединения не успевали корректно закрыться перед созданием новых.

## Внесенные изменения

### 1. Улучшение метода `cleanupDevice` (OSCMixerEngine.cpp)

```cpp
void OSCMixerEngine::cleanupDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    
    // Remove OSC sender
    auto senderIt = oscSenders_.find(deviceId);
    if (senderIt != oscSenders_.end()) {
        // Ensure sender is properly shut down before removal
        // This prevents hanging connections
        senderIt->second.reset();
        oscSenders_.erase(senderIt);
        std::cout << "Cleaned up OSC sender for device: " << deviceId << std::endl;
    }
    
    // Remove OSC receiver
    auto receiverIt = oscReceivers_.find(deviceId);
    if (receiverIt != oscReceivers_.end()) {
        // Properly stop receiver before removal to avoid hang
        if (receiverIt->second) {
            receiverIt->second->stop();
        }
        receiverIt->second.reset();
        oscReceivers_.erase(receiverIt);
        std::cout << "Cleaned up OSC receiver for device: " << deviceId << std::endl;
    }
    
    // For audio devices, remove audio stream
    if ((deviceId.find("audio_") == 0 || deviceId.find("real_audio_") == 0) && audioDeviceIntegration_) {
        audioDeviceIntegration_->removeAudioStream(deviceId);
        std::cout << "Cleaned up audio stream for device: " << deviceId << std::endl;
    }
}
```

### 2. Исправление метода `updateDeviceConfig` для предотвращения deadlock

```cpp
bool OSCMixerEngine::updateDeviceConfig(const std::string& deviceId, const OSCDeviceConfig& newConfig) {
    if (!validateDeviceConfig(newConfig)) {
        return false;
    }
    
    // Use unique_lock to allow manual unlock before cleanupDevice
    std::unique_lock<std::mutex> lock(stateMutex_);
    
    // Find and update the device in all channels
    bool found = false;
    bool needsCleanup = false;
    bool isInput = false;
    ChannelState channelState = ChannelState::STOPPED;
    int channelIndex = -1;
    
    // ... поиск устройства ...
    
    // Unlock before cleanup to avoid deadlock
    lock.unlock();
    
    // Clean up old configuration if needed
    if (needsCleanup) {
        // Small delay to ensure any ongoing operations complete
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        cleanupDevice(deviceId);
        
        // Another small delay to ensure old connections are fully closed
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Recreate device with new configuration if channel is running
        if (channelState == ChannelState::RUNNING && newConfig.enabled) {
            // Create new device connection
            if (isInput) {
                createOSCReceiver(newConfig);
            } else {
                createOSCSender(newConfig);
            }
        }
    }
    
    return found;
}
```

## Ключевые улучшения

1. **Использование `unique_lock`**: Позволяет вручную разблокировать мьютекс перед вызовом `cleanupDevice`, предотвращая deadlock.

2. **Вызов `reset()` на smart pointers**: Гарантирует полное освобождение ресурсов перед удалением из коллекций.

3. **Явная остановка приемников**: Вызов `stop()` на OSC приемниках перед их удалением.

4. **Добавление задержек**: Небольшие паузы (50мс) дают время для корректного завершения операций и закрытия соединений.

5. **Поддержка real_audio_ устройств**: Расширена логика для обработки как стандартных audio_, так и real_audio_ устройств.

## Результат

После применения исправлений:
- Приложение больше не зависает при переключении между OSC устройствами
- Ресурсы корректно освобождаются при смене устройств
- Отсутствуют deadlock ситуации
- Соединения правильно закрываются перед созданием новых

## Рекомендации

1. При дальнейшей разработке следует избегать вложенных блокировок мьютексов
2. Всегда полностью освобождать сетевые ресурсы перед их удалением
3. Использовать RAII и smart pointers для автоматического управления ресурсами
4. Добавить unit тесты для проверки корректности переключения устройств
