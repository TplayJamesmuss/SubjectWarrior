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

#define SCREEN_W 1280
#define SCREEN_H 720
#define FPS 60.0
#define GROUND_Y 620.0

#define PLAYER_SPEED 5.0
#define JUMP_FORCE 15.0
#define GRAVITY 0.6

#define PLAYER_W 112.0
#define PLAYER_H 176.0

#define ENEMY_W 80.0
#define ENEMY_H 80.0
#define ENEMY_SPEED 2.5
#define ENEMY_RESPAWN_TIME 3.0

#define TOTAL_FRAMES 4
#define ENEMY_FRAMES 2
#define FRAME_DELAY 8
#define ENEMY_FRAME_DELAY 10

typedef enum { MENU, JOGANDO, SAINDO } GameState;

typedef struct {
    float x, y;
    float velocity_y;
    bool is_jumping;
} Player;

typedef struct {
    float x, y;
    bool active;
    float respawn_timer;
    int direction;
    int current_frame;
    int frame_count;
} Enemy;

typedef struct {
    float x, y, width, height;
    const char* text;
    bool hovered;
} Button;

void draw_button(Button* btn, ALLEGRO_FONT* font) {
    ALLEGRO_COLOR bg_color =
        btn->hovered ? al_map_rgb(100, 100, 120) : al_map_rgb(70, 70, 90);
    al_draw_filled_rectangle(btn->x, btn->y, btn->x + btn->width,
        btn->y + btn->height, bg_color);
    al_draw_text(font, al_map_rgb(255, 255, 255), btn->x + btn->width / 2,
        btn->y + 10, ALLEGRO_ALIGN_CENTER, btn->text);
}

bool check_collision(float x1, float y1, float w1, float h1,
                     float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 &&
            x1 + w1 > x2 &&
            y1 < y2 + h2 &&
            y1 + h1 > y2);
}

void spawn_enemy(Enemy* enemy) {
    enemy->active = true;
    enemy->y = GROUND_Y - ENEMY_H;
    
    if (rand() % 2 == 0) {
        enemy->x = -ENEMY_W;
        enemy->direction = 1;
    } else {
        enemy->x = SCREEN_W;
        enemy->direction = -1;
    }
    
    enemy->respawn_timer = 0.0;
    enemy->current_frame = 0;
    enemy->frame_count = 0;
}

void reset_game(Player* player, Enemy* enemy, int* score) {
    player->x = (SCREEN_W / 2.0) - (PLAYER_W / 2.0);
    player->y = GROUND_Y - PLAYER_H;
    player->velocity_y = 0.0;
    player->is_jumping = false;
    
    enemy->active = false;
    enemy->respawn_timer = ENEMY_RESPAWN_TIME;
    
    *score = 0;
}

void clear_keys(bool keys[]) {
    for (int i = 0; i < ALLEGRO_KEY_MAX; i++) {
        keys[i] = false;
    }
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

    ALLEGRO_BITMAP* player_frames_right[TOTAL_FRAMES];
    ALLEGRO_BITMAP* enemy_frames[ENEMY_FRAMES];

    char filename[25];
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        sprintf_s(filename, sizeof(filename), "./assets/knight%d.png", i + 1);
        player_frames_right[i] = al_load_bitmap(filename);
        if (!player_frames_right[i]) {
            fprintf(stderr, "Nao foi possivel carregar o arquivo %s\n", filename);
            return -1;
        }
    }
    
    enemy_frames[0] = al_load_bitmap("./assets/ant1.png");
    enemy_frames[1] = al_load_bitmap("./assets/ant2.png");
    
    if (!enemy_frames[0] || !enemy_frames[1]) {
        printf("Sprites do inimigo nao encontrados. Criando placeholders...\n");
        for (int i = 0; i < ENEMY_FRAMES; i++) {
            enemy_frames[i] = al_create_bitmap(ENEMY_W, ENEMY_H);
            ALLEGRO_BITMAP* old_target = al_get_target_bitmap();
            al_set_target_bitmap(enemy_frames[i]);
            al_clear_to_color(al_map_rgb(255, 100 + i * 50, 0));
            al_set_target_bitmap(old_target);
        }
    }

    ALLEGRO_BITMAP* title_logo_bitmap = al_load_bitmap("./assets/title_logo.png");
    ALLEGRO_BITMAP* menu_background_bitmap = al_load_bitmap("./assets/menu_background.png");
    ALLEGRO_BITMAP* game_background_bitmap = al_load_bitmap("./assets/phase.png");
    ALLEGRO_BITMAP* ground_bitmap = al_load_bitmap("./assets/ground.png");

    if (!font || !title_logo_bitmap || !menu_background_bitmap || !game_background_bitmap || !ground_bitmap) {
        fprintf(stderr, "Erro ao carregar recursos\n");
        return -1;
    }

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());

    GameState game_state = MENU;
    Player player = { 
        (SCREEN_W / 2.0) - (PLAYER_W / 2.0), 
        GROUND_Y - PLAYER_H,
        0.0,
        false
    };

    Enemy enemy = { 0, 0, false, 0.0, 1, 0, 0 };

    int direction = 1;
    int current_frame = 0;
    int frame_count = 0;
    bool moving = false;
    int score = 0;

    Button play_button = { SCREEN_W / 2.0 - 150, 500, 300, 60, "Jogar", false };
    bool keys[ALLEGRO_KEY_MAX] = { false };
    bool redraw = true;

    al_start_timer(timer);
    while (game_state != SAINDO) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            game_state = SAINDO;
        }

        if (game_state == MENU) {
            if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
                play_button.hovered = (event.mouse.x >= play_button.x &&
                    event.mouse.x <= play_button.x + play_button.width &&
                    event.mouse.y >= play_button.y &&
                    event.mouse.y <= play_button.y + play_button.height);
            }
            if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && play_button.hovered) {
                game_state = JOGANDO;
                reset_game(&player, &enemy, &score);
                spawn_enemy(&enemy);
                clear_keys(keys);
            }
        }
        else if (game_state == JOGANDO) {
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    game_state = MENU;
                    reset_game(&player, &enemy, &score);
                    clear_keys(keys);
                }
                keys[event.keyboard.keycode] = true;
                
                if ((event.keyboard.keycode == ALLEGRO_KEY_SPACE || 
                     event.keyboard.keycode == ALLEGRO_KEY_W) && 
                    !player.is_jumping) {
                    player.velocity_y = -JUMP_FORCE;
                    player.is_jumping = true;
                }
            }
            if (event.type == ALLEGRO_EVENT_KEY_UP) {
                keys[event.keyboard.keycode] = false;
            }
        }

        if (event.type == ALLEGRO_EVENT_TIMER) {
            if (game_state == JOGANDO) {
                moving = false;

                if (keys[ALLEGRO_KEY_A] || keys[ALLEGRO_KEY_LEFT]) {
                    player.x -= PLAYER_SPEED;
                    direction = -1;
                    moving = true;
                }
                if (keys[ALLEGRO_KEY_D] || keys[ALLEGRO_KEY_RIGHT]) {
                    player.x += PLAYER_SPEED;
                    direction = 1;
                    moving = true;
                }

                if (player.is_jumping) {
                    player.y += player.velocity_y;
                    player.velocity_y += GRAVITY;
                    
                    if (player.y >= GROUND_Y - PLAYER_H) {
                        player.y = GROUND_Y - PLAYER_H;
                        player.velocity_y = 0.0;
                        player.is_jumping = false;
                    }
                }

                if (enemy.active) {
                    if (enemy.x < player.x) {
                        enemy.x += ENEMY_SPEED;
                    } else {
                        enemy.x -= ENEMY_SPEED;
                    }

                    if (enemy.x < player.x) {
                        enemy.direction = 1;
                    } else {
                        enemy.direction = -1;
                    }

                    enemy.frame_count++;
                    if (enemy.frame_count >= ENEMY_FRAME_DELAY) {
                        enemy.current_frame = (enemy.current_frame + 1) % ENEMY_FRAMES;
                        enemy.frame_count = 0;
                    }

                    if (check_collision(player.x, player.y, PLAYER_W, PLAYER_H,
                                       enemy.x, enemy.y, ENEMY_W, ENEMY_H)) {
                        if (player.velocity_y > 0 && 
                            player.y + PLAYER_H < enemy.y + ENEMY_H / 2) {
                            enemy.active = false;
                            enemy.respawn_timer = ENEMY_RESPAWN_TIME;
                            player.velocity_y = -JUMP_FORCE * 0.7;
                            score += 100;
                        } else {
                            game_state = MENU;
                            reset_game(&player, &enemy, &score);
                            clear_keys(keys);
                        }
                    }
                } else {
                    enemy.respawn_timer -= 1.0 / FPS;
                    if (enemy.respawn_timer <= 0) {
                        spawn_enemy(&enemy);
                    }
                }

                if (moving) {
                    frame_count++;
                    if (frame_count >= FRAME_DELAY) {
                        current_frame = (current_frame + 1) % TOTAL_FRAMES;
                        frame_count = 0;
                    }
                }
                else {
                    current_frame = 0;
                }

                if (player.x < 0) player.x = 0;
                if (player.x + PLAYER_W > SCREEN_W) player.x = SCREEN_W - PLAYER_W;
            }
            redraw = true;
        }

        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;
            al_clear_to_color(al_map_rgb(20, 20, 40));

            if (game_state == MENU) {
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
                draw_button(&play_button, font);
            }
            else if (game_state == JOGANDO) {
                al_draw_scaled_bitmap(game_background_bitmap,
                    0, 0, al_get_bitmap_width(game_background_bitmap), al_get_bitmap_height(game_background_bitmap),
                    0, 0, SCREEN_W, SCREEN_H, 0);
                
                float ground_w = al_get_bitmap_width(ground_bitmap);
                for (int i = 0; i * ground_w < SCREEN_W; i++) {
                    al_draw_bitmap(ground_bitmap, i * ground_w, GROUND_Y, 0);
                }
                
                if (enemy.active) {
                    ALLEGRO_BITMAP* enemy_bitmap_to_draw = enemy_frames[enemy.current_frame];
                    int enemy_flip_flag = (enemy.direction == 1) ? ALLEGRO_FLIP_HORIZONTAL : 0;
                    al_draw_bitmap(enemy_bitmap_to_draw, enemy.x, enemy.y, enemy_flip_flag);
                }
                
                ALLEGRO_BITMAP* bitmap_to_draw = player_frames_right[current_frame];
                int flip_flag = (direction == -1) ? ALLEGRO_FLIP_HORIZONTAL : 0;
                al_draw_bitmap(bitmap_to_draw, player.x, player.y, flip_flag);
                
                al_draw_text(font, al_map_rgb(255, 255, 255), 10, 10, 0,
                    "ESC para voltar | ESPACO ou W para pular");
                
                char score_text[50];
                sprintf_s(score_text, sizeof(score_text), "Score: %d", score);
                al_draw_text(font, al_map_rgb(255, 255, 255), 10, 50, 0, score_text);
            }
            al_flip_display();
        }
    }

    for (int i = 0; i < TOTAL_FRAMES; i++) {
        al_destroy_bitmap(player_frames_right[i]);
    }
    for (int i = 0; i < ENEMY_FRAMES; i++) {
        al_destroy_bitmap(enemy_frames[i]);
    }
    al_destroy_bitmap(title_logo_bitmap);
    al_destroy_bitmap(game_background_bitmap);
    al_destroy_font(font);
    al_destroy_bitmap(menu_background_bitmap);
    al_destroy_timer(timer);
    al_destroy_bitmap(ground_bitmap);
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);
    return 0;
}
