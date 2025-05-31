#include <TM1637Display.h>

// Пины TM1637
#define CLK 2
#define DIO 3

// Пины устройства
#define BUTTON_PIN 4
#define BUZZER_PIN 9

TM1637Display display(CLK, DIO);

// Время и состояние
unsigned long startMillis;
unsigned long lastButtonPressMillis = 0;
unsigned long lastDisplayUpdate = 0;

const unsigned long debounceDelay = 50;      // антидребезг кнопки
const unsigned long buttonHoldDelay = 1000;  // защита от зажатия
const unsigned long timeoutDisplay = 10000;  // 10 сек до возврата на время

int buttonState = HIGH;          // текущий статус кнопки (pull-up)
int lastButtonState = HIGH;      // прошлое состояние кнопки
unsigned long lastDebounceTime = 0;

uint8_t mode = 0; // 0 - показываем время, 1 - время до ближайшего автобуса, 2 - время до следующего после ближайшего

// Параметры автобусов
const int busIntervalMin = 3; // автобусы каждые 3 минуты

// Функция для воспроизведения звука при нажатии (тихий короткий)
void beepShortQuiet() {
  tone(BUZZER_PIN, 1000, 50); // 1000 Гц, 50 мс
}

// Функция для двух средних сигналов за минуту до автобуса
void beepDoubleMedium() {
  tone(BUZZER_PIN, 1500, 200);
  delay(300);
  tone(BUZZER_PIN, 1500, 200);
}

// Вывод времени в формате MM:SS на дисплей
void displayTime(int totalSeconds) {
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;

  uint8_t data[] = {
    display.encodeDigit(minutes / 10),
    display.encodeDigit(minutes % 10) | 0x80, // точка после минут
    display.encodeDigit(seconds / 10),
    display.encodeDigit(seconds % 10)
  };
  display.setSegments(data);
}

// Возвращает сколько секунд осталось до ближайшего автобуса, учитывая текущее время с 12:00
int secondsToNextBus(unsigned long elapsedSeconds) {
  // автобусы каждые 3 минуты = 180 секунд
  int remainder = elapsedSeconds % (busIntervalMin * 60);
  if (remainder == 0) return 0;
  return (busIntervalMin * 60) - remainder;
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  display.setBrightness(0x0f); // яркость максимальная

  startMillis = millis();
  display.clear();
}

void loop() {
  unsigned long currentMillis = millis();
  unsigned long elapsedSeconds = (currentMillis - startMillis) / 1000;

  // Обработка кнопки с антидребезгом
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = currentMillis;
  }
  if ((currentMillis - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) { // кнопка нажата
        // Проверяем защиту от зажатия (не чаще 1 сек)
        if (currentMillis - lastButtonPressMillis > buttonHoldDelay) {
          lastButtonPressMillis = currentMillis;
          mode++;
          if (mode > 2) mode = 0;
          beepShortQuiet();
        }
      }
    }
  }
  lastButtonState = reading;

  // Автоматический возврат к отображению времени через 10 сек без нажатий
  if (mode != 0 && (currentMillis - lastButtonPressMillis > timeoutDisplay)) {
    mode = 0;
  }

  // Обновление дисплея каждую секунду
  if (currentMillis - lastDisplayUpdate >= 1000) {
    lastDisplayUpdate = currentMillis;

    if (mode == 0) {
      // Показываем текущее время с 12:00 в формате MM:SS
      displayTime(elapsedSeconds);
    } else if (mode == 1) {
      // Время до ближайшего автобуса
      int secToNext = secondsToNextBus(elapsedSeconds);
      displayTime(secToNext);
    } else if (mode == 2) {
      // Время до следующего после ближайшего автобуса (следующий + 3 минуты)
      int secToNext = secondsToNextBus(elapsedSeconds);
      int secToNextNext = secToNext + busIntervalMin * 60;
      displayTime(secToNextNext);
    }

    // Проверка сигнала за минуту до автобуса (только в режиме времени)
    if (mode == 0) {
      int secToNext = secondsToNextBus(elapsedSeconds);
      if (secToNext == 60) {
        beepDoubleMedium();
      }
    }
  }
}
