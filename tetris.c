#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_video.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define SQUARE_SIZE 20

struct block{
    bool exists;
    SDL_Colour col;
};

struct tetronimo {
    bool base[4][4];
    char type;
    int x;
    int y;
};

struct presses {
    bool rotc;
    bool sdrop;
    bool right;
    bool left;
    bool enter;
    bool hdrop;
    bool rota;
    bool rot180;
    bool hold;
    bool quit;
};
struct presses presses_default = {false, false, false, false, false, false, false, false, false, false};

enum states{
    MENU_STATE,
    GAME_STATE,
    END_STATE
};

const struct block_colours {
    SDL_Colour I;
    SDL_Colour Z;
    SDL_Colour S;
    SDL_Colour T;
    SDL_Colour L;
    SDL_Colour J;
    SDL_Colour O;
} block_colours = {
    {0, 0, 255, 255},
    {255, 0, 0, 255},
    {0, 255, 0, 255},
    {255, 0, 255, 255},
    {255, 165, 0, 255},
    {0, 0, 100, 255},
    {255, 255, 0, 255}
};

struct game_data {
    int level;
    int score;
    struct tetronimo current;
    struct tetronimo *upcoming[14];
    struct block matrix[40][10];
};

struct assets {
    SDL_Texture *logo;
    SDL_Texture *play_button;
};

struct pos {
    int x;
    int y;
};

struct assets assets;

//define shapes of tetronimoes
const bool O[4][4] = {{0, 1, 1, 0}, 
                      {0, 1, 1, 0}, 
                      {0, 0, 0, 0},
                      {0, 0, 0, 0}};

const bool I[4][4] = {{0, 0, 0, 0}, 
                      {1, 1, 1, 1}, 
                      {0, 0, 0, 0},
                      {0, 0, 0, 0}};

const bool S[4][4] = {{0, 1, 1, 0}, 
                      {1, 1, 0, 0}, 
                      {0, 0, 0, 0},
                      {0, 0, 0, 0}};

const bool Z[4][4] = {{1, 1, 0, 0}, 
                      {0, 1, 1, 0}, 
                      {0, 0, 0, 0},
                      {0, 0, 0, 0}};

const bool L[4][4] = {{0, 0, 1, 0}, 
                      {1, 1, 1, 0}, 
                      {0, 0, 0, 0},
                      {0, 0, 0, 0}};

const bool J[4][4] = {{1, 0, 0, 0}, 
                      {1, 1, 1, 0}, 
                      {0, 0, 0, 0},
                      {0, 0, 0, 0}};

const bool T[4][4] = {{0, 1, 0, 0}, 
                      {1, 1, 1, 0}, 
                      {0, 0, 0, 0},
                      {0, 0, 0, 0}};

const bool tetronimoes[7][4][4] = {O, I, S, Z, L, J, T};
const char names[7] = {'O', 'I', 'S', 'Z', 'L', 'J', 'T'};

void emptyMatrix(struct block matrix[40][10]) {
    int i, j;
    for (i = 0; i < 40; i++) {
        for (j = 0; j < 10; j++) {
            matrix[i][j].exists = true;
        }
    }
}

//function to draw empty board
void drawBoard(SDL_Renderer * renderer, int x, int y, int size) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);

    int i;
    for (i = 0; i < 11; i++) {
        SDL_RenderDrawLine(renderer, x + i*size, y, x +i*size, y+size*20);
    }
    for (i = 0; i < 21; i++) {
        SDL_RenderDrawLine(renderer, x, y+size*i, x +10*size, y+size*i);
    }

    //makes it so that there is a 2 pixel wide border
    SDL_Point border[5] = {{x-1, y-1}, {x-1, y+20*size+1}, {x+10*size+1, y + 20*size+1}, {x+10*size+1, y-1}, {x-1, y-1}};
    SDL_RenderDrawLines(renderer, border, 5);
}

//function to draw a single filled square
void drawBlock(SDL_Renderer * renderer, int x, int y, SDL_Colour col) {
    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
    SDL_Rect rect = {x, y, SQUARE_SIZE, SQUARE_SIZE};
    SDL_RenderFillRect(renderer, &rect);
}

void drawMatrix(SDL_Renderer * renderer, struct block **matrix, int x, int y) {
    int i, j;
    for (i = 0; i < 20; i++) {
        for (j = 0; j < 10; j++) {
            if (matrix[i][j].exists) {
                drawBlock(renderer, x + j*SQUARE_SIZE,y + (20-i)*SQUARE_SIZE, matrix[i][j].col);
            }
        }
    }
}

void drawTetronimo(SDL_Renderer *renderer, struct tetronimo tetronimo, int x, int y) {
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (tetronimo.base[i][j]) {
                drawBlock(renderer, x + j*SQUARE_SIZE, y + i*SQUARE_SIZE, getColour(tetronimo.type));
            }
        }
    }
}

void choicesArray(const bool tetronimoes[7], bool *dest) {
    int i;
    for (i = 0; i < 7; i++) {
        dest[i] = tetronimoes[i];
    }
}

//here we generate a new array of 7 blocks and add those blocks to the array 'upcoming' from the point 'from'
void extendUpcoming(bool *upcoming[], int from) {
    bool choices[7];
    choicesArray(tetronimoes, &choices);
    char choice_names[7];
    choicesArray(names, &choice_names);
    int i;
    for (i = 0; i < 20; i++) {
        int first = rand() %7;
        bool value = choices[first];
        int second = rand() %7;
        choices[first] = choices[second];
        choices[second] = value;
    }
    for (i = 0; i < 7; i++) {
        upcoming[from + i] = choices[i];
    }
}

struct presses updatePressed (struct presses *pressed) {
    SDL_Event e;
    struct presses just_pressed = presses_default;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        pressed->rotc = true;
                        just_pressed.rotc = true;
                        break;
                    case SDLK_DOWN:
                        pressed->sdrop = true;
                        just_pressed.sdrop = true;
                        break;
                    case SDLK_LEFT:
                        pressed->left = true;
                        just_pressed.left = true;
                        break;
                    case SDLK_RIGHT:
                        pressed->right = true;
                        just_pressed.right = true;
                        break;
                    case SDLK_RETURN:
                        pressed->enter = true;
                        just_pressed.enter = true;
                        break;
                    case SDLK_SPACE:
                        pressed->hdrop = true;
                        just_pressed.hdrop = true;
                        break;
                    case SDLK_z:
                        pressed->rota = true;
                        just_pressed.rota = true;
                        break;
                    case SDLK_x:
                        pressed->rot180 = true;
                        just_pressed.rot180 = true;
                        break;
                    case SDLK_RSHIFT:
                        pressed->hold = true;
                        just_pressed.hold = true;
                        break;
                    case SDLK_ESCAPE:
                        pressed->quit = true;
                        just_pressed.quit = true;
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        pressed->rotc = false;
                        break;
                    case SDLK_DOWN:
                        pressed->sdrop = false;
                        break;
                    case SDLK_LEFT:
                        pressed->left = false;
                        break;
                    case SDLK_RIGHT:
                        pressed->right = false;
                        break;
                    case SDLK_KP_ENTER:
                        pressed->enter = false;
                        break;
                    case SDLK_SPACE:
                        pressed->hdrop = false;
                        break;
                    case SDLK_z:
                        pressed->rota = false;
                        break;
                    case SDLK_x:
                        pressed->rot180 = false;
                        break;
                    case SDLK_RSHIFT:
                        pressed->hold = false;
                        break;
                    case SDLK_ESCAPE:
                        pressed->quit = false;
                        break;
                }
                break;
            case SDL_QUIT:
                pressed->quit = true;
                just_pressed.quit = true;
                break;
            default:
                break;
        }
    }
    return just_pressed;
}

struct SDL_Colour getColour(char type) {
    switch(type) {
        case 'I':
            return block_colours.I;
    }
}

void newCurrent(bool *upcoming[], struct tetronimo *current) {
    static int count = 0;
    count += 1;
    bool *next = &current->base;
    next = upcoming[0];
    current->x = 3;
    current->y = 21;
    int i;
    for (i = 1; i < 14; i++) {
        upcoming[i-1] = upcoming[i];
    }
    if (count == 7) {
        count = 0;
        extendUpcoming(upcoming, 7);
    }
}

void physics(int level, Uint64 *elapsed_time) {

    int gravity = (0.8-((level-1)*0.007));

}

SDL_Texture* loadTexture(SDL_Renderer *renderer, const char *path) {
    SDL_Surface *surface = IMG_Load(path);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);
    
    return texture;
}

void renderTexture(SDL_Texture *texture, SDL_Renderer *renderer, int x, int y, int w, int h){
	//Setup the destination rectangle to be at the position we want

	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
    dst.w = w;
    dst.h = h;
	//Query the texture to get its width and height to use

	SDL_RenderCopy(renderer, texture, NULL, &dst);
}

void clearScreen(SDL_Renderer *renderer, SDL_Colour col) {
    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
    SDL_RenderClear(renderer);
}

void render(SDL_Renderer *renderer) {
    SDL_RenderPresent(renderer);
}

void preloadAssets(SDL_Renderer *renderer) {
    assets.logo = loadTexture(renderer, "logo.png");
    assets.play_button = loadTexture(renderer, "play button.png");
}

void initGame(struct game_data *data) {
    extendUpcoming(data->upcoming, 0);
    extendUpcoming(data->upcoming, 7);
    newCurrent(data->upcoming, &(data->current));
    data->level = 1;
    emptyMatrix(data->matrix);
    data->score = 0;
}

enum states gameRun(SDL_Renderer *renderer, struct game_data *data, struct presses pressed, struct presses just_pressed, Uint64 elapsed_time) {
    struct pos board_pos = {WINDOW_WIDTH/2-SQUARE_SIZE*5, WINDOW_HEIGHT/2-SQUARE_SIZE*10};
    drawBoard(renderer, board_pos.x, board_pos.y, SQUARE_SIZE);
    //drawMatrix(renderer, &data->matrix, WINDOW_WIDTH/2-SQUARE_SIZE*5, WINDOW_HEIGHT/2-SQUARE_SIZE*10);
    drawTetronimo(renderer, data->current, board_pos.x, board_pos.y);

    return GAME_STATE;
}

enum states menuRun(SDL_Renderer *renderer, struct presses pressed) {
    renderTexture(assets.logo, renderer, WINDOW_WIDTH/4, WINDOW_HEIGHT/6, WINDOW_WIDTH/2, WINDOW_WIDTH/2*795/957);
    renderTexture(assets.play_button, renderer, WINDOW_WIDTH/3, WINDOW_HEIGHT/6 + WINDOW_WIDTH/2*795/957*0.8, WINDOW_WIDTH/3, WINDOW_WIDTH/3*766/1352);
    if (pressed.enter) {
        return GAME_STATE;
    }
    return MENU_STATE;
}

enum states endRun(SDL_Renderer *renderer, struct presses pressed) {

}

int main() {
    //initialise
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)!= 0) {
        printf("error initialising SDL: %s\n", SDL_GetError());
        return 1;
    }

    //create window and renderer
    SDL_Window * win = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!win) {
        printf("error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer * renderer = SDL_CreateRenderer(win, -1, 0);
    if (!renderer) {
        printf("error creating renderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    preloadAssets(renderer);

    struct presses pressed = presses_default;
    struct presses just_pressed = presses_default;
    bool exit = false;
    enum states state = MENU_STATE;
    Uint64 elapsed_time = SDL_GetPerformanceCounter();
    struct game_data data;

    while (!exit) {
        just_pressed = updatePressed(&pressed);

        if (pressed.quit) {
            exit = true;
        }

        Uint64 start = SDL_GetPerformanceCounter();

        clearScreen(renderer, (SDL_Colour) {255, 255, 255, 255});

        switch (state) {
            //each function carries out one frame and returns the enum of what the next state should be
            case MENU_STATE:
                state = menuRun(renderer, just_pressed);
                if (state == GAME_STATE) {
                    initGame(&data);
                }
                break;
            case GAME_STATE:
                state = gameRun(renderer, &data, pressed, just_pressed, elapsed_time);
                break;
            case END_STATE:
                state = endRun(renderer, just_pressed);
                break;
        }

        render(renderer);

        //wait
        Uint64 end = SDL_GetPerformanceCounter();
        float elapsedMS = (end - start) / (float)SDL_GetPerformanceFrequency() * 1000.0f;
        if (!(elapsedMS > 16.666f)) {
            SDL_Delay(floor(16.666f - elapsedMS));
        }
    }

    //destroy window and renderer and uninitialise SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}