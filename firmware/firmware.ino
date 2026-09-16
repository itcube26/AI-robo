const int PINS[] = {22, 23, 24, 25, 26, 27};
const int NUM_PINS = 6;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(PINS[i], OUTPUT);
    digitalWrite(PINS[i], LOW);
  }
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
  // Ожидаем формат: SEQ:КОМАНДА1;КОМАНДА2;КОМАНДА3
  if (!cmd.startsWith("SEQ:")) {
    Serial.println("ERR:FORMAT");
    return;
  }
  
  String sequence = cmd.substring(4); // Убираем "SEQ:"
  
  // Разбиваем по точке с запятой
  int startIndex = 0;
  int endIndex = sequence.indexOf(';');
  
  while (endIndex != -1) {
    String step = sequence.substring(startIndex, endIndex);
    executeStep(step);
    startIndex = endIndex + 1;
    endIndex = sequence.indexOf(';', startIndex);
  }
  
  // Последний шаг (без ;)
  if (startIndex < sequence.length()) {
    String step = sequence.substring(startIndex);
    executeStep(step);
  }
  
  // Выключаем все после завершения
  allOff();
  Serial.println("DONE");
}

void executeStep(String step) {
  // Формат: ON:ПИНЫ:МС или OFF:ПИНЫ:0 или BLINK:ПИНЫ:МС:РАЗЫ
  int firstColon = step.indexOf(':');
  if (firstColon == -1) return;
  
  String action = step.substring(0, firstColon);
  String rest = step.substring(firstColon + 1);
  
  if (action == "ON" || action == "OFF") {
    int secondColon = rest.indexOf(':');
    if (secondColon == -1) return;
    
    String pinsStr = rest.substring(0, secondColon);
    int duration = rest.substring(secondColon + 1).toInt();
    
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
  else if (action == "BLINK") {
    // BLINK:ПИНЫ:МС:РАЗЫ
    int c1 = rest.indexOf(':');
    int c2 = rest.indexOf(':', c1 + 1);
    if (c1 == -1 || c2 == -1) return;
    
    String pinsStr = rest.substring(0, c1);
    int interval = rest.substring(c1 + 1, c2).toInt();
    int times = rest.substring(c2 + 1).toInt();
    
    int pins[NUM_PINS];
    int count = parsePins(pinsStr, pins);
    
    for (int t = 0; t < times; t++) {
      for (int i = 0; i < count; i++) digitalWrite(pins[i], HIGH);
      delay(interval);
      for (int i = 0; i < count; i++) digitalWrite(pins[i], LOW);
      delay(interval);
    }
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

void allOff() {
  for (int i = 0; i < NUM_PINS; i++) digitalWrite(PINS[i], LOW);
}