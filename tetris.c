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
#define SDROP_GRAVITY 0.1
#define LOCK_DELAY 0.5
#define DAS 0.133
#define DAS_MOVE_SPEED 0.05

struct block{
    bool exists;
    SDL_Colour col;
};

struct tetromino {
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
} BLOCK_COLOURS = {
    {0, 255, 255, 255},
    {255, 0, 0, 255},
    {0, 255, 0, 255},
    {200, 0, 200, 255},
    {255, 165, 0, 255},
    {0, 0, 255, 255},
    {255, 255, 0, 255}
};

struct game_data {
    int level;
    int score;
    struct tetromino current;
    struct tetromino upcoming[14];
    struct block matrix[40][10];
    double last_drop;
    double right_das;
    double left_das;
    double last_das_move;
    bool locking;
    double started_locking;
};

struct assets {
    SDL_Texture *logo;
    SDL_Texture *play_button;
};

struct pos {
    float x;
    float y;
};

const struct block_rotations {
    struct pos I;
    struct pos Z;
    struct pos S;
    struct pos T;
    struct pos L;
    struct pos J;
    struct pos O;
} BLOCK_ROTATIONS = {
    {2, 2},
    {1.5, 1.5},
    {1.5, 1.5},
    {1.5, 1.5},
    {1.5, 1.5},
    {1.5, 1.5},
    {2, 1}
};

struct assets assets;

//define shapes of tetrominoes
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

bool tetrominoes[7][4][4];
const char names[7] = {'I', 'T', 'Z', 'S', 'L', 'J', 'O'};

void emptyMatrix(struct block matrix[40][10]) {
    int i, j;
    for (i = 0; i < 40; i++) {
        for (j = 0; j < 10; j++) {
            matrix[i][j].exists = false;
        }
    }
}

SDL_Colour getBlockColour(char type) {
    switch(type) {
        case 'I':
            return BLOCK_COLOURS.I;
        case 'T':
            return BLOCK_COLOURS.T;
        case 'L':
            return BLOCK_COLOURS.L;
        case 'J':
            return BLOCK_COLOURS.J;
        case 'S':
            return BLOCK_COLOURS.S;
        case 'Z':
            return BLOCK_COLOURS.Z;
        case 'O':
            return BLOCK_COLOURS.O;
        default:
            return (SDL_Colour) {0, 0, 0, 255};
    }
}

//function to draw empty board
void drawBoard(SDL_Renderer * renderer, struct pos board_pos, int size) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);

    int i;
    for (i = 0; i < 11; i++) {
        SDL_RenderDrawLine(renderer, board_pos.x + i*size, board_pos.y, board_pos.x +i*size, board_pos.y+size*20);
    }
    for (i = 0; i < 21; i++) {
        SDL_RenderDrawLine(renderer, board_pos.x, board_pos.y+size*i, board_pos.x +10*size, board_pos.y+size*i);
    }

    //makes it so that there is a 2 pixel wide border
    SDL_Point border[5] = {{board_pos.x-1, board_pos.y-1}, {board_pos.x-1, board_pos.y+20*size+1}, {board_pos.x+10*size+1, board_pos.y + 20*size+1}, {board_pos.x+10*size+1, board_pos.y-1}, {board_pos.x-1, board_pos.y-1}};
    SDL_RenderDrawLines(renderer, border, 5);
}

//function to draw a single filled square
void drawBlock(SDL_Renderer * renderer, struct pos pos, SDL_Colour col) {
    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
    SDL_Rect rect = {pos.x, pos.y, SQUARE_SIZE, SQUARE_SIZE};
    SDL_RenderFillRect(renderer, &rect);
}

void drawMatrix(SDL_Renderer * renderer, struct block matrix[][10], struct pos board_pos) {
    int i, j;
    for (i = 0; i < 20; i++) {
        for (j = 0; j < 10; j++) {
            if (matrix[i][j].exists) {
                drawBlock(renderer, (struct pos) {board_pos.x + j*SQUARE_SIZE, board_pos.y + (19-i)*SQUARE_SIZE}, matrix[i][j].col);
            }
        }
    }
}

void drawShape(SDL_Renderer *renderer, bool piece[4][4], struct pos pos, SDL_Colour col) {
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (piece[i][j]) {
                drawBlock(renderer, (struct pos) {pos.x + j*SQUARE_SIZE, pos.y + i*SQUARE_SIZE}, col);
            }
        }
    }
}

void duplicateBase(bool base[][4], bool target[][4]) {
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            target[i][j] = base[i][j];
        }
    }
}

void initTetrominoes() {
    duplicateBase(I, tetrominoes[0]);
    duplicateBase(T, tetrominoes[1]);
    duplicateBase(Z, tetrominoes[2]);
    duplicateBase(S, tetrominoes[3]);
    duplicateBase(L, tetrominoes[4]);
    duplicateBase(J, tetrominoes[5]);
    duplicateBase(O, tetrominoes[6]);
}

void getTetrominoes(bool tetrs[][4][4]) {
    int i;
    for (i = 0; i < 7; i++) {
        duplicateBase(tetrominoes[i], tetrs[i]);
    }
}

void getTetrominoesNames(char *nams) {
    int i;
    for (i = 0; i < 7; i++) {
        nams[i] = names[i];
    }
}

//here we generate a new array of 7 blocks and add those blocks to the array 'upcoming' from the point 'from'
void extendUpcoming(struct tetromino upcoming[], int from) {
    struct tetromino selection[7];
    bool choices[7][4][4];
    getTetrominoes(&choices);

    char choice_names[7];
    getTetrominoesNames(&choice_names);

    int length;
    for (length = 0; length < 7; length++) {
        int random = rand() % (7-length);
        struct tetromino temp;

        duplicateBase(&choices[random], &temp.base);
        temp.type = choice_names[random];
        temp.y = 20;
        temp.x = 3;

        selection[length] = temp;

        int i;
        for (i = random; i < 7; i++) {
            duplicateBase(choices[i+1], choices[i]);
            choice_names[i] = choice_names[i + 1];
        }
    }
    int i;
    for (i = 0; i < 7; i++) {
        upcoming[from + i] = selection[i];
    }
}

struct pos rotationPoint(char type) {
    switch(type) {
        case 'I':
            return BLOCK_ROTATIONS.I;
        case 'T':
            return BLOCK_ROTATIONS.T;
        case 'L':
            return BLOCK_ROTATIONS.L;
        case 'J':
            return BLOCK_ROTATIONS.J;
        case 'S':
            return BLOCK_ROTATIONS.S;
        case 'Z':
            return BLOCK_ROTATIONS.Z;
        case 'O':
            return BLOCK_ROTATIONS.O;
        default:         
            return (struct pos) {1.5, 1.5};
    }
}

struct presses updatePressed (struct presses *pressed) {
    SDL_Event e;
    struct presses just_pressed = presses_default;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_KEYDOWN:
                if (e.key.repeat == 0) {
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
                }
                break;
            case SDL_KEYUP:
                if (e.key.repeat == 0) {
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

void clearShape(bool piece[4][4]) {
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            piece[i][j] = false;
        }
    }
}

void rotateShape(bool base[4][4], char type, bool new_shape[4][4], signed int amount) {
    clearShape(new_shape);
    struct pos rot_point = rotationPoint(type);
    amount = amount % 4;
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (base[i][j]) {
                float y = i + 0.5 - rot_point.y;
                float x = j + 0.5 - rot_point.x;
                switch (amount) {
                    case 0:
                        new_shape[(int)(rot_point.y + y - 0.5)][(int)(rot_point.x + x - 0.5)] = true;
                        break;
                    case 1:
                        new_shape[(int)(rot_point.y + x - 0.5)][(int)(rot_point.x - y - 0.5)] = true;
                        break;
                    case 2:
                        new_shape[(int)(rot_point.y - y - 0.5)][(int)(rot_point.x - x - 0.5)] = true;
                        break;
                    case 3:
                        new_shape[(int)(rot_point.y - x - 0.5)][(int)(rot_point.x + y - 0.5)] = true;
                        break;
                }
            }
        }
    }
}

bool newCurrent(struct tetromino upcoming[], struct tetromino *current) {
    static int count = 0;
    count += 1;
    duplicateBase(upcoming[0].base, current->base);
    current->y = upcoming[0].y;
    current->x = upcoming[0].x;
    current->type = upcoming[0].type;
    int i;
    for (i = 1; i < 14; i++) {
        upcoming[i-1] = upcoming[i];
    }
    if (count == 7) {
        count = 0;
        extendUpcoming(upcoming, 7);
    }
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

void initGame(struct game_data *data, double elapsed_time) {
    extendUpcoming(data->upcoming, 0);
    extendUpcoming(data->upcoming, 7);
    newCurrent(data->upcoming, &(data->current));
    data->level = 1;
    emptyMatrix(data->matrix);
    data->score = 0;
    data->last_drop = elapsed_time;
    data->locking = false;
    data->started_locking = elapsed_time;
    data->last_das_move = elapsed_time;
}

void drawGhost(SDL_Renderer *renderer, struct block matrix[40][10], struct tetromino piece, struct pos board_pos) {
    int offset = getDroppedPos(matrix, piece);
    SDL_Colour col = getBlockColour(piece.type);
    col.r = col.r/2;
    col.g = col.g/2;
    col.b = col.b/2;
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (piece.base[i][j]) {
                drawBlock(renderer, (struct pos) {board_pos.x + piece.x*SQUARE_SIZE + j*SQUARE_SIZE, board_pos.y + (19-piece.y+offset)*SQUARE_SIZE + i*SQUARE_SIZE}, col);
            }
        }
    }
}

void drawUpcoming(SDL_Renderer *renderer, struct tetromino upcoming[14], struct pos board_pos) {
    board_pos.x = board_pos.x + 11*SQUARE_SIZE;
    board_pos.y = board_pos.y + SQUARE_SIZE;
    int i;
    for (i = 0; i < 5; i++) {
        drawShape(renderer, upcoming[i].base, board_pos, getBlockColour(upcoming[i].type));
        board_pos.y = board_pos.y + SQUARE_SIZE*4;
    }
}

void drawGame(SDL_Renderer *renderer, struct block matrix[40][20], struct tetromino current, struct tetromino upcoming[14]) {
    struct pos board_pos = {WINDOW_WIDTH/2-SQUARE_SIZE*5, WINDOW_HEIGHT/2-SQUARE_SIZE*10};
    drawBoard(renderer, board_pos, SQUARE_SIZE);
    drawMatrix(renderer, matrix, board_pos);
    drawGhost(renderer, matrix, current, board_pos);
    drawShape(renderer, current.base, (struct pos) {board_pos.x + current.x*SQUARE_SIZE, board_pos.y + (19-current.y)*SQUARE_SIZE}, getBlockColour(current.type));
    drawUpcoming(renderer, upcoming, board_pos);
}

bool collides(struct block matrix[40][10], bool shape[4][4], struct pos pos) {
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (shape[i][j]) {
                //printf("%i\n", current.y - i - offset_y);
                if (pos.x + j < 0 || pos.x + j > 9 || pos.y - i < 0 || pos.y - i > 40 || matrix[(int) pos.y - i][(int) pos.x + j].exists) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool tryDrop(int level, struct tetromino *current, struct block matrix[40][10], double *last_drop, double elapsed_time, bool sdrop) {
    float gravity;
    if (!sdrop) {
        gravity = (0.8-((level-1)*0.007));
    } else {
        gravity = SDROP_GRAVITY;
    }
    if (elapsed_time > *last_drop + gravity) {
        *last_drop = elapsed_time;

        if (!collides(matrix, current->base, (struct pos) {current->x, current->y - 1})) {
            current->y = current->y - 1;
        } else {
            return false;
        }
    }
    return true;
}

bool overflows(struct tetromino piece) {
    bool result = true;
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (piece.base[i][j]) {
                if (i < 20) {
                    result = false;
                }
            }
        }
    }
    return result;
}

bool lockPiece(struct tetromino piece, struct block matrix[40][10]) {
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (piece.base[i][j]) {
                matrix[piece.y - i][piece.x + j].exists = true;
                matrix[piece.y - i][piece.x + j].col = getBlockColour(piece.type);
            }
        }
    }
    return overflows(piece);
}

int getDroppedPos(struct block matrix[40][10], struct tetromino piece) {
    int drop = 0;
    while (true) {
        if (collides(matrix, piece.base, (struct pos) {piece.x, piece.y - drop})) {
            return drop - 1;
        }
        drop = drop + 1;
    }
}

int getDASsedPos(struct block matrix[40][10], struct tetromino piece, int direction) {
    int move = 0;
    while (true) {
        if (collides(matrix, piece.base, (struct pos) {piece.x + move, piece.y})) {
            return piece.x + move - direction;
        }
        move = move + direction;
    }
}

bool lock_piece(struct tetromino *piece, struct block matrix[40][10], struct tetromino upcoming[14]) {
    if (lockPiece(*piece, matrix)) {
        return false;
    }
    newCurrent(upcoming, piece);
    if (collides(matrix, piece->base, (struct pos) {piece->x, piece->y})) {
        return false;
    }
    return true;
}

bool gameKeyboardHandling(struct game_data *data, struct presses pressed, struct presses just_pressed, double elapsed_time) {
    if (pressed.left) {
        if (elapsed_time > data->left_das + DAS) {
            if (DAS_MOVE_SPEED == 0) {
                data->current.x = getDASsedPos(data->matrix, data->current, -1);
            }
            else if (elapsed_time > data->last_das_move + DAS_MOVE_SPEED) {
                if(!collides(data->matrix, data->current.base, (struct pos) {data->current.x - 1, data->current.y})) {
                    data->current.x = data->current.x - 1;
                    data->last_das_move = elapsed_time;
                }
            }
        } else {
            data->last_das_move = elapsed_time;
        }
    } else {
        data->left_das = elapsed_time;
    }

    if (pressed.right) {
        if (elapsed_time > data->right_das + DAS) {
            if (DAS_MOVE_SPEED == 0) {
                data->current.x = getDASsedPos(data->matrix, data->current, 1);
            }
            else if (elapsed_time > data->last_das_move + DAS_MOVE_SPEED) {
                if(!collides(data->matrix, data->current.base, (struct pos) {data->current.x + 1, data->current.y})) {
                    data->current.x = data->current.x + 1;
                    data->last_das_move = elapsed_time;
                }
            }
        } else {
            data->last_das_move = elapsed_time;
        }
    } else {
        data->right_das = elapsed_time;
    }

    if (just_pressed.left) {
        if (!collides(data->matrix, data->current.base, (struct pos) {data->current.x - 1, data->current.y})) {
            data->current.x = data->current.x - 1;
            data->locking = false;
        }
    }

    if (just_pressed.right) {
        if (!collides(data->matrix, data->current.base, (struct pos) {data->current.x + 1, data->current.y})) {
            data->current.x = data->current.x + 1;
            data->locking = false;
        }
    }

    if (just_pressed.rotc || just_pressed.rota || just_pressed.rot180) {
        int amount;
        if (just_pressed.rotc) {
            amount = 1;
        } else if (just_pressed.rota) {
            amount = 3;
        } else if (just_pressed.rot180) {
            amount = 2;
        }
        bool new_shape[4][4];
        rotateShape(data->current.base, data->current.type, new_shape, amount);
        if (!collides(data->matrix, new_shape, (struct pos) {data->current.x, data->current.y})) {
            duplicateBase(new_shape, data->current.base);
            data->locking = false;
        }
    }

    if (just_pressed.hdrop) {
        data->current.y = data->current.y - getDroppedPos(data->matrix, data->current);
        if (!lock_piece(&data->current, data->matrix, data->upcoming)) {
            return false;
        }
        data->locking = false;
    }
}

bool gameGravity(struct game_data *data, struct presses pressed, double elapsed_time) {
    tryDrop(data->level, &data->current, data->matrix, &data->last_drop, elapsed_time, pressed.sdrop);

    if (collides(data->matrix, data->current.base, (struct pos) {data->current.x, data->current.y - 1})) {
        if (data->locking) {
            if (elapsed_time > data->started_locking + LOCK_DELAY) {
                if (!lock_piece(&data->current, data->matrix, data->upcoming)) {
                    return false;
                }
                data->locking = false;
            }
        } else {
            data->locking = true;
            data->started_locking = elapsed_time;
        }
    } else {
        data->locking = false;
    }
    return true;
}

enum states gameRun(SDL_Renderer *renderer, struct game_data *data, struct presses pressed, struct presses just_pressed, double elapsed_time) {
    if (!gameGravity(data, pressed, elapsed_time)) {
        return END_STATE;
    }

    if (!gameKeyboardHandling(data, pressed, just_pressed, elapsed_time)) {
        return END_STATE;
    }
    
    drawGame(renderer, data->matrix, data->current, data->upcoming);    

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

    srand(time(NULL) );

    preloadAssets(renderer);
    initTetrominoes();

    struct presses pressed = presses_default;
    struct presses just_pressed = presses_default;
    bool exit = false;
    enum states state = MENU_STATE;
    double elapsed_time;
    struct game_data data;

    while (!exit) {
        elapsed_time = SDL_GetPerformanceCounter()/(float)SDL_GetPerformanceFrequency();
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
                    initGame(&data, elapsed_time);
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