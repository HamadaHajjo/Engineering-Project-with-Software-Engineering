#ifndef GRID_H
#define GRID_H

#define GRID_SIZE 10

extern const int directions[8][2];

typedef enum {
    EMPTY,
    FIRE,
    FIREMAN
} CellState;

typedef struct {
    int x, y;
} Position;

void print_grid(CellState grid[GRID_SIZE][GRID_SIZE]);
int find_shortest_path(CellState grid[GRID_SIZE][GRID_SIZE], Position start, Position target, Position path[GRID_SIZE * GRID_SIZE]);

#endif