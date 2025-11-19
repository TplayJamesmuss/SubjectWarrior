#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define SCREEN_W 1280
#define SCREEN_H 720
#define FPS 60.0
#define GROUND_Y 600.0

#define PLAYER_SPEED 5.0
#define JUMP_FORCE 15.0
#define GRAVITY 0.6

#define PLAYER_SCALE 2.7f
#define PLAYER_W 110.7
#define PLAYER_H 178.2

#define MAX_FRAMES 10
#define MAX_JUMP_FRAMES 4
#define FRAME_DELAY 8

#define LEVEL_WIDTH 5760
#define CAMERA_MARGIN 400

#define MAX_ENEMIES 10
#define ENEMY_SPEED 3.0
#define ENEMY_SCALE 2.5f
#define ENEMY_W 79.0
#define ENEMY_H 35.0
#define ENEMY_FRAME_DELAY 10
#define MAX_ENEMY_FRAMES 4
#define ENEMY_SPAWN_DISTANCE 1100.0f

#define MAX_LIVES 3

#define MAX_TIPS 4
#define TIP_DISTANCE 1500.0f

#define TIP_SCALE 1.3f

#define MAX_QUESTIONS 3
#define MAX_OPTIONS 4
#define DOOR_SCALE 1.0f
#define DOOR_W 80.0f
#define DOOR_H 160.0f
#define INTERACTION_DISTANCE 100.0f

typedef enum { PROLOGUE, MENU, PLAYING, GAME_OVER, END_SCREEN, EXITING, SHOWING_TIP, DIALOG } GameState;

typedef enum { AREA_1 } AreaCurrent;

typedef struct {
    float x, y;
    bool collected;
    AreaCurrent area;
} Tip;

typedef struct {
    float x, y;
    float velocity_y;
    bool is_jumping;
    AreaCurrent area;
    int lives;
    bool invincible;
    double invincible_time;
    bool dead;
    double death_time;
    ALLEGRO_BITMAP* death_sprite;
    int tips_collected;
} Player;

typedef struct {
    float x, y;
    bool active;
    bool spawned;
    AreaCurrent area;
    ALLEGRO_BITMAP* sprites[MAX_ENEMY_FRAMES];
    int current_frame;
    int frame_counter;
    int direction;
} Enemy;

typedef struct {
    float x, y;
    float scale;
    ALLEGRO_BITMAP* normal;
    ALLEGRO_BITMAP* hover;
    bool hovered;
} Button;

typedef enum {
    SPEECH_WELCOME,
    SPEECH_PREPARED,
    SPEECH_QUESTION1,
    SPEECH_QUESTION2,
    SPEECH_QUESTION3,
    SPEECH_WRONG_ANSWER,
    SPEECH_CORRECT_ANSWER,
    SPEECH_FINAL
} SpeechType;

typedef struct {
    char question[200];
    char options[MAX_OPTIONS][50];
    int correct_answer;
} Question;

typedef struct {
    float x, y;
    bool open;
    int open_phase;
    ALLEGRO_BITMAP* sprites[4];
    SpeechType current_speech;
    int current_question;
    bool showing_dialog;
    bool waiting_answer;
    int selected_option;
    AreaCurrent area;
    int sprite_w;
    int sprite_h;
    double feedback_start_time;
} Door;

float camera_x = 0.0;

int prologue_index = 0;
double prologue_start_time = 0.0;
double game_over_start_time = 0.0;
double end_start_time = 0.0;
ALLEGRO_BITMAP* prologue_images[5] = { NULL };
ALLEGRO_BITMAP* game_over_bitmap = NULL;
ALLEGRO_BITMAP* end_bitmap = NULL;

ALLEGRO_BITMAP* area1_background = NULL;
ALLEGRO_BITMAP* ground1_bitmap = NULL;
ALLEGRO_BITMAP* play_button_normal = NULL;
ALLEGRO_BITMAP* play_button_hover = NULL;
ALLEGRO_BITMAP* mouse_sprite = NULL;
ALLEGRO_BITMAP* heart_full = NULL;
ALLEGRO_BITMAP* heart_empty = NULL;

ALLEGRO_BITMAP* tip_icon = NULL;
ALLEGRO_BITMAP* tip_images[MAX_TIPS] = { NULL };
ALLEGRO_BITMAP* tip_icon_small = NULL;
ALLEGRO_BITMAP* next_button = NULL;
ALLEGRO_BITMAP* prev_button = NULL;

ALLEGRO_BITMAP* dialog_box = NULL;
ALLEGRO_BITMAP* option_selected_bg = NULL;
ALLEGRO_BITMAP* option_normal_bg = NULL;

ALLEGRO_BITMAP* player_frames_scaled[MAX_FRAMES] = { NULL };
ALLEGRO_BITMAP* player_jump_frames_scaled[MAX_JUMP_FRAMES] = { NULL };
ALLEGRO_BITMAP* player_death_scaled = NULL;

Enemy enemies[MAX_ENEMIES];

Tip tips[MAX_TIPS];

Door door;

Question questions[MAX_QUESTIONS] = {
    {
        "Qual filo as formigas fazem parte?",
        {"Insetos", "Nematodeos", "Aracnideos", "Platelmintos"},
        0
    },
    {
        "", {"", "", "", ""}, -1
    },
    {
        "", {"", "", "", ""}, -1
    }
};

int mouse_x = 0;
int mouse_y = 0;
bool mouse_visible = true;

int current_tip_index = 0;
bool showing_tip = false;

void draw_button(Button* btn) {
    ALLEGRO_BITMAP* bitmap_to_draw = btn->hovered ? btn->hover : btn->normal;
    if (!bitmap_to_draw) bitmap_to_draw = btn->normal;

    if (bitmap_to_draw) {
        int original_width = al_get_bitmap_width(bitmap_to_draw);
        int original_height = al_get_bitmap_height(bitmap_to_draw);
        float scaled_width = original_width * btn->scale;
        float scaled_height = original_height * btn->scale;

        al_draw_scaled_bitmap(bitmap_to_draw,
            0, 0, original_width, original_height,
            btn->x, btn->y, scaled_width, scaled_height, 0);
    }
}

bool create_scaled_sprites(ALLEGRO_BITMAP* original_frames[], ALLEGRO_BITMAP* dest_frames[], int total_frames, float scale) {
    for (int i = 0; i < total_frames; i++) {
        if (!original_frames[i]) {
            return false;
        }

        int original_width = al_get_bitmap_width(original_frames[i]);
        int original_height = al_get_bitmap_height(original_frames[i]);
        int scaled_width = (int)(original_width * scale);
        int scaled_height = (int)(original_height * scale);

        ALLEGRO_BITMAP* scaled_frame = al_create_bitmap(scaled_width, scaled_height);
        if (!scaled_frame) {
            return false;
        }

        al_set_target_bitmap(scaled_frame);
        al_draw_scaled_bitmap(original_frames[i],
            0, 0, original_width, original_height,
            0, 0, scaled_width, scaled_height, 0);

        al_set_target_backbuffer(al_get_current_display());
        dest_frames[i] = scaled_frame;
    }
    return true;
}

ALLEGRO_BITMAP* create_scaled_sprite(ALLEGRO_BITMAP* original_sprite, float scale) {
    if (!original_sprite) {
        return NULL;
    }

    int original_width = al_get_bitmap_width(original_sprite);
    int original_height = al_get_bitmap_height(original_sprite);
    int scaled_width = (int)(original_width * scale);
    int scaled_height = (int)(original_height * scale);

    ALLEGRO_BITMAP* scaled_sprite = al_create_bitmap(scaled_width, scaled_height);
    if (!scaled_sprite) {
        return NULL;
    }

    al_set_target_bitmap(scaled_sprite);
    al_draw_scaled_bitmap(original_sprite,
        0, 0, original_width, original_height,
        0, 0, scaled_width, scaled_height, 0);

    al_set_target_backbuffer(al_get_current_display());
    return scaled_sprite;
}

void destroy_scaled_sprites(ALLEGRO_BITMAP* frames[], int total_frames) {
    for (int i = 0; i < total_frames; i++) {
        if (frames[i]) {
            al_destroy_bitmap(frames[i]);
            frames[i] = NULL;
        }
    }
}

float offset_y = 50.0f;

void init_enemies() {
    int spawn_count = 4;
    float start_x = 1500.0f;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (i < spawn_count) {
            enemies[i].x = start_x + (i * ENEMY_SPAWN_DISTANCE);
            enemies[i].y = GROUND_Y - ENEMY_H - offset_y;
            enemies[i].active = false;
            enemies[i].spawned = false;
            enemies[i].area = AREA_1;
            enemies[i].current_frame = 0;
            enemies[i].frame_counter = 0;
            enemies[i].direction = -1;

            for (int j = 0; j < MAX_ENEMY_FRAMES; j++) {
                char filename[50];
                sprintf_s(filename, sizeof(filename), "./assets/enemy_walk%d.png", j + 1);
                ALLEGRO_BITMAP* sprite_original = al_load_bitmap(filename);

                if (!sprite_original) {
                    enemies[i].sprites[j] = al_create_bitmap((int)ENEMY_W, (int)ENEMY_H);
                    if (enemies[i].sprites[j]) {
                        al_set_target_bitmap(enemies[i].sprites[j]);
                        al_clear_to_color(al_map_rgb(255, 0, 0));
                        al_set_target_backbuffer(al_get_current_display());
                    }
                }
                else {
                    int original_width = al_get_bitmap_width(sprite_original);
                    int original_height = al_get_bitmap_height(sprite_original);
                    int scaled_width = (int)(original_width * ENEMY_SCALE);
                    int scaled_height = (int)(original_height * ENEMY_SCALE);

                    enemies[i].sprites[j] = al_create_bitmap(scaled_width, scaled_height);
                    if (enemies[i].sprites[j]) {
                        al_set_target_bitmap(enemies[i].sprites[j]);
                        al_draw_scaled_bitmap(sprite_original,
                            0, 0, original_width, original_height,
                            0, 0, scaled_width, scaled_height, 0);
                        al_set_target_backbuffer(al_get_current_display());
                    }
                    al_destroy_bitmap(sprite_original);
                }
            }
        }
        else {
            enemies[i].x = -10000.0f;
            enemies[i].y = -10000.0f;
            enemies[i].active = false;
            enemies[i].spawned = false;
            enemies[i].area = AREA_1;
            enemies[i].current_frame = 0;
            enemies[i].frame_counter = 0;
            enemies[i].direction = -1;
            for (int j = 0; j < MAX_ENEMY_FRAMES; j++) {
                enemies[i].sprites[j] = NULL;
            }
        }
    }
}

void init_tips() {
    for (int i = 0; i < MAX_TIPS; i++) {
        tips[i].x = 800.0f + (i * TIP_DISTANCE);
        tips[i].y = GROUND_Y - 100.0f;
        tips[i].collected = false;
        tips[i].area = AREA_1;
    }
}

bool load_door() {
    for (int i = 0; i < 4; i++) {
        char filename[50];
        sprintf_s(filename, sizeof(filename), "./assets/door%d.png", i + 1);
        ALLEGRO_BITMAP* sprite_original = al_load_bitmap(filename);

        if (!sprite_original) {
            door.sprites[i] = al_create_bitmap((int)DOOR_W, (int)DOOR_H);
            if (door.sprites[i]) {
                al_set_target_bitmap(door.sprites[i]);
                al_clear_to_color(al_map_rgb(100, 50, 0));
                al_set_target_backbuffer(al_get_current_display());
            }
            if (i == 0) {
                door.sprite_w = (int)DOOR_W;
                door.sprite_h = (int)DOOR_H;
            }
            continue;
        }

        int original_width = al_get_bitmap_width(sprite_original);
        int original_height = al_get_bitmap_height(sprite_original);
        int scaled_width = (int)(original_width * DOOR_SCALE);
        int scaled_height = (int)(original_height * DOOR_SCALE);

        door.sprites[i] = al_create_bitmap(scaled_width, scaled_height);
        if (door.sprites[i]) {
            al_set_target_bitmap(door.sprites[i]);
            al_draw_scaled_bitmap(sprite_original,
                0, 0, original_width, original_height,
                0, 0, scaled_width, scaled_height, 0);
            al_set_target_backbuffer(al_get_current_display());
        }
        al_destroy_bitmap(sprite_original);

        if (i == 0) {
            door.sprite_w = scaled_width;
            door.sprite_h = scaled_height;
        }
    }

    return true;
}

void init_door() {
    if (door.sprite_w <= 0) door.sprite_w = (int)DOOR_W;
    if (door.sprite_h <= 0) door.sprite_h = (int)DOOR_H;

    door.x = LEVEL_WIDTH - door.sprite_w - 100.0f;
    door.y = GROUND_Y - door.sprite_h;
    door.open = false;
    door.open_phase = 0;
    door.current_speech = SPEECH_WELCOME;
    door.current_question = 0;
    door.showing_dialog = false;
    door.waiting_answer = false;
    door.selected_option = -1;
    door.area = AREA_1;
    door.feedback_start_time = 0.0;
}

void destroy_door() {
    for (int i = 0; i < 4; i++) {
        if (door.sprites[i]) {
            al_destroy_bitmap(door.sprites[i]);
            door.sprites[i] = NULL;
        }
    }
}

bool player_near_door(Player* player) {
    if (player->area != door.area) return false;

    float dx = fabs(player->x - door.x);
    float dy = fabs(player->y - door.y);

    return (dx < INTERACTION_DISTANCE && dy < INTERACTION_DISTANCE);
}

void update_door_opening() {
    if (door.open_phase > 0 && door.open_phase < 3) {
        static double last_update = 0.0;
        double now = al_get_time();

        if (now - last_update > 0.5) {
            door.open_phase++;
            last_update = now;

            if (door.open_phase >= 3) {
                door.open = true;
            }
        }
    }
}

void draw_door() {
    if (door.sprites[door.open_phase]) {
        al_draw_bitmap(door.sprites[door.open_phase], door.x - camera_x, door.y, 0);
    }
}

void draw_wrapped_text(ALLEGRO_FONT* font, const char* text, int x, int y, int max_width, int max_lines, int line_height, ALLEGRO_COLOR color) {
    if (!font || !text) return;

    char buffer[2048];
    strncpy_s(buffer, sizeof(buffer), text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    const char* delim = " ";
    char* context = NULL;
    char* token = NULL;
    token = strtok_s(buffer, delim, &context);
    char line[1024] = "";
    int lines_drawn = 0;
    int cur_x = x;
    int cur_y = y;

    while (token && lines_drawn < max_lines) {
        char candidate[1024];
        if (strlen(line) == 0) {
            snprintf(candidate, sizeof(candidate), "%s", token);
        }
        else {
            snprintf(candidate, sizeof(candidate), "%s %s", line, token);
        }

        int w = al_get_text_width(font, candidate);
        if (w <= max_width) {
            strncpy_s(line, sizeof(line), candidate, _TRUNCATE);
        }
        else {
            if (strlen(line) > 0) {
                al_draw_text(font, color, cur_x, cur_y, 0, line);
                lines_drawn++;
                cur_y += line_height;
                if (lines_drawn >= max_lines) break;
            }
            strncpy_s(line, sizeof(line), token, _TRUNCATE);
            line[sizeof(line) - 1] = '\0';
        }

        token = strtok_s(NULL, delim, &context);
    }

    if (lines_drawn < max_lines && strlen(line) > 0) {
        al_draw_text(font, color, cur_x, cur_y, 0, line);
    }
}

void destroy_enemy_sprites() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        for (int j = 0; j < MAX_ENEMY_FRAMES; j++) {
            if (enemies[i].sprites[j]) {
                al_destroy_bitmap(enemies[i].sprites[j]);
                enemies[i].sprites[j] = NULL;
            }
        }
    }
}

void destroy_tip_sprites() {
    if (tip_icon) {
        al_destroy_bitmap(tip_icon);
        tip_icon = NULL;
    }
    if (tip_icon_small) {
        al_destroy_bitmap(tip_icon_small);
        tip_icon_small = NULL;
    }
    if (next_button) {
        al_destroy_bitmap(next_button);
        next_button = NULL;
    }
    if (prev_button) {
        al_destroy_bitmap(prev_button);
        prev_button = NULL;
    }
    for (int i = 0; i < MAX_TIPS; i++) {
        if (tip_images[i]) {
            al_destroy_bitmap(tip_images[i]);
            tip_images[i] = NULL;
        }
    }
}

void reset_game(Player* player) {
    player->x = 100.0;
    player->y = GROUND_Y - PLAYER_H;
    player->velocity_y = 0.0;
    player->is_jumping = false;
    player->area = AREA_1;
    player->lives = MAX_LIVES;
    player->invincible = false;
    player->invincible_time = 0.0;
    player->dead = false;
    player->death_time = 0.0;
    player->tips_collected = 0;
    camera_x = 0.0;
    current_tip_index = 0;
    showing_tip = false;

    for (int i = 0; i < MAX_TIPS; i++) {
        tips[i].collected = false;
    }

    init_door();
    init_enemies();
}

void clear_keys(bool keys[]) {
    for (int i = 0; i < ALLEGRO_KEY_MAX; i++) {
        keys[i] = false;
    }
}

void update_camera(Player* player) {
    if (player->x > camera_x + SCREEN_W - CAMERA_MARGIN) {
        camera_x = player->x - (SCREEN_W - CAMERA_MARGIN);
    }

    if (player->x < camera_x + CAMERA_MARGIN) {
        camera_x = player->x - CAMERA_MARGIN;
    }

    if (camera_x < 0) {
        camera_x = 0;
    }
    if (camera_x > LEVEL_WIDTH - SCREEN_W) {
        camera_x = LEVEL_WIDTH - SCREEN_W;
    }
}

void check_area_change(Player* player) {
    if (player->x < 0) player->x = 0;
    if (player->x + PLAYER_W > LEVEL_WIDTH) player->x = LEVEL_WIDTH - PLAYER_W;
}

bool load_prologue() {
    prologue_images[0] = al_load_bitmap("./assets/prologo1.png");
    if (!prologue_images[0]) {
        return false;
    }

    prologue_images[1] = al_load_bitmap("./assets/prologo2.png");
    if (!prologue_images[1]) {
        return false;
    }

    prologue_images[2] = al_load_bitmap("./assets/prologo3.png");
    if (!prologue_images[2]) {
        return false;
    }

    prologue_images[3] = al_load_bitmap("./assets/prologo4.png");
    if (!prologue_images[3]) {
        return false;
    }

    prologue_images[4] = al_load_bitmap("./assets/prologo5.png");
    if (!prologue_images[4]) {
        return false;
    }

    return true;
}

bool load_game_over() {
    game_over_bitmap = al_load_bitmap("./assets/game_over.png");
    if (!game_over_bitmap) {
        return false;
    }

    end_bitmap = al_load_bitmap("./assets/end.png");
    if (!end_bitmap) {
        end_bitmap = NULL;
    }

    return true;
}

bool load_player_death_sprite(Player* player) {
    ALLEGRO_BITMAP* death_original = al_load_bitmap("./assets/death.png");
    if (!death_original) {
        player->death_sprite = NULL;
        return false;
    }

    player->death_sprite = create_scaled_sprite(death_original, PLAYER_SCALE);
    al_destroy_bitmap(death_original);

    if (!player->death_sprite) {
        return false;
    }

    return true;
}

bool load_areas() {
    area1_background = al_load_bitmap("./assets/phase.png");
    ground1_bitmap = al_load_bitmap("./assets/ground1.png");

    if (!area1_background || !ground1_bitmap) {
        return false;
    }
    return true;
}

bool load_buttons() {
    play_button_normal = al_load_bitmap("./assets/button_play.png");
    if (!play_button_normal) {
        return false;
    }

    play_button_hover = al_load_bitmap("./assets/button_play_hover.png");
    if (!play_button_hover) {
        play_button_hover = play_button_normal;
    }

    return true;
}

bool load_mouse_sprite() {
    mouse_sprite = al_load_bitmap("./assets/mouse.png");
    if (!mouse_sprite) {
        return false;
    }
    return true;
}

bool load_hearts_sprites() {
    heart_full = al_load_bitmap("./assets/heart1.png");
    if (!heart_full) {
        return false;
    }

    heart_empty = al_load_bitmap("./assets/heart2.png");
    if (!heart_empty) {
        return false;
    }

    return true;
}

bool load_tip_sprites() {
    tip_icon = al_load_bitmap("./assets/tip.png");
    if (!tip_icon) {
        return false;
    }

    tip_icon_small = al_load_bitmap("./assets/tip.png");
    if (!tip_icon_small) {
        tip_icon_small = tip_icon;
    }

    for (int i = 0; i < MAX_TIPS; i++) {
        char filename[50];
        sprintf_s(filename, sizeof(filename), "./assets/tip%d.png", i + 1);
        tip_images[i] = al_load_bitmap(filename);
        if (!tip_images[i]) {
            tip_images[i] = al_create_bitmap(400, 300);
            al_set_target_bitmap(tip_images[i]);
            al_clear_to_color(al_map_rgb(50, 50, 100));
            ALLEGRO_FONT* fallback_font = al_load_ttf_font("./assets/pirulen.ttf", 24, 0);
            if (fallback_font) {
                al_draw_textf(fallback_font, al_map_rgb(255, 255, 255), 200, 150, ALLEGRO_ALIGN_CENTER,
                    "Dica %d", i + 1);
                al_destroy_font(fallback_font);
            }
            al_set_target_backbuffer(al_get_current_display());
        }
    }

    next_button = al_create_bitmap(50, 50);
    al_set_target_bitmap(next_button);
    al_clear_to_color(al_map_rgba(0, 0, 0, 0));
    al_draw_filled_triangle(10, 10, 10, 40, 40, 25, al_map_rgb(255, 255, 255));
    al_set_target_backbuffer(al_get_current_display());

    prev_button = al_create_bitmap(50, 50);
    al_set_target_bitmap(prev_button);
    al_clear_to_color(al_map_rgba(0, 0, 0, 0));
    al_draw_filled_triangle(40, 10, 40, 40, 10, 25, al_map_rgb(255, 255, 255));
    al_set_target_backbuffer(al_get_current_display());

    return true;
}

void destroy_prologue() {
    for (int i = 0; i < 5; i++) {
        if (prologue_images[i]) {
            al_destroy_bitmap(prologue_images[i]);
            prologue_images[i] = NULL;
        }
    }
}

void destroy_game_over() {
    if (game_over_bitmap) {
        al_destroy_bitmap(game_over_bitmap);
        game_over_bitmap = NULL;
    }
    if (end_bitmap) {
        al_destroy_bitmap(end_bitmap);
        end_bitmap = NULL;
    }
}

void destroy_player_death_sprite(Player* player) {
    if (player->death_sprite) {
        al_destroy_bitmap(player->death_sprite);
        player->death_sprite = NULL;
    }
}

void destroy_areas() {
    if (area1_background) {
        al_destroy_bitmap(area1_background);
        area1_background = NULL;
    }
    if (ground1_bitmap) {
        al_destroy_bitmap(ground1_bitmap);
        ground1_bitmap = NULL;
    }
}

void destroy_buttons() {
    if (play_button_normal) {
        al_destroy_bitmap(play_button_normal);
        play_button_normal = NULL;
    }
    if (play_button_hover && play_button_hover != play_button_normal) {
        al_destroy_bitmap(play_button_hover);
        play_button_hover = NULL;
    }
}

void destroy_mouse_sprite() {
    if (mouse_sprite) {
        al_destroy_bitmap(mouse_sprite);
        mouse_sprite = NULL;
    }
}

void destroy_heart_sprites() {
    if (heart_full) {
        al_destroy_bitmap(heart_full);
        heart_full = NULL;
    }
    if (heart_empty) {
        al_destroy_bitmap(heart_empty);
        heart_empty = NULL;
    }
}

void advance_prologue(GameState* game_state) {
    prologue_index++;
    if (prologue_index >= 5) {
        *game_state = MENU;
        prologue_index = 0;
    }
    else {
        prologue_start_time = al_get_time();
    }
}

void draw_mouse() {
    if (mouse_visible && mouse_sprite) {
        int sprite_w = al_get_bitmap_width(mouse_sprite);
        int sprite_h = al_get_bitmap_height(mouse_sprite);
        al_draw_bitmap(mouse_sprite, mouse_x - sprite_w / 2, mouse_y - sprite_h / 2, 0);
    }
}

void draw_hearts(Player* player) {
    int heart_size = 40;
    int spacing = 10;
    int start_x = 10;
    int start_y = 50;

    for (int i = 0; i < MAX_LIVES; i++) {
        ALLEGRO_BITMAP* heart_bitmap = (i < player->lives) ? heart_full : heart_empty;
        if (heart_bitmap) {
            al_draw_scaled_bitmap(heart_bitmap,
                0, 0,
                al_get_bitmap_width(heart_bitmap),
                al_get_bitmap_height(heart_bitmap),
                start_x + i * (heart_size + spacing),
                start_y,
                heart_size,
                heart_size, 0);
        }
    }
}

void draw_tip_hud(Player* player, ALLEGRO_FONT* font) {
    int icon_size = 30;
    int start_x = SCREEN_W - 150;
    int start_y = 10;

    if (tip_icon_small) {
        al_draw_scaled_bitmap(tip_icon_small,
            0, 0,
            al_get_bitmap_width(tip_icon_small),
            al_get_bitmap_height(tip_icon_small),
            start_x, start_y,
            icon_size, icon_size, 0);
    }

    char counter_text[20];
    sprintf_s(counter_text, sizeof(counter_text), "%d/%d", player->tips_collected, MAX_TIPS);
    al_draw_text(font, al_map_rgb(255, 255, 255), start_x + icon_size + 10, start_y + 5, 0, counter_text);
}

void draw_tip_screen(Player* player, ALLEGRO_FONT* font) {
    if (showing_tip && current_tip_index < player->tips_collected) {
        al_draw_filled_rectangle(0, 0, SCREEN_W, SCREEN_H, al_map_rgba(0, 0, 0, 200));

        ALLEGRO_BITMAP* tip_current = tip_images[current_tip_index];
        if (tip_current) {
            int img_w = al_get_bitmap_width(tip_current);
            int img_h = al_get_bitmap_height(tip_current);
            float scale = TIP_SCALE;
            float scaled_w = img_w * scale;
            float scaled_h = img_h * scale;
            float x = (SCREEN_W - scaled_w) / 2.0f;
            float y = (SCREEN_H - scaled_h) / 2.0f;

            al_draw_scaled_bitmap(tip_current,
                0, 0, img_w, img_h,
                x, y, scaled_w, scaled_h, 0);

            if (player->tips_collected > 1) {
                float btn_y = y + scaled_h + 20;

                if (current_tip_index > 0 && prev_button) {
                    al_draw_bitmap(prev_button, x - 60, btn_y, 0);
                }

                if (current_tip_index < player->tips_collected - 1 && next_button) {
                    al_draw_bitmap(next_button, x + scaled_w + 10, btn_y, 0);
                }

                char page_text[20];
                sprintf_s(page_text, sizeof(page_text), "%d/%d", current_tip_index + 1, player->tips_collected);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, btn_y + 15, ALLEGRO_ALIGN_CENTER, page_text);
            }

            al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H - 50, ALLEGRO_ALIGN_CENTER, "Clique fora da dica para fechar");
        }
    }
}

void draw_tips_world() {
    for (int i = 0; i < MAX_TIPS; i++) {
        if (!tips[i].collected && tip_icon) {
            al_draw_bitmap(tip_icon, tips[i].x - camera_x, tips[i].y, 0);
        }
    }
}

bool check_collision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 &&
        x1 + w1 > x2 &&
        y1 < y2 + h2 &&
        y1 + h1 > y2);
}

void check_enemy_spawn(Player* player) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].spawned && enemies[i].area == player->area) {
            if (enemies[i].x - camera_x < SCREEN_W + 200) {
                enemies[i].active = true;
                enemies[i].spawned = true;
            }
        }
    }
}

void check_tip_collection(Player* player, GameState* game_state) {
    for (int i = 0; i < MAX_TIPS; i++) {
        if (!tips[i].collected && tips[i].area == player->area) {
            float tip_w = tip_icon ? al_get_bitmap_width(tip_icon) : 32;
            float tip_h = tip_icon ? al_get_bitmap_height(tip_icon) : 32;

            if (check_collision(player->x, player->y, PLAYER_W, PLAYER_H,
                tips[i].x, tips[i].y, tip_w, tip_h)) {
                tips[i].collected = true;
                player->tips_collected++;
                showing_tip = true;
                current_tip_index = player->tips_collected - 1;
                if (game_state) {
                    *game_state = SHOWING_TIP;
                }
                return;
            }
        }
    }
}

void update_enemies(Player* player) {
    check_enemy_spawn(player);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active && enemies[i].spawned && enemies[i].area == player->area) {

            enemies[i].x += ENEMY_SPEED * enemies[i].direction;

            if (enemies[i].x + ENEMY_W < 0) {
                enemies[i].x = LEVEL_WIDTH - ENEMY_W;
            }
            else if (enemies[i].x > LEVEL_WIDTH) {
                enemies[i].x = 0 - ENEMY_W;
            }

            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (i != j && enemies[j].active && enemies[j].spawned && enemies[j].area == player->area) {
                    if (check_collision(enemies[i].x, enemies[i].y, ENEMY_W, ENEMY_H,
                        enemies[j].x, enemies[j].y, ENEMY_W, ENEMY_H)) {
                        enemies[i].direction *= -1;
                        enemies[j].direction *= -1;
                        enemies[i].x += ENEMY_SPEED * enemies[i].direction * 2;
                        enemies[j].x += ENEMY_SPEED * enemies[j].direction * 2;
                        break;
                    }
                }
            }

            enemies[i].frame_counter++;
            if (enemies[i].frame_counter >= ENEMY_FRAME_DELAY) {
                enemies[i].current_frame = (enemies[i].current_frame + 1) % MAX_ENEMY_FRAMES;
                enemies[i].frame_counter = 0;
            }

            if (check_collision(player->x, player->y, PLAYER_W, PLAYER_H,
                enemies[i].x, enemies[i].y, ENEMY_W, ENEMY_H)) {

                if (player->velocity_y > 0 &&
                    player->y + PLAYER_H < enemies[i].y + ENEMY_H / 2) {
                    enemies[i].active = false;
                    player->velocity_y = -JUMP_FORCE / 2;
                }
                else if (!player->invincible && !player->dead) {
                    player->lives--;
                    player->invincible = true;
                    player->invincible_time = al_get_time();

                    if (player->x < enemies[i].x) {
                        player->x -= 50;
                    }
                    else {
                        player->x += 50;
                    }

                    if (player->x < 0) player->x = 0;
                    if (player->x > LEVEL_WIDTH - PLAYER_W) player->x = LEVEL_WIDTH - PLAYER_W;

                    if (player->lives <= 0) {
                        player->lives = 0;
                        player->dead = true;
                        player->death_time = al_get_time();
                    }
                }
            }
        }
    }
}

void draw_enemies(Player* player) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active && enemies[i].spawned && enemies[i].area == player->area) {
            ALLEGRO_BITMAP* sprite_current = enemies[i].sprites[enemies[i].current_frame];
            if (sprite_current) {
                int flip_flag = (enemies[i].direction == 1) ? ALLEGRO_FLIP_HORIZONTAL : 0;
                al_draw_bitmap(sprite_current, enemies[i].x - camera_x, enemies[i].y, flip_flag);
            }
        }
    }
}

bool check_click_tip_button(int mouse_x, int mouse_y, Player* player) {
    if (!showing_tip || player->tips_collected <= 1) return false;

    ALLEGRO_BITMAP* tip_current = tip_images[current_tip_index];
    if (!tip_current) return false;

    int img_w = al_get_bitmap_width(tip_current);
    int img_h = al_get_bitmap_height(tip_current);
    float scale = TIP_SCALE;
    float scaled_w = img_w * scale;
    float scaled_h = img_h * scale;
    float x = (SCREEN_W - scaled_w) / 2.0f;
    float y = (SCREEN_H - scaled_h) / 2.0f;
    float btn_y = y + scaled_h + 20;

    if (current_tip_index > 0 && mouse_x >= x - 60 && mouse_x <= x - 10 &&
        mouse_y >= btn_y && mouse_y <= btn_y + 50) {
        current_tip_index--;
        return true;
    }

    if (current_tip_index < player->tips_collected - 1 &&
        mouse_x >= x + scaled_w + 10 && mouse_x <= x + scaled_w + 60 &&
        mouse_y >= btn_y && mouse_y <= btn_y + 50) {
        current_tip_index++;
        return true;
    }

    return false;
}

bool check_click_outside_tip(int mouse_x, int mouse_y) {
    if (!showing_tip) return false;

    ALLEGRO_BITMAP* tip_current = tip_images[current_tip_index];
    if (!tip_current) return false;

    int img_w = al_get_bitmap_width(tip_current);
    int img_h = al_get_bitmap_height(tip_current);
    float scale = TIP_SCALE;
    float scaled_w = img_w * scale;
    float scaled_h = img_h * scale;
    float x = (SCREEN_W - scaled_w) / 2.0f;
    float y = (SCREEN_H - scaled_h) / 2.0f;

    return (mouse_x < x || mouse_x > x + scaled_w ||
        mouse_y < y || mouse_y > y + scaled_h);
}

int main(void) {
    al_init();
    al_install_keyboard();
    al_install_mouse();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_image_addon();

    srand((unsigned int)time(NULL));

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
    ALLEGRO_FONT* font = al_load_ttf_font("./assets/pirulen.ttf", 32, 0);
    ALLEGRO_FONT* font_small = al_load_ttf_font("./assets/pirulen.ttf", 24, 0);

    al_hide_mouse_cursor(display);

    ALLEGRO_BITMAP* player_frames_right[MAX_FRAMES] = { NULL };
    ALLEGRO_BITMAP* player_jump_frames_right[MAX_JUMP_FRAMES] = { NULL };

    int frames_loaded = 0;
    char filename[25];
    for (int i = 0; i < MAX_FRAMES; i++) {
        sprintf_s(filename, sizeof(filename), "./assets/knight%d.png", i + 1);
        player_frames_right[i] = al_load_bitmap(filename);
        if (!player_frames_right[i]) {
            frames_loaded = i;
            break;
        }
        frames_loaded++;
    }

    if (frames_loaded == 0) {
        fprintf(stderr, "Erro: Nao foi possivel carregar nenhum sprite do knight\n");
        return -1;
    }

    int jump_frames_loaded = 0;
    for (int i = 0; i < MAX_JUMP_FRAMES; i++) {
        sprintf_s(filename, sizeof(filename), "./assets/jump%d.png", i + 1);
        player_jump_frames_right[i] = al_load_bitmap(filename);
        if (!player_jump_frames_right[i]) {
            jump_frames_loaded = i;
            break;
        }
        jump_frames_loaded++;
    }

    if (!create_scaled_sprites(player_frames_right, player_frames_scaled, frames_loaded, PLAYER_SCALE)) {
        fprintf(stderr, "Erro ao criar sprites escalados\n");
        return -1;
    }

    if (jump_frames_loaded > 0) {
        if (!create_scaled_sprites(player_jump_frames_right, player_jump_frames_scaled, jump_frames_loaded, PLAYER_SCALE)) {
        }
    }

    if (!load_mouse_sprite()) {
        al_show_mouse_cursor(display);
        mouse_visible = false;
    }

    if (!load_hearts_sprites()) {
        fprintf(stderr, "Erro ao carregar sprites de vidas\n");
        return -1;
    }

    load_tip_sprites();

    load_door();
    init_door();

    load_game_over();

    ALLEGRO_BITMAP* title_logo_bitmap = al_load_bitmap("./assets/title_logo.png");
    ALLEGRO_BITMAP* menu_background_bitmap = al_load_bitmap("./assets/menu_background.png");

    if (!font || !title_logo_bitmap || !menu_background_bitmap) {
        fprintf(stderr, "Erro ao carregar recursos\n");
        return -1;
    }

    // Carrega as imagens do prólogo, mas NÃO inicia o jogo em PROLOGUE.
    bool prologue_loaded = load_prologue();
    GameState game_state = MENU;

    if (!load_areas()) {
        return -1;
    }

    load_buttons();

    Player player = {
        100.0,
        GROUND_Y - PLAYER_H,
        0.0,
        false,
        AREA_1,
        MAX_LIVES,
        false,
        0.0,
        false,
        0.0,
        NULL,
        0
    };

    load_player_death_sprite(&player);

    init_enemies();

    init_tips();

    int direction = 1;
    int current_frame = 0;
    int frame_count = 0;
    bool moving = false;

    int walking_frame_index = 1;
    int total_walking_frames = 0;

    int jump_frame_index = 0;
    bool showing_jump_animation = false;

    if (frames_loaded >= 9) {
        total_walking_frames = 8;
    }
    else {
        total_walking_frames = frames_loaded - 1;
        if (total_walking_frames < 1) total_walking_frames = 1;
    }

    float button_scale = 0.5f;
    float btn_x = SCREEN_W / 2.0f;
    float btn_y = 500.0f;
    if (play_button_normal) {
        float original_width = al_get_bitmap_width(play_button_normal);
        float scaled_width = original_width * button_scale;
        btn_x -= scaled_width / 2.0f;
    }
    else {
        btn_x -= 75.0f;
    }

    Button play_button = {
        btn_x,
        btn_y,
        button_scale,
        play_button_normal,
        play_button_hover,
        false
    };

    bool keys[ALLEGRO_KEY_MAX] = { false };
    bool redraw = true;

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());

    al_start_timer(timer);
    while (game_state != EXITING) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            game_state = EXITING;
        }

        if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
            mouse_x = event.mouse.x;
            mouse_y = event.mouse.y;
        }

        if (game_state == PROLOGUE) {
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                    // Ao pular o prólogo, inicia o jogo
                    game_state = PLAYING;
                    reset_game(&player);
                    prologue_index = 0;
                    clear_keys(keys);
                }
            }

            if (al_get_time() - prologue_start_time >= 10.0) {
                prologue_index++;
                if (prologue_index >= 5) {
                    // Ao terminar o prólogo, inicia o jogo
                    game_state = PLAYING;
                    reset_game(&player);
                    prologue_index = 0;
                    clear_keys(keys);
                }
                else {
                    prologue_start_time = al_get_time();
                }
            }
        }
        else if (game_state == MENU) {
            if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
                if (play_button_normal) {
                    float original_width = al_get_bitmap_width(play_button_normal);
                    float original_height = al_get_bitmap_height(play_button_normal);
                    float scaled_width = original_width * play_button.scale;
                    float scaled_height = original_height * play_button.scale;

                    play_button.hovered = (event.mouse.x >= play_button.x &&
                        event.mouse.x <= play_button.x + scaled_width &&
                        event.mouse.y >= play_button.y &&
                        event.mouse.y <= play_button.y + scaled_height);
                }
            }
            if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && play_button.hovered) {
                // Ao clicar Play: se o prólogo foi carregado, mostra prólogo primeiro.
                if (prologue_loaded) {
                    game_state = PROLOGUE;
                    prologue_index = 0;
                    prologue_start_time = al_get_time();
                }
                else {
                    game_state = PLAYING;
                    reset_game(&player);
                }
                clear_keys(keys);
            }
        }
        else if (game_state == PLAYING) {
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    game_state = MENU;
                    reset_game(&player);
                    clear_keys(keys);
                }
                keys[event.keyboard.keycode] = true;

                if ((event.keyboard.keycode == ALLEGRO_KEY_SPACE ||
                    event.keyboard.keycode == ALLEGRO_KEY_W) &&
                    !player.is_jumping && !player.dead) {

                    if (player_near_door(&player)) {
                        if (!door.open) {
                            door.showing_dialog = true;
                            door.waiting_answer = true;
                            door.current_question = 0;
                            door.selected_option = -1;
                            door.current_speech = SPEECH_QUESTION1;
                            door.feedback_start_time = 0.0;
                            clear_keys(keys);
                        }
                        else {
                            game_state = END_SCREEN;
                            end_start_time = al_get_time();
                            clear_keys(keys);
                        }
                    }
                    else {
                        player.velocity_y = -JUMP_FORCE;
                        player.is_jumping = true;
                        showing_jump_animation = true;
                        jump_frame_index = 0;
                    }
                }
            }
            if (event.type == ALLEGRO_EVENT_KEY_UP) {
                keys[event.keyboard.keycode] = false;
            }

            if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                if (door.showing_dialog && door.waiting_answer) {
                    int dlg_w = 700;
                    int dlg_h = 300;
                    int dlg_x = (SCREEN_W - dlg_w) / 2;
                    int dlg_y = (SCREEN_H - dlg_h) / 2;
                    int alt_x = dlg_x + 30;
                    int alt_w = dlg_w - 60;
                    int alt_start_y = dlg_y + 120;
                    int alt_h = 40;
                    int alt_spacing = 10;

                    for (int i = 0; i < MAX_OPTIONS; i++) {
                        int ay = alt_start_y + i * (alt_h + alt_spacing);
                        if (mouse_x >= alt_x && mouse_x <= alt_x + alt_w &&
                            mouse_y >= ay && mouse_y <= ay + alt_h) {
                            if (i == questions[door.current_question].correct_answer) {
                                door.current_speech = SPEECH_CORRECT_ANSWER;
                                door.waiting_answer = false;
                                door.feedback_start_time = al_get_time();
                                if (door.open_phase == 0) door.open_phase = 1;
                            }
                            else {
                                door.current_speech = SPEECH_WRONG_ANSWER;
                                door.waiting_answer = false;
                                door.feedback_start_time = al_get_time();
                            }
                            break;
                        }
                    }
                }
                else {
                    int icon_size = 30;
                    int start_x = SCREEN_W - 150;
                    int start_y = 10;

                    if (mouse_x >= start_x && mouse_x <= start_x + icon_size &&
                        mouse_y >= start_y && mouse_y <= start_y + icon_size &&
                        player.tips_collected > 0) {
                        showing_tip = true;
                        current_tip_index = 0;
                        game_state = SHOWING_TIP;
                        clear_keys(keys);
                    }
                }
            }

            if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
                if (door.showing_dialog) {
                    int dlg_w = 700;
                    int dlg_h = 300;
                    int dlg_x = (SCREEN_W - dlg_w) / 2;
                    int dlg_y = (SCREEN_H - dlg_h) / 2;
                    int alt_x = dlg_x + 30;
                    int alt_w = dlg_w - 60;
                    int alt_start_y = dlg_y + 120;
                    int alt_h = 40;
                    int alt_spacing = 10;

                    door.selected_option = -1;
                    for (int i = 0; i < MAX_OPTIONS; i++) {
                        int ay = alt_start_y + i * (alt_h + alt_spacing);
                        if (mouse_x >= alt_x && mouse_x <= alt_x + alt_w &&
                            mouse_y >= ay && mouse_y <= ay + alt_h) {
                            door.selected_option = i;
                            break;
                        }
                    }
                }
            }
        }
        else if (game_state == SHOWING_TIP) {
            if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                if (check_click_tip_button(mouse_x, mouse_y, &player)) {
                }
                else if (check_click_outside_tip(mouse_x, mouse_y)) {
                    showing_tip = false;
                    game_state = PLAYING;
                    clear_keys(keys);
                }
            }
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    showing_tip = false;
                    game_state = PLAYING;
                    clear_keys(keys);
                }
            }
        }
        else if (game_state == GAME_OVER) {
            if (al_get_time() - game_over_start_time >= 10.0) {
                game_state = MENU;
            }
        }
        else if (game_state == END_SCREEN) {
            if (al_get_time() - end_start_time >= 10.0) {
                game_state = MENU;
            }
        }

        if (event.type == ALLEGRO_EVENT_TIMER) {
            if (game_state == PLAYING) {
                if (player.invincible && al_get_time() - player.invincible_time >= 2.0) {
                    player.invincible = false;
                }

                if (player.dead && al_get_time() - player.death_time >= 2.0) {
                    game_state = GAME_OVER;
                    game_over_start_time = al_get_time();
                    continue;
                }

                if (player.dead) {
                    redraw = true;
                    continue;
                }

                if (door.open_phase > 0) {
                    update_door_opening();
                }

                moving = false;

                if (!door.showing_dialog) {
                    if (keys[ALLEGRO_KEY_A]) {
                        player.x -= PLAYER_SPEED;
                        direction = -1;
                        moving = true;
                    }
                    if (keys[ALLEGRO_KEY_D]) {
                        player.x += PLAYER_SPEED;
                        direction = 1;
                        moving = true;
                    }
                }

                if (player.is_jumping) {
                    player.y += player.velocity_y;
                    player.velocity_y += GRAVITY;

                    if (showing_jump_animation && jump_frames_loaded > 0) {
                        frame_count++;
                        if (frame_count >= FRAME_DELAY) {
                            jump_frame_index++;
                            if (jump_frame_index >= jump_frames_loaded) {
                                jump_frame_index = jump_frames_loaded - 1;
                            }
                            frame_count = 0;
                        }
                    }

                    if (player.y >= GROUND_Y - PLAYER_H) {
                        player.y = GROUND_Y - PLAYER_H;
                        player.velocity_y = 0.0;
                        player.is_jumping = false;
                        showing_jump_animation = false;
                    }
                }

                check_tip_collection(&player, &game_state);
                update_enemies(&player);
                check_area_change(&player);

                if (!player.is_jumping) {
                    if (moving) {
                        frame_count++;
                        if (frame_count >= FRAME_DELAY) {
                            walking_frame_index++;
                            if (walking_frame_index > (1 + total_walking_frames - 1)) {
                                walking_frame_index = 1;
                            }
                            current_frame = walking_frame_index;
                            frame_count = 0;
                        }
                    }
                    else {
                        current_frame = 0;
                        walking_frame_index = 1;
                    }
                }

                if (player.x < 0) player.x = 0;
                if (player.x + PLAYER_W > LEVEL_WIDTH) player.x = LEVEL_WIDTH - PLAYER_W;

                update_camera(&player);

                if (door.showing_dialog && !door.waiting_answer) {
                    double now = al_get_time();
                    if (now - door.feedback_start_time > 1.5) {
                        if (door.current_speech == SPEECH_CORRECT_ANSWER) {
                            door.showing_dialog = false;
                            door.waiting_answer = false;
                        }
                        else if (door.current_speech == SPEECH_WRONG_ANSWER) {
                            door.current_speech = SPEECH_QUESTION1;
                            door.waiting_answer = true;
                            door.selected_option = -1;
                        }
                    }
                }
            }
            else if (game_state == END_SCREEN) {
                redraw = true;
                continue;
            }

            redraw = true;
        }

        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;
            al_clear_to_color(al_map_rgb(20, 20, 40));

            if (game_state == PROLOGUE) {
                if (prologue_index < 5 && prologue_images[prologue_index]) {
                    ALLEGRO_BITMAP* image_current = prologue_images[prologue_index];
                    int img_w = al_get_bitmap_width(image_current);
                    int img_h = al_get_bitmap_height(image_current);

                    float x = (SCREEN_W - img_w) / 2.0f;
                    float y = (SCREEN_H - img_h) / 2.0f;

                    al_draw_bitmap(image_current, x, y, 0);
                }
            }
            else if (game_state == MENU) {
                float logo_scale = 0.80;
                float original_w = al_get_bitmap_width(title_logo_bitmap);
                float original_h = al_get_bitmap_height(title_logo_bitmap);
                float dest_w = original_w * logo_scale;
                float dest_h = original_h * logo_scale;
                float logo_x = (SCREEN_W / 2.0) - (dest_w / 2.0);

                al_draw_scaled_bitmap(menu_background_bitmap,
                    0, 0, al_get_bitmap_width(menu_background_bitmap), al_get_bitmap_height(menu_background_bitmap),
                    0, 0, SCREEN_W, SCREEN_H, 0);
                al_draw_scaled_bitmap(title_logo_bitmap,
                    0, 0, original_w, original_h,
                    logo_x, 100, dest_w, dest_h, 0);

                draw_button(&play_button);
            }
            else if (game_state == PLAYING || game_state == SHOWING_TIP) {
                ALLEGRO_BITMAP* background_current = area1_background;

                al_draw_scaled_bitmap(background_current,
                    0, 0, al_get_bitmap_width(background_current), al_get_bitmap_height(background_current),
                    -camera_x, 0, LEVEL_WIDTH, SCREEN_H, 0);

                ALLEGRO_BITMAP* ground_current = ground1_bitmap;
                float ground_w = al_get_bitmap_width(ground_current);
                for (int i = 0; i * ground_w < LEVEL_WIDTH; i++) {
                    al_draw_bitmap(ground_current, i * ground_w - camera_x, GROUND_Y, 0);
                }

                draw_tips_world();

                if (door.area == player.area) {
                    draw_door();
                }

                draw_enemies(&player);

                ALLEGRO_BITMAP* bitmap_to_draw = NULL;

                if (player.dead && player.death_sprite) {
                    bitmap_to_draw = player.death_sprite;
                }
                else if (player.is_jumping && jump_frames_loaded > 0) {
                    if (jump_frame_index < jump_frames_loaded && player_jump_frames_scaled[jump_frame_index]) {
                        bitmap_to_draw = player_jump_frames_scaled[jump_frame_index];
                    }
                }
                else {
                    if (current_frame < frames_loaded && player_frames_scaled[current_frame]) {
                        bitmap_to_draw = player_frames_scaled[current_frame];
                    }
                }

                if (bitmap_to_draw) {
                    int flip_flag = (player.dead ? 0 : (direction == -1) ? ALLEGRO_FLIP_HORIZONTAL : 0);
                    al_draw_bitmap(bitmap_to_draw, player.x - camera_x, player.y, flip_flag);
                }

                draw_hearts(&player);

                draw_tip_hud(&player, font_small);

                al_draw_text(font, al_map_rgb(255, 255, 255), 10, 10, 0, "ESC para voltar");

                if (game_state == SHOWING_TIP) {
                    draw_tip_screen(&player, font_small);
                }

                if (door.showing_dialog) {
                    int dlg_w = 700;
                    int dlg_h = 300;
                    int dlg_x = (SCREEN_W - dlg_w) / 2;
                    int dlg_y = (SCREEN_H - dlg_h) / 2;

                    al_draw_filled_rectangle(0, 0, SCREEN_W, SCREEN_H, al_map_rgba(0, 0, 0, 140));
                    al_draw_filled_rectangle(dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + dlg_h, al_map_rgb(30, 30, 60));
                    al_draw_rectangle(dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + dlg_h, al_map_rgb(200, 200, 200), 2);

                    Question* p = &questions[door.current_question];
                    draw_wrapped_text(font_small, p->question, dlg_x + 20, dlg_y + 20, dlg_w - 40, 3, 26, al_map_rgb(255, 255, 255));

                    int alt_x = dlg_x + 30;
                    int alt_w = dlg_w - 60;
                    int alt_start_y = dlg_y + 120;
                    int alt_h = 40;
                    int alt_spacing = 10;

                    for (int i = 0; i < MAX_OPTIONS; i++) {
                        int ay = alt_start_y + i * (alt_h + alt_spacing);
                        ALLEGRO_COLOR bg;
                        if (!door.waiting_answer && i == questions[door.current_question].correct_answer && door.current_speech == SPEECH_CORRECT_ANSWER) {
                            bg = al_map_rgb(30, 150, 70);
                        }
                        else {
                            bg = (i == door.selected_option) ? al_map_rgb(80, 120, 200) : al_map_rgb(60, 60, 80);
                        }
                        al_draw_filled_rectangle(alt_x, ay, alt_x + alt_w, ay + alt_h, bg);
                        al_draw_rectangle(alt_x, ay, alt_x + alt_w, ay + alt_h, al_map_rgb(200, 200, 200), 1);
                        al_draw_text(font_small, al_map_rgb(255, 255, 255), alt_x + 10, ay + 8, 0, p->options[i]);
                    }

                    if (door.current_speech == SPEECH_WRONG_ANSWER) {
                        al_draw_text(font_small, al_map_rgb(255, 80, 80), dlg_x + dlg_w / 2, dlg_y + dlg_h - 40, ALLEGRO_ALIGN_CENTER, "Errado! Tente novamente.");
                    }
                    else if (door.current_speech == SPEECH_CORRECT_ANSWER) {
                        al_draw_text(font_small, al_map_rgb(80, 255, 120), dlg_x + dlg_w / 2, dlg_y + dlg_h - 40, ALLEGRO_ALIGN_CENTER, "Exato! Pode passar!");
                    }
                }
            }
            else if (game_state == GAME_OVER) {
                if (game_over_bitmap) {
                    int img_w = al_get_bitmap_width(game_over_bitmap);
                    int img_h = al_get_bitmap_height(game_over_bitmap);
                    float x = (SCREEN_W - img_w) / 2.0f;
                    float y = (SCREEN_H - img_h) / 2.0f;
                    al_draw_bitmap(game_over_bitmap, x, y, 0);
                }
                else {
                    al_draw_text(font, al_map_rgb(255, 0, 0), SCREEN_W / 2, SCREEN_H / 2 - 50, ALLEGRO_ALIGN_CENTER, "GAME OVER");
                    al_draw_text(font_small, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 + 50, ALLEGRO_ALIGN_CENTER, "Voltando ao menu em 10 segundos...");
                }
            }
            else if (game_state == END_SCREEN) {
                if (end_bitmap) {
                    int img_w = al_get_bitmap_width(end_bitmap);
                    int img_h = al_get_bitmap_height(end_bitmap);
                    float x = (SCREEN_W - img_w) / 2.0f;
                    float y = (SCREEN_H - img_h) / 2.0f;
                    al_draw_bitmap(end_bitmap, x, y, 0);
                }
                else {
                    al_draw_text(font, al_map_rgb(255, 255, 0), SCREEN_W / 2, SCREEN_H / 2 - 50, ALLEGRO_ALIGN_CENTER, "FIM DE JOGO");
                    al_draw_text(font_small, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 + 50, ALLEGRO_ALIGN_CENTER, "Voltando ao menu em 10 segundos...");
                }
            }

            draw_mouse();
            al_flip_display();
        }
    }

    destroy_prologue();
    destroy_game_over();
    destroy_areas();
    destroy_buttons();
    destroy_mouse_sprite();
    destroy_heart_sprites();
    destroy_enemy_sprites();
    destroy_tip_sprites();
    destroy_player_death_sprite(&player);
    destroy_door();
    destroy_scaled_sprites(player_frames_scaled, frames_loaded);
    destroy_scaled_sprites(player_jump_frames_scaled, jump_frames_loaded);

    for (int i = 0; i < frames_loaded; i++) {
        if (player_frames_right[i]) {
            al_destroy_bitmap(player_frames_right[i]);
        }
    }

    for (int i = 0; i < jump_frames_loaded; i++) {
        if (player_jump_frames_right[i]) {
            al_destroy_bitmap(player_jump_frames_right[i]);
        }
    }

    al_destroy_bitmap(title_logo_bitmap);
    al_destroy_font(font);
    al_destroy_font(font_small);
    al_destroy_bitmap(menu_background_bitmap);
    al_destroy_timer(timer);
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);
    return 0;
}
