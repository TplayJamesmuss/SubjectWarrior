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

#define MAX_ENEMYS 10
#define ENEMY_VELOCIDADE 3.0
#define ENEMY_SCALE 2.5f
#define ENEMY_W 79.0
#define ENEMY_H 35.0
#define ENEMY_FRAME_DELAY 10
#define MAX_ENEMY_FRAMES 4
#define ENEMY_SPAWN_DISTANCE 1100.0f

#define MAX_VIDAS 3

#define MAX_DICAS 4
#define DICA_DISTANCE 1500.0f

#define DICA_SCALE 1.3f

#define MAX_PERGUNTAS 3
#define MAX_ALTERNATIVAS 4
#define DOOR_SCALE 1.0f
#define DOOR_W 80.0f
#define DOOR_H 160.0f
#define INTERACTION_DISTANCE 100.0f

typedef enum { PROLOGO, MENU, JOGANDO, GAME_OVER, END_SCREEN, SAINDO, MOSTRANDO_DICA, DIALOGO } GameState;

typedef enum { AREA_1, AREA_2 } AreaAtual;

typedef struct {
    float x, y;
    bool coletada;
    AreaAtual area;
} Dica;

typedef struct {
    float x, y;
    float velocity_y;
    bool is_jumping;
    AreaAtual area;
    int vidas;
    bool invencivel;
    double tempo_invencivel;
    bool morto;
    double tempo_morte;
    ALLEGRO_BITMAP* death_sprite;
    int dicas_coletadas;
} Player;

typedef struct {
    float x, y;
    bool ativo;
    bool spawnado;
    AreaAtual area;
    ALLEGRO_BITMAP* sprites[MAX_ENEMY_FRAMES];
    int frame_atual;
    int contador_frame;
    int direcao;
} Enemy;

typedef struct {
    float x, y;
    float scale;
    ALLEGRO_BITMAP* normal;
    ALLEGRO_BITMAP* hover;
    bool hovered;
} Button;

typedef enum {
    FALA_BOAS_VINDAS,
    FALA_PREPARADO,
    FALA_PERGUNTA1,
    FALA_PERGUNTA2,
    FALA_PERGUNTA3,
    FALA_RESPOSTA_ERRADA,
    FALA_RESPOSTA_CERTA,
    FALA_FINAL
} TipoFala;

typedef struct {
    char pergunta[200];
    char alternativas[MAX_ALTERNATIVAS][50];
    int resposta_correta;
} Pergunta;

typedef struct {
    float x, y;
    bool aberta;
    int fase_abertura;
    ALLEGRO_BITMAP* sprites[4];
    TipoFala fala_atual;
    int pergunta_atual;
    bool mostrando_dialogo;
    bool aguardando_resposta;
    int alternativa_selecionada;
    AreaAtual area;
    int sprite_w;
    int sprite_h;
    double tempo_feedback_inicio;
} Porta;

float camera_x = 0.0;

int prologo_atual = 0;
double tempo_prologo_inicio = 0.0;
double tempo_game_over_inicio = 0.0;
double tempo_end_inicio = 0.0;
ALLEGRO_BITMAP* prologo_imagens[5] = { NULL };
ALLEGRO_BITMAP* game_over_bitmap = NULL;
ALLEGRO_BITMAP* end_bitmap = NULL;

ALLEGRO_BITMAP* area1_background = NULL;
ALLEGRO_BITMAP* area2_background = NULL;
ALLEGRO_BITMAP* ground1_bitmap = NULL;
ALLEGRO_BITMAP* ground2_bitmap = NULL;
ALLEGRO_BITMAP* play_button_normal = NULL;
ALLEGRO_BITMAP* play_button_hover = NULL;
ALLEGRO_BITMAP* mouse_sprite = NULL;
ALLEGRO_BITMAP* heart_full = NULL;
ALLEGRO_BITMAP* heart_empty = NULL;

ALLEGRO_BITMAP* dica_icon = NULL;
ALLEGRO_BITMAP* dica_images[MAX_DICAS] = { NULL };
ALLEGRO_BITMAP* dica_icon_small = NULL;
ALLEGRO_BITMAP* next_button = NULL;
ALLEGRO_BITMAP* prev_button = NULL;

ALLEGRO_BITMAP* dialog_box = NULL;
ALLEGRO_BITMAP* alternativa_selecionada_bg = NULL;
ALLEGRO_BITMAP* alternativa_normal_bg = NULL;

ALLEGRO_BITMAP* player_frames_scaled[MAX_FRAMES] = { NULL };
ALLEGRO_BITMAP* player_jump_frames_scaled[MAX_JUMP_FRAMES] = { NULL };
ALLEGRO_BITMAP* player_death_scaled = NULL;

Enemy enemys[MAX_ENEMYS];

Dica dicas[MAX_DICAS];

Porta porta;

Pergunta perguntas[MAX_PERGUNTAS] = {
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

int dica_atual_tela = 0;
bool mostrando_dica = false;

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

bool criar_sprites_escalados(ALLEGRO_BITMAP* frames_originais[], ALLEGRO_BITMAP* frames_destino[], int total_frames, float scale) {
    for (int i = 0; i < total_frames; i++) {
        if (!frames_originais[i]) {
            fprintf(stderr, "Frame original %d é NULL\n", i);
            return false;
        }

        int original_width = al_get_bitmap_width(frames_originais[i]);
        int original_height = al_get_bitmap_height(frames_originais[i]);
        int scaled_width = (int)(original_width * scale);
        int scaled_height = (int)(original_height * scale);

        ALLEGRO_BITMAP* scaled_frame = al_create_bitmap(scaled_width, scaled_height);
        if (!scaled_frame) {
            fprintf(stderr, "Não foi possível criar bitmap escalado para frame %d\n", i);
            return false;
        }

        al_set_target_bitmap(scaled_frame);
        al_draw_scaled_bitmap(frames_originais[i],
            0, 0, original_width, original_height,
            0, 0, scaled_width, scaled_height, 0);

        al_set_target_backbuffer(al_get_current_display());
        frames_destino[i] = scaled_frame;
    }
    return true;
}

ALLEGRO_BITMAP* criar_sprite_escalado(ALLEGRO_BITMAP* sprite_original, float scale) {
    if (!sprite_original) {
        fprintf(stderr, "Sprite original é NULL\n");
        return NULL;
    }

    int original_width = al_get_bitmap_width(sprite_original);
    int original_height = al_get_bitmap_height(sprite_original);
    int scaled_width = (int)(original_width * scale);
    int scaled_height = (int)(original_height * scale);

    ALLEGRO_BITMAP* scaled_sprite = al_create_bitmap(scaled_width, scaled_height);
    if (!scaled_sprite) {
        fprintf(stderr, "Não foi possível criar bitmap escalado\n");
        return NULL;
    }

    al_set_target_bitmap(scaled_sprite);
    al_draw_scaled_bitmap(sprite_original,
        0, 0, original_width, original_height,
        0, 0, scaled_width, scaled_height, 0);

    al_set_target_backbuffer(al_get_current_display());
    return scaled_sprite;
}

void liberar_sprites_escalados(ALLEGRO_BITMAP* frames[], int total_frames) {
    for (int i = 0; i < total_frames; i++) {
        if (frames[i]) {
            al_destroy_bitmap(frames[i]);
            frames[i] = NULL;
        }
    }
}

float offset_y = 50.0f;

void inicializar_enemys() {
    int spawn_count = 4;
    float start_x = 1500.0f;

    for (int i = 0; i < MAX_ENEMYS; i++) {
        if (i < spawn_count) {
            enemys[i].x = start_x + (i * ENEMY_SPAWN_DISTANCE);
            enemys[i].y = GROUND_Y - ENEMY_H - offset_y;
            enemys[i].ativo = false;
            enemys[i].spawnado = false;
            enemys[i].area = AREA_1;
            enemys[i].frame_atual = 0;
            enemys[i].contador_frame = 0;
            enemys[i].direcao = -1;

            for (int j = 0; j < MAX_ENEMY_FRAMES; j++) {
                char filename[50];
                sprintf_s(filename, sizeof(filename), "./assets/enemy_walk%d.png", j + 1);
                ALLEGRO_BITMAP* sprite_original = al_load_bitmap(filename);

                if (!sprite_original) {
                    enemys[i].sprites[j] = al_create_bitmap((int)ENEMY_W, (int)ENEMY_H);
                    if (enemys[i].sprites[j]) {
                        al_set_target_bitmap(enemys[i].sprites[j]);
                        al_clear_to_color(al_map_rgb(255, 0, 0));
                        al_set_target_backbuffer(al_get_current_display());
                    }
                }
                else {
                    int original_width = al_get_bitmap_width(sprite_original);
                    int original_height = al_get_bitmap_height(sprite_original);
                    int scaled_width = (int)(original_width * ENEMY_SCALE);
                    int scaled_height = (int)(original_height * ENEMY_SCALE);

                    enemys[i].sprites[j] = al_create_bitmap(scaled_width, scaled_height);
                    if (enemys[i].sprites[j]) {
                        al_set_target_bitmap(enemys[i].sprites[j]);
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
            enemys[i].x = -10000.0f;
            enemys[i].y = -10000.0f;
            enemys[i].ativo = false;
            enemys[i].spawnado = false;
            enemys[i].area = AREA_2;
            enemys[i].frame_atual = 0;
            enemys[i].contador_frame = 0;
            enemys[i].direcao = -1;
            for (int j = 0; j < MAX_ENEMY_FRAMES; j++) {
                enemys[i].sprites[j] = NULL;
            }
        }
    }
}

void inicializar_dicas() {
    for (int i = 0; i < MAX_DICAS; i++) {
        dicas[i].x = 800.0f + (i * DICA_DISTANCE);
        dicas[i].y = GROUND_Y - 100.0f;
        dicas[i].coletada = false;
        dicas[i].area = AREA_1;
    }
}

bool carregar_porta() {
    for (int i = 0; i < 4; i++) {
        char filename[50];
        sprintf_s(filename, sizeof(filename), "./assets/door%d.png", i + 1);
        ALLEGRO_BITMAP* sprite_original = al_load_bitmap(filename);

        if (!sprite_original) {
            fprintf(stderr, "Aviso: nao foi possivel carregar %s, usando fallback\n", filename);
            porta.sprites[i] = al_create_bitmap((int)DOOR_W, (int)DOOR_H);
            if (porta.sprites[i]) {
                al_set_target_bitmap(porta.sprites[i]);
                al_clear_to_color(al_map_rgb(100, 50, 0));
                al_set_target_backbuffer(al_get_current_display());
            }
            if (i == 0) {
                porta.sprite_w = (int)DOOR_W;
                porta.sprite_h = (int)DOOR_H;
            }
            continue;
        }

        int original_width = al_get_bitmap_width(sprite_original);
        int original_height = al_get_bitmap_height(sprite_original);
        int scaled_width = (int)(original_width * DOOR_SCALE);
        int scaled_height = (int)(original_height * DOOR_SCALE);

        porta.sprites[i] = al_create_bitmap(scaled_width, scaled_height);
        if (porta.sprites[i]) {
            al_set_target_bitmap(porta.sprites[i]);
            al_draw_scaled_bitmap(sprite_original,
                0, 0, original_width, original_height,
                0, 0, scaled_width, scaled_height, 0);
            al_set_target_backbuffer(al_get_current_display());
        }
        al_destroy_bitmap(sprite_original);

        if (i == 0) {
            porta.sprite_w = scaled_width;
            porta.sprite_h = scaled_height;
        }
    }

    return true;
}

void inicializar_porta() {
    if (porta.sprite_w <= 0) porta.sprite_w = (int)DOOR_W;
    if (porta.sprite_h <= 0) porta.sprite_h = (int)DOOR_H;

    porta.x = LEVEL_WIDTH - porta.sprite_w - 100.0f;
    porta.y = GROUND_Y - porta.sprite_h;
    porta.aberta = false;
    porta.fase_abertura = 0;
    porta.fala_atual = FALA_BOAS_VINDAS;
    porta.pergunta_atual = 0;
    porta.mostrando_dialogo = false;
    porta.aguardando_resposta = false;
    porta.alternativa_selecionada = -1;
    porta.area = AREA_2;
    porta.tempo_feedback_inicio = 0.0;
}

void liberar_porta() {
    for (int i = 0; i < 4; i++) {
        if (porta.sprites[i]) {
            al_destroy_bitmap(porta.sprites[i]);
            porta.sprites[i] = NULL;
        }
    }
}

bool jogador_perto_da_porta(Player* player) {
    if (player->area != AREA_2) return false;

    float dx = fabs(player->x - porta.x);
    float dy = fabs(player->y - porta.y);

    return (dx < INTERACTION_DISTANCE && dy < INTERACTION_DISTANCE);
}

void atualizar_abertura_porta() {
    if (porta.fase_abertura > 0 && porta.fase_abertura < 3) {
        static double last_update = 0.0;
        double now = al_get_time();

        if (now - last_update > 0.5) {
            porta.fase_abertura++;
            last_update = now;

            if (porta.fase_abertura >= 3) {
                porta.aberta = true;
            }
        }
    }
}

void desenhar_porta() {
    if (porta.sprites[porta.fase_abertura]) {
        al_draw_bitmap(porta.sprites[porta.fase_abertura], porta.x - camera_x, porta.y, 0);
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

void liberar_sprites_enemys() {
    for (int i = 0; i < MAX_ENEMYS; i++) {
        for (int j = 0; j < MAX_ENEMY_FRAMES; j++) {
            if (enemys[i].sprites[j]) {
                al_destroy_bitmap(enemys[i].sprites[j]);
                enemys[i].sprites[j] = NULL;
            }
        }
    }
}

void liberar_sprites_dicas() {
    if (dica_icon) {
        al_destroy_bitmap(dica_icon);
        dica_icon = NULL;
    }
    if (dica_icon_small) {
        al_destroy_bitmap(dica_icon_small);
        dica_icon_small = NULL;
    }
    if (next_button) {
        al_destroy_bitmap(next_button);
        next_button = NULL;
    }
    if (prev_button) {
        al_destroy_bitmap(prev_button);
        prev_button = NULL;
    }
    for (int i = 0; i < MAX_DICAS; i++) {
        if (dica_images[i]) {
            al_destroy_bitmap(dica_images[i]);
            dica_images[i] = NULL;
        }
    }
}

void reset_game(Player* player) {
    player->x = 100.0;
    player->y = GROUND_Y - PLAYER_H;
    player->velocity_y = 0.0;
    player->is_jumping = false;
    player->area = AREA_1;
    player->vidas = MAX_VIDAS;
    player->invencivel = false;
    player->tempo_invencivel = 0.0;
    player->morto = false;
    player->tempo_morte = 0.0;
    player->dicas_coletadas = 0;
    camera_x = 0.0;
    dica_atual_tela = 0;
    mostrando_dica = false;

    for (int i = 0; i < MAX_DICAS; i++) {
        dicas[i].coletada = false;
    }

    inicializar_porta();
    inicializar_enemys();
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

void verificar_mudanca_area(Player* player) {
    if (player->area == AREA_1 && player->x >= LEVEL_WIDTH - PLAYER_W - 10) {
        player->area = AREA_2;
        player->x = 100.0;
        camera_x = 0.0;
    }

    if (player->area == AREA_2 && player->x < 0) {
        player->x = 0;
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

    return true;
}

bool carregar_game_over() {
    game_over_bitmap = al_load_bitmap("./assets/game_over.png");
    if (!game_over_bitmap) {
        fprintf(stderr, "Nao foi possivel carregar game_over.png\n");
        return false;
    }

    end_bitmap = al_load_bitmap("./assets/end.png");
    if (!end_bitmap) {
        fprintf(stderr, "Aviso: Nao foi possivel carregar end.png\n");
        end_bitmap = NULL;
    }

    return true;
}

bool carregar_player_death_sprite(Player* player) {
    ALLEGRO_BITMAP* death_original = al_load_bitmap("./assets/death.png");
    if (!death_original) {
        fprintf(stderr, "Aviso: Nao foi possivel carregar death.png\n");
        player->death_sprite = NULL;
        return false;
    }

    player->death_sprite = criar_sprite_escalado(death_original, PLAYER_SCALE);
    al_destroy_bitmap(death_original);

    if (!player->death_sprite) {
        fprintf(stderr, "Erro ao criar sprite de morte escalado\n");
        return false;
    }

    return true;
}

bool carregar_areas() {
    area1_background = al_load_bitmap("./assets/phase.png");
    area2_background = al_load_bitmap("./assets/phase2.png");

    ground1_bitmap = al_load_bitmap("./assets/ground1.png");
    ground2_bitmap = al_load_bitmap("./assets/ground2.png");

    if (!area1_background || !area2_background || !ground1_bitmap || !ground2_bitmap) {
        fprintf(stderr, "Erro ao carregar fundos das areas ou grounds\n");
        return false;
    }
    return true;
}

bool carregar_botoes() {
    play_button_normal = al_load_bitmap("./assets/button_play.png");
    if (!play_button_normal) {
        fprintf(stderr, "Erro ao carregar button_play.png\n");
        return false;
    }

    play_button_hover = al_load_bitmap("./assets/button_play_hover.png");
    if (!play_button_hover) {
        play_button_hover = play_button_normal;
    }

    return true;
}

bool carregar_mouse_sprite() {
    mouse_sprite = al_load_bitmap("./assets/mouse.png");
    if (!mouse_sprite) {
        fprintf(stderr, "Aviso: Nao foi possivel carregar mouse.png, usando cursor padrão\n");
        return false;
    }
    return true;
}

bool carregar_sprites_vidas() {
    heart_full = al_load_bitmap("./assets/heart1.png");
    if (!heart_full) {
        fprintf(stderr, "Erro ao carregar heart1.png\n");
        return false;
    }

    heart_empty = al_load_bitmap("./assets/heart2.png");
    if (!heart_empty) {
        fprintf(stderr, "Erro ao carregar heart2.png\n");
        return false;
    }

    return true;
}

bool carregar_sprites_dicas() {
    dica_icon = al_load_bitmap("./assets/tip.png");
    if (!dica_icon) {
        fprintf(stderr, "Aviso: Nao foi possivel carregar tip.png\n");
        return false;
    }

    dica_icon_small = al_load_bitmap("./assets/tip.png");
    if (!dica_icon_small) {
        fprintf(stderr, "Aviso: Nao foi possivel carregar tip.png para HUD\n");
        return false;
    }

    for (int i = 0; i < MAX_DICAS; i++) {
        char filename[50];
        sprintf_s(filename, sizeof(filename), "./assets/tip%d.png", i + 1);
        dica_images[i] = al_load_bitmap(filename);
        if (!dica_images[i]) {
            fprintf(stderr, "Aviso: Nao foi possivel carregar %s\n", filename);
            dica_images[i] = al_create_bitmap(400, 300);
            al_set_target_bitmap(dica_images[i]);
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

void liberar_prologo() {
    for (int i = 0; i < 5; i++) {
        if (prologo_imagens[i]) {
            al_destroy_bitmap(prologo_imagens[i]);
            prologo_imagens[i] = NULL;
        }
    }
}

void liberar_game_over() {
    if (game_over_bitmap) {
        al_destroy_bitmap(game_over_bitmap);
        game_over_bitmap = NULL;
    }
    if (end_bitmap) {
        al_destroy_bitmap(end_bitmap);
        end_bitmap = NULL;
    }
}

void liberar_player_death_sprite(Player* player) {
    if (player->death_sprite) {
        al_destroy_bitmap(player->death_sprite);
        player->death_sprite = NULL;
    }
}

void liberar_areas() {
    if (area1_background) {
        al_destroy_bitmap(area1_background);
        area1_background = NULL;
    }
    if (area2_background) {
        al_destroy_bitmap(area2_background);
        area2_background = NULL;
    }
    if (ground1_bitmap) {
        al_destroy_bitmap(ground1_bitmap);
        ground1_bitmap = NULL;
    }
    if (ground2_bitmap) {
        al_destroy_bitmap(ground2_bitmap);
        ground2_bitmap = NULL;
    }
}

void liberar_botoes() {
    if (play_button_normal) {
        al_destroy_bitmap(play_button_normal);
        play_button_normal = NULL;
    }
    if (play_button_hover && play_button_hover != play_button_normal) {
        al_destroy_bitmap(play_button_hover);
        play_button_hover = NULL;
    }
}

void liberar_mouse_sprite() {
    if (mouse_sprite) {
        al_destroy_bitmap(mouse_sprite);
        mouse_sprite = NULL;
    }
}

void liberar_sprites_vidas() {
    if (heart_full) {
        al_destroy_bitmap(heart_full);
        heart_full = NULL;
    }
    if (heart_empty) {
        al_destroy_bitmap(heart_empty);
        heart_empty = NULL;
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

void desenhar_mouse() {
    if (mouse_visible && mouse_sprite) {
        int sprite_w = al_get_bitmap_width(mouse_sprite);
        int sprite_h = al_get_bitmap_height(mouse_sprite);
        al_draw_bitmap(mouse_sprite, mouse_x - sprite_w / 2, mouse_y - sprite_h / 2, 0);
    }
}

void desenhar_vidas(Player* player) {
    int heart_size = 40;
    int spacing = 10;
    int start_x = 10;
    int start_y = 50;

    for (int i = 0; i < MAX_VIDAS; i++) {
        ALLEGRO_BITMAP* heart_bitmap = (i < player->vidas) ? heart_full : heart_empty;
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

void desenhar_hud_dicas(Player* player, ALLEGRO_FONT* font) {
    int icon_size = 30;
    int start_x = SCREEN_W - 150;
    int start_y = 10;

    if (dica_icon_small) {
        al_draw_scaled_bitmap(dica_icon_small,
            0, 0,
            al_get_bitmap_width(dica_icon_small),
            al_get_bitmap_height(dica_icon_small),
            start_x, start_y,
            icon_size, icon_size, 0);
    }

    char contador_text[20];
    sprintf_s(contador_text, sizeof(contador_text), "%d/%d", player->dicas_coletadas, MAX_DICAS);
    al_draw_text(font, al_map_rgb(255, 255, 255), start_x + icon_size + 10, start_y + 5, 0, contador_text);
}

void desenhar_dica_tela(Player* player, ALLEGRO_FONT* font) {
    if (mostrando_dica && dica_atual_tela < player->dicas_coletadas) {
        al_draw_filled_rectangle(0, 0, SCREEN_W, SCREEN_H, al_map_rgba(0, 0, 0, 200));

        ALLEGRO_BITMAP* dica_atual = dica_images[dica_atual_tela];
        if (dica_atual) {
            int img_w = al_get_bitmap_width(dica_atual);
            int img_h = al_get_bitmap_height(dica_atual);
            float scale = DICA_SCALE;
            float scaled_w = img_w * scale;
            float scaled_h = img_h * scale;
            float x = (SCREEN_W - scaled_w) / 2.0f;
            float y = (SCREEN_H - scaled_h) / 2.0f;

            al_draw_scaled_bitmap(dica_atual,
                0, 0, img_w, img_h,
                x, y, scaled_w, scaled_h, 0);

            if (player->dicas_coletadas > 1) {
                float btn_y = y + scaled_h + 20;

                if (dica_atual_tela > 0 && prev_button) {
                    al_draw_bitmap(prev_button, x - 60, btn_y, 0);
                }

                if (dica_atual_tela < player->dicas_coletadas - 1 && next_button) {
                    al_draw_bitmap(next_button, x + scaled_w + 10, btn_y, 0);
                }

                char pagina_text[20];
                sprintf_s(pagina_text, sizeof(pagina_text), "%d/%d", dica_atual_tela + 1, player->dicas_coletadas);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, btn_y + 15, ALLEGRO_ALIGN_CENTER, pagina_text);
            }

            al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H - 50, ALLEGRO_ALIGN_CENTER, "Clique fora da dica para fechar");
        }
    }
}

void desenhar_dicas_mundo() {
    for (int i = 0; i < MAX_DICAS; i++) {
        if (!dicas[i].coletada && dica_icon) {
            al_draw_bitmap(dica_icon, dicas[i].x - camera_x, dicas[i].y, 0);
        }
    }
}

bool verificar_colisao(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 &&
        x1 + w1 > x2 &&
        y1 < y2 + h2 &&
        y1 + h1 > y2);
}

void verificar_spawn_enemys(Player* player) {
    for (int i = 0; i < MAX_ENEMYS; i++) {
        if (!enemys[i].spawnado && enemys[i].area == player->area) {
            if (enemys[i].x - camera_x < SCREEN_W + 200) {
                enemys[i].ativo = true;
                enemys[i].spawnado = true;
            }
        }
    }
}

void verificar_coleta_dicas(Player* player, GameState* game_state) {
    for (int i = 0; i < MAX_DICAS; i++) {
        if (!dicas[i].coletada && dicas[i].area == player->area) {
            float dica_w = dica_icon ? al_get_bitmap_width(dica_icon) : 32;
            float dica_h = dica_icon ? al_get_bitmap_height(dica_icon) : 32;

            if (verificar_colisao(player->x, player->y, PLAYER_W, PLAYER_H,
                dicas[i].x, dicas[i].y, dica_w, dica_h)) {
                dicas[i].coletada = true;
                player->dicas_coletadas++;
                mostrando_dica = true;
                dica_atual_tela = player->dicas_coletadas - 1;
                if (game_state) {
                    *game_state = MOSTRANDO_DICA;
                }
                return;
            }
        }
    }
}

void atualizar_enemys(Player* player) {
    if (player->area == AREA_2) {
        return;
    }

    verificar_spawn_enemys(player);

    for (int i = 0; i < MAX_ENEMYS; i++) {
        if (enemys[i].ativo && enemys[i].spawnado && enemys[i].area == player->area) {

            enemys[i].x += ENEMY_VELOCIDADE * enemys[i].direcao;

            if (enemys[i].x + ENEMY_W < 0) {
                enemys[i].x = LEVEL_WIDTH - ENEMY_W;
            }
            else if (enemys[i].x > LEVEL_WIDTH) {
                enemys[i].x = 0 - ENEMY_W;
            }

            for (int j = 0; j < MAX_ENEMYS; j++) {
                if (i != j && enemys[j].ativo && enemys[j].spawnado && enemys[j].area == player->area) {
                    if (verificar_colisao(enemys[i].x, enemys[i].y, ENEMY_W, ENEMY_H,
                        enemys[j].x, enemys[j].y, ENEMY_W, ENEMY_H)) {
                        enemys[i].direcao *= -1;
                        enemys[j].direcao *= -1;
                        enemys[i].x += ENEMY_VELOCIDADE * enemys[i].direcao * 2;
                        enemys[j].x += ENEMY_VELOCIDADE * enemys[j].direcao * 2;
                        break;
                    }
                }
            }

            enemys[i].contador_frame++;
            if (enemys[i].contador_frame >= ENEMY_FRAME_DELAY) {
                enemys[i].frame_atual = (enemys[i].frame_atual + 1) % MAX_ENEMY_FRAMES;
                enemys[i].contador_frame = 0;
            }

            if (verificar_colisao(player->x, player->y, PLAYER_W, PLAYER_H,
                enemys[i].x, enemys[i].y, ENEMY_W, ENEMY_H)) {

                if (player->velocity_y > 0 &&
                    player->y + PLAYER_H < enemys[i].y + ENEMY_H / 2) {
                    enemys[i].ativo = false;
                    player->velocity_y = -JUMP_FORCE / 2;
                }
                else if (!player->invencivel && !player->morto) {
                    player->vidas--;
                    player->invencivel = true;
                    player->tempo_invencivel = al_get_time();

                    if (player->x < enemys[i].x) {
                        player->x -= 50;
                    }
                    else {
                        player->x += 50;
                    }

                    if (player->x < 0) player->x = 0;
                    if (player->x > LEVEL_WIDTH - PLAYER_W) player->x = LEVEL_WIDTH - PLAYER_W;

                    if (player->vidas <= 0) {
                        player->vidas = 0;
                        player->morto = true;
                        player->tempo_morte = al_get_time();
                    }
                }
            }
        }
    }
}

void desenhar_enemys(Player* player) {
    for (int i = 0; i < MAX_ENEMYS; i++) {
        if (enemys[i].ativo && enemys[i].spawnado && enemys[i].area == player->area) {
            ALLEGRO_BITMAP* sprite_atual = enemys[i].sprites[enemys[i].frame_atual];
            if (sprite_atual) {
                int flip_flag = (enemys[i].direcao == 1) ? ALLEGRO_FLIP_HORIZONTAL : 0;
                al_draw_bitmap(sprite_atual, enemys[i].x - camera_x, enemys[i].y, flip_flag);
            }
        }
    }
}

bool verificar_clique_botao_dica(int mouse_x, int mouse_y, Player* player) {
    if (!mostrando_dica || player->dicas_coletadas <= 1) return false;

    ALLEGRO_BITMAP* dica_atual = dica_images[dica_atual_tela];
    if (!dica_atual) return false;

    int img_w = al_get_bitmap_width(dica_atual);
    int img_h = al_get_bitmap_height(dica_atual);
    float scale = DICA_SCALE;
    float scaled_w = img_w * scale;
    float scaled_h = img_h * scale;
    float x = (SCREEN_W - scaled_w) / 2.0f;
    float y = (SCREEN_H - scaled_h) / 2.0f;
    float btn_y = y + scaled_h + 20;

    if (dica_atual_tela > 0 && mouse_x >= x - 60 && mouse_x <= x - 10 &&
        mouse_y >= btn_y && mouse_y <= btn_y + 50) {
        dica_atual_tela--;
        return true;
    }

    if (dica_atual_tela < player->dicas_coletadas - 1 &&
        mouse_x >= x + scaled_w + 10 && mouse_x <= x + scaled_w + 60 &&
        mouse_y >= btn_y && mouse_y <= btn_y + 50) {
        dica_atual_tela++;
        return true;
    }

    return false;
}

bool verificar_clique_fora_dica(int mouse_x, int mouse_y) {
    if (!mostrando_dica) return false;

    ALLEGRO_BITMAP* dica_atual = dica_images[dica_atual_tela];
    if (!dica_atual) return false;

    int img_w = al_get_bitmap_width(dica_atual);
    int img_h = al_get_bitmap_height(dica_atual);
    float scale = DICA_SCALE;
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
    ALLEGRO_FONT* font_pequena = al_load_ttf_font("./assets/pirulen.ttf", 24, 0);

    al_hide_mouse_cursor(display);

    ALLEGRO_BITMAP* player_frames_right[MAX_FRAMES] = { NULL };
    ALLEGRO_BITMAP* player_jump_frames_right[MAX_JUMP_FRAMES] = { NULL };

    int frames_carregados = 0;
    char filename[25];
    for (int i = 0; i < MAX_FRAMES; i++) {
        sprintf_s(filename, sizeof(filename), "./assets/knight%d.png", i + 1);
        player_frames_right[i] = al_load_bitmap(filename);
        if (!player_frames_right[i]) {
            printf("Aviso: Nao foi possivel carregar o arquivo %s (carregados %d frames)\n", filename, i);
            frames_carregados = i;
            break;
        }
        frames_carregados++;
    }

    if (frames_carregados == 0) {
        fprintf(stderr, "Erro: Nao foi possivel carregar nenhum sprite do knight\n");
        return -1;
    }

    printf("Carregados %d frames do knight\n", frames_carregados);

    int jump_frames_carregados = 0;
    for (int i = 0; i < MAX_JUMP_FRAMES; i++) {
        sprintf_s(filename, sizeof(filename), "./assets/jump%d.png", i + 1);
        player_jump_frames_right[i] = al_load_bitmap(filename);
        if (!player_jump_frames_right[i]) {
            printf("Aviso: Nao foi possivel carregar o arquivo %s (carregados %d frames de pulo)\n", filename, i);
            jump_frames_carregados = i;
            break;
        }
        jump_frames_carregados++;
    }

    if (jump_frames_carregados == 0) {
        printf("Aviso: Nao foi possivel carregar sprites de pulo, usando sprites normais\n");
    }
    else {
        printf("Carregados %d frames de pulo\n", jump_frames_carregados);
    }

    if (!criar_sprites_escalados(player_frames_right, player_frames_scaled, frames_carregados, PLAYER_SCALE)) {
        fprintf(stderr, "Erro ao criar sprites escalados\n");
        return -1;
    }

    if (jump_frames_carregados > 0) {
        if (!criar_sprites_escalados(player_jump_frames_right, player_jump_frames_scaled, jump_frames_carregados, PLAYER_SCALE)) {
            fprintf(stderr, "Erro ao criar sprites de pulo escalados\n");
        }
    }

    if (!carregar_mouse_sprite()) {
        al_show_mouse_cursor(display);
        mouse_visible = false;
    }

    if (!carregar_sprites_vidas()) {
        fprintf(stderr, "Erro ao carregar sprites de vidas\n");
        return -1;
    }

    if (!carregar_sprites_dicas()) {
        fprintf(stderr, "Aviso: Nao foi possivel carregar alguns sprites de dicas\n");
    }

    if (!carregar_porta()) {
        fprintf(stderr, "Aviso: Nao foi possivel carregar recursos da porta\n");
    }
    inicializar_porta();

    if (!carregar_game_over()) {
        fprintf(stderr, "Aviso: Nao foi possivel carregar game_over.png\n");
    }

    ALLEGRO_BITMAP* title_logo_bitmap = al_load_bitmap("./assets/title_logo.png");
    ALLEGRO_BITMAP* menu_background_bitmap = al_load_bitmap("./assets/menu_background.png");

    if (!font || !title_logo_bitmap || !menu_background_bitmap) {
        fprintf(stderr, "Erro ao carregar recursos\n");
        return -1;
    }

    GameState game_state;
    if (carregar_prologo()) {
        game_state = PROLOGO;
        tempo_prologo_inicio = al_get_time();
    }
    else {
        game_state = MENU;
    }

    if (!carregar_areas()) {
        return -1;
    }

    if (!carregar_botoes()) {
        fprintf(stderr, "Aviso: Nao foi possivel carregar botoes, usando fallback\n");
    }

    Player player = {
        100.0,
        GROUND_Y - PLAYER_H,
        0.0,
        false,
        AREA_1,
        MAX_VIDAS,
        false,
        0.0,
        false,
        0.0,
        NULL,
        0
    };

    if (!carregar_player_death_sprite(&player)) {
        fprintf(stderr, "Aviso: Nao foi possivel carregar sprite de morte do player\n");
    }

    inicializar_enemys();

    inicializar_dicas();

    int direction = 1;
    int current_frame = 0;
    int frame_count = 0;
    bool moving = false;

    int walking_frame_index = 1;
    int total_walking_frames = 0;

    int jump_frame_index = 0;
    bool showing_jump_animation = false;

    if (frames_carregados >= 9) {
        total_walking_frames = 8;
    }
    else {
        total_walking_frames = frames_carregados - 1;
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
    while (game_state != SAINDO) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            game_state = SAINDO;
        }

        if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
            mouse_x = event.mouse.x;
            mouse_y = event.mouse.y;
        }

        if (game_state == PROLOGO) {
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                    game_state = MENU;
                    prologo_atual = 0;
                }
            }

            if (al_get_time() - tempo_prologo_inicio >= 10.0) {
                prologo_atual++;
                if (prologo_atual >= 5) {
                    game_state = MENU;
                    prologo_atual = 0;
                }
                else {
                    tempo_prologo_inicio = al_get_time();
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
                    !player.is_jumping && !player.morto) {

                    if (jogador_perto_da_porta(&player)) {
                        if (!porta.aberta) {
                            porta.mostrando_dialogo = true;
                            porta.aguardando_resposta = true;
                            porta.pergunta_atual = 0;
                            porta.alternativa_selecionada = -1;
                            porta.fala_atual = FALA_PERGUNTA1;
                            porta.tempo_feedback_inicio = 0.0;
                            clear_keys(keys);
                        }
                        else {
                            game_state = END_SCREEN;
                            tempo_end_inicio = al_get_time();
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
                if (porta.mostrando_dialogo && porta.aguardando_resposta) {
                    int dlg_w = 700;
                    int dlg_h = 300;
                    int dlg_x = (SCREEN_W - dlg_w) / 2;
                    int dlg_y = (SCREEN_H - dlg_h) / 2;
                    int alt_x = dlg_x + 30;
                    int alt_w = dlg_w - 60;
                    int alt_start_y = dlg_y + 120;
                    int alt_h = 40;
                    int alt_spacing = 10;

                    for (int i = 0; i < MAX_ALTERNATIVAS; i++) {
                        int ay = alt_start_y + i * (alt_h + alt_spacing);
                        if (mouse_x >= alt_x && mouse_x <= alt_x + alt_w &&
                            mouse_y >= ay && mouse_y <= ay + alt_h) {
                            if (i == perguntas[porta.pergunta_atual].resposta_correta) {
                                porta.fala_atual = FALA_RESPOSTA_CERTA;
                                porta.aguardando_resposta = false;
                                porta.tempo_feedback_inicio = al_get_time();
                                if (porta.fase_abertura == 0) porta.fase_abertura = 1;
                            }
                            else {
                                porta.fala_atual = FALA_RESPOSTA_ERRADA;
                                porta.aguardando_resposta = false;
                                porta.tempo_feedback_inicio = al_get_time();
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
                        player.dicas_coletadas > 0) {
                        mostrando_dica = true;
                        dica_atual_tela = 0;
                        game_state = MOSTRANDO_DICA;
                        clear_keys(keys);
                    }
                }
            }

            if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
                if (porta.mostrando_dialogo) {
                    int dlg_w = 700;
                    int dlg_h = 300;
                    int dlg_x = (SCREEN_W - dlg_w) / 2;
                    int dlg_y = (SCREEN_H - dlg_h) / 2;
                    int alt_x = dlg_x + 30;
                    int alt_w = dlg_w - 60;
                    int alt_start_y = dlg_y + 120;
                    int alt_h = 40;
                    int alt_spacing = 10;

                    porta.alternativa_selecionada = -1;
                    for (int i = 0; i < MAX_ALTERNATIVAS; i++) {
                        int ay = alt_start_y + i * (alt_h + alt_spacing);
                        if (mouse_x >= alt_x && mouse_x <= alt_x + alt_w &&
                            mouse_y >= ay && mouse_y <= ay + alt_h) {
                            porta.alternativa_selecionada = i;
                            break;
                        }
                    }
                }
            }
        }
        else if (game_state == MOSTRANDO_DICA) {
            if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                if (verificar_clique_botao_dica(mouse_x, mouse_y, &player)) {
                }
                else if (verificar_clique_fora_dica(mouse_x, mouse_y)) {
                    mostrando_dica = false;
                    game_state = JOGANDO;
                    clear_keys(keys);
                }
            }
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    mostrando_dica = false;
                    game_state = JOGANDO;
                    clear_keys(keys);
                }
            }
        }
        else if (game_state == GAME_OVER) {
            if (al_get_time() - tempo_game_over_inicio >= 10.0) {
                game_state = MENU;
            }
        }
        else if (game_state == END_SCREEN) {
            if (al_get_time() - tempo_end_inicio >= 10.0) {
                game_state = MENU;
            }
        }

        if (event.type == ALLEGRO_EVENT_TIMER) {
            if (game_state == JOGANDO) {
                if (player.invencivel && al_get_time() - player.tempo_invencivel >= 2.0) {
                    player.invencivel = false;
                }

                if (player.morto && al_get_time() - player.tempo_morte >= 2.0) {
                    game_state = GAME_OVER;
                    tempo_game_over_inicio = al_get_time();
                    continue;
                }

                if (player.morto) {
                    redraw = true;
                    continue;
                }

                if (porta.fase_abertura > 0) {
                    atualizar_abertura_porta();
                }

                moving = false;

                if (!porta.mostrando_dialogo) {
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

                    if (showing_jump_animation && jump_frames_carregados > 0) {
                        frame_count++;
                        if (frame_count >= FRAME_DELAY) {
                            jump_frame_index++;
                            if (jump_frame_index >= jump_frames_carregados) {
                                jump_frame_index = jump_frames_carregados - 1;
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

                verificar_coleta_dicas(&player, &game_state);
                atualizar_enemys(&player);
                verificar_mudanca_area(&player);

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

                if (porta.mostrando_dialogo && !porta.aguardando_resposta) {
                    double now = al_get_time();
                    if (now - porta.tempo_feedback_inicio > 1.5) {
                        if (porta.fala_atual == FALA_RESPOSTA_CERTA) {
                            porta.mostrando_dialogo = false;
                            porta.aguardando_resposta = false;
                        }
                        else if (porta.fala_atual == FALA_RESPOSTA_ERRADA) {
                            porta.fala_atual = FALA_PERGUNTA1;
                            porta.aguardando_resposta = true;
                            porta.alternativa_selecionada = -1;
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

                draw_button(&play_button);
            }
            else if (game_state == JOGANDO || game_state == MOSTRANDO_DICA) {
                ALLEGRO_BITMAP* background_atual = (player.area == AREA_1) ? area1_background : area2_background;

                al_draw_scaled_bitmap(background_atual,
                    0, 0, al_get_bitmap_width(background_atual), al_get_bitmap_height(background_atual),
                    -camera_x, 0, LEVEL_WIDTH, SCREEN_H, 0);

                ALLEGRO_BITMAP* ground_atual = (player.area == AREA_1) ? ground1_bitmap : ground2_bitmap;
                float ground_w = al_get_bitmap_width(ground_atual);
                for (int i = 0; i * ground_w < LEVEL_WIDTH; i++) {
                    al_draw_bitmap(ground_atual, i * ground_w - camera_x, GROUND_Y, 0);
                }

                desenhar_dicas_mundo();

                if (player.area == AREA_2) {
                    desenhar_porta();
                }

                desenhar_enemys(&player);

                ALLEGRO_BITMAP* bitmap_to_draw = NULL;

                if (player.morto && player.death_sprite) {
                    bitmap_to_draw = player.death_sprite;
                }
                else if (player.is_jumping && jump_frames_carregados > 0) {
                    if (jump_frame_index < jump_frames_carregados && player_jump_frames_scaled[jump_frame_index]) {
                        bitmap_to_draw = player_jump_frames_scaled[jump_frame_index];
                    }
                }
                else {
                    if (current_frame < frames_carregados && player_frames_scaled[current_frame]) {
                        bitmap_to_draw = player_frames_scaled[current_frame];
                    }
                }

                if (bitmap_to_draw) {
                    int flip_flag = (player.morto ? 0 : (direction == -1) ? ALLEGRO_FLIP_HORIZONTAL : 0);
                    al_draw_bitmap(bitmap_to_draw, player.x - camera_x, player.y, flip_flag);
                }

                desenhar_vidas(&player);

                desenhar_hud_dicas(&player, font_pequena);

                al_draw_text(font, al_map_rgb(255, 255, 255), 10, 10, 0, "ESC para voltar");

                if (game_state == MOSTRANDO_DICA) {
                    desenhar_dica_tela(&player, font_pequena);
                }

                if (porta.mostrando_dialogo) {
                    int dlg_w = 700;
                    int dlg_h = 300;
                    int dlg_x = (SCREEN_W - dlg_w) / 2;
                    int dlg_y = (SCREEN_H - dlg_h) / 2;

                    al_draw_filled_rectangle(0, 0, SCREEN_W, SCREEN_H, al_map_rgba(0, 0, 0, 140));
                    al_draw_filled_rectangle(dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + dlg_h, al_map_rgb(30, 30, 60));
                    al_draw_rectangle(dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + dlg_h, al_map_rgb(200, 200, 200), 2);

                    Pergunta* p = &perguntas[porta.pergunta_atual];
                    draw_wrapped_text(font_pequena, p->pergunta, dlg_x + 20, dlg_y + 20, dlg_w - 40, 3, 26, al_map_rgb(255, 255, 255));

                    int alt_x = dlg_x + 30;
                    int alt_w = dlg_w - 60;
                    int alt_start_y = dlg_y + 120;
                    int alt_h = 40;
                    int alt_spacing = 10;

                    for (int i = 0; i < MAX_ALTERNATIVAS; i++) {
                        int ay = alt_start_y + i * (alt_h + alt_spacing);
                        ALLEGRO_COLOR bg;
                        if (!porta.aguardando_resposta && i == perguntas[porta.pergunta_atual].resposta_correta && porta.fala_atual == FALA_RESPOSTA_CERTA) {
                            bg = al_map_rgb(30, 150, 70);
                        } else {
                            bg = (i == porta.alternativa_selecionada) ? al_map_rgb(80, 120, 200) : al_map_rgb(60, 60, 80);
                        }
                        al_draw_filled_rectangle(alt_x, ay, alt_x + alt_w, ay + alt_h, bg);
                        al_draw_rectangle(alt_x, ay, alt_x + alt_w, ay + alt_h, al_map_rgb(200, 200, 200), 1);
                        al_draw_text(font_pequena, al_map_rgb(255, 255, 255), alt_x + 10, ay + 8, 0, p->alternativas[i]);
                    }

                    if (porta.fala_atual == FALA_RESPOSTA_ERRADA) {
                        al_draw_text(font_pequena, al_map_rgb(255, 80, 80), dlg_x + dlg_w / 2, dlg_y + dlg_h - 40, ALLEGRO_ALIGN_CENTER, "Errado! Tente novamente.");
                    }
                    else if (porta.fala_atual == FALA_RESPOSTA_CERTA) {
                        al_draw_text(font_pequena, al_map_rgb(80, 255, 120), dlg_x + dlg_w / 2, dlg_y + dlg_h - 40, ALLEGRO_ALIGN_CENTER, "Exato! Pode passar!");
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
                    al_draw_text(font_pequena, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 + 50, ALLEGRO_ALIGN_CENTER, "Voltando ao menu em 10 segundos...");
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
                    al_draw_text(font_pequena, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 + 50, ALLEGRO_ALIGN_CENTER, "Voltando ao menu em 10 segundos...");
                }
            }

            desenhar_mouse();
            al_flip_display();
        }
    }

    liberar_prologo();
    liberar_game_over();
    liberar_areas();
    liberar_botoes();
    liberar_mouse_sprite();
    liberar_sprites_vidas();
    liberar_sprites_enemys();
    liberar_sprites_dicas();
    liberar_player_death_sprite(&player);
    liberar_porta();
    liberar_sprites_escalados(player_frames_scaled, frames_carregados);
    liberar_sprites_escalados(player_jump_frames_scaled, jump_frames_carregados);

    for (int i = 0; i < frames_carregados; i++) {
        if (player_frames_right[i]) {
            al_destroy_bitmap(player_frames_right[i]);
        }
    }

    for (int i = 0; i < jump_frames_carregados; i++) {
        if (player_jump_frames_right[i]) {
            al_destroy_bitmap(player_jump_frames_right[i]);
        }
    }

    al_destroy_bitmap(title_logo_bitmap);
    al_destroy_font(font);
    al_destroy_font(font_pequena);
    al_destroy_bitmap(menu_background_bitmap);
    al_destroy_timer(timer);
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);
    return 0;
}
