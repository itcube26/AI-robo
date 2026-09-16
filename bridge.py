import serial
import time
import ollama
import re

COM_PORT = '/dev/ttyUSB0'
BAUD_RATE = 9600
OLLAMA_MODEL = 'qwen2.5:1.5b'

SYSTEM_PROMPT = """Ты генератор команд для Arduino с моторами и светодиодами.

ФОРМАТ: SEQ:КОМАНДА1;КОМАНДА2;КОМАНДА3

КОМАНДЫ ДЛЯ МОТОРОВ:
- MOTOR:FORWARD:МС - вперёд
- MOTOR:BACKWARD:МС - назад
- MOTOR:STOP:0 - стоп
- MOTOR:LEFT:МС - поворот налево (левый мотор назад, правый вперёд)
- MOTOR:RIGHT:МС - поворот направо (правый мотор назад, левый вперёд)
- MOTOR:SPIN_LEFT:МС - разворот на месте налево
- MOTOR:SPIN_RIGHT:МС - разворот на месте направо
- SPEED:ПРОЦЕНТ - установить скорость 0-100%

КОМАНДЫ ДЛЯ СВЕТОДИОДОВ:
- ON:ПИНЫ:МС - включить
- OFF:ПИНЫ:0 - выключить
- BLINK:ПИНЫ:МС:РАЗЫ - мигать

ПИНЫ: ALL, EVEN, ODD, или числа через запятую (22,24,27)

ПРИМЕРЫ:

Ехать вперёд 2 секунды:
SEQ:SPEED:80;MOTOR:FORWARD:2000;MOTOR:STOP:0

Назад и стоп:
SEQ:MOTOR:BACKWARD:1500;MOTOR:STOP:0

Поворот направо:
SEQ:SPEED:60;MOTOR:RIGHT:1000;MOTOR:STOP:0

Разворот на месте:
SEQ:SPEED:70;MOTOR:SPIN_LEFT:1500;MOTOR:STOP:0

Вперёд с миганием:
SEQ:SPEED:70;ON:22,27:0;MOTOR:FORWARD:2000;MOTOR:STOP:0;OFF:ALL:0

Змейка (влево-вправо):
SEQ:SPEED:60;MOTOR:FORWARD:500;MOTOR:LEFT:800;MOTOR:FORWARD:500;MOTOR:RIGHT:800;MOTOR:STOP:0

Правила:
1. Отвечай ТОЛЬКО SEQ:... без объяснений
2. Всегда начинай со SPEED если нужно движение
3. Всегда заканчивай MOTOR:STOP:0 после движения
4. Для поворотов используй LEFT/RIGHT (плавный) или SPIN_LEFT/SPIN_RIGHT (резкий)
5. Скорость 0-100%"""

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
        if 'SEQ:' in raw_cmd.upper():
            # Оставляем только валидные символы
            clean_cmd = re.sub(r'[^SEQ:MOTORFWARDLEFTRIGHTSPIN0-9,;]', '', raw_cmd.upper())
            return clean_cmd
        return None
    except Exception as e:
        print(f"❌ Ошибка нейросети: {e}")
        return None

def main():
    ser = connect_arduino()
    if not ser:
        return

    print("🧠 Нейросеть управляет моторами и светодиодами. Примеры:")
    print("  • вперёд 2 секунды")
    print("  • назад")
    print("  • поворот направо")
    print("  • разворот на месте")
    print("  • змейка")
    print("  • вперёд с миганием")
    print("  • exit - выход")

    while True:
        user_input = input("\n👤 Вы: ").strip()
        if user_input.lower() in ['exit', 'quit', 'выход']:
            break
        if not user_input:
            continue

        print("🤖 Генерирую...", end=" ", flush=True)
        machine_cmd = get_command_from_llm(user_input)
        print(f"\n {machine_cmd}")

        if machine_cmd:
            ser.write((machine_cmd + '\n').encode('utf-8'))
            time.sleep(0.3)
            while ser.in_waiting > 0:
                print(f"🔌 {ser.readline().decode('utf-8').strip()}")

    ser.close()

if __name__ == "__main__":
    main()
