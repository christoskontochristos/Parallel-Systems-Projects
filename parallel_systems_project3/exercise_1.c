#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <mpi.h>


#include "timer.h"

#define ALIVE (1)
#define DEAD (0)


//function to count neighbors in a flattened 2D array
int count_neighbours(int i, int j, int *grid, int rows, int cols) {
    int count = 0;
    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1}, 
        {0, -1},         {0, 1},    
        {1, -1}, {1, 0}, {1, 1}     
    };

    for (int d = 0; d < 8; d++) {
        int ni = i + directions[d][0];  //neighbor row
        int nj = j + directions[d][1];  //neighbor col

        //check if the neighbor is within bounds

        if(ni >= 0 && ni < rows && nj >= 0 && nj < cols){
            int index = ni * cols + nj; 

            if(grid[index] == 1){
                count++;
            }
        }
    }

    return count;
}




// Initialize grid with random values (0 or 1)
int ** init_grid(int **grid, int n){
    srand(time(0));

    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++) {
            grid[i][j] = rand() % 2;
        }
    }

    return grid;
}

void printFlattenedGrid(int *grid, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", grid[i * n + j]); 
        }
        printf("\n"); 
    }
}

//function to allocate memory for the grid 
int ** allocate_grid(int n) {
    int ** grid = malloc(sizeof(int*) * n);
    if (grid == NULL) {
        perror("malloc");
        exit(1);
    }

    //allocate memory for each row in parallel
    for (int i = 0; i < n; i++) {
        grid[i] = malloc(sizeof(int) * n);
        if (grid[i] == NULL) {
            perror("malloc");
            exit(1);
        }
    }

    return grid;
}

// Function to flatten a 2D grid into a 1D grid
int *flatten_grid(int **grid, int rows, int cols){

    int *flattened = (int *)malloc(rows * cols * sizeof(int));
    if(flattened == NULL){
        perror("Failed to allocate memory for flattened grid");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            flattened[i * cols + j] = grid[i][j];
        }
    }

    return flattened;
}

int *concat_local_array(int *local_array, int *bottom_row, int *top_row, int n, int rows){

    int size;
    if (bottom_row[0] != -1 && top_row[0] != -1)
        size = (rows + 2) * n;  //both ghost rows exist
    else
        size = (rows + 1) * n;  //only one ghost row exists

    int *concatenated_array = malloc(sizeof(int) * size);
    if (concatenated_array == NULL) {
        perror("malloc");
        exit(1);
    }

    int offset = 0;  //keep track of current index in concatenated_array

    //if top row exists, copy it first
    if (top_row[0] != -1) {
        for (int i = 0; i < n; i++) {
            concatenated_array[offset++] = top_row[i];
        }
    }

    //copy local array
    for (int i = 0; i < rows * n; i++) {
        concatenated_array[offset++] = local_array[i];
    }

    //if bottom row exists, copy it last
    if (bottom_row[0] != -1) {
        for (int i = 0; i < n; i++) {
            concatenated_array[offset++] = bottom_row[i];
        }
    }


    return concatenated_array;
}



//update the local grid (excluding ghost rows)
int *update_local_grid(int rows_to_scatter, int *local_grid, int * local_grid_with_rows ,int n, int rows, bool has_top_row){

    int *new_local_grid = malloc(sizeof(int) * (rows_to_scatter) * n); 
    if (new_local_grid == NULL) {
        perror("malloc");
        exit(1);
    }

    for (int i = 0; i < rows_to_scatter; i++) {
        for (int j = 0; j < n; j++) {
            int index = i * n + j;

            int x = i, y = j;

            int adjusted_row = i + (has_top_row ? 1 : 0);  //shift index if top row exists
            int counter = count_neighbours(adjusted_row, j, local_grid_with_rows, rows, n); 

            //printf("i = %d    j = %d     counter = %d\n", i, j, counter);

            if (local_grid[index] == ALIVE) {
                if (counter < 2 || counter > 3) {
                    new_local_grid[index] = DEAD;
                } else {
                    new_local_grid[index] = ALIVE;
                }
            } else {
                if (counter == 3) {
                    new_local_grid[index] = ALIVE;
                } else {
                    new_local_grid[index] = DEAD;
                }
            }
        }
    }

    return new_local_grid;
}

double start_game(int gen, int n) {

    int comm_sz, rank;
    //start up MPI
    MPI_Init(NULL, NULL); 
    //get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    //get the rank of the process  
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   

    int **grid = NULL;
    int *flattened_grid = NULL;
    double start, finish, elapsed;

    if (rank == 0) {
        grid = allocate_grid(n);
        init_grid(grid, n);
        flattened_grid = flatten_grid(grid, n, n);

        //printFlattenedGrid(flattened_grid, n);
    }

    int * final_grid = NULL;

    GET_TIME(start);

    for (int i = 0; i < gen; i++) {

        int rows_to_scatter = n / comm_sz;  
        int *recvbuf = malloc(sizeof(int) * rows_to_scatter * n);  

        if (recvbuf == NULL) {
            perror("malloc");
            exit(1);
        }

        //scatter the rows of the flattened grid to all processes
        MPI_Scatter(flattened_grid, rows_to_scatter * n, MPI_INT, recvbuf, rows_to_scatter * n, MPI_INT, 0, MPI_COMM_WORLD);

        //send ghost rows to neighboring processes
        int *sendbuf_top = NULL, *sendbuf_bottom = NULL;
        if (rank > 0) {
            sendbuf_top = &recvbuf[0];  // First row to send (ghost row)
            MPI_Send(sendbuf_top, n, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
        }
        if (rank < comm_sz - 1) {
            sendbuf_bottom = &recvbuf[(rows_to_scatter - 1) * n];  // Last row to send (ghost row)
            MPI_Send(sendbuf_bottom, n, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
        }

        //receive ghost rows
        int *recvbuf_bottom = NULL;
        int *recvbuf_top = NULL;
        recvbuf_top = malloc(n * sizeof(int));
        recvbuf_bottom = malloc(n * sizeof(int));
        if(recvbuf_top == NULL || recvbuf_bottom == NULL){
            perror("malloc");
            exit(1);
        }
        for(int i = 0; i < n; i++){
            recvbuf_bottom[i] = -1;
            recvbuf_top[i] = -1;
        }

        if (rank > 0) {
            MPI_Recv(recvbuf_top, n, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        if (rank < comm_sz - 1) {
            MPI_Recv(recvbuf_bottom, n, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // if(recvbuf_bottom != NULL){
        //     printf("Process %d received bottom ghost row: ", rank);
        //     for (int j = 0; j < n; j++) {
        //         printf("%d ", recvbuf_bottom[j]);
        //     }
        //     printf("\n");
        // }
        // if(recvbuf_top != NULL){
        //     printf("Process %d received top ghost row: ", rank);
        //     for (int j = 0; j < n; j++) {
        //         printf("%d ", recvbuf_top[j]);
        //     }
        //     printf("\n");
        // }

        int * local_array = concat_local_array(recvbuf, recvbuf_bottom, recvbuf_top, n, rows_to_scatter);

        int rows;
        if (recvbuf_bottom[0] != -1 && recvbuf_top[0] != -1) { rows = (rows_to_scatter + 2); }
        else rows = (rows_to_scatter + 1);  
        
        //flag to check if a local grid has a top row
        bool flag = false;
        if(recvbuf_top[0] != -1) flag = true;

        MPI_Barrier(MPI_COMM_WORLD);  //synchronize processes

        //update the local grid
        int *new_local_grid;
        new_local_grid = update_local_grid(rows_to_scatter, recvbuf, local_array, n, rows, flag);


        // printf("Process %d received grid portion:\n", rank);
        // for (int i = 0; i < rows_to_scatter; i++) {
        //     for (int j = 0; j < n; j++) {
        //         printf("%d ", recvbuf[i * n + j]);  // Access the correct index in the flattened array
        //     }
        //     printf("\n");
        // }


        //allocate final grid on rank 0 and gather all updated grids
        final_grid = NULL;
        if (rank == 0) {
            final_grid = malloc(sizeof(int) * n * n);
            if (final_grid == NULL) {
                perror("malloc");
                exit(1);
            }
        }


        //gather the updated local grids back to rank 0 (final_grid)
        MPI_Gather(new_local_grid, rows_to_scatter * n, MPI_INT, final_grid, rows_to_scatter * n, MPI_INT, 0, MPI_COMM_WORLD);


        //free the allocated memory
        // if(rank == 0){
        //     free(flattened_grid);  
        //     free(recvbuf); 
        //     free(recvbuf_bottom);
        //     free(recvbuf_top);
        // }

    }

    // After gathering, rank 0 will have the full final grid
    // if (rank == 0) {
    //     printf("\n\n\n");
    //     // Print the final grid at rank 0
    //     printFlattenedGrid(final_grid, n);
    //     free(final_grid);
    // }

    GET_TIME(finish);


    MPI_Finalize();  // Shut down MPI

    elapsed = finish - start;
    return elapsed;
}






int main(int argc, char *argv[]){


    char *choice = NULL;
    char *mode = NULL;
    int option, t = -1, n = -1, g = -1, m = -1;
    extern char *optarg;
    extern int optopt, optind;

    if(argc == 1){
        printf("Usage: %s -n <grid size> -g <number of generations>\n", argv[0]);
        exit(1);
    }


    //GETTING ARGUMENTS FROM COMMAND LINE
    while ((option = getopt(argc, argv, ":n:g:")) != -1) {
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

    //start the game
    double elapsed = start_game(g, n);
    printf("Elapsed time : %f\n", elapsed);

    return 0;
}