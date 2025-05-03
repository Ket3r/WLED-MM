#pragma once

#include "wled.h"

#define ADD_TO_CONFIG(cfg_name) \
        top[FPSTR(GamesUsermod::Config::_##cfg_name##_name)] = GamesUsermod::Config::cfg_name;

#define GET_FROM_CONFIG(cfg_name) \
        config_complete &= getJsonValue(top[FPSTR(GamesUsermod::Config::_##cfg_name##_name)], GamesUsermod::Config::cfg_name);

#define DEFINE_CONFIG_PARAM(name, type, defaultValue)                                \
        const char GamesUsermod::Config::_##name##_name[] PROGMEM = #name "_" #type; \
        type GamesUsermod::Config::name = defaultValue

#define DEFINE_CONFIG_STRUCT(cfg_name, type) \
        static type cfg_name; \
        static const char _##cfg_name##_name[]

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
        void score();
public:
        uint16_t max_x, max_y, min_x, min_y; // Playfield borders
        float dir_x;
        float dir_y;
        float speed;
        uint8_t scoreLeft, scoreRight; // TODO: Move that somewhere else

        void update(Item *racket_left, Item *racket_right);
        void move(Item *racket_left, Item *racket_right);
        float calc_bounce(Item *racket_right, Item *racket_left);
        bool is_racket_hit(Item *racket_right, float racket_hit_y);
        void draw();
};


class Racket : public Item {
public:
        uint16_t max_y;
        int8_t pinPoti;
        float poti_min;
        float poti_range;
        bool is_rotation_inverted;
        char id;

        void update() override;
        void draw() override;

        void setupLeft();
        void setupRight();
};

class Rectangle : public Item 
{
public:
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
        
        
        Rectangle id_select_rectangle_right;
        Rectangle id_select_rectangle_left;
        void idSelectSetup();
        uint16_t idSelectLoop();

        void countDownStrategySetup();
        uint16_t countDownStrategyLoop();

        float playStrategyCurrentSpeed;
        void playStrategySetup();
        uint16_t playStrategyLoop();
        
        void finishStrategySetup(bool is_winner_left);
        void finishWithIDsStrategySetup(bool is_winner_left);
        uint16_t finishStrategyLoop();
};



uint16_t mode_pongGame(void) {
        // Initialization on first segment run.
        if (SEGENV.call == 0) {
                if (!SEGENV.allocateData(sizeof(PongGame))) {
                        SEGMENT.fill(SEGCOLOR(0));
                        return 350;
                }
                SEGMENT.fill(SEGCOLOR(0));
                SEGMENT.fill(SEGCOLOR(1));
                SEGMENT.fill(SEGCOLOR(2));

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
                DEFINE_CONFIG_STRUCT(speed, float);
                DEFINE_CONFIG_STRUCT(speed_increment, float);
                DEFINE_CONFIG_STRUCT(speed_multiplier, float);
                DEFINE_CONFIG_STRUCT(adc_averaging_count, uint8_t);
                DEFINE_CONFIG_STRUCT(is_left_inverted, bool);
                DEFINE_CONFIG_STRUCT(is_right_inverted, bool);
                DEFINE_CONFIG_STRUCT(win_count, uint8_t);
                DEFINE_CONFIG_STRUCT(pin_button_left, int8_t);
                DEFINE_CONFIG_STRUCT(pin_poti_left, int8_t);
                DEFINE_CONFIG_STRUCT(pin_button_right, int8_t);
                DEFINE_CONFIG_STRUCT(pin_poti_right, int8_t);
                DEFINE_CONFIG_STRUCT(is_scorer_server, bool);
                DEFINE_CONFIG_STRUCT(use_bounce_zones, bool);
                DEFINE_CONFIG_STRUCT(zone0_angle_rad, float);
                DEFINE_CONFIG_STRUCT(zone1_angle_rad, float);
                DEFINE_CONFIG_STRUCT(zone2_angle_rad, float);
                DEFINE_CONFIG_STRUCT(use_int_collision, bool);
                DEFINE_CONFIG_STRUCT(use_ids, bool);
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

DEFINE_CONFIG_PARAM(speed, float, 1.0);
DEFINE_CONFIG_PARAM(speed_increment, float, 0.05);
DEFINE_CONFIG_PARAM(speed_multiplier, float, 1.0);
DEFINE_CONFIG_PARAM(adc_averaging_count, uint8_t, 5);
DEFINE_CONFIG_PARAM(is_left_inverted, bool, false);
DEFINE_CONFIG_PARAM(is_right_inverted, bool, false);
DEFINE_CONFIG_PARAM(win_count, uint8_t, 15);
DEFINE_CONFIG_PARAM(pin_button_left, int8_t, 26);
DEFINE_CONFIG_PARAM(pin_button_right, int8_t, 27);
DEFINE_CONFIG_PARAM(pin_poti_left, int8_t, 34);
DEFINE_CONFIG_PARAM(pin_poti_right, int8_t, 35);
DEFINE_CONFIG_PARAM(is_scorer_server, bool, false);
DEFINE_CONFIG_PARAM(use_bounce_zones, bool, true);
DEFINE_CONFIG_PARAM(zone2_angle_rad, float, 0.785398);
DEFINE_CONFIG_PARAM(zone1_angle_rad, float, 0.3926991);
DEFINE_CONFIG_PARAM(zone0_angle_rad, float, 0.0);
DEFINE_CONFIG_PARAM(use_int_collision, bool, true);
DEFINE_CONFIG_PARAM(use_ids, bool, false);

void PongGame::setupPins()
{
        pinMode(GamesUsermod::Config::pin_button_left, INPUT_PULLUP);
        pinMode(GamesUsermod::Config::pin_button_right, INPUT_PULLUP);
        analogSetAttenuation(ADC_11db);
}

void Item::update()
{
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

        int i = 0;
        while (speed > 0) {
                move(racket_left, racket_right);
                if (i++ > 4) {
                        break;
                }
        }
}

void PongBall::score()
{
        if (dir_x > 0) {
                scoreLeft += 1;
                if (GamesUsermod::Config::is_scorer_server)
                        dir_x = +1;
                else
                        dir_x = -1;
        } else {
                scoreRight += 1;
                if (GamesUsermod::Config::is_scorer_server)
                        dir_x = -1;
                else
                        dir_x = +1;
        }
        x = max_x / 2;
        y = max_y / 2;
        float minAngle = -PI / 4.0f; // -π/4
        float maxAngle = PI / 4.0f;  // +π/4
        // Generate a random float between 0 and 1
        float randUnit = random(0, 10001) / 10000.0f;
        // Scale to desired range
        float randomAngle = minAngle + (maxAngle - minAngle) * randUnit;

        dir_y = tan(randomAngle);
}

void PongBall::move(Item *racket_left, Item *racket_right)
{
        // Get Time required to hit top or bottom border
        float hit_y_time = calc_hit_time(max_y, min_y, dir_y, speed, y);
        float hit_x_time = calc_hit_time(max_x, min_x, dir_x, speed, x);
        float hit_racket_time = calc_hit_time(racket_right->x-1, racket_left->x+1, dir_x, speed, x);


        // NO hit at all
        if (hit_y_time >= 1 && hit_x_time >= 1 && hit_racket_time >= 1) {
                // No hits continue traveling
                x += dir_x * speed;
                y += dir_y * speed;
                speed -= speed;
                return; // off we go
        }

        // Increase racket_hit_time by so much we'd rather hit the border
        float racket_hit_y = 0;
        if (hit_racket_time <= 1) {
                // Check if we'd hit the racket
                racket_hit_y = y + dir_y * speed * hit_racket_time;
                if (dir_x > 0 && !is_racket_hit(racket_right, racket_hit_y))
                        hit_racket_time += hit_x_time;
                if (dir_x < 0 && !is_racket_hit(racket_left, racket_hit_y))
                        hit_racket_time += hit_x_time;
                DEBUG_PRINTF("racket_hit_y %f, hit_racket_time %f\n", racket_hit_y, hit_racket_time);
        }

        // TOP/BOTTOM hit
        if (hit_y_time < hit_x_time && hit_y_time < hit_racket_time)
        {
                x += dir_x * speed * hit_y_time;
                y += dir_y * speed * hit_y_time;
                speed *= (1 - hit_y_time);
                dir_y = -1.0f * dir_y;
                return; // let's get outta here
        }

        // RACKET hit
        if (hit_x_time > hit_racket_time)
        {
                x += dir_x * speed * hit_racket_time;
                y = racket_hit_y;
                if (dir_x > 0)
                        color = racket_right->color;
                else
                        color = racket_left->color;

                if (GamesUsermod::Config::use_bounce_zones)
                {
                        dir_y = calc_bounce(racket_right, racket_left);
                } else {
                        dir_y = dir_y;
                }

                speed *= (1 - hit_racket_time);
                dir_x = -1.0f * dir_x;
        // left/right Barrier hit
        } else {
                x += dir_x * speed * hit_x_time;
                y += dir_y * speed * hit_x_time;
                speed -= speed;
                DEBUG_PRINTF("x %f y %f dir_x %f dir_y %f\n", x, y, dir_x, dir_y);
                score();
        }
}
float PongBall::calc_bounce(Item *racket_right, Item *racket_left)
{
        float zone;
        Item *racket;
        if (dir_x > 0)
                racket = racket_right;
        else
                racket = racket_left;

        if (GamesUsermod::Config::use_int_collision)
                zone = (int) y - (int) racket->y;
        else
                zone = y - racket->y;

        if (zone < 1.0f)
                return -tan(GamesUsermod::Config::zone2_angle_rad) * abs(dir_x);
        else if (zone < 2.0f)
                return -tan(GamesUsermod::Config::zone1_angle_rad) * abs(dir_x);
        else if (zone < 3.0f)
                return +tan(GamesUsermod::Config::zone0_angle_rad) * abs(dir_x);
        else if (zone < 4.0f)
                return +tan(GamesUsermod::Config::zone1_angle_rad) * abs(dir_x);
        else if (zone < 5.0f)
                return +tan(GamesUsermod::Config::zone2_angle_rad) * abs(dir_x);
}
bool PongBall::is_racket_hit(Item *racket, float racket_hit_y)
{
        DEBUG_PRINTF("racket_y %f, racket_hit_y %f, racket_y+height %f\n", racket->y, racket_hit_y, (racket->y + racket->height));
        if (GamesUsermod::Config::use_int_collision)
                return ((int) racket->y <= (int) racket_hit_y) && ((int) racket_hit_y <= (int) (racket->y + racket->height));
        else
                return (racket->y <= racket_hit_y) && (racket_hit_y <= (racket->y + racket->height));
}

void PongBall::draw()
{
        SEGMENT.setPixelColorXY((uint16_t)x, (uint16_t)y, color);
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

        racket_left.setupLeft();
        racket_right.setupRight();

        if (GamesUsermod::Config::use_ids) {
                idSelectSetup();
                currentStrategy = &PongGame::idSelectLoop;
        } else {
                countDownStrategySetup();
                currentStrategy = &PongGame::countDownStrategyLoop;
        }

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

        ADD_TO_CONFIG(speed);
        ADD_TO_CONFIG(speed_increment);
        ADD_TO_CONFIG(speed_multiplier);
        ADD_TO_CONFIG(adc_averaging_count);
        ADD_TO_CONFIG(is_left_inverted);
        ADD_TO_CONFIG(is_right_inverted);
        ADD_TO_CONFIG(win_count);
        ADD_TO_CONFIG(pin_button_left);
        ADD_TO_CONFIG(pin_poti_left);
        ADD_TO_CONFIG(pin_button_right);
        ADD_TO_CONFIG(pin_poti_right);
        ADD_TO_CONFIG(is_scorer_server);
        ADD_TO_CONFIG(use_bounce_zones);
        ADD_TO_CONFIG(zone0_angle_rad);
        ADD_TO_CONFIG(zone1_angle_rad);
        ADD_TO_CONFIG(zone2_angle_rad);
        ADD_TO_CONFIG(use_int_collision);
        ADD_TO_CONFIG(use_ids);
}

bool GamesUsermod::readFromConfig(JsonObject& root)
{
        bool config_complete = Usermod::readFromConfig(root);
        JsonObject top = root[FPSTR(_name)];

        GET_FROM_CONFIG(speed);
        GET_FROM_CONFIG(speed_increment);
        GET_FROM_CONFIG(speed_multiplier);
        GET_FROM_CONFIG(adc_averaging_count);
        GET_FROM_CONFIG(is_left_inverted);
        GET_FROM_CONFIG(is_right_inverted);
        GET_FROM_CONFIG(win_count);
        GET_FROM_CONFIG(pin_button_left);
        GET_FROM_CONFIG(pin_poti_left);
        GET_FROM_CONFIG(pin_button_right);
        GET_FROM_CONFIG(pin_poti_right);
        GET_FROM_CONFIG(is_scorer_server);
        GET_FROM_CONFIG(use_bounce_zones);
        GET_FROM_CONFIG(zone0_angle_rad);
        GET_FROM_CONFIG(zone1_angle_rad);
        GET_FROM_CONFIG(zone2_angle_rad);
        GET_FROM_CONFIG(use_int_collision);
        GET_FROM_CONFIG(use_ids);

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
        ball.dir_x = -1;
        ball.dir_y = 0.3;
        ball.min_x = 0;
        ball.min_y = 0;
        ball.max_y = vH;
        ball.max_x = vW;

        ball.scoreLeft = 0;
        ball.scoreRight = 0;

        racket_left.setupLeft();
        racket_right.setupRight();

        ball.color = racket_left.color;

        playStrategyCurrentSpeed = GamesUsermod::Config::speed;
}

uint16_t PongGame::playStrategyLoop()
{
        SEGMENT.fill(BLACK);

        playStrategyCurrentSpeed += GamesUsermod::Config::speed_increment;
        playStrategyCurrentSpeed *= GamesUsermod::Config::speed_multiplier;

        uint8_t left_score = ball.scoreLeft;
        uint8_t right_score = ball.scoreRight;
        ball.speed = playStrategyCurrentSpeed;

        racket_right.update();
        racket_left.update();
        ball.update(&racket_left, &racket_right);


        racket_left.draw();
        racket_right.draw();
        ball.draw();


        for (int i=0; i<vH; i+=2) {
                SEGMENT.setPixelColorXY(vW/2, i, SEGCOLOR(2));
        }

        char tempString[5] = { '\0' };
        snprintf(tempString, 5, "%2d%2d", ball.scoreLeft, ball.scoreRight);

        uint8_t char_width = 5;
        uint8_t char_height = 8;
        SEGMENT.drawCharacter(tempString[0], vW/2-2-char_width-char_width, -2, char_width, char_height, SEGCOLOR(0));
        SEGMENT.drawCharacter(tempString[1], vW/2-2-char_width, -2, char_width, char_height, SEGCOLOR(0));
        SEGMENT.drawCharacter(tempString[2], vW/2+2, -2, char_width, char_height, SEGCOLOR(0));
        SEGMENT.drawCharacter(tempString[3], vW/2+2+char_width, -2, char_width, char_height, SEGCOLOR(0));

        if (ball.scoreLeft != left_score || ball.scoreRight != right_score)
                playStrategyCurrentSpeed = GamesUsermod::Config::speed;  // Reset speed

        if (ball.scoreRight >= GamesUsermod::Config::win_count) {
                if (GamesUsermod::Config::use_ids)
                        finishWithIDsStrategySetup(false);
                else
                        finishStrategySetup(false);
                currentStrategy = &PongGame::finishStrategyLoop;
        }
        if (ball.scoreLeft >= GamesUsermod::Config::win_count) {
                if (GamesUsermod::Config::use_ids)
                        finishWithIDsStrategySetup(true);
                else
                        finishStrategySetup(true);
                currentStrategy = &PongGame::finishStrategyLoop;
        }

        return FRAMETIME;
}

void PongGame::countDownStrategySetup()
{
        racket_left.setupLeft();
        racket_right.setupRight();
        SEGENV.aux0 = 0;
        SEGENV.aux1 = 3;
}
uint16_t PongGame::countDownStrategyLoop()
{
        char tempString[4] = { '\0' };
        snprintf(tempString, 4, "%1d", SEGENV.aux1);

        SEGMENT.fill(BLACK);

        if (SEGENV.aux0++ > (FRAMETIME/2))
                SEGMENT.drawCharacter(tempString[0], vW/2-3, vH/2-4, 6, 8, SEGCOLOR(0));

        if (SEGENV.aux0 > FRAMETIME) {
                SEGENV.aux1 -= 1;
                SEGENV.aux0 = 0;
        }

        if (SEGENV.aux1 == 0 && SEGENV.aux0 > (FRAMETIME-1)) {
                playStrategySetup();
                currentStrategy = &PongGame::playStrategyLoop;
        }

        racket_left.update();
        racket_left.draw();
        racket_right.update();
        racket_right.draw();

        return FRAMETIME;
}


void PongGame::finishWithIDsStrategySetup(bool is_winner_left)
{
        SEGMENT.fill(BLACK);
        char line0[] = "X WINS!";
        DEBUG_PRINTLN(line0);
        DEBUG_PRINTF("left %c right %c\n", racket_left.id, racket_right.id);
        if (is_winner_left)
                line0[0] = racket_left.id;
        else
                line0[0] = racket_right.id;
        
        for (int i = 0; i < strlen(line0); i++)
                SEGMENT.drawCharacter(line0[i], 5+5*i, 5, 5, 8, SEGCOLOR(0));
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
        if (LOW == digitalRead(GamesUsermod::Config::pin_button_left) || LOW == digitalRead(GamesUsermod::Config::pin_button_right)) {
                if (GamesUsermod::Config::use_ids)
                {
                        idSelectSetup();
                        currentStrategy = &PongGame::idSelectLoop;
                }
                else
                {
                        countDownStrategySetup();
                        currentStrategy = &PongGame::countDownStrategyLoop;
                }
        }
        return FRAMETIME;
}

void Racket::setupLeft()
{
        uint16_t vH= SEGMENT.virtualHeight();
        width = 1;
        height = 5;
        x = 0;
        y = vH/2 - height/2;
        max_y = vH - height;
        pinPoti = GamesUsermod::Config::pin_poti_left;
        poti_min = 150.0;   // Minimalwert des Potentiometers (in mV)
        poti_range = 3000.0 - poti_min;
        is_rotation_inverted = GamesUsermod::Config::is_left_inverted;
        color = SEGCOLOR(0);
}
void Racket::setupRight()
{
        uint16_t vH= SEGMENT.virtualHeight();
        uint16_t vW= SEGMENT.virtualWidth();
        width = 1;
        height = 5;
        x = vW - 1;
        y = vH/2 - height/2;
        max_y = vH - height;
        pinPoti = GamesUsermod::Config::pin_poti_right;
        poti_min = 150.0;   // Minimalwert des Potentiometers (in mV)
        poti_range = 3100.0 - poti_min;
        is_rotation_inverted = GamesUsermod::Config::is_right_inverted;
        color = SEGCOLOR(1);
}

void Rectangle::draw()
{
        SEGMENT.drawLine(x, y, x+width, y, color); //right
        SEGMENT.drawLine(x+width, y, x+width, y+height, color); // up
        SEGMENT.drawLine(x+width, y+height, x, y+height, color); // left
        SEGMENT.drawLine(x, y+height, x, y, color);
}

void PongGame::idSelectSetup()
{
        racket_left.id = 0;
        racket_right.id = 0;

        id_select_rectangle_right.color = SEGCOLOR(0);
        id_select_rectangle_right.width = 8;
        id_select_rectangle_right.height = 10;
        id_select_rectangle_right.x = vW*3/4 - 8/2;
        id_select_rectangle_right.y = vH/2 - 10/2;


        id_select_rectangle_left.color = SEGCOLOR(0);
        id_select_rectangle_left.width = 8;
        id_select_rectangle_left.height = 10;
        id_select_rectangle_left.x = vW/4 - 8/2;
        id_select_rectangle_left.y = vH/2 - 10/2;
}

static float getPotiPercentage(int16_t pin)
{
        uint32_t total_millis = 0;
        for (int i = 0; i < GamesUsermod::Config::adc_averaging_count; i++)
        {
                total_millis += analogReadMilliVolts(pin); // Reads the potentiometer voltage
        }
        float average_voltage = (float)total_millis / GamesUsermod::Config::adc_averaging_count;
        // Assuming the potentiometer range is roughly 150mV to 3150mV (adjust if needed)
        // TODO: Create a 150mV and 3000 mV parameter/config
        float percentage = (average_voltage - 150.0) / 3000.0;
        // Clamp the percentage to the range [0.0, 1.0]
        return constrain(percentage, 0.0, 1.0);
}

static char doIdSelection(uint16_t vH, uint16_t select_y, uint16_t char_x, float potiPercentage)
{
        uint8_t char_range = 'Z' - 'A' + 1; // Total number of characters (26)
        int16_t char_height = 10;               // Assuming character height is 8 pixels

        // Calculate the vertical offset based on the potentiometer percentage
        int16_t current_offset = static_cast<int16_t>(char_range * char_height * potiPercentage);


        // Determine the index of the character at the center of the scroll area
        int selected_char_index = current_offset / char_height;
        // Ensure the index stays within the valid range
        selected_char_index = constrain(selected_char_index, 0, char_range - 1);

        //---

        // Define how many characters to draw above and below the selection area
        int num_visible_chars = 5; // Adjust as needed
        char selected_char = 0;
        for (int i = 0; i < num_visible_chars; ++i)
        {
                int char_index_to_draw = selected_char_index + (i - num_visible_chars / 2);
                if (char_index_to_draw >= 0 && char_index_to_draw < char_range)
                {
                        char char_to_draw = 'A' + char_index_to_draw;
                        int16_t char_y = vH/2 + (i - num_visible_chars / 2) * char_height - (current_offset % char_height);
                        if (char_y == select_y) {
                                selected_char = char_to_draw;
                                SEGMENT.drawCharacter(char_to_draw, char_x, char_y-1, 6, 8, SEGCOLOR(1));
                        } else {
                                SEGMENT.drawCharacter(char_to_draw, char_x, char_y-1, 6, 8, SEGCOLOR(0));
                        }
                }
        }
        return selected_char;

}

uint16_t PongGame::idSelectLoop()
{
        SEGMENT.fill(BLACK);

        static float percentageLeft, percentageRight;
        if (racket_left.id == 0)
                percentageLeft = getPotiPercentage(GamesUsermod::Config::pin_poti_left);
        if (racket_right.id == 0)
                percentageRight = getPotiPercentage(GamesUsermod::Config::pin_poti_right);

        char leftId = doIdSelection(vH, id_select_rectangle_right.y + 3, vW/4-3, percentageLeft);
        char rightId = doIdSelection(vH, id_select_rectangle_right.y + 3, vW*3/4-3, percentageRight);

        // Optionally draw a visual indicator for the selection area
        id_select_rectangle_right.draw();
        id_select_rectangle_left.draw();

        if (LOW == digitalRead(GamesUsermod::Config::pin_button_left) && leftId != 0) {
                racket_left.id = leftId;
                DEBUG_PRINTF("Selected char left is '%c'\n", racket_left.id);
        }
        if (LOW == digitalRead(GamesUsermod::Config::pin_button_right) && rightId != 0) {
                racket_right.id = rightId;
                DEBUG_PRINTF("Selected char left is '%c'\n", racket_right.id);
        }

        if (racket_left.id != 0 && racket_right.id != 0) {
                countDownStrategySetup();
                currentStrategy = &PongGame::countDownStrategyLoop;
        }

        return FRAMETIME;
}