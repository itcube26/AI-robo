// ==================== НАСТРОЙКИ ПИНОВ ====================
#define M1_dir 45      // Направление мотора 1
#define M1_Speed 44    // Скорость (PWM) мотора 1

#define M2_dir 47      // Направление мотора 2
#define M2_Speed 46    // Скорость (PWM) мотора 2

// Светодиоды (остались без изменений)
const int PINS[] = {22, 23, 24, 25, 26, 27};
const int NUM_PINS = 6;

int currentSpeedPercent = 100; // Глобальная скорость 0-100%

void setup() {
  Serial.begin(9600);
  
  // Инициализация пинов моторов
  pinMode(M1_dir, OUTPUT);
  pinMode(M1_Speed, OUTPUT);
  pinMode(M2_dir, OUTPUT);
  pinMode(M2_Speed, OUTPUT);
  
  // Инициализация пинов светодиодов
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(PINS[i], OUTPUT);
    digitalWrite(PINS[i], LOW);
  }
  
  stopMotors();
  Serial.println("READY");
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
      executeSequence(cmd);
    }
  }
}

void executeSequence(String cmd) {
  if (!cmd.startsWith("SEQ:")) {
    Serial.println("ERR:FORMAT");
    return;
  }
  
  String sequence = cmd.substring(4);
  
  int startIndex = 0;
  int endIndex = sequence.indexOf(';');
  
  while (endIndex != -1) {
    String step = sequence.substring(startIndex, endIndex);
    executeStep(step);
    startIndex = endIndex + 1;
    endIndex = sequence.indexOf(';', startIndex);
  }
  
  if (startIndex < sequence.length()) {
    String step = sequence.substring(startIndex);
    executeStep(step);
  }
  
  stopMotors(); // Гарантированный стоп после выполнения последовательности
  Serial.println("DONE");
}

void executeStep(String step) {
  int firstColon = step.indexOf(':');
  if (firstColon == -1) return;
  
  String action = step.substring(0, firstColon);
  String rest = step.substring(firstColon + 1);
  
  if (action == "MOTOR") {
    executeMotor(rest);
  } else if (action == "SPEED") {
    currentSpeedPercent = constrain(rest.toInt(), 0, 100);
  } else if (action == "ON" || action == "OFF") {
    executeLED(action, rest);
  } else if (action == "BLINK") {
    executeBlink(rest);
  }
}

void executeMotor(String params) {
  int colon = params.indexOf(':');
  if (colon == -1) return;
  
  String direction = params.substring(0, colon);
  int duration = params.substring(colon + 1).toInt();
  
  int pwmValue = map(currentSpeedPercent, 0, 100, 0, 255);
  
  if (direction == "FORWARD") {
    setMotor1(true, pwmValue);
    setMotor2(true, pwmValue);
  } else if (direction == "BACKWARD") {
    setMotor1(false, pwmValue);
    setMotor2(false, pwmValue);
  } else if (direction == "LEFT") {
    // Плавный поворот: левый мотор стоп, правый вперёд
    setMotor1(false, 0);
    setMotor2(true, pwmValue);
  } else if (direction == "RIGHT") {
    // Плавный поворот: правый мотор стоп, левый вперёд
    setMotor1(true, pwmValue);
    setMotor2(false, 0);
  } else if (direction == "SPIN_LEFT") {
    // Разворот на месте: левый назад, правый вперёд
    setMotor1(false, pwmValue);
    setMotor2(true, pwmValue);
  } else if (direction == "SPIN_RIGHT") {
    // Разворот на месте: правый назад, левый вперёд
    setMotor1(true, pwmValue);
    setMotor2(false, pwmValue);
  } else if (direction == "STOP") {
    stopMotors();
  }
  
  if (duration > 0) {
    delay(duration);
    stopMotors();
  }
}

// ==================== УПРАВЛЕНИЕ МОТОРАМИ ====================

// Универсальная функция для одного мотора
// forward: true = вперёд, false = назад
// speed: 0-255 (0 = стоп)
void setMotor1(bool forward, int speed) {
  if (speed == 0) {
    analogWrite(M1_Speed, 0);
  } else {
    digitalWrite(M1_dir, forward ? HIGH : LOW);
    analogWrite(M1_Speed, speed);
  }
}

void setMotor2(bool forward, int speed) {
  if (speed == 0) {
    analogWrite(M2_Speed, 0);
  } else {
    digitalWrite(M2_dir, forward ? HIGH : LOW);
    analogWrite(M2_Speed, speed);
  }
}

void stopMotors() {
  analogWrite(M1_Speed, 0);
  analogWrite(M2_Speed, 0);
}

// ==================== УПРАВЛЕНИЕ СВЕТОДИОДАМИ ====================

void executeLED(String action, String params) {
  int colon = params.indexOf(':');
  if (colon == -1) return;
  
  String pinsStr = params.substring(0, colon);
  int duration = params.substring(colon + 1).toInt();
  
  int pins[NUM_PINS];
  int count = parsePins(pinsStr, pins);
  
  bool state = (action == "ON") ? HIGH : LOW;
  for (int i = 0; i < count; i++) {
    digitalWrite(pins[i], state);
  }
  
  if (duration > 0) {
    delay(duration);
  }
}

void executeBlink(String params) {
  int c1 = params.indexOf(':');
  int c2 = params.indexOf(':', c1 + 1);
  if (c1 == -1 || c2 == -1) return;
  
  String pinsStr = params.substring(0, c1);
  int interval = params.substring(c1 + 1, c2).toInt();
  int times = params.substring(c2 + 1).toInt();
  
  int pins[NUM_PINS];
  int count = parsePins(pinsStr, pins);
  
  for (int t = 0; t < times; t++) {
    for (int i = 0; i < count; i++) digitalWrite(pins[i], HIGH);
    delay(interval);
    for (int i = 0; i < count; i++) digitalWrite(pins[i], LOW);
    delay(interval);
  }
}

int parsePins(String pinsStr, int* pins) {
  int count = 0;
  if (pinsStr == "ALL") {
    for (int i = 0; i < NUM_PINS; i++) pins[count++] = PINS[i];
  } else if (pinsStr == "EVEN") {
    for (int i = 0; i < NUM_PINS; i++) if (PINS[i] % 2 == 0) pins[count++] = PINS[i];
  } else if (pinsStr == "ODD") {
    for (int i = 0; i < NUM_PINS; i++) if (PINS[i] % 2 != 0) pins[count++] = PINS[i];
  } else {
    int start = 0;
    for (int i = 0; i <= pinsStr.length(); i++) {
      if (i == pinsStr.length() || pinsStr.charAt(i) == ',') {
        int pin = pinsStr.substring(start, i).toInt();
        for (int j = 0; j < NUM_PINS; j++) {
          if (PINS[j] == pin) {
            pins[count++] = pin;
            break;
          }
        }
        start = i + 1;
      }
    }
  }
  return count;
}