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
    Uint8 r;
    Uint8 g;
    Uint8 b;
};

struct tetronimo {
    bool *base;
    int rot;
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

enum states{
    MENU_STATE,
    GAME_STATE,
    END_STATE
};

struct game_data {
    int level;
    int score;
    struct tetronimo current;
    bool *upcoming[14];
    struct block matrix[40][10];
};

//define shapes of tetronimoes
const bool O[3][4] = {{0, 1, 1, 0}, 
                      {0, 1, 1, 0}, 
                      {0, 0, 0, 0}};

const bool I[4][4] = {{0, 0, 0, 0}, 
                      {1, 1, 1, 1}, 
                      {0, 0, 0, 0},
                      {0, 0, 0, 0}};

const bool S[3][3] = {{0, 1, 1}, 
                      {1, 1, 0}, 
                      {0, 0, 0}};

const bool Z[3][3] = {{1, 1, 0}, 
                      {0, 1, 1}, 
                      {0, 0, 0}};

const bool L[3][3] = {{0, 0, 1}, 
                      {1, 1, 1}, 
                      {0, 0, 0}};

const bool J[3][3] = {{1, 0, 0}, 
                      {1, 1, 1}, 
                      {0, 0, 0}};

const bool T[3][3] = {{0, 1, 0}, 
                      {1, 1, 1}, 
                      {0, 0, 0}};

const bool *tetronimoes[7] = {&O[0][0], &I[0][0], &S[0][0], &Z[0][0], &L[0][0], &J[0][0], &T[0][0]};

enum states (*state_functions[3])(void *) = {(*menuRun), (*gameRun), (*endRun)};

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
void drawBlock(SDL_Renderer * renderer, int x, int y, Uint8 r, Uint8 g, Uint8 b) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_Rect rect = {x, y, SQUARE_SIZE, SQUARE_SIZE};
    SDL_RenderFillRect(renderer, &rect);
}

void drawMatrix(SDL_Renderer * renderer, struct block **matrix, int x, int y) {
    int i, j;
    for (i = 0; i < 20; i++) {
        for (j = 0; j < 10; j++) {
            if (matrix[i][j].exists) {
                drawBlock(renderer, x + j*SQUARE_SIZE,y + i*SQUARE_SIZE, matrix[i][j].r, matrix[i][j].g, matrix[i][j].b);
            }
        }
    }
}

void choicesArray(const bool *tetronimoes, bool *dest) {
    int i;
    for (i = 0; i < 7; i++) {
        dest[i] = tetronimoes[i];
    }
}

//here we generate a new array of 7 blocks and add those blocks to the array 'upcoming' from the point 'from'
void bag(bool *upcoming, int from) {
    bool choices[7];
    choicesArray(&tetronimoes[0][0], choices);
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

void updatePressed (struct presses *pressed) {
    SDL_Event e;
    struct presses just_pressed;
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
                    case SDLK_KP_ENTER:
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
                break;
            default:
                break;
        }
    }

}

void newCurrent(bool *upcoming[14], struct tetronimo *current) {
    current->base = upcoming[0];
    current->rot = 0;
    current->x = 3;
    current->y = 21;
    int i;
    for (i = 1; i < 14; i++) {
        upcoming[i-1] = upcoming[i];
    }
}

void render(SDL_Renderer *renderer) {
    //clear window
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    //draw on items
    drawBoard(renderer, WINDOW_WIDTH/2 - SQUARE_SIZE*5, WINDOW_HEIGHT/2 - SQUARE_SIZE*10, SQUARE_SIZE);
    

    //render to window
    SDL_RenderPresent(renderer);
}

void physics(int level, Uint64 *elapsed_time) {

    int gravity = (0.8-((level-1)*0.007));

}

void initGame() {}

int main() {
    //initialise
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)!= 0) {
        printf("error initialising SDL: %s\n", SDL_GetError());
        return 1;
    }

    //create window and renderer
    SDL_Window * win = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer * renderer = SDL_CreateRenderer(win, -1, 0);

    if (!win) {
        printf("error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    struct presses pressed;
    bool exit = false;
    enum states state = MENU_STATE;
    Uint64 elapsed_time = SDL_GetPerformanceCounter();
    struct game_data game;

    while (!exit) {
        updatePressed(&pressed);
        Uint64 start = SDL_GetPerformanceCounter();


        //while (game) {
            
          //  physics(level, &elapsed_time);
        render(renderer);
        //}

        //wait
        Uint64 end = SDL_GetPerformanceCounter();
        float elapsedMS = (end - start) / (float)SDL_GetPerformanceFrequency() * 1000.0f;
        if (!(elapsedMS > 16.666f)) {
            SDL_Delay(floor(16.666f - elapsedMS));
        }
    }

    printf("exit\n");

    //destroy window and renderer and uninitialise SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}