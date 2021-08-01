#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
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
    bool up;
    bool down;
    bool right;
    bool left;
    bool start;
    
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

const bool *tetronimoes[7] = {&O, &I, &S, &Z, &L, &J, &T};

void emptyMatrix(struct block **matrix) {
    int i, j;
    for (i = 0; i < 40; i++) {
        for (j = 0; j < 10; j++) {
            matrix[i][j].exists = false;
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

bool bag(bool *upcoming, int from) {
    bool *choices[7] = tetronimoes;
    int i;
    for (i = 0; i < 20; i++) {
        int first = rand() %7;
        bool *value = choices[first];
        int second = rand() %7;
        choices[first] = choices[second];
        choices[second] = *value;
    }
    for (i = 0; i < 7; i++) {
        upcoming[from + i] = choices[i];
    }
}

void newCurrent(struct tetronimo *current) {

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

void eventHandling(bool *exit) {
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            exit = true;
        }
    }
}

void physics(int level, Uint64 *elapsed_time) {

    int gravity = (0.8-((level-1)*0.007));

}

void gameEventHandling(bool *game, int *score) {

}

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
    
    int level = 1;
    int score = 0;
    Uint64 elapsed_time = SDL_GetPerformanceCounter();
    struct tetronimo current;
    bool *upcoming[14];
    struct block matrix[40][10];
    emptyMatrix(matrix);

    bool exit = false;
    bool game = true;
    while (!exit) {
        Uint64 start = SDL_GetPerformanceCounter();

        eventHandling(&exit);

        while (game) {
            move
            physics(level, &elapsed_time);
            render(renderer);
        }

        //wait
        Uint64 end = SDL_GetPerformanceCounter();
        float elapsedMS = (end - start) / (float)SDL_GetPerformanceFrequency() * 1000.0f;
        SDL_Delay(floor(16.666f - elapsedMS));
    }

    //destroy window and renderer and uninitialise SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}