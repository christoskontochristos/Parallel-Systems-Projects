#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <omp.h>


#include "timer.h"

#define ALIVE (1)
#define DEAD (0)


//function to count how many alive neighbours a cell with coordinates x,y has 
int count_neighbours(int x, int y, int **grid, int n){

    int sum = 0;

    for(int i = -1; i <= 1; i++){
        for (int j = -1; j <= 1; j++){

            int newX = x + i;
            int newY = y + j;

            if(newX >= 0 && newX < n && newY >= 0 && newY < n && !(i == 0 && j == 0)){
                sum += grid[newX][newY];
            }
        }
    }

    return sum;
}


//function to initialize the grid with random values (0 or 1)
int ** init_grid(int **grid, int n){

    srand(time(0));

    for(int i = 0; i < n; i++){

        for(int j = 0; j < n; j++) grid[i][j] = rand() % 2;

    }

    return grid;
}


//function to allocate memory for the grid 
int ** allocate_grid(int n) {
    int ** grid = malloc(sizeof(int*) * n);
    if (grid == NULL) {
        perror("malloc");
        exit(1);
    }

    //allocate memory for each row in parallel
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        grid[i] = malloc(sizeof(int) * n);
        if (grid[i] == NULL) {
            perror("malloc");
            exit(1);
        }
    }

    //initialize the grid in parallel
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            grid[i][j] = -1;  //initialize each cell to -1
        }
    }

    return grid;
}

//serial algorithm to update the grid 
int ** update_grid_serial(int ** grid, int n){

    int ** new_grid = allocate_grid(n);

    for(int i = 0; i < n; i++){

        for(int j = 0; j < n; j++){

            int counter = count_neighbours(i,j,grid,n);

            if(grid[i][j] == ALIVE){
                if(counter < 2 || counter > 3){
                    new_grid[i][j] = DEAD;
                }
                else{
                    new_grid[i][j] = ALIVE;
                }
            }
            else{
                if(counter == 3){
                    new_grid[i][j] = ALIVE;
                }
                else{
                    new_grid[i][j] = DEAD;
                }
            }
        }

    }

    return new_grid;
}

//parallel algorithm to update the grid 
int ** update_grid_parallel(int ** grid, int n){

    int ** new_grid = allocate_grid(n);

    #pragma omp parallel for collapse(2) //schedule(dynamic)
    for(int i = 0; i < n; i++){

        for(int j = 0; j < n; j++){

            int counter = count_neighbours(i,j,grid,n);

            if(grid[i][j] == ALIVE){
                if(counter < 2 || counter > 3){
                    new_grid[i][j] = DEAD;
                }
                else{
                    new_grid[i][j] = ALIVE;
                }
            }
            else{
                if(counter == 3){
                    new_grid[i][j] = ALIVE;
                }
                else{
                    new_grid[i][j] = DEAD;
                }
            }
        }

    }

    return new_grid;
}

//function to print the grid
void print_grid(int ** grid, int n){

    for(int i = 0; i < n; i++){

        for(int j = 0; j < n; j++){
            printf("%d ", grid[i][j]);
        }

        printf("\n");

    }

    return;
}


//function to start the game
void start_game(int choice, int gen, int n){

    int **grid = allocate_grid(n);      
    init_grid(grid, n);                  

    //print_grid(grid, n);

    for(int i = 0; i < gen; i++){


        int **new_grid;
        if(choice == 1){                //serial
            new_grid = update_grid_serial(grid, n);
            //print_grid(new_grid,n);
        }
        else if(choice == 2){         //parallel
            new_grid = update_grid_parallel(grid, n);
            //print_grid(new_grid,n);
        }

        for(int j = 0; j < n; j++){
            free(grid[j]);
        }
        free(grid);

        grid = new_grid;  
                        
    }

    for (int i = 0; i < n; i++) {
        free(grid[i]);
    }
    free(grid);

    return;
}




int main(int argc, char *argv[]) {
    char *choice = NULL;
    char *mode = NULL;
    int option, t = -1, n = -1, g = -1, m = -1;
    extern char *optarg;
    extern int optopt, optind;

    if(argc == 1){
        printf("Usage: %s -t <threads> -n <grid size> -g <number of generations> -m <serial|parallel>\n", argv[0]);
        exit(1);
    }


    //GETTING ARGUMENTS FROM COMMAND LINE
    while ((option = getopt(argc, argv, ":t:n:g:m:")) != -1) {
        switch (option) {
        case 'g':  //number of generations
            if (g == -1) {
                if ((g = atoi(optarg)) <= 0) {
                    printf("Malformed input for -g. Must be a positive integer\n");
                    exit(1);
                }
                break;
            } else {
                printf("Argument duplicates. Terminating\n");
                exit(1);
            }
        case 'n':  //number of elements on the grid 
            if (n == -1) {
                if ((n = atoi(optarg)) <= 0) {
                    printf("Malformed input for -n. Must be a positive integer\n");
                    exit(1);
                }
                break;
            } else {
                printf("Argument duplicates. Terminating\n");
                exit(1);
            }
        case 't':  //number of threads
            if (t == -1) {
                if ((t = atoi(optarg)) <= 0) {
                    printf("Malformed input for -t. Must be a positive integer\n");
                    exit(1);
                }
                break;
            } else {
                printf("Argument duplicates. Terminating\n");
                exit(1);
            }
        case 'm':  //mode: "serial" or "parallel"
            if (mode == NULL) {
                mode = optarg;
                if (strcmp(mode, "serial") == 0) {
                    m = 1;  //assign 1 for "serial"
                } else if (strcmp(mode, "parallel") == 0) {
                    m = 2;  //assign 2 for "parallel"
                } else {
                    printf("Invalid value for -m. Use 'serial' or 'parallel'. Terminating\n");
                    exit(1);
                }
                break;
            } else {
                printf("Argument duplicates. Terminating\n");
                exit(1);
            }
        case ':':   
            printf("Missing argument for option. Terminating\n");
            exit(1);
        case '?':
            printf("Unknown argument '%c'. Terminating\n", optopt);
            exit(1);
        default:
            printf("Unknown error. Terminating\n");
            exit(1);
        }
    }

    //check for excess arguments
    for (; optind < argc; optind++) {
        printf("Too many arguments. Terminating\n");
        exit(1);
    }

    //set the number of threads 
    omp_set_num_threads(t);

    //start the game
    double start,finish,elapsed;
    GET_TIME(start);

    start_game(m, g, n);

    GET_TIME(finish);

    elapsed = finish - start;
    printf("Elapsed time : %f\n", elapsed);

    return 0;
}