// Uso:  ./tetris           corre los tests y despues el juego
//       ./tetris --test    corre solo los tests (valgrind --leak-check=full ./tetris --test)

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ========= CONSTANTES DEL JUEGO =========
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 800
#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_SIZE 35
#define OFFSET_X 75
#define OFFSET_Y 50
#define TICK_RATE 30

const int TETROMINOS[7][4][4][4] = {
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// ========= ESTRUCTURAS DE DATOS =========

// 1. Nodo para la fila del tablero (Lista Enlazada)
typedef struct RowNode {
    int cells[GRID_COLS];
    struct RowNode* next;
} RowNode;

// 2. Nodo para la cola de próximas piezas (Queue)
typedef struct PieceQueueNode {
    int type;
    struct PieceQueueNode* next;
} PieceQueueNode;

// 3. Nodo para el sistema de partículas
typedef struct ParticleNode {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    struct ParticleNode* next;
} ParticleNode;

typedef struct ActivePiece {
    int col, row;
    int type;
    int rotation;
    int active;
} ActivePiece;

typedef struct GameBoard {
    RowNode* first_row;
    PieceQueueNode* queue_head;
    PieceQueueNode* queue_tail;
    ParticleNode* first_particle;
    ActivePiece current_piece;
    int score;
    int tick_timer;
} GameBoard;

// ========= VARIABLES GLOBALES =========
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameBoard* game_board = NULL;

// ========= EJERCICIO 1: STRINGS (NO USAR string.h) =========

// Cuenta los caracteres de s sin incluir el '\0' (reemplaza a strlen).
int strLen(char* s){
    if (s==NULL){
        return 0;
    }
    int i=0;
    while (s[i]!='\0'){
        i++;
    }
    return i;
}

// Reserva largo + 1 bytes: el +1 es el lugar del terminador '\0'.
char* strDuplicate(char* src) {
    if (src==NULL){
        return NULL;
    }
    int largo= strLen(src);
    char* newStr= (char*) malloc(sizeof(char) * (largo + 1));
    if (newStr==NULL){
        return NULL;
    }
    for (int i=0; i<largo; i++){
        newStr[i]= src[i];
    }
    newStr[largo]= '\0';
    return newStr;
}

// Convencion inversa a strcmp: 1 si s1 < s2, -1 si s1 > s2, 0 si son iguales.
int strCompare(char* s1, char* s2) {
    int lens1= strLen(s1);
    int lens2= strLen(s2);
    int i=0;
    //Compara carácter por carácter mientras ambas cadenas tengan contenido
    while (i<lens1 && i<lens2){
        if (s1[i] < s2[i]){
            return 1;
        }
        if (s1[i] > s2[i]){
            return -1;
        }
        i++;
    }
    //Chequeo si los strings son idénticos o uno terminó siendo prefijo del otro.
    if (lens1 == lens2){
        return 0;
    } else if (lens1 < lens2){
        return 1;
    } else {
        return -1;
    }
}

// Devuelve un string nuevo (malloc) y libera src1 y src2: ambos deben venir de malloc
// y ser punteros distintos. Si el malloc falla se liberan igual, porque la funcion
// siempre es duena de sus argumentos.
char* strConcatenate(char* src1, char* src2) {
    int lensrc1= strLen(src1);
    int lensrc2= strLen(src2);
    char* res= (char*) malloc(sizeof(char)* (lensrc1 + lensrc2 + 1));
    if (res==NULL){
        free(src1);
        free(src2);
        return NULL;
    }
    int k=0;
    //Copio el contenido del primer string
    for (int i=0; i<lensrc1; i++){
        res[k]= src1[i];
        k++;
    }
    //Copio el contenido del segundo string
    for (int j=0; j<lensrc2; j++){
        res[k]= src2[j];
        k++;
    }
    res[k]= '\0';
    free(src1);
    free(src2);
    return res;
}

// ========= EJERCICIO 2: FUNCIONES A IMPLEMENTAR (EL TP) =========

// Tablero = lista enlazada de 20 filas (malloc una por una). Si algun malloc falla, se
// liberan las filas ya creadas y el board, y se devuelve NULL (sin dejar memoria perdida).
GameBoard* gameBoardNew() {
    GameBoard* board= (GameBoard*) malloc(sizeof(GameBoard));
    if (board==NULL){
        return NULL;
    }
    board->first_row= NULL;
    board->queue_head= NULL;
    board->queue_tail= NULL;
    board->first_particle= NULL;
    board->current_piece.col = 0;
    board->current_piece.row = 0;
    board->current_piece.type = 0;
    board->current_piece.rotation = 0;
    board->current_piece.active = 0;
    board->score= 0;
    board->tick_timer= 0;
    RowNode* last= NULL;
    for (int i = 0; i < GRID_ROWS; i++) {
        // Reservar memoria para un nodo de fila
        RowNode* new_row = (RowNode*) malloc(sizeof(RowNode));
        // Si falla malloc, liberamos lo reservado hasta el momento
        if (new_row == NULL) {
            RowNode* curr = board->first_row;
            while (curr != NULL) {
                RowNode* next = curr->next;
                free(curr);
                curr = next;
            }
            free(board);
            return NULL;
        }
        // Inicializamos las 10 celdas de la fila en 0 (vacías)
        for (int c = 0; c < GRID_COLS; c++) {
            new_row->cells[c] = 0;
        }
        new_row->next = NULL;
        // Conectamos el nodo en la lista enlazada
        if (board->first_row == NULL) {
            board->first_row = new_row; // El nodo creado es la primera fila
        } else {
            last->next = new_row;       // Enlazamos la fila anterior/última con la nueva
        }
        last = new_row;
    }
    return board;
}

// En cada lista se guarda el siguiente nodo ANTES de hacer free (no se lee un nodo ya liberado).
void gameBoardDelete(GameBoard* board) {
    if (board==NULL){
        return;
    }
    RowNode* curr_row= board->first_row;
    while (curr_row != NULL){
        RowNode* temp= curr_row;
        curr_row= curr_row->next;
        free(temp);
    }
    PieceQueueNode* curr_piece= board->queue_head;
    while (curr_piece != NULL){
        PieceQueueNode* temp= curr_piece;
        curr_piece= curr_piece->next;
        free(temp);
    }
    ParticleNode* curr_particle= board->first_particle;
    while (curr_particle != NULL){
        ParticleNode* temp= curr_particle;
        curr_particle= curr_particle->next;
        free(temp);
    }
    free(board);
}

// Inserta una particula al frente de la lista: es O(1) y no hace falta puntero al final.
void gameBoardAddParticle(GameBoard* board, float x, float y, int color) {
    if (board== NULL){
        return;
    }
    ParticleNode* newParticle= (ParticleNode*) malloc(sizeof(ParticleNode));
    if (newParticle==NULL){
        return;
    }
    newParticle->x = x;
    newParticle->y = y;

    // vel_x: Flotante aleatorio entre -5.0f y +4.9f para dispersar las partículas horizontalmente con fluidez.
    newParticle->vel_x = ((rand() % 100) / 10.0f) - 5.0f;

    // vel_y: Impulso inicial negativo (hacia arriba) entre -2.0f y -6.9f.
    // Se complementa con la gravedad (vel_y += 0.5f) que aplica gameBoardUpdate() en cada tick.
    newParticle->vel_y = ((rand() % 50) / -10.0f) - 2.0f;

    newParticle->color = color;

    /* Evitamos poner un número fijo para que no desaparezcan todas las partículas de golpe,
    usaremos una duración aleatoria entre 30 y 59 ticks para una desaparición progresiva. */
    newParticle->life_timer=  30 + (rand() % 30);

    newParticle->next= board->first_particle;
    board->first_particle= newParticle;
}

// Borra las filas completas solo re-enlazando punteros (no se copian celdas).
// Se recorre con prev/curr; las filas vacias de reemplazo se reservan antes de desenlazar y se
// insertan en la cabeza al final, asi el tablero siempre queda con exactamente 20 filas.
int gameBoardCheckAndClearLines(GameBoard* board) {
    if (board == NULL) {
        return 0;
    }
    int cleared = 0;
    int index = 0;                 // indice de la fila actual (para la posicion de las particulas)
    RowNode* prev = NULL;          // ultima fila que NO se borro (predecesora de curr)
    RowNode* curr = board->first_row;

    // Filas vacias de reemplazo. Se juntan en una lista aparte y se insertan en la cabeza
    // recien al terminar el recorrido, para no mezclar la insercion con el desenlazado.
    RowNode* blanks = NULL;
    RowNode* blanks_tail = NULL;

    while (curr != NULL) {
        // Guardamos next ANTES de hacer free(curr): despues de liberar no se puede leer curr->next.
        RowNode* next = curr->next;

        int is_full = 1;
        for (int c = 0; c < GRID_COLS; c++) {
            if (curr->cells[c] == 0) {
                is_full = 0;
                break;
            }
        }

        // Pedimos la fila de reemplazo ANTES de desenlazar: si malloc falla, la fila llena
        // queda en su lugar y el tablero conserva siempre exactamente 20 filas.
        RowNode* blank = NULL;
        if (is_full) {
            blank = (RowNode*) malloc(sizeof(RowNode));
        }

        if (blank != NULL) {
            for (int c = 0; c < GRID_COLS; c++) {
                blank->cells[c] = 0;
                gameBoardAddParticle(board,
                                     OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2,
                                     OFFSET_Y + index * CELL_SIZE + CELL_SIZE / 2,
                                     curr->cells[c]);
            }
            // Desenlazar: si era la primera fila se actualiza first_row, si no, el next del anterior.
            if (prev == NULL) {
                board->first_row = next;
            } else {
                prev->next = next;
            }
            free(curr);
            cleared++;

            blank->next = blanks;
            blanks = blank;
            if (blanks_tail == NULL) {
                blanks_tail = blank;
            }
            // prev no cambia: sigue siendo la ultima fila que quedo en la lista.
        } else {
            prev = curr;
        }
        curr = next;
        index++;
    }

    // Insertar las filas vacias arriba de todo. Las filas que estaban arriba de las borradas
    // "bajan" solas, porque su posicion es su lugar en la lista.
    if (blanks != NULL) {
        blanks_tail->next = board->first_row;
        board->first_row = blanks;
    }
    return cleared;
}

// Cola FIFO con punteros a head y tail: encolar y desencolar son O(1).
void gameBoardEnqueuePiece(GameBoard* board, int type) {
    if (board==NULL){
        return;
    }
    PieceQueueNode* newPiece= (PieceQueueNode*) malloc(sizeof(PieceQueueNode));
    if (newPiece==NULL){
        return;
    }
    newPiece->type= type;
    newPiece->next= NULL;

    if (board->queue_head == NULL){ //Si la lista está vacía
        board->queue_head= newPiece;
        board->queue_tail= newPiece;
    } else { //Si la lista ya tiene elementos
        board->queue_tail->next= newPiece;
        board->queue_tail= newPiece;
    }
}

int gameBoardDequeuePiece(GameBoard* board, int* out_type) {
    if (board== NULL || out_type == NULL || board->queue_head == NULL){
        // Si el tablero no existe, no hay dónde guardar el tipo o la cola está vacía, no hacemos nada.
        return 0;
    }

    PieceQueueNode* temp= board->queue_head;

    *out_type = board->queue_head->type;
    board->queue_head= board->queue_head->next;
    if (board->queue_head == NULL){ // Si la cola quedó vacía, actualizar el final.
        board->queue_tail= NULL;
    }
    free(temp); //Liberamos la memoria del nodo extraído.
    return 1; //Retornamos 1 indicando éxito.
}

// ========= LOGICA DE DIBUJADO Y COLISIONES (NO MODIFICAR) =========

RowNode* getRowAt(GameBoard* board, int index) {
    RowNode* curr = board->first_row;
    for(int i = 0; i < index && curr != NULL; i++) curr = curr->next;
    return curr;
}

int canMove(GameBoard* board, int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[board->current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0) {
                    RowNode* row_node = getRowAt(board, board_row);
                    if (row_node && row_node->cells[board_col] != 0) return 0;
                }
            }
        }
    }
    return 1;
}

void lockPiece(GameBoard* board) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][r][c];
            if (val != 0) {
                RowNode* row_node = getRowAt(board, board->current_piece.row + r);
                if (row_node) row_node->cells[board->current_piece.col + c] = val;
            }
        }
    }
    board->current_piece.active = 0;
    board->score += gameBoardCheckAndClearLines(board) * 100;
}

// Devuelve 0 si la pieza nueva no entra (GAME OVER), 1 en otro caso.
int spawnNextPiece(GameBoard* board) {
    int type;
    if (gameBoardDequeuePiece(board, &type)) {
        board->current_piece.type = type;
        board->current_piece.rotation = 0;
        board->current_piece.col = GRID_COLS / 2 - 2;
        board->current_piece.row = 0;
        board->current_piece.active = 1;
        gameBoardEnqueuePiece(board, rand() % 7);
        if (!canMove(board, board->current_piece.col, board->current_piece.row, board->current_piece.rotation)) {
            printf("GAME OVER. Puntaje final: %d\n", board->score);
            board->current_piece.active = 0;
            return 0;   // main sale del loop y libera tablero y SDL
        }
    }
    return 1;
}

// Devuelve 0 cuando termina la partida.
int gameBoardUpdate(GameBoard* board) {
    if (!board) return 0;
    if (!board->current_piece.active && !spawnNextPiece(board)) return 0;

    board->tick_timer++;
    if (board->tick_timer >= TICK_RATE) {
        board->tick_timer = 0;
        if (canMove(board, board->current_piece.col, board->current_piece.row + 1, board->current_piece.rotation)) {
            board->current_piece.row++;
        } else {
            lockPiece(board);
        }
    }

    ParticleNode* curr_part = board->first_particle;
    ParticleNode* prev_part = NULL;
    while (curr_part) {
        curr_part->x += curr_part->vel_x;
        curr_part->y += curr_part->vel_y;
        curr_part->vel_y += 0.5f;
        curr_part->life_timer--;

        if (curr_part->life_timer <= 0) {
            ParticleNode* to_delete = curr_part;
            if (prev_part == NULL) {
                board->first_particle = curr_part->next;
                curr_part = board->first_particle;
            } else {
                prev_part->next = curr_part->next;
                curr_part = prev_part->next;
            }
            free(to_delete);
        } else {
            prev_part = curr_part;
            curr_part = curr_part->next;
        }
    }
    return 1;
}

void gameBoardDraw(GameBoard* board) {
    if (!board) return;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    RowNode* current_row = board->first_row;
    int r = 0;
    while (current_row) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (current_row->cells[c] > 0) {
                SDL_Color col = colors[current_row->cells[c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
        current_row = current_row->next;
        r++;
    }

    if (board->current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int cell_val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][pr][pc];
                if (cell_val != 0) {
                    SDL_Color col = colors[cell_val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (board->current_piece.col + pc) * CELL_SIZE,
                                       OFFSET_Y + (board->current_piece.row + pr) * CELL_SIZE,
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    ParticleNode* curr_part = board->first_particle;
    while (curr_part) {
        SDL_Color col = colors[curr_part->color];
        int alpha = curr_part->life_timer * 8;   // se satura: SDL recibe un Uint8
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
        SDL_Rect part_rect = {(int)curr_part->x, (int)curr_part->y, 6, 6};
        SDL_RenderFillRect(renderer, &part_rect);
        curr_part = curr_part->next;
    }
    SDL_RenderPresent(renderer);
}

int inicializar() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base - TP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}

// ========= TESTS =========

// Cuenta las filas de la lista (el tablero debe tener siempre GRID_ROWS).
static int contarFilas(GameBoard* board) {
    int n = 0;
    for (RowNode* r = board->first_row; r != NULL; r = r->next) n++;
    return n;
}

// Llena todas las celdas de la fila de indice idx con un valor distinto de 0.
static void llenarFila(GameBoard* board, int idx) {
    RowNode* row = getRowAt(board, idx);
    for (int c = 0; c < GRID_COLS; c++) row->cells[c] = 1;
}

static int filaVacia(RowNode* row) {
    for (int c = 0; c < GRID_COLS; c++) {
        if (row->cells[c] != 0) return 0;
    }
    return 1;
}

static void resultado(int ok, char* nombre) {
    if (ok) {
        printf("[OK] %s\n", nombre);
    } else {
        printf("[ERROR] %s\n", nombre);
    }
}

void correrTests() {
    printf("===== CORRIENDO TESTS: EJERCICIO 1 (STRINGS) =====\n");

    //1. Test de la función strDuplicate
    char* d_vacio = strDuplicate("");
    char* d_un_char = strDuplicate("x");
    char* d_varios = strDuplicate("Tetris C para TD2");
    // Verifico que los punteros recibidos no sean NULL y que el contenido sea idéntico
    if (d_vacio != NULL && strCompare(d_vacio, "") == 0 &&
        d_un_char != NULL && strCompare(d_un_char, "x") == 0 &&
        d_varios != NULL && strCompare(d_varios, "Tetris C para TD2") == 0) {
        printf("[OK] strDuplicate pasa los 3 casos requeridos.\n");
    } else {
        printf("[ERROR] strDuplicate falló en algún caso.\n");
    }
    // Liberamos la memoria pedida por strDuplicate
    free(d_vacio);
    free(d_un_char);
    free(d_varios);

    //2. Test de la función strCompare
    int test_iguales_vacios = (strCompare("", "") == 0);
    int test_iguales_un_char = (strCompare("a", "a") == 0);
    int test_menor = (strCompare("a", "b") == 1);  // "a" es menor a "b" -> devuelve 1
    int test_mayor = (strCompare("b", "a") == -1); // "b" es mayor a "a" -> devuelve -1
    int test_prefijo = (strCompare("casa", "casas") == 1 && strCompare("casas", "casa") == -1);
    if (test_iguales_vacios && test_iguales_un_char && test_menor && test_mayor && test_prefijo) {
        printf("[OK] strCompare pasa todas las comparaciones.\n");
    } else {
        printf("[ERROR] strCompare falló en alguna comparación.\n");
    }

    //3. Test de la función strConcatenate
    char* c1 = strConcatenate(strDuplicate(""), strDuplicate("abc"));
    char* c2 = strConcatenate(strDuplicate("hola "), strDuplicate("mundo"));

    if (c1 != NULL && strCompare(c1, "abc") == 0 &&
        c2 != NULL && strCompare(c2, "hola mundo") == 0) {
        printf("[OK] strConcatenate pasa las dos concatenaciones.\n");
    } else {
        printf("[ERROR] strConcatenate falló.\n");
    }
    free(c1);
    free(c2);
    printf("===== FIN DE TESTS DEL EJERCICIO 1 =====\n");

    printf("===== CORRIENDO TESTS: EJERCICIO 2 (TABLERO) =====\n");

    // Cola: 5 push y 6 pop validando el orden FIFO
    GameBoard* b = gameBoardNew();
    if (b == NULL) {
        printf("[ERROR] gameBoardNew devolvio NULL.\n");
        return;
    }
    resultado(contarFilas(b) == GRID_ROWS && b->queue_head == NULL && b->queue_tail == NULL &&
              b->first_particle == NULL && b->score == 0 && b->current_piece.active == 0,
              "gameBoardNew: 20 filas y campos inicializados");

    for (int i = 1; i <= 5; i++) gameBoardEnqueuePiece(b, i);
    int fifo_ok = 1;
    int tipo = -1;
    for (int i = 1; i <= 5; i++) {
        if (!gameBoardDequeuePiece(b, &tipo) || tipo != i) fifo_ok = 0;
    }
    resultado(fifo_ok, "Cola: 5 push y 5 pop respetan el orden FIFO");

    tipo = 99;
    int sexto = gameBoardDequeuePiece(b, &tipo);
    resultado(sexto == 0 && tipo == 99 && b->queue_head == NULL && b->queue_tail == NULL,
              "Cola: el 6to pop devuelve 0, no toca out_type y head/tail quedan en NULL");

    gameBoardEnqueuePiece(b, 6);
    resultado(gameBoardDequeuePiece(b, &tipo) == 1 && tipo == 6 &&
              b->queue_head == NULL && b->queue_tail == NULL,
              "Cola: un push despues de quedar vacia se puede extraer");

    // Una sola linea: fila 19, con una marca en la 18 que debe bajar a la 19
    RowNode* vieja_cima = b->first_row;
    llenarFila(b, 19);
    getRowAt(b, 18)->cells[3] = 5;
    int borradas = gameBoardCheckAndClearLines(b);
    resultado(borradas == 1, "Lineas: llenar la fila 19 borra 1 linea");
    resultado(b->first_row != vieja_cima && filaVacia(b->first_row),
              "Lineas: la fila nueva vacia es ahora first_row");
    resultado(contarFilas(b) == GRID_ROWS, "Lineas: la lista sigue teniendo 20 filas");
    resultado(getRowAt(b, 19)->cells[3] == 5 && filaVacia(getRowAt(b, 18)),
              "Lineas: la fila de arriba bajo un lugar");
    resultado(gameBoardCheckAndClearLines(b) == 0 && contarFilas(b) == GRID_ROWS,
              "Lineas: sin filas llenas no borra nada");

    // Dos lineas a la vez, incluyendo la primera de la lista (filas 0 y 1)
    llenarFila(b, 0);
    llenarFila(b, 1);
    borradas = gameBoardCheckAndClearLines(b);
    resultado(borradas == 2, "Lineas: llenar las filas 0 y 1 borra 2 lineas");
    resultado(contarFilas(b) == GRID_ROWS && filaVacia(b->first_row),
              "Lineas: tras borrar 2 filas la lista sigue teniendo 20 filas");

    // Dos lineas separadas por una fila incompleta (el caso que se saltea con arrays)
    llenarFila(b, 10);
    llenarFila(b, 12);
    borradas = gameBoardCheckAndClearLines(b);
    resultado(borradas == 2 && contarFilas(b) == GRID_ROWS,
              "Lineas: dos lineas no consecutivas se borran y quedan 20 filas");

    // Las particulas se crearon en lista (10 por linea) y gameBoardDelete las libera
    int n_part = 0;
    for (ParticleNode* p = b->first_particle; p != NULL; p = p->next) n_part++;
    resultado(n_part == 10 * (1 + 2 + 2), "Particulas: 10 por cada linea borrada, sin limite fijo");

    // Delete libera todo, incluso una cola con piezas pendientes
    gameBoardEnqueuePiece(b, 1);
    gameBoardEnqueuePiece(b, 2);
    gameBoardDelete(b);
    gameBoardDelete(NULL);
    printf("[OK] gameBoardDelete libera tablero, cola y particulas (verificar con valgrind)\n");
    printf("===== FIN DE TESTS DEL EJERCICIO 2 =====\n");
}

// Compara contra "--test" sin depender de string.h ni del Ejercicio 1.
int esFlagTest(char* arg) {
    char* flag = "--test";
    int i = 0;
    while (arg[i] != '\0' && arg[i] == flag[i]) i++;
    return arg[i] == '\0' && flag[i] == '\0';
}

int main(int argc, char* args[]) {
    srand(time(NULL));

    // Los tests corren antes de SDL. Con --test termina aca, asi valgrind solo
    // reporta la memoria del TP (SDL tiene bloques propios que no se liberan).
    correrTests();
    if (argc > 1 && esFlagTest(args[1])) return 0;

    if (!inicializar()) return 1;

    game_board = gameBoardNew();
    if (game_board == NULL) {
        printf("ADVERTENCIA: Implementa gameBoardNew() para iniciar el juego.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for(int i=0; i<3; i++) gameBoardEnqueuePiece(game_board, rand() % 7);

    SDL_Event e;
    int game_over = 0;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN && game_board->current_piece.active) {
                if (e.key.keysym.sym == SDLK_LEFT && canMove(game_board, game_board->current_piece.col - 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && canMove(game_board, game_board->current_piece.col + 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (game_board->current_piece.rotation + 1) % 4;
                    if (canMove(game_board, game_board->current_piece.col, game_board->current_piece.row, next_rot)) game_board->current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE) {
                     while(canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                     lockPiece(game_board);
                }
            }
        }
        if (!gameBoardUpdate(game_board)) game_over = 1;
        gameBoardDraw(game_board);
        SDL_Delay(16);
    }
    gameBoardDelete(game_board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
