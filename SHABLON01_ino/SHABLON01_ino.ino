//-слэш - разрез /слэш в гору \слэш с горы
//Маркеры памяти
//void - пустота - космос - ничего - не использует память
//int  - интеджер - целое - память для чисел 12345
//char - символы - буквы
//long - для всего
//------setup - Название
//------() - круглые скобки (SHIFT+9, SHIFT+0) настройки
//------{} - фигурные скобки (SHIFT+Х, SHIFT+Ъ) Начало и конец
//Сложных приказов

//-----------настройка модуля реального времени-------
#include <DS1302.h>

// Инициализация модуля DS1302
DS1302 rtc(6, 7, 8); // RST, DAT, CLK
//--------------------------

// способности 
// узнать время с помощью DS1302
void vrem(){
  // Чтение времени с модуля DS1302
  rtc.readTime();
  int hour = rtc.getHour();
  int minute = rtc.getMinute();
  int second = rtc.getSecond();
  // Вывод времени в монитор порта
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);
  delay(1000);  // Задержка 1 секунду

}// Инициализация модуля DS1302
DS1302 rtc(6, 7, 8); // RST, DAT, CLK

 // Настройка порта времени DS1302
  rtc.halt(false);
  rtc.writeProtect(false);
  // Установка времени
  rtc.setDOW(SATURDAY);  // Установка дня недели
  rtc.setTime(12, 30, 0);  // Установка времени (часы, минуты, секунды)
  rtc.setDate(5, 5, 2023);  // Установка даты (день, месяц, год)
}
void loop() {
  // Чтение времени с модуля DS1302
  rtc.readTime();
  int hour = rtc.getHour();
  int minute = rtc.getMinute();
  int second = rtc.getSecond();
  // Вывод времени в монитор порта
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);
  delay(1000);  // Задержка 1 секунду
}



// проверить расписание 
void rasp(){

}
// вывести на экран время автобуса
void avt(){

}

void setup() {  


}

void loop() {
  
}
