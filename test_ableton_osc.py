#!/usr/bin/env python3
"""
Тестовый скрипт для проверки OSC соединения с Ableton Live
"""

import argparse
import time
from pythonosc import udp_client

def test_ableton_osc():
    # Создаем OSC клиент
    client = udp_client.SimpleUDPClient("127.0.0.1", 9000)
    
    print("🎹 Тестирование OSC сообщений для Ableton Live")
    print("=" * 50)
    
    # Тест 1: Отправка на стандартный адрес для каналов
    print("\n📨 Тест 1: Отправка значений на /live/channel")
    for i in range(8):
        value = i / 7.0  # Нормализованное значение от 0 до 1
        client.send_message(f"/live/channel/{i}", value)
        print(f"   Канал {i}: {value:.2f}")
        time.sleep(0.1)
    
    # Тест 2: Отправка на адреса для управления громкостью
    print("\n📨 Тест 2: Управление громкостью треков")
    for track in range(4):
        for vol in [0.0, 0.5, 1.0]:
            client.send_message(f"/live/track/{track}/volume", vol)
            print(f"   Трек {track} громкость: {vol}")
            time.sleep(0.2)
    
    # Тест 3: Отправка MIDI CC сообщений через OSC
    print("\n📨 Тест 3: MIDI CC через OSC")
    for cc in range(1, 5):
        for value in range(0, 128, 32):
            client.send_message(f"/live/midi/cc/{cc}", value)
            print(f"   CC{cc}: {value}")
            time.sleep(0.1)
    
    # Тест 4: Проверка RMS значений
    print("\n📨 Тест 4: Эмуляция RMS сигнала")
    import math
    for i in range(100):
        # Генерируем синусоидальный сигнал для эмуляции RMS
        rms_value = (math.sin(i * 0.1) + 1) / 2
        client.send_message("/live/rms/0", rms_value)
        print(f"\r   RMS: {'█' * int(rms_value * 20):<20} {rms_value:.2f}", end='')
        time.sleep(0.05)
    
    print("\n\n✅ Тест завершен!")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Тест OSC для Ableton')
    args = parser.parse_args()
    
    test_ableton_osc()
