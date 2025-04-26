#pragma once

#include "wled.h"

static int8_t pinUp = 26;
static int8_t pinDown = 27;
static int8_t pinPotiRight = 34;
static int8_t pinPotiLeft = 35;

static const char _data_FX_MODE_PONGGAME[] PROGMEM = "🎮 Pong ☾@!;!;!;2";


class Item {
public:
        float x;
        float y;
        uint8_t width;
        uint8_t height;
        uint32_t color = SEGCOLOR(0);

        virtual void update();
        virtual void draw() {};
};

class PongBall : public Item {
private:
        static float calc_hit_time(uint16_t max, uint16_t min, float dir, float speed, float position);
public:
        uint16_t max_x, max_y, min_x, min_y; // Playfield borders
        float dir_x;
        float dir_y;
        float speed;
        uint8_t scoreLeft, scoreRight; // TODO: Move that somewhere else

        void update(Item *racket_left, Item *racket_right);
        void move(Item *racket_left, Item *racket_right);
        void draw();
        void vec2_norm();
};


class Racket : public Item {
public:
        uint16_t max_y;
        int8_t pinPoti;
        float poti_min;
        float poti_range;
        bool is_rotation_inverted;

        void update() override;
        void draw() override;
};

class PongGame {
private:
        uint16_t vW, vH;

        PongBall ball;
        Racket racket_left, racket_right;

        // Function pointer for strategy
        uint16_t (PongGame::*currentStrategy)();
public:
        static void setupPins();
        PongGame(uint16_t vW, uint16_t vH);
        uint16_t loop();

        void playStrategySetup();
        uint16_t playStrategyLoop();

        void countDownStrategySetup();
        uint16_t countDownStrategyLoop();
        
        void finishStrategySetup(bool is_winner_left);
        uint16_t finishStrategyLoop();
};



uint16_t mode_pongGame(void) {
        // Initialization on first segment run.
        if (SEGENV.call == 0) {
                if (!SEGENV.allocateData(sizeof(PongGame))) {
                        SEGMENT.fill(SEGCOLOR(0));
                        return 350;
                }

                new (SEGENV.data) PongGame(SEGMENT.virtualWidth(), SEGMENT.virtualHeight());
        }
        PongGame *game = reinterpret_cast<PongGame*>(SEGENV.data);
        return game->loop();
}


class GamesUsermod : public Usermod {
private:
        /* configuration */
        bool enabled = false;

public:
        struct Config
        {
                static float speed;
                static const char _speed_name[];

                static uint8_t adc_averaging_count;
                static const char _adc_averaging_count_name[];

                static bool is_left_inverted;
                static const char _is_left_inverted_name[];

                static bool is_right_inverted;
                static const char _is_right_inverted_name[];

                static uint8_t win_count;
                static const char _win_count_name[];

        };

        GamesUsermod(const char *name, bool enabled):Usermod(name, enabled) {} //WLEDMM: this shouldn't be necessary (passthrough of constructor), maybe because Usermod is an abstract class

        void setup();
        void addToConfig(JsonObject& root);
        bool readFromConfig(JsonObject& root);
        void appendConfigData();
        uint16_t getId();

        // empty
        void handleOverlayDraw(){}
        void connected() {}
        void loop() {}
};

//==============================================================================
// This would be in a CPP file :(
//==============================================================================

const char GamesUsermod::Config::_speed_name[]                     PROGMEM = "speed_f";
const char GamesUsermod::Config::_adc_averaging_count_name[]       PROGMEM = "AdcAveragingCount_u8";
const char GamesUsermod::Config::_is_right_inverted_name[]         PROGMEM = "isRightInverted_b";
const char GamesUsermod::Config::_is_left_inverted_name[]          PROGMEM = "isLeftInverted_b";
const char GamesUsermod::Config::_win_count_name[]                 PROGMEM = "WinCount_u8";


float GamesUsermod::Config::speed = 1;
uint8_t GamesUsermod::Config::adc_averaging_count = 5;
bool GamesUsermod::Config::is_left_inverted = false;
bool GamesUsermod::Config::is_right_inverted = false;
uint8_t GamesUsermod::Config::win_count = 20;


void PongGame::setupPins()
{
        pinMode(pinUp, INPUT_PULLUP);
        pinMode(pinDown, INPUT_PULLUP);
        analogSetAttenuation(ADC_11db);
}

void Item::update()
{
        color = SEGCOLOR(0);
}

float PongBall::calc_hit_time(uint16_t max, uint16_t min, float dir, float speed, float position)
{
        float hit_time = 999999999999999999;
        if (dir > 0)
                hit_time = ((float)max - position) / (dir * speed);
        else if (dir < 0)
                hit_time = (position - (float)min) / (-1.0f * dir * speed);
        return hit_time;
}

void PongBall::update(Item *racket_left, Item *racket_right)
{
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

        speed = GamesUsermod::Config::speed;

        int i = 0;
        while (speed > 0) {
                move(racket_left, racket_right);
                if (i++ > 4) {
                        DEBUG_PRINT("Paniced @speed: ");
                        DEBUG_PRINTLN(speed) ;
                        break;
                }
        }
}

void PongBall::move(Item *racket_left, Item *racket_right)
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
void PongBall::draw()
{
        SEGMENT.setPixelColorXY((uint16_t)x, (uint16_t)y, color);
}


void PongBall::vec2_norm()
{
        // sets a vectors length to 1 (which means that x + y == 1)
        float length = sqrt((dir_x * dir_x) + (dir_y * dir_y));
        if (length != 0.0f) {
                length = 1.0f / length;
                dir_x *= length;
                dir_x *= length;
        }
}

void Racket::update()
{
        Item::update();

        uint32_t millis = 0;
        for (int i = 0; i < GamesUsermod::Config::adc_averaging_count; i++)
                millis += analogReadMilliVolts(pinPoti); // Liest die Spannung des Potentiometers
        float tmp = (((float) millis / GamesUsermod::Config::adc_averaging_count) - poti_min) / poti_range * (float) max_y;

        if (is_rotation_inverted)
                y = max_y - tmp;
        else
                y = tmp;

}

void Racket::draw()
{
        SEGMENT.drawLine(x, y, x, y + height-1, color);
}

PongGame::PongGame(uint16_t vW, uint16_t vH) :
        ball(),
        racket_left(),
        racket_right()
{
        this->vW = vW;
        this->vH = vH;

        countDownStrategySetup();
        currentStrategy = &PongGame::countDownStrategyLoop;

        setupPins();
}

uint16_t PongGame::loop()
{
    if (currentStrategy) {
        return (this->*currentStrategy)();
    }
    return 0; // Default/fallback behavior
}

void GamesUsermod::setup() {
        strip.addEffect(255, &mode_pongGame, _data_FX_MODE_PONGGAME);
}

void GamesUsermod::addToConfig(JsonObject& root)
{
        Usermod::addToConfig(root);
        JsonObject top = root[FPSTR(_name)];

        top[FPSTR(GamesUsermod::Config::_speed_name)]  = GamesUsermod::Config::speed;
        top[FPSTR(GamesUsermod::Config::_adc_averaging_count_name)] = GamesUsermod::Config::adc_averaging_count;
        top[FPSTR(GamesUsermod::Config::_is_left_inverted_name)] = GamesUsermod::Config::is_left_inverted;
        top[FPSTR(GamesUsermod::Config::_is_right_inverted_name)] = GamesUsermod::Config::is_right_inverted;
        top[FPSTR(GamesUsermod::Config::_win_count_name)] = GamesUsermod::Config::win_count;
}

bool GamesUsermod::readFromConfig(JsonObject& root)
{
        bool config_complete = Usermod::readFromConfig(root);
        JsonObject top = root[FPSTR(_name)];

        config_complete &= getJsonValue(top[FPSTR(GamesUsermod::Config::_speed_name)], GamesUsermod::Config::speed);
        config_complete &= getJsonValue(top[FPSTR(GamesUsermod::Config::_adc_averaging_count_name)], GamesUsermod::Config::adc_averaging_count);
        config_complete &= getJsonValue(top[FPSTR(GamesUsermod::Config::_is_left_inverted_name)], GamesUsermod::Config::is_left_inverted);
        config_complete &= getJsonValue(top[FPSTR(GamesUsermod::Config::_is_right_inverted_name)], GamesUsermod::Config::is_right_inverted);
        config_complete &= getJsonValue(top[FPSTR(GamesUsermod::Config::_win_count_name)], GamesUsermod::Config::win_count);

        return config_complete;
}

void GamesUsermod::appendConfigData()
{
        oappend(SET_F("addHB('PongGame');"));
}

uint16_t GamesUsermod::getId()
{
        return USERMOD_ID_GAMES;
}

void PongGame::playStrategySetup()
{
        ball.width = 1;
        ball.height = 1;
        ball.x = vW/2;
        ball.y = vH/2;
        ball.dir_x = -0.1;
        ball.dir_y = 0.18;
        ball.vec2_norm();
        ball.min_x = 0;
        ball.min_y = 0;
        ball.max_y = vH;
        ball.max_x = vW;

        racket_left.width = 1;
        racket_left.height = vH/4;
        racket_left.x = 0;
        racket_left.y = vH/2 - racket_left.height/2;
        racket_left.max_y = vH - racket_left.height;
        racket_left.pinPoti = pinPotiLeft;
        racket_left.poti_min = 150.0;   // Minimalwert des Potentiometers (in mV)
        racket_left.poti_range = 3100.0 - racket_left.poti_min;
        racket_left.is_rotation_inverted = GamesUsermod::Config::is_left_inverted;

        racket_right.width = 1;
        racket_right.height = vH/4;
        racket_right.x = vW - 1;
        racket_right.y = vH/2 - racket_right.height/2;
        racket_right.max_y = vH - racket_right.height;
        racket_right.pinPoti = pinPotiRight;
        racket_right.poti_min = 150.0;   // Minimalwert des Potentiometers (in mV)
        racket_right.poti_range = 3100.0 - racket_right.poti_min;
        racket_right.is_rotation_inverted = GamesUsermod::Config::is_right_inverted;
}

uint16_t PongGame::playStrategyLoop()
{
        SEGMENT.fill(BLACK);

        racket_right.update();
        racket_left.update();
        ball.update(&racket_left, &racket_right);


        racket_left.draw();
        racket_right.draw();
        ball.draw();


        for (int i=0; i<vH; i+=2) {
                SEGMENT.setPixelColorXY(vW/2, i, SEGCOLOR(0));
        }

        char tempString[5] = { '\0' };
        snprintf(tempString, 5, "%2d%2d", ball.scoreRight, ball.scoreLeft);

        uint8_t char_width = 5;
        uint8_t char_height = 8;
        SEGMENT.drawCharacter(tempString[0], vW/2-2-char_width-char_width, -2, char_width, char_height, SEGCOLOR(0));
        SEGMENT.drawCharacter(tempString[1], vW/2-2-char_width, -2, char_width, char_height, SEGCOLOR(0));
        SEGMENT.drawCharacter(tempString[2], vW/2+2, -2, char_width, char_height, SEGCOLOR(0));
        SEGMENT.drawCharacter(tempString[3], vW/2+2+char_width, -2, char_width, char_height, SEGCOLOR(0));

        if (ball.scoreRight >= GamesUsermod::Config::win_count) {
                finishStrategySetup(false);
                currentStrategy = &PongGame::finishStrategyLoop;
        }
        if (ball.scoreLeft >= GamesUsermod::Config::win_count) {
                finishStrategySetup(true);
                currentStrategy = &PongGame::finishStrategyLoop;
        }

        return FRAMETIME;
}

void PongGame::countDownStrategySetup()
{
        SEGENV.aux0 = 0;
        SEGENV.aux1 = 3;
}
uint16_t PongGame::countDownStrategyLoop()
{
        char tempString[4] = { '\0' };
        snprintf(tempString, 4, "%1d", SEGENV.aux1);

        SEGMENT.fill(BLACK);

        if (SEGENV.aux0++ > 24)
                SEGMENT.drawCharacter(tempString[0], vW/2-5, -2, 5, 8, SEGCOLOR(0));

        if (SEGENV.aux0 > 48) {
                SEGENV.aux1 -= 1;
                SEGENV.aux0 = 0;
        }

        if (SEGENV.aux1 == 0 && SEGENV.aux0 > 47) {
                playStrategySetup();
                currentStrategy = &PongGame::playStrategyLoop;
        }

        return FRAMETIME;
}

void PongGame::finishStrategySetup(bool is_winner_left)
{
        SEGMENT.fill(BLACK);
        const char line0[] = "WINNER:";
        char line1[10] = {0};
        if (is_winner_left)
                strcpy(line1, "LEFT");
        else
                strcpy(line1, "RIGHT");
        
        for (int i = 0; i < strlen(line0); i++)
                SEGMENT.drawCharacter(line0[i], 5+5*i, 0, 5, 8, SEGCOLOR(0));
        for (int i = 0; i < strlen(line1); i++)
                SEGMENT.drawCharacter(line1[i], 5+5*i, 10, 5, 8, SEGCOLOR(0));
}


uint16_t PongGame::finishStrategyLoop()
{
        return FRAMETIME;
}
