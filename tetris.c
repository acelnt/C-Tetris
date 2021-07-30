#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

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

bool matrix[40][10];

int main() {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)!= 0) {
        printf("error initialising SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Hello World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    if (!win) {
        printf("error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}