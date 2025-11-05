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

#define TOTAL_FRAMES 4
#define FRAME_DELAY 8

#define LEVEL_WIDTH 5760
#define CAMERA_MARGIN 400

typedef enum { PROLOGO, MENU, JOGANDO, SAINDO } GameState;

typedef struct {
    float x, y;
    float velocity_y;
    bool is_jumping;
} Player;

typedef struct {
    float x, y, width, height;
    const char* text;
    bool hovered;
} Button;

float camera_x = 0.0;

int prologo_atual = 0;
double tempo_prologo_inicio = 0.0;
ALLEGRO_BITMAP* prologo_imagens[5] = { NULL };

void draw_button(Button* btn, ALLEGRO_FONT* font) {
    ALLEGRO_COLOR bg_color =
        btn->hovered ? al_map_rgb(100, 100, 120) : al_map_rgb(70, 70, 90);
    al_draw_filled_rectangle(btn->x, btn->y, btn->x + btn->width,
        btn->y + btn->height, bg_color);
    al_draw_text(font, al_map_rgb(255, 255, 255), btn->x + btn->width / 2,
        btn->y + 10, ALLEGRO_ALIGN_CENTER, btn->text);
}

void reset_game(Player* player) {
    player->x = 100.0;
    player->y = GROUND_Y - PLAYER_H;
    player->velocity_y = 0.0;
    player->is_jumping = false;
    camera_x = 0.0;
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

bool carregar_prologo() {
    prologo_imagens[0] = al_load_bitmap("./assets/prologo1.png");
    if (!prologo_imagens[0]) {
        fprintf(stderr, "Nao foi possivel carregar prologo1.png\n");
        return false;
    }

    prologo_imagens[1] = al_load_bitmap("./assets/prologo2.png");
    if (!prologo_imagens[1]) {
        fprintf(stderr, "Nao foi possivel carregar prologo2.png\n");
        return false;
    }

    prologo_imagens[2] = al_load_bitmap("./assets/prologo3.png");
    if (!prologo_imagens[2]) {
        fprintf(stderr, "Nao foi possivel carregar prologo3.png\n");
        return false;
    }

    prologo_imagens[3] = al_load_bitmap("./assets/prologo4.png");
    if (!prologo_imagens[3]) {
        fprintf(stderr, "Nao foi possivel carregar prologo4.png\n");
        return false;
    }

    prologo_imagens[4] = al_load_bitmap("./assets/prologo5.png");
    if (!prologo_imagens[4]) {
        fprintf(stderr, "Nao foi possivel carregar prologo5.png\n");
        return false;
    }

    printf("Todas as imagens do prologo foram carregadas com sucesso!\n");
    return true;
}

void liberar_prologo() {
    for (int i = 0; i < 5; i++) {
        if (prologo_imagens[i]) {
            al_destroy_bitmap(prologo_imagens[i]);
            prologo_imagens[i] = NULL;
        }
    }
}

void avancar_prologo(GameState* game_state) {
    prologo_atual++;
    if (prologo_atual >= 5) {
        *game_state = MENU;
        prologo_atual = 0;
    }
    else {
        tempo_prologo_inicio = al_get_time();
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

    char filename[25];
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        sprintf_s(filename, sizeof(filename), "./assets/knight%d.png", i + 1);
        player_frames_right[i] = al_load_bitmap(filename);
        if (!player_frames_right[i]) {
            fprintf(stderr, "Nao foi possivel carregar o arquivo %s\n", filename);
            return -1;
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

    GameState game_state;
    if (carregar_prologo()) {
        game_state = PROLOGO;
        tempo_prologo_inicio = al_get_time();
    }
    else {
        fprintf(stderr, "Pulando prologo devido a erro no carregamento\n");
        game_state = MENU;
    }

    Player player = {
        100.0,
        GROUND_Y - PLAYER_H,
        0.0,
        false
    };

    int direction = 1;
    int current_frame = 0;
    int frame_count = 0;
    bool moving = false;

    Button play_button = { SCREEN_W / 2.0 - 150, 500, 300, 60, "Jogar", false };
    bool keys[ALLEGRO_KEY_MAX] = { false };
    bool redraw = true;

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());

    al_start_timer(timer);
    while (game_state != SAINDO) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            game_state = SAINDO;
        }

        if (game_state == PROLOGO) {
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                    game_state = MENU;
                    prologo_atual = 0;
                }
            }

            if (al_get_time() - tempo_prologo_inicio >= 10.0) {
                avancar_prologo(&game_state);
            }
        }
        else if (game_state == MENU) {
            if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
                play_button.hovered = (event.mouse.x >= play_button.x &&
                    event.mouse.x <= play_button.x + play_button.width &&
                    event.mouse.y >= play_button.y &&
                    event.mouse.y <= play_button.y + play_button.height);
            }
            if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && play_button.hovered) {
                game_state = JOGANDO;
                reset_game(&player);
                clear_keys(keys);
            }
        }
        else if (game_state == JOGANDO) {
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    game_state = MENU;
                    reset_game(&player);
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
                if (player.x + PLAYER_W > LEVEL_WIDTH) player.x = LEVEL_WIDTH - PLAYER_W;

                update_camera(&player);
            }
            redraw = true;
        }

        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;
            al_clear_to_color(al_map_rgb(20, 20, 40));

            if (game_state == PROLOGO) {
                if (prologo_atual < 5 && prologo_imagens[prologo_atual]) {
                    ALLEGRO_BITMAP* imagem_atual = prologo_imagens[prologo_atual];
                    int img_w = al_get_bitmap_width(imagem_atual);
                    int img_h = al_get_bitmap_height(imagem_atual);

                    float x = (SCREEN_W - img_w) / 2.0f;
                    float y = (SCREEN_H - img_h) / 2.0f;

                    al_draw_bitmap(imagem_atual, x, y, 0);
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
                draw_button(&play_button, font);
            }
            else if (game_state == JOGANDO) {
                al_draw_scaled_bitmap(game_background_bitmap,
                    0, 0, al_get_bitmap_width(game_background_bitmap), al_get_bitmap_height(game_background_bitmap),
                    -camera_x, 0, LEVEL_WIDTH, SCREEN_H, 0);

                float ground_w = al_get_bitmap_width(ground_bitmap);
                for (int i = 0; i * ground_w < LEVEL_WIDTH; i++) {
                    al_draw_bitmap(ground_bitmap, i * ground_w - camera_x, GROUND_Y, 0);
                }

                ALLEGRO_BITMAP* bitmap_to_draw = player_frames_right[current_frame];
                int flip_flag = (direction == -1) ? ALLEGRO_FLIP_HORIZONTAL : 0;
                al_draw_bitmap(bitmap_to_draw, player.x - camera_x, player.y, flip_flag);

                al_draw_text(font, al_map_rgb(255, 255, 255), 10, 10, 0,
                    "ESC para voltar");
            }
            al_flip_display();
        }
    }

    liberar_prologo();

    for (int i = 0; i < TOTAL_FRAMES; i++) {
        al_destroy_bitmap(player_frames_right[i]);
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
