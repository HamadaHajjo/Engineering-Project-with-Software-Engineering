#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "grid.h"
#include "mdf_common.h"

typedef enum {
    IDLE,
    MOVING
} State;

State current_state = IDLE;
Position fireman_pos = {0, 0};
Position fire_pos = {-1, -1}; 
bool fire_exists = false;
int fire_generation_counter = 0;
bool tick = false;
static const char *TAG = "statemachine";

void idle_state(CellState grid[GRID_SIZE][GRID_SIZE]);
void moving_state(CellState grid[GRID_SIZE][GRID_SIZE]);
void generate_fire(CellState grid[GRID_SIZE][GRID_SIZE]);


void set_tick(){
    tick = true;
}

void set_fireman_pos(int x, int y){
    fireman_pos = (Position){x, y};
    MDF_LOGW("Updated fireman position: (%d, %d)", x, y);
}

void state_machine() {
    MDF_LOGW("State machine started");

    srand(time(NULL));
    CellState grid[GRID_SIZE][GRID_SIZE] = {EMPTY};
    grid[fireman_pos.x][fireman_pos.y] = FIREMAN;

    while (1) {
    
        // fire_generation_counter++;
        // if (fire_generation_counter > 50 && !fire_exists) {
        //     generate_fire(grid);
        //     fire_generation_counter = 0;
        // }
        if(tick){
            MDF_LOGW("Tick i state_machine");
            MDF_LOGW("Fireman current position: (%d, %d)", fireman_pos.x, fireman_pos.y);
            tick = false;
            switch (current_state) {
                case IDLE:
                    idle_state(grid);
                    break;
                case MOVING:
                    moving_state(grid);
                    break;
            }
        }

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void idle_state(CellState grid[GRID_SIZE][GRID_SIZE]) {
    printf("\nState: IDLE\n");
    print_grid(grid);

    if (fire_exists) {
        current_state = MOVING;
    }
}

void moving_state(CellState grid[GRID_SIZE][GRID_SIZE]) {
    printf("\nState: MOVING\n");
    Position path[GRID_SIZE * GRID_SIZE];
    int steps = find_shortest_path(grid, fireman_pos, fire_pos, path);

    if (steps > 0) {
        for (int i = steps - 1; i >= 0; i--) {
            grid[fireman_pos.x][fireman_pos.y] = EMPTY;
            fireman_pos = path[i];
            grid[fireman_pos.x][fireman_pos.y] = FIREMAN;
            printf("\nGrid after moving to (%d, %d):\n", fireman_pos.x, fireman_pos.y);
            //print_grid(grid);
        }
        fire_exists = false;
        fire_pos = (Position){-1, -1};
        current_state = IDLE;
    } else {
        printf("\nNo path to fire found.\n");
        current_state = IDLE;
    }
}

void generate_fire(CellState grid[GRID_SIZE][GRID_SIZE]) {
   
    int x = rand() % GRID_SIZE;
    int y = rand() % GRID_SIZE;

    if (grid[x][y] == EMPTY) {
        fire_pos = (Position){x, y};
        grid[x][y] = FIRE;
        fire_exists = true;
        printf("\nFire generated at (%d, %d)\n", x, y);
        //print_grid(grid);
    }
}


void set_up_state_machine(){
    xTaskCreate(state_machine, "state_machine", 4 * 1024, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
}