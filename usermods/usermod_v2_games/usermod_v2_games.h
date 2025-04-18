#pragma once

#include "wled.h"
#include "limits.h"

static int8_t pinUp = 26;
static int8_t pinDown = 27;
static int8_t pinPotiRight = 34;
static int8_t pinPotiLeft = 35;


static void setupPins()
{
  pinMode(pinUp, INPUT_PULLUP);
  pinMode(pinDown, INPUT_PULLUP);
  analogSetAttenuation(ADC_11db);
}

class Item {
public:
  float x;
  float y;
  uint8_t width;
  uint8_t height;
  uint32_t color = SEGCOLOR(0);

  virtual void update()
  {
    color = SEGCOLOR(0);
  }
  virtual void draw() {};
};

float calc_hit_time(uint16_t max, uint16_t min, float dir, float speed, float position)
{
    float hit_time = 999999999999999999;
    if (dir > 0)
      hit_time = ((float)max - position) / (dir * speed);
    else if (dir < 0)
      hit_time = (position - (float)min) / (-1.0f * dir * speed);
    return hit_time;
}

class PongBall : public Item {
public:
  uint16_t max_x, max_y, min_x, min_y; // Playfield borders
  float dir_x;
  float dir_y;
  float speed;
  uint8_t scoreLeft, scoreRight; // TODO: Move that somewhere else

  void update(Item *racket_left, Item *racket_right) {
    Item::update();
    DEBUG_PRINT("x ");
    DEBUG_PRINTLN(x);
    DEBUG_PRINT("y ");
    DEBUG_PRINTLN(y);
    DEBUG_PRINT("speed ");
    DEBUG_PRINTLN(speed);
    DEBUG_PRINT("dir_x ");
    DEBUG_PRINTLN(dir_x);
    DEBUG_PRINT("dir_y ");
    DEBUG_PRINTLN(dir_y);

    speed = 1.0f;

    int i = 0;
    while (speed > 0)
    {
      move(racket_left, racket_right);
      if (i++ > 4)
        {
          DEBUG_PRINT("Paniced @speed: ");
           DEBUG_PRINTLN(speed) ;
          break;
        }
    }
  }

  void move(Item *racket_left, Item *racket_right)
  {
    // Get Time required to hit top or bottom border
    float hit_y_time = calc_hit_time(max_y, min_y, dir_y, speed, y);
    float hit_x_time = calc_hit_time(max_x, min_x, dir_x, speed, x);
    float hit_racket_time = calc_hit_time(racket_right->x, racket_left->x, dir_x, speed, x);
    DEBUG_PRINT("hit_times: ");
    DEBUG_PRINTLN(hit_y_time);
    DEBUG_PRINTLN(hit_x_time);
    DEBUG_PRINTLN(hit_racket_time);


    // replace hit_x_time with racket_time if racket would be actually hit.
    if (hit_racket_time <= 1) {
      // Racket would be hit at hight:
      float racket_hit_y = y + dir_y * speed * hit_racket_time;
      if (dir_x > 0) {
        if ((racket_right->y <= racket_hit_y) && (racket_hit_y <= (racket_right->y + racket_right->height))) {
          hit_x_time = hit_racket_time;
        }
      } else {
        if ((racket_left->y <= racket_hit_y) && (racket_hit_y <= (racket_left->y + racket_left->height))) {
          hit_x_time = hit_racket_time;
        }
      }
    }

    if (hit_y_time > 1 && hit_x_time > 1 && hit_racket_time > 1) {
      // No hits continue traveling
      x += dir_x * speed;
      y += dir_y * speed;
      speed -= speed;
      DEBUG_PRINTLN("Normal travel");
    } else {
      if (hit_y_time < hit_x_time) {
        x += dir_x * speed * hit_y_time;
        y += dir_y * speed * hit_y_time;
        speed *= (1 - hit_y_time);
        dir_y = -1.0f * dir_y;
        DEBUG_PRINT("New DIR_Y: ");
        DEBUG_PRINTLN(dir_y);
      } else {
        x += dir_x * speed * hit_x_time;
        y += dir_y * speed * hit_x_time;
        speed *= (1 - hit_x_time);
        dir_x = -1.0f * dir_x;
        DEBUG_PRINT("New DIR_X: ");
        DEBUG_PRINTLN(dir_x);
      }
    }
  }

  void draw() override 
  {
    SEGMENT.setPixelColorXY((uint16_t)x, (uint16_t)y, color);
  }

  void vec2_norm() {
    // sets a vectors length to 1 (which means that x + y == 1)
    float length = sqrt((dir_x * dir_x) + (dir_y * dir_y));
    if (length != 0.0f) {
        length = 1.0f / length;
        dir_x *= length;
        dir_x *= length;
    }
  }

  void hit() {
    // hit left wall?
    if (x <= 0) {
        dir_x = fabs(dir_x); // force it to be positive
        // scoreLeft++;
        // if (scoreLeft>9) scoreLeft = 0;
    }
    // hit right wall?
    if (x + width-1 >= SEGMENT.virtualWidth()-1) {
        dir_x = -fabs(dir_x); // force it to be negative
        // scoreRight++;
        // if (scoreRight>9) scoreRight = 0;
    }
    // hit top wall?
    if (y <= 0) {
        dir_y = fabs(dir_y); // force it to be positive
    }
    // hit bottom wall? 
    if (y + height-1 >= SEGMENT.virtualHeight()-1) {
        dir_y = -fabs(dir_y); // force it to be negative
    }
  }

  bool hit(Item *other) {
    if (x < other->x + other->width && 
      x >= other->x &&
      y < other->y + other->height &&
      y >= other->y) {
      // set fly direction depending on where it hit the racket
      // (t is 0.5 if hit at top, 0 at center, -0.5 at bottom)
      float t = ((y - other->y) / other->height) - 0.5f;
      dir_x = fabs(dir_x); // force it to be positive
      dir_y = t;
      return true;
    }
    else
      return false;
  }
};


class Racket : public Item {
public:
  uint16_t max_y;
  int8_t pinPoti;
  float poti_min;
  float poti_range;
  bool is_rotation_inverted;

  void update() override 
  {
    Item::update();

    uint32_t millis = analogReadMilliVolts(pinPoti); // Liest die Spannung des Potentiometers
    float tmp = (millis - poti_min) / poti_range * (float) max_y;
    if (is_rotation_inverted)
      y = max_y - tmp;
    else
      y = tmp;

  }

  void draw() override 
  {
    SEGMENT.drawLine(x, y, x, y + height-1, color);
  }
};


//effect functions
uint16_t mode_pongGame(void) { 

  uint16_t dataSize = 1 * sizeof(PongBall) + 2 * sizeof(Racket);
  if (!SEGENV.allocateData(dataSize)) {SEGMENT.fill(SEGCOLOR(0)); return 350;} //allocation failed

  PongBall* ball = reinterpret_cast<PongBall*>(SEGENV.data);
  Racket* racket_left = reinterpret_cast<Racket*>(SEGENV.data + sizeof(PongBall));
  Racket* racket_right = reinterpret_cast<Racket*>(SEGENV.data + sizeof(Racket) + sizeof(PongBall));

  uint16_t vW = SEGMENT.virtualWidth();
  uint16_t vH = SEGMENT.virtualHeight();

  if (SEGENV.call == 0) {
    new (ball) PongBall();
    new (racket_left) Racket();
    new (racket_right) Racket();

    ball->width = 1;
    ball->height = 1;
    ball->x = vW/2;
    ball->y = vH/2;
    ball->dir_x = -0.1;
    ball->dir_y = 0.18;
    ball->vec2_norm();
    ball->min_x = 0;
    ball->min_y = 0;
    ball->max_y = vH;
    ball->max_x = vW;

    racket_left->width = 1;
    racket_left->height = vH/4;
    racket_left->x = 0;
    racket_left->y = vH/2 - racket_left->height/2;
    racket_left->max_y = vH - racket_left->height;
    racket_left->pinPoti = pinPotiLeft;
    racket_left->poti_min = 150.0;   // Minimalwert des Potentiometers (in mV)
    racket_left->poti_range = 3100.0 - racket_left->poti_min;
    racket_left->is_rotation_inverted = true;

    racket_right->width = 1;
    racket_right->height = vH/4;
    racket_right->x = vW - 1;
    racket_right->y = vH/2 - racket_right->height/2;
    racket_right->max_y = vH - racket_right->height;
    racket_right->pinPoti = pinPotiRight;
    racket_right->poti_min = 150.0;   // Minimalwert des Potentiometers (in mV)
    racket_right->poti_range = 3100.0 - racket_right->poti_min;
    racket_right->is_rotation_inverted = false;


    setupPins();
  }


  SEGMENT.fill(BLACK);

  racket_right->update();
  racket_left->update();
  ball->update(racket_left, racket_right);


  racket_left->draw();
  racket_right->draw();
  ball->draw();


  for (int i=0; i<vH; i+=2) {
    SEGMENT.setPixelColorXY(vW/2, i, SEGCOLOR(0));
  }

  char tempString[4] = { '\0' };
  snprintf(tempString, 4, "%1d%1d", ball->scoreRight, ball->scoreLeft);
  SEGMENT.drawCharacter(tempString[0], vW/2-5, -2, 5, 8, SEGCOLOR(0));
  SEGMENT.drawCharacter(tempString[1], vW/2+2, -2, 5, 8, SEGCOLOR(0));

  return FRAMETIME;
}

static const char _data_FX_MODE_PONGGAME[] PROGMEM = "🎮 Pong ☾@!;!;!;2";

class GamesUsermod : public Usermod {
  private:
    // strings to reduce flash memory usage (used more than twice)
    static const char _name[];

  public:

    void setup() {
      setupPins();
      strip.addEffect(255, &mode_pongGame, _data_FX_MODE_PONGGAME);
    }

    void connected() {
    }

    void loop() {
    }

    void addToJsonState(JsonObject& root)
    {
      //root["user0"] = userVar0;
    }

    void readFromJsonState(JsonObject& root)
    {
      //userVar0 = root["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
    }

    void addToConfig(JsonObject& root)
    {
      JsonObject top = root[FPSTR(_name)];
      if (top.isNull()) {
        top = root.createNestedObject(FPSTR(_name));
      }
    }

    bool readFromConfig(JsonObject& root)
    {

      JsonObject top = root[FPSTR(_name)];

      bool configComplete = !top.isNull();

      return configComplete;
    }

    void handleOverlayDraw()
    {
    }

    uint16_t getId()
    {
      return USERMOD_ID_GAMES;
    }
};

const char GamesUsermod::_name[]                      PROGMEM = "Pong Game by Uli & Pete";
