#pragma once

#include "wled.h"
#include "limits.h"

static int8_t pinUp = 26;
static int8_t pinDown = 27;
static int8_t pinPotiRight = 34;
static int8_t pinPotiLeft = 35;

static const char _data_FX_MODE_PONGGAME[] PROGMEM = "🎮 Pong ☾@!;!;!;2";

static float game_speed = 1.0;
static int adc_averaging_count = 1;

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

    speed = game_speed;

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
        } else {
          scoreLeft += 1;
          x = max_x/2;
          y = max_y/2;
          float minAngle = -PI / 4.0f; // -π/4
          float maxAngle = PI / 4.0f;  // +π/4
          // Generate a random float between 0 and 1
          float randUnit = random(0, 10001) / 10000.0f;
          // Scale to desired range
          float randomAngle = minAngle + (maxAngle - minAngle) * randUnit;

          dir_y = sin(randomAngle);
          dir_x = -cos(randomAngle);
          speed -= speed;
        }
      } else {
        if ((racket_left->y <= racket_hit_y) && (racket_hit_y <= (racket_left->y + racket_left->height))) {
          hit_x_time = hit_racket_time;
        } else {
          scoreRight += 1;
          x = max_x/2;
          y = max_y/2;
          float minAngle = -PI / 4.0f; // -π/4
          float maxAngle = PI / 4.0f;  // +π/4
          // Generate a random float between 0 and 1
          float randUnit = random(0, 10001) / 10000.0f;
          // Scale to desired range
          float randomAngle = minAngle + (maxAngle - minAngle) * randUnit;

          dir_y = sin(randomAngle);
          dir_x = -cos(randomAngle);

          speed -= speed;
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

    uint32_t millis = 0;
    for (int i = 0; i < adc_averaging_count; i++)
      millis += analogReadMilliVolts(pinPoti); // Liest die Spannung des Potentiometers
    float tmp = (((float) millis / adc_averaging_count) - poti_min) / poti_range * (float) max_y;
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



class GamesUsermod : public Usermod {
  private:
    /* configuration */
    bool enabled = false;

    // strings to reduce flash memory usage (used more than twice)
    static const char _speed[];
    static const char _AdcAvgCnt[];


  public:
    GamesUsermod(const char *name, bool enabled):Usermod(name, enabled) {} //WLEDMM: this shouldn't be necessary (passthrough of constructor), maybe because Usermod is an abstract class

    void setup() {
      setupPins();
      strip.addEffect(255, &mode_pongGame, _data_FX_MODE_PONGGAME);
    }

    void connected() {
    }

    void loop() {
    }

    void addToConfig(JsonObject& root)
    {
      Usermod::addToConfig(root);
      JsonObject top = root[FPSTR(_name)];

      top[FPSTR(_speed)]  = game_speed;     // usermodparam
      top[FPSTR(_AdcAvgCnt)] = adc_averaging_count;
    }

    bool readFromConfig(JsonObject& root)
    {
      Usermod::readFromConfig(root); //WLEDMM: configComplete not implemented here (todo?)
      JsonObject top = root[FPSTR(_name)];
      DEBUG_PRINT(FPSTR(_name));


      if (top.isNull()) {
        DEBUG_PRINTLN(F(": No config found. (Using defaults.)"));
        return false;
      }

      game_speed = top[FPSTR(_speed)] | game_speed;
      adc_averaging_count = top[FPSTR(_AdcAvgCnt)] | adc_averaging_count;

      return true;
    }

  void appendConfigData() {
      oappend(SET_F("addHB('PongGame');"));

      //oappend(SET_F("dd=addDropdown('staircase','selectfield');"));
      //oappend(SET_F("addOption(dd,'1st value',0);"));
      //oappend(SET_F("addOption(dd,'2nd value',1);"));
      //oappend(SET_F("addInfo('staircase:selectfield',1,'additional info');"));  // 0 is field type, 1 is actual field
    }

    void handleOverlayDraw()
    {
    }

    uint16_t getId()
    {
      return USERMOD_ID_GAMES;
    }
};

//effect functions
const char GamesUsermod::_speed[]                     PROGMEM = "speed";
const char GamesUsermod::_AdcAvgCnt[]                     PROGMEM = "AdcAveragingCount";
