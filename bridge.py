import serial
import time
import ollama
import re

COM_PORT = '/dev/ttyUSB0'
BAUD_RATE = 9600
OLLAMA_MODEL = 'qwen2.5:1.5b'

SYSTEM_PROMPT = """Ты генератор паттернов для Arduino. Создавай последовательности команд.

ФОРМАТ: SEQ:КОМАНДА1;КОМАНДА2;КОМАНДА3

ДОСТУПНЫЕ КОМАНДЫ:
- ON:ПИНЫ:МС - включить пины на время (мс)
- OFF:ПИНЫ:0 - выключить пины
- BLINK:ПИНЫ:МС:РАЗЫ - мигать

ПИНЫ: ALL, EVEN, ODD, или числа через запятую (22,24,27)

ПРИМЕРЫ ПАТТЕРНОВ:

Светофор:
SEQ:ON:22:3000;OFF:22:0;ON:23:1000;OFF:23:0;ON:24:3000;OFF:24:0;ON:23:1000;OFF:23:0

Мигание поочерёдно:
SEQ:ON:EVEN:300;OFF:ALL:0;ON:ODD:300;OFF:ALL:0;ON:EVEN:300;OFF:ALL:0;ON:ODD:300;OFF:ALL:0

Лезгинка (быстро):
SEQ:ON:22,24,26:100;OFF:ALL:50;ON:23,25,27:100;OFF:ALL:50;ON:22,24,26:100;OFF:ALL:50;ON:23,25,27:100;OFF:ALL:50

Волна:
SEQ:ON:22:200;OFF:ALL:0;ON:23:200;OFF:ALL:0;ON:24:200;OFF:ALL:0;ON:25:200;OFF:ALL:0;ON:26:200;OFF:ALL:0;ON:27:200;OFF:ALL:0

Танец (случайный ритм):
SEQ:ON:22,25:400;OFF:ALL:100;ON:23,27:300;OFF:ALL:150;ON:24,26:500;OFF:ALL:100;ON:22,23,24:200;OFF:ALL:0

Правила:
1. Отвечай ТОЛЬКО SEQ:... без объяснений
2. Используй разные пины для разнообразия
3. Чередуй включение и выключение
4. Для сложных паттернов используй 6-12 шагов
5. Время в миллисекундах (1000 = 1 секунда)"""

def connect_arduino():
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)
        print(f"✅ Arduino подключена к {COM_PORT}")
        return ser
    except serial.SerialException as e:
        print(f"❌ Ошибка подключения: {e}")
        return None

def get_command_from_llm(user_text):
    try:
        response = ollama.chat(model=OLLAMA_MODEL, messages=[
            {'role': 'system', 'content': SYSTEM_PROMPT},
            {'role': 'user', 'content': user_text}
        ])
        raw_cmd = response['message']['content'].strip()
        # Оставляем только SEQ: и содержимое
        if 'SEQ:' in raw_cmd.upper():
            clean_cmd = re.sub(r'[^SEQ:ONFBILK0-9,;]', '', raw_cmd.upper())
            return clean_cmd
        return None
    except Exception as e:
        print(f"❌ Ошибка нейросети: {e}")
        return None

def main():
    ser = connect_arduino()
    if not ser:
        return

    print("🧠 Нейросеть генерирует паттерны. Примеры:")
    print("  • светофор")
    print("  • лезгинка")
    print("  • волна")
    print("  • танец")
    print("  • мигание поочерёдно")
    print("  • exit - выход")

    while True:
        user_input = input("\n👤 Вы: ").strip()
        if user_input.lower() in ['exit', 'quit', 'выход']:
            break
        if not user_input:
            continue

        print("🤖 Генерирую паттерн...", end=" ", flush=True)
        machine_cmd = get_command_from_llm(user_input)
        print(f"\n📡 {machine_cmd}")

        if machine_cmd:
            ser.write((machine_cmd + '\n').encode('utf-8'))
            time.sleep(0.3)
            while ser.in_waiting > 0:
                print(f"🔌 {ser.readline().decode('utf-8').strip()}")

    ser.close()

if __name__ == "__main__":
    main()
