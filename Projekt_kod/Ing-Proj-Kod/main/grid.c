#include <stdio.h>
#include <stdbool.h>
#include "grid.h"

const int directions[8][2] = {
    {-1, 0},
    {1, 0},
    {0, -1},
    {0, 1},
    {-1, -1},
    {-1, 1}, 
    {1, -1}, 
    {1, 1}    
};

void print_grid(CellState grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == EMPTY) printf(". ");
            else if (grid[i][j] == FIRE) printf("F ");
            else if (grid[i][j] == FIREMAN) printf("M ");
        }
        printf("\n");
    }
}

int find_shortest_path(CellState grid[GRID_SIZE][GRID_SIZE], Position start, Position target, Position path[GRID_SIZE * GRID_SIZE]) {
    bool visited[GRID_SIZE][GRID_SIZE] = {false};
    Position queue[GRID_SIZE * GRID_SIZE];
    Position parent[GRID_SIZE][GRID_SIZE];
    int front = 0, rear = 0;

    queue[rear++] = start;
    visited[start.x][start.y] = true;
    parent[start.x][start.y] = (Position){-1, -1};

    while (front < rear) {
        Position current = queue[front++];

        if (current.x == target.x && current.y == target.y) {
            int steps = 0;
            while (current.x != -1) {
                path[steps++] = current;
                current = parent[current.x][current.y];
            }
            return steps;
        }

        for (int i = 0; i < 8; i++) {
            int nx = current.x + directions[i][0];
            int ny = current.y + directions[i][1];

            if (nx >= 0 && ny >= 0 && nx < GRID_SIZE && ny < GRID_SIZE && !visited[nx][ny] && grid[nx][ny] != FIREMAN) {
                queue[rear++] = (Position){nx, ny};
                visited[nx][ny] = true;
                parent[nx][ny] = current;
            }
        }
    }
    return 0;
}