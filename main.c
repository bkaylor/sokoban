#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"

typedef enum {
    PLAY,
    QUIT
} Button_Type;

typedef struct {
    SDL_Rect rect;
    char *text;
    SDL_Color text_color;
    SDL_Color color;
    SDL_Color hovered_color;
    SDL_Color pressed_color;

    Button_Type type;

    bool hovered;
} Button;

typedef enum {
    GAME,
    LOADING,
    TITLE
} Mode;

typedef enum {
    FLOOR,
    WALL,
    GOAL,
} Tile_Type;

typedef struct {
    Tile_Type type;

    int x, y;

    bool has_player;
    bool has_box;
} Tile;

typedef enum {
    NORTH,
    EAST,
    WEST,
    SOUTH,
} Direction;

typedef enum {
    MOVE,
} Event_Type;

typedef struct {
    Event_Type type;
    union {
        Direction direction;
    };
    bool is_handled;
} Event;

typedef struct {
    Tile tiles[100][100];
    int w, h;
} Board;

typedef struct {
    bool quit;
    bool reset;

    Mode mode;

    struct {
        int x;
        int y;
    } window;

    struct {
        TTF_Font *title_font;
        TTF_Font *font;
        SDL_Color font_color;
        Button buttons[10];
        int button_count;

        struct {
            int x;
            int y;
        } mouse_position;

        bool clicked;
    } ui;

    struct {
        float total_time;
        float time_elapsed;
    } loading;

    Board board;

    Event events[100];
    int event_count;

    int level;

} Game_State;

typedef struct {
    char *filename;
    int rows;

    int width;
    int height;

    int row_lengths[100];
    SDL_Texture *texture;
} Sprite_Sheet;

typedef struct {
    char *sheet_name;
    int x, y;
} Sprite;

Sprite_Sheet sheet = {
    "../assets/sokoban_tilesheet.png",
    8,
    64, 64,
    {13, 13, 13, 13, 13, 13, 13, 13}
};

SDL_Texture *texture;

void load_image(SDL_Renderer *renderer, char *filename)
{
    SDL_Surface *surface = IMG_Load(filename);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
}

void load_images(SDL_Renderer *renderer)
{
    load_image(renderer, sheet.filename);
}

Event make_event(Event_Type type, Direction direction)
{
    return (Event) {
        type,
        direction,
        false
    };
}

void get_input(Game_State *game_state)
{
    SDL_GetMouseState(&game_state->ui.mouse_position.x, &game_state->ui.mouse_position.y);
    game_state->ui.clicked = false;
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        game_state->quit = true;
                        break;

                    case SDLK_r:
                        game_state->reset = true;
                        break;

                    case SDLK_w:
                        game_state->events[game_state->event_count] = make_event(MOVE, NORTH);
                        game_state->event_count += 1;
                        break;

                    case SDLK_a:
                        game_state->events[game_state->event_count] = make_event(MOVE, WEST);
                        game_state->event_count += 1;
                        break;

                    case SDLK_s:
                        game_state->events[game_state->event_count] = make_event(MOVE, SOUTH);
                        game_state->event_count += 1;
                        break;

                    case SDLK_d:
                        game_state->events[game_state->event_count] = make_event(MOVE, EAST);
                        game_state->event_count += 1;
                        break;

                    default:
                        break;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT ) {
                    game_state->ui.clicked = true;
                }

                /*
                if (event.button.button == SDL_BUTTON_RIGHT ) {
                    game_state->mouse_right_click = true;
                }
                */
                break;

            case SDL_QUIT:
                game_state->quit = true;
                break;

            default:
                break;
        }
    }
}

void apply_event(Event event, Board *board)
{
    switch (event.type)
    {
        case MOVE: {
            int x_increment = 0;
            int y_increment = 0;

            switch (event.direction)
            {
                case NORTH: x_increment = -1;    y_increment = 0;   break;
                case WEST:  x_increment = 0;     y_increment = -1;    break;
                case SOUTH: x_increment = 1;     y_increment = 0;    break;
                case EAST:  x_increment = 0;     y_increment = 1;    break;
                default: break;
            }

            Tile *player_tile;
            for (int i = 0; i < board->h; i += 1)
            {
                for (int j = 0; j < board->w; j += 1)
                {
                    if (board->tiles[i][j].has_player) player_tile = &board->tiles[i][j];
                }
            }

            int target_x = player_tile->x + x_increment;
            int target_y = player_tile->y + y_increment;
            Tile *target_tile = &board->tiles[target_x][target_y];

            if (target_tile->type == WALL) {
                return;
            } else if (target_tile->type == FLOOR || target_tile->type == GOAL) {
                if (target_tile->has_box) {
                    int next_target_x = target_tile->x + x_increment;
                    int next_target_y = target_tile->y + y_increment;
                    Tile *next_target_tile = &board->tiles[next_target_x][next_target_y];

                    if (next_target_tile->type == WALL) {
                        return;
                    } else if (next_target_tile->type == FLOOR || next_target_tile->type == GOAL) {
                        target_tile->has_box = false;
                        next_target_tile->has_box = true;

                        target_tile->has_player = true;

                        player_tile->has_player = false;
                    }
                } else {
                    player_tile->has_player = false;
                    target_tile->has_player = true;
                }
            }

        } break;

        default: {
        } break;
    }
}

bool handle_events(Game_State *game_state)
{
    bool did_something = false;

    int i = game_state->event_count-1;
    while (!game_state->events[i].is_handled)
    {
        apply_event(game_state->events[i], &game_state->board);
        game_state->events[i].is_handled = true;

        did_something = true;
    }

    return did_something;
}

void handle_button(Game_State *game_state, Button *button)
{
    switch (button->type)
    {
        case PLAY: {
            game_state->mode = LOADING;
            game_state->reset = true;
        } break;

        case QUIT: {
            game_state->quit = true;
        } break;

        default: {
        } break;
    }
}

bool populate_board_with_level(Board *board, int level_number)
{
    char level[50];
    sprintf(level, "../assets/levels/%d.txt", level_number);

    FILE *file = fopen(level, "r");

    if (!file) return false;

    char *data;
    int size;

    if (file) 
    {
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fseek(file, 0, SEEK_SET);

        data = malloc(size+1);
        fread(data, 1, size, file);
        data[size] = 0;

        fclose(file);
    }

    int columns = 0;
    while (data[columns] != '\n') {
        columns += 1;
    }

    int rows = 1;
    {
        int i = 0;
        while (i < size) {
            if (data[i] == '\n') rows += 1;

            i += 1;
        }
    }

    board->w = columns;
    board->h = rows;

    int raw_index = 0;

    int i = 0;
    while (i < rows)
    {
        int j = 0;
        while (j < columns)
        {
            Tile *tile = &board->tiles[i][j];

            if (data[raw_index] == '\n') {
                raw_index += 1;
                continue;
            }

            tile->has_player = false;
            tile->has_box = false;
            tile->x = i;
            tile->y = j;

            switch (data[raw_index]) {
                case '.':
                    tile->type = FLOOR;
                break;
                case 'w':
                    tile->type = WALL;
                break;
                case 'g':
                    tile->type = GOAL;
                break;
                case '@':
                    tile->type = FLOOR;
                    tile->has_player = true;
                break;
                case 'o':
                    tile->type = FLOOR;
                    tile->has_box = true;
                break;
                default:
                    tile->type = WALL;
                break;
            }

            raw_index += 1;
            j += 1;
        }

        i += 1;
    }

    return true;
}

bool check_win_conditions(Board board)
{
    for (int i = 0; i < board.h; i += 1)
    {
        for (int j = 0; j < board.w; j += 1)
        {
            if (board.tiles[i][j].type == GOAL && !board.tiles[i][j].has_box) {
                return false;
            }
        }
    }

    return true;
}

void update(Game_State *game_state, float delta_t)
{
    switch (game_state->mode)
    {
        case TITLE: {
            if (game_state->reset) {
                game_state->ui.button_count = 0;

                // Play button
                Button button;
                button.rect.w = 140;
                button.rect.h = 45;
                button.rect.x = game_state->window.x/2 - button.rect.w/2;
                button.rect.y = game_state->window.y/2;

                button.type = PLAY;
                button.text = "Play";
                button.text_color = (SDL_Color){255, 255, 255, 0};
                button.color = (SDL_Color){50, 50, 50, 0};

                game_state->ui.buttons[game_state->ui.button_count] = button;
                game_state->ui.button_count += 1;

                // Quit button
                button.rect.w = 140;
                button.rect.h = 45;
                button.rect.x = game_state->window.x/2 - button.rect.w/2;
                button.rect.y = game_state->window.y/2 + button.rect.h * 1.5;

                button.type = QUIT;
                button.text = "Quit";
                button.text_color = (SDL_Color){255, 255, 255, 0};
                button.color = (SDL_Color){50, 50, 50, 0};

                game_state->ui.buttons[game_state->ui.button_count] = button;
                game_state->ui.button_count += 1;

                game_state->reset = false;
            }

        } break;

        case GAME: {
            if (game_state->reset) {
                game_state->ui.button_count = 0;

                bool next_level_exists = populate_board_with_level(&game_state->board, game_state->level);

                if (!next_level_exists) {
                    game_state->mode = TITLE;
                    return;
                }

                game_state->reset = false;
            }

            if (game_state->event_count > 0) {
                bool did_something = handle_events(game_state);

                if (did_something) {
                    bool won = check_win_conditions(game_state->board);

                    if (won) {
                        game_state->level += 1;
                        game_state->reset = true;
                    }
                }
            }
        } break;

        case LOADING: {
            if (game_state->reset) {
                game_state->ui.button_count = 0;
                game_state->loading.total_time = 0.2;
                game_state->reset = false;
            }

            game_state->loading.time_elapsed += delta_t;

            if (game_state->loading.time_elapsed > game_state->loading.total_time) {
                game_state->mode = GAME;
                game_state->level = 1;
                game_state->reset = true;
            }
        } break;

        default: {
        } break;
    }

    // Check for button intersections
    for (int i = 0; i < game_state->ui.button_count; i += 1)
    {
        SDL_Rect mouse_position_rect = {
            game_state->ui.mouse_position.x,
            game_state->ui.mouse_position.y,
            1,
            1
        };

        SDL_Rect intersection_rect;

        if (SDL_IntersectRect(&mouse_position_rect, 
                              &game_state->ui.buttons[i].rect, 
                              &intersection_rect)) {
            game_state->ui.buttons[i].hovered = true;

            if (game_state->ui.clicked) {
                handle_button(game_state, &game_state->ui.buttons[i]);
            }
        } else {
            game_state->ui.buttons[i].hovered = false;
        }
    }
}

void draw_text(SDL_Renderer *renderer, int x, int y, char *string, TTF_Font *font, SDL_Color font_color) {
    SDL_Surface *surface = TTF_RenderText_Blended(font, string, font_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    int x_from_texture, y_from_texture;
    SDL_QueryTexture(texture, NULL, NULL, &x_from_texture, &y_from_texture);
    SDL_Rect rect = {x, y, x_from_texture, y_from_texture};

    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void draw_centered_text(SDL_Renderer *renderer, SDL_Rect rect, char *string, TTF_Font *font, SDL_Color font_color)
{
    SDL_Surface *surface = TTF_RenderText_Blended(font, string, font_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    int w_from_texture, h_from_texture;
    SDL_QueryTexture(texture, NULL, NULL, &w_from_texture, &h_from_texture);

    SDL_Rect draw_rect = {
        rect.x + rect.w/2 - w_from_texture/2, 
        rect.y + rect.h/2 - h_from_texture/2, 
        w_from_texture, 
        h_from_texture
    };

    SDL_RenderCopy(renderer, texture, NULL, &draw_rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void draw_button(SDL_Renderer *renderer, Game_State game_state, Button button)
{
    SDL_SetRenderDrawColor(renderer, 
                           button.color.r * 0.7, 
                           button.color.g * 0.7, 
                           button.color.b * 0.7, 
                           button.color.a);

    SDL_Rect shadow_rect = {
        button.rect.x + button.rect.w * 0.04,
        button.rect.y + button.rect.w * 0.04,
        button.rect.w,
        button.rect.h
    };

    SDL_RenderFillRect(renderer, &shadow_rect);

    if (button.hovered) {
        SDL_SetRenderDrawColor(renderer, 
                               button.color.r * 1.5, 
                               button.color.g * 1.5, 
                               button.color.b * 1.5, 
                               button.color.a);
    } else {
        SDL_SetRenderDrawColor(renderer, 
                               button.color.r, 
                               button.color.g, 
                               button.color.b, 
                               button.color.a);
    }

    SDL_RenderFillRect(renderer, &button.rect);
    draw_centered_text(renderer, 
              button.rect, 
              button.text,
              game_state.ui.font,
              button.text_color);
}

void draw_sprite(SDL_Renderer *renderer, SDL_Rect source, SDL_Rect destination)
{
    SDL_RenderCopy(renderer, texture, &source, &destination);
}

void render_game(SDL_Renderer *renderer, Game_State game_state)
{
    SDL_Rect viewport = {
        game_state.window.x * 0.1f,
        game_state.window.y * 0.1f,
        0,
        0,
    };

    SDL_Rect source;
    source.w = sheet.width;
    source.h = sheet.height;

    SDL_Rect second_layer_source;
    second_layer_source.w = sheet.width;
    second_layer_source.h = sheet.height;


    SDL_Rect destination = {
        viewport.x,
        viewport.y,
        sheet.width, sheet.height 
    };

    for (int i = 0; i < game_state.board.h; i += 1)
    {
        for (int j = 0; j < game_state.board.w; j += 1)
        {
            Tile tile = game_state.board.tiles[i][j];

            switch (tile.type) {
                case FLOOR:
                    source.x = 11 * source.w;
                    source.y = 6 * source.h;
                break;
                case WALL:
                    source.x = 6 * source.w;
                    source.y = 6 * source.h;
                break;
                case GOAL:
                    source.x = 11 * source.w;
                    source.y = 1 * source.h;
                break;
            }

            draw_sprite(renderer, source, destination);

            if (tile.has_box) {
                second_layer_source.x = 6 * second_layer_source.w;
                second_layer_source.y = 0 * second_layer_source.h;
                draw_sprite(renderer, second_layer_source, destination);
            }

            if (tile.has_player) {
                second_layer_source.x = 0 * second_layer_source.w;
                second_layer_source.y = 4 * second_layer_source.h;
                draw_sprite(renderer, second_layer_source, destination);
            }

            destination.x += destination.w; 
        }

        destination.y += destination.h;
        destination.x = viewport.x;
    }
}

void render_loading(SDL_Renderer *renderer, Game_State game_state)
{
    SDL_Rect loading_rect = {
        0,
        0,
        game_state.window.x,
        game_state.window.y/2
    };

    draw_centered_text(renderer, 
                       loading_rect, 
                       "Loading ...", 
                       game_state.ui.title_font, 
                       game_state.ui.font_color);
}

void render_title(SDL_Renderer *renderer, Game_State game_state)
{
    SDL_Rect title_rect = {
        0,
        0,
        game_state.window.x,
        game_state.window.y/2
    };

    draw_centered_text(renderer, 
                       title_rect, 
                       "Sokoban", 
                       game_state.ui.title_font, 
                       game_state.ui.font_color);

    for (int i = 0; i < game_state.ui.button_count; i += 1) {
        draw_button(renderer, game_state, game_state.ui.buttons[i]);
    }
}

void render(SDL_Renderer *renderer, Game_State game_state)
{
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderFillRect(renderer, NULL);

    switch (game_state.mode)
    {
        case GAME:
            render_game(renderer, game_state);
        break;
        case LOADING:
            render_loading(renderer, game_state);
        break;
        case TITLE:
        default:
            render_title(renderer, game_state);
        break;
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init video error: %s\n", SDL_GetError());
        return 1;
    }

    if (SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        printf("SDL_Init audio error: %s\n", SDL_GetError());
        return 1;
    }

	// Setup window
	SDL_Window *window = SDL_CreateWindow("Sokoban",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			800, 800,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	// Setup renderer
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Setup font
	TTF_Init();
	TTF_Font *font = TTF_OpenFont("Constantia.ttf", 24);
	TTF_Font *title_font = TTF_OpenFont("Constantia.ttf", 60);
	if (!font)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error: Font", TTF_GetError(), window);
		return -666;
	}
	SDL_Color font_color = {255, 255, 255};

    srand(time(NULL));

    Game_State game_state;
    game_state.quit = false;
    game_state.reset = true;
    game_state.mode = TITLE;
    game_state.ui.font = font;
    game_state.ui.title_font = title_font;
    game_state.ui.font_color = font_color;
    game_state.event_count = 0;
    game_state.level = 1;

    load_images(renderer);

    int frame_time_start, frame_time_finish;
    float delta_t = 0;

    while (!game_state.quit)
    {
        frame_time_start = SDL_GetTicks();

        SDL_PumpEvents();
        get_input(&game_state);

        if (!game_state.quit)
        {
            SDL_GetWindowSize(window, &game_state.window.x, &game_state.window.y);

            update(&game_state, delta_t);
            render(renderer, game_state);

            frame_time_finish = SDL_GetTicks();
            delta_t = (float)((frame_time_finish - frame_time_start) / 1000.0f);
        }
    }

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

    return 0;
}
