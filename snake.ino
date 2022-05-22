#include <LinkedList.h>
#include <charMap.h>
#include <GyverGFX.h>
#include <GyverMAX7219.h>

MAX7219 <4, 1, 5, 12, 13> matrix;

/*
* Переменная для improvedTimeDelay()
*/
volatile unsigned long globalTimeBufferMillis = 0;

/*
* Условие закончена игра или нет
*/
bool isGameOver = false;

/*
* Время появления яблок
*/
long appleTimer = 5000;
long timeFromLastApple = millis();

/*
* Пины для джойстика
*/
#define joystick_axis_X 5
#define joystick_axis_Y 6

/*
* Хвост змеи
*/
LinkedList<byte> snakeTailX;
LinkedList<byte> snakeTailY;

/*
* Все яблоки
*/
LinkedList<byte> appleX;
LinkedList<byte> appleY;

/*
* Положение головы
*/
byte x_head_position = 5;
byte y_head_position = 3;

/*
* Ускорение змеи основаннео на последнем положении джойстика
* x, -x, y, -y
*/
boolean acceleration[4] = {0, 0, 0, 0};


/*
* Инициилизируем экран, змейку и генерируем яблоки
*/
void setup() {
	Serial.begin(9600);

	matrix.begin();       
	matrix.setBright(5);  
  // matrix.setRotation(3);
	matrix.dot(x_head_position, y_head_position);

	matrix.rect(0, 0, 31, 7, 2);
	matrix.update();

  randomSeed(analogRead(8));
  generateNewApple();

}

/*
* Основной цикл
*/
void loop() {
  checkRules();
	drawScene();
  if (isGameOver) {
    startOver();
  }
  ping();

  if (appleTimer < millis() - timeFromLastApple) {
    timeFromLastApple = millis();
    generateNewApple();
  }

  stepByAacceleration();
	improvedDelay(200);
}

/*
* Обнуляем все переменные
*/
void startOver() {
  setup();
  setAxel(0, 0, 0, 0);
  x_head_position = 5;
  y_head_position = 3;

  for (int i = 0; i < appleX.size(); i++) {
    appleX.remove(i);
    appleY.remove(i);
  }

  for (int i = 0; i < snakeTailX.size(); i++) {
    snakeTailX.remove(i);
    snakeTailY.remove(i);
  }

}

/*
* Генерируем яблоки
*/
void generateNewApple() {
  appleX.add((byte)random(0, 31));
  appleY.add((byte)random(0, 7));  
}

/*
* Проверяем все правила игры
* Если координата головы змеи будет равна координате стены игры проиграна 
* Если координата головы будет равна одному из яблок, то добавляем хвост 
*/
void checkRules() {
  checkOnLose();
  checkOnAddTail();
}

void checkOnLose() {
  if (x_head_position == 0 || x_head_position == 7) {
    isGameOver = true;
    return;
  }

  if (y_head_position == 0 || y_head_position == 31) {
    isGameOver = true;
    return;
  }
  
  for (int i = 0; i < snakeTailX.size(); i++) {
    if (x_head_position == snakeTailX.get(i) && y_head_position == snakeTailY.get(i)) {
      isGameOver = true; 
      return;
    }
  }
}

/*
* Удаляем яблоко если оно было съедено и добавляем единицу к хвосту
*/
void checkOnAddTail() {
  for (int i = 0; i < appleX.size(); i++) {
    if (x_head_position == appleX.get(i) && y_head_position == appleY.get(i)) {
      appleX.remove(i);
      appleY.remove(i);
      snakeTailX.add(x_head_position);
      snakeTailY.add(y_head_position);
      return;
    }
  }

}

/*
* Отрисовка сцены 
* 
* Игра не проиграна:
* Очищаем экран
* Рисуем игровую область
* Рисуем саму змейку
* Рисуем яблоки
*
* Игра проиграна:
* Заливаем всю область
*/
void drawScene() {
  matrix.clearDisplay();
  matrix.clear();
  if (!isGameOver) {
    matrix.rect(0, 0, 31, 7, 2);

    for (int i = 0; i < snakeTailX.size(); i++) {
      matrix.dot(snakeTailX.get(i), snakeTailY.get(i));
    }

    for (int i = 0; i < appleX.size(); i++) {
      matrix.dot(appleX.get(i), appleY.get(i));
    }

  } else {
    for (int i = 0; i < 4; i++) {
      matrix.rect(0, 0, 31, 7, 1);
      matrix.update();
      improvedDelay(100);
      matrix.rect(0, 0, 31, 7, 2);
      matrix.update();
      improvedDelay(100);
    }
  }

}

/*
* Опрос джойстика
* Перемещаем хвост и голову
* Также задаём змее новое ускорение
*/
void ping()  {

  moveTail();
  if (analogRead(joystick_axis_X) > 1000) {
		matrix.dot(++x_head_position, y_head_position);
    setAxel(1, 0, 0, 0);
	}
  if (analogRead(joystick_axis_X) < 200) {
		matrix.dot(--x_head_position, y_head_position);
    setAxel(0, 1, 0, 0);
	}

  if (analogRead(joystick_axis_Y) > 1000) {
    matrix.dot(x_head_position, ++y_head_position);
    setAxel(0, 0, 1, 0);
  }

  if (analogRead(joystick_axis_Y) < 200) {
    matrix.dot(x_head_position, --y_head_position);
    setAxel(0, 0, 0, 1);
  }

}

/*
* Предпоследней единице хвоста ставим положение координаты головы
* И в обратном порядке меняем последней единице хвоста координату предпоследней и так далее
*/
void moveTail() {

  snakeTailX.set(0, x_head_position);
  snakeTailY.set(0, y_head_position);

  for (int i = snakeTailX.size(); i >= 1; i--) {
    snakeTailX.set(i, snakeTailX.get(i - 1));
    snakeTailY.set(i, snakeTailY.get(i - 1));
  }

}

/*
* Устанавливем ускорение в отдельный массив
*/
void setAxel(boolean x, boolean x_negative, boolean y, boolean y_negative) {

  acceleration[0] = x;
  acceleration[1] = x_negative;
  acceleration[2] = y;
  acceleration[3] = y_negative;

}

/*
* Двигаем змею при бездействии игрока
*/
void stepByAacceleration() {
  if (acceleration[0] == 1) {
    ++x_head_position;
  }
  if (acceleration[1] == 1) {
    --x_head_position;
  }
  if (acceleration[2] == 1) {
    ++y_head_position;
  }
  if (acceleration[3] == 1) {
    --y_head_position;
  }
}

/*
* Улучшенный метод ожидания
*/
void improvedDelay(unsigned int waitTime) {
    globalTimeBufferMillis = millis();
    boolean cooldownState = true;

    while (cooldownState) {
        if (millis() - globalTimeBufferMillis > waitTime) 
            cooldownState = false;
    }
}