#include <Wire.h> // Библиотека для работы с I2C (может понадобиться для RTC)
#include <RTClib.h> // Библиотека для работы с RTC DS1302 (или аналогичная)
#include <TM1637Display.h> // Библиотека для управления дисплеем TM1637

// Пины для дисплея TM1637
#define CLK 2   // Пин для тактового сигнала дисплея
#define DIO 3   // Пин для данных дисплея

// Пины для кнопок
#define BUTTON_FRONT 4  // Кнопка листания вперёд
#define BUTTON_BACK 5   // Кнопка листания назад (может быть не использована)
#define BUTTON_POWER 6  // Кнопка включения/выключения устройства

// Пин для пьезо динамика
#define BUZZER_PIN 9    // Пин, подключённый к пьезо динамику

// Создаем объекты для RTC и дисплея
RTC_DS1302 rtc(7, 8, 10); // Инициализация RTC DS1302 с пинами SCLK=7, IO=8, CE=10
TM1637Display display(CLK, DIO); // Инициализация дисплея TM1637

// Перечисление состояний устройства
enum State {
  WAITING,       // Ожидание (включено, ждёт команды)
  NEAREST_BUS,   // Показывать ближайший автобус
  LOW_BATTERY    // Разряжено (низкий заряд батареи)
};

State currentState = WAITING; // Текущее состояние устройства

// Расписание автобусов в минутах от полуночи (пример)
const int schedule[] = {5, 15, 25, 35, 45}; // автобусы каждые 10 минут начиная с 5-й минуты
const int scheduleSize = sizeof(schedule) / sizeof(schedule[0]); // Размер массива расписания

// Переменные для обработки кнопок (для дебаунса)
bool prevButtonFront = LOW;   // Предыдущее состояние кнопки "вперед"
bool prevButtonBack = LOW;    // Предыдущее состояние кнопки "назад"
bool prevButtonPower = LOW;   // Предыдущее состояние кнопки питания

// Время последней обработки кнопки (для дебаунса)
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // задержка в миллисекундах

// Время последнего мигания при низком заряде батареи
unsigned long lastBlinkTime = 0;
bool isScreenOn = true; // Флаг включенности экрана при мигании

void setup() {
  Serial.begin(9600); // Инициализация последовательного порта для отладки
  
  display.setBrightness(0x0f); // Установка максимальной яркости дисплея
  
  rtc.begin(); // Инициализация RTC
  
  if (!rtc.isrunning()) { 
    // Если RTC не работает или не настроен — установить время по компилятору
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 
    // Можно установить вручную: rtc.adjust(DateTime(2023,10,1,12,0,0));
  }
  
  pinMode(BUTTON_FRONT, INPUT_PULLUP);   // Настройка кнопки "вперед" как вход с подтяжкой вверх
  pinMode(BUTTON_BACK, INPUT_PULLUP);    // Аналогично для другой кнопки
  pinMode(BUTTON_POWER, INPUT_PULLUP);   // Аналогично для кнопки питания
  
  pinMode(BUZZER_PIN, OUTPUT);           // Пин динамика как выход
  
  currentState = WAITING;                  // Начинаем в состоянии ожидания
}

void loop() {
  handleButtons();                        // Обработка нажатий кнопок
  
  switch (currentState) {
    case WAITING:
      checkPowerButton();                 // Проверяем кнопку питания в режиме ожидания
      break;
    case NEAREST_BUS:
      showNextBus();                      // Показываем ближайший автобус
      break;
    case LOW_BATTERY:
      indicateLowBattery();               // Индицируем низкий заряд батареи
      break;
    default:
      break;
  }
}

// Обработка нажатий кнопок с дебаунсом и переключением состояний
void handleButtons() {
  bool buttonFrontState = digitalRead(BUTTON_FRONT) == LOW;   // Активный низ — нажата ли кнопка "вперед"
  bool buttonBackState = digitalRead(BUTTON_BACK) == LOW;     // Аналогично для другой кнопки
  bool buttonPowerState = digitalRead(BUTTON_POWER) == LOW;   // Для кнопки питания

  unsigned long currentTime = millis();                        // Текущее время в миллисекундах

  if (buttonPowerState && !prevButtonPower && (currentTime - lastDebounceTime > debounceDelay)) {
    togglePower();                                               // Включение/выключение по нажатию кнопки питания
    lastDebounceTime = currentTime;                              // Обновляем время последнего нажатия
    prevButtonPower = true;                                      // Запоминаем состояние кнопки как нажатое
    return;                                                      // Выходим из функции — дальше не обрабатываем другие действия сейчас
  } else if (!buttonPowerState) {
    prevButtonPower = false;                                     // Обновляем состояние при отпускании кнопки
  }

  if (currentState == WAITING) {                                   // В режиме ожидания обрабатываем другие кнопки
    if (buttonFrontState && !prevButtonFront && (currentTime - lastDebounceTime > debounceDelay)) {
      currentState = NEAREST_BUS;                                  // Переключаемся на отображение ближайшего автобуса при нажатии впереди
      lastDebounceTime = currentTime;
    } else if (buttonBackState && !prevButtonBack && (currentTime - lastDebounceTime > debounceDelay)) {
      /* Можно реализовать листание по расписанию или другую функцию */
      lastDebounceTime = currentTime;
    }
    
    prevButtonFront = buttonFrontState;                            // Обновляем предыдущее состояние кнопок
    prevButtonBack = buttonBackState;

    checkBatteryLevel();                                             // Проверяем уровень заряда батареи
    
    if (currentState != LOW_BATTERY) {                               
      delay(100);                                                  // Небольшая задержка для предотвращения дребезга и быстрого повторного срабатывания
    }
    
    if (currentState == LOW_BATTERY) {                               
      indicateLowBattery();                                          // Если разряжено — мигаем экран медленно
    }
    
    checkPowerButton();                                              // Проверяем кнопку питания ещё раз при необходимости
    
}

// Проверка уровня заряда батареи или аккумулятора.
// В реальности нужен датчик уровня. Здесь условно читается аналоговый вход A0.
void checkBatteryLevel() {
   int batteryLevelPercent = analogRead(A0) /10;                     /* Предположим есть делитель напряжения и датчик */
   
   if (batteryLevelPercent <10) {                                      /* Если уровень ниже порога — считаем разряженным */
     currentState=LOW_BATTERY;                                         /* Переключаемся в режим низкого заряда */
   }
}

// Включение или выключение устройства по нажатию кнопки питания.
void togglePower() {
   static bool isOn=false;                                            /* Статическая переменная — включено ли устройство */
   isOn= !isOn;                                                        /* Переключаем состояние */

   if (!isOn) {                                                        /* Если выключили — очищаем дисплей и возвращаемся к ожиданию */
     display.clear();
     currentState=WAITING;
   } else {
     currentState=WAITING;                                             /* При включении можно сразу показывать что-то или перейти к ближайшему автобусу */
   }
}

// Проверка состояния питания и уровня заряда батареи.
void checkPowerButton() {
   /* Можно добавить логику включения/выключения по отдельной кнопке или датчику */
}

// Отображение времени до следующего автобуса.
void showNextBus() {
   DateTime now=rtc.now();                                              /* Получаем текущее время из RTC */

   int minutesNow= now.hour()*60 + now.minute();                         /* Переводим текущее время в минуты от полуночи */

   int minutesToNextBus=-1;                                              /* Изначально — неизвестно */

   for(int i=0;i<scheduleSize;i++) {                                       /* Проходим по расписанию */
     int busMinutesToday=schedule[i];                                    /* Минуты автобуса от полуночи */

     if(busMinutesToday>= minutesNow%1440){                                /* Если автобус позже текущего времени */
       minutesToNextBus= busMinutesToday - (minutesNow%1440);             /* Расчет минут до него */
       break;
     }
   }

   if(minutesToNextBus==-1){                                               /* Если все автобусы уже прошли сегодня */
     minutesToNextBus= (1440 - (minutesNow%1440)) + schedule[0];          /* Следующий — завтра в первой минуте расписания + остаток дня */
   }

   display.showNumberDec(minutesToNextBus);                                /* Отображаем минуты до прибытия автобуса */

   speakMinutes(minutesToNextBus);                                          /* Озвучиваем через динамик */
}

// Функция озвучивания времени до автобуса через динамик.
void speakMinutes(int minutes) {
   tone(BUZZER_PIN,1000);                                                  /* Воспроизводим тон частотой1000 Гц */
   delay(200);                                                             /* Звук длится около200мс */
   noTone(BUZZER_PIN);                                                     /* Останавливаем звук */

   /* Можно расширить функцию голосового оповещения или использовать модуль TTS */
}

// Индикация низкого заряда батареи: медленное мигание экрана.
void indicateLowBattery() {
   unsigned long currentMillis= millis();

   if(currentMillis - lastBlinkTime >=1000){                              /* Каждую секунду меняем состояние экрана */
     isScreenOn= !isScreenOn;                                              /* Переключаем флаг включенности экрана */
     lastBlinkTime= currentMillis;

     if(isScreenOn){
       display.setBrightness(0x0f);                                         /* Включаем яркость и показываем число или сообщение */
       display.showNumberDec(9999);                                           /* Можно показывать сообщение о низком заряде или оставить пустым */
     } else{
       display.clear();                                                     /* Выключаем подсветку/экран чтобы мигать эффектом */
     }
   }
}