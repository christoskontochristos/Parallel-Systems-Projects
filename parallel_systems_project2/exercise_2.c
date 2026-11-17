
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#include "timer.h"



//function to print the solution matrix
void print_solutions(long double * solutions, int n){

    for(int i = 0; i < n; i++){
        printf("x[%d] = %.2Lf\n",i, solutions[i]);
    }

    return;
}

//gaussian elimination algorithm 
void gaussian_elimination(int n,  double **A,  double *b){

    for(int pivot = 0; pivot < n - 1; pivot++){

        int max_row = pivot;
        for(int i = pivot + 1; i < n; i++){
            if(fabs(A[i][pivot]) > fabs(A[max_row][pivot])){
                max_row = i;
            }
        }

        if(max_row != pivot){
            double *temp = A[pivot];
            A[pivot] = A[max_row];
            A[max_row] = temp;

            double tmp = b[pivot];
            b[pivot] = b[max_row];
            b[max_row] = tmp;
        }

        for(int row = pivot + 1; row < n; row++){
            double factor = A[row][pivot] / A[pivot][pivot];
            for(int col = pivot; col < n; col++){
                A[row][col] -= factor * A[pivot][col];
            }
            b[row] -= factor * b[pivot];
        }
    }
}


//row serial algorithm 
long double * row_serial(int n, double **A, double *b){

    long double *x = malloc(sizeof(long double) * n);
    if(x == NULL){
        perror("malloc");
        exit(1);
    }

    for(int i = 0; i < n; i++){
        x[i] = 0;
    }

    for(int row = n-1; row >= 0; row--){
        x[row] = b[row];
        for(int col = row + 1; col < n; col++){
            x[row] -= A[row][col] * x[col];
        }
        x[row] /= A[row][row];
    }

    return x;
}

//row parallel algorithm 
long double * row_parallel(int n, double **A, double *b) {

    long double *x = malloc(sizeof(long double) * n);
    if(x == NULL){
        perror("malloc");
        exit(1);
    }

    for(int i = 0; i < n; i++){
        x[i] = 0;
    }

    for(int row = n - 1; row >= 0; row--) {
    
        x[row] = b[row];

        //temporary variable 
        long double temp = 0.0;

        //using omp parallel for reduction to update the temp variable
        #pragma omp parallel for reduction(+:temp) schedule(runtime)
        for(int col = row + 1; col < n; col++){
            temp += A[row][col] * x[col];
        }

        x[row] -= temp;

        if(fabs(A[row][row]) < 1e-10) {  //check for small pivot
            fprintf(stderr, "Error: Small pivot at row %d, value: %.2f. Matrix may be ill-conditioned.\n", row, A[row][row]);
            free(x);
            exit(1);
        }

        x[row] /= A[row][row];
    }

    return x;
}


//column serial algorithm 
long double * col_serial(int n, double **A, double *b){
    
    long double *x = malloc(sizeof(long double) * n);
    if(x == NULL){
        perror("malloc");
        exit(1);
    }

    for(int row = 0; row < n; row++) x[row] = b[row];

    for(int col = n - 1; col >= 0; col--){

        x[col] /= A[col][col];

        for(int row = 0; row < col; row++){
            x[row] -= A[row][col] * x[col];
        }

    }

    return x;
}

//column parallel algorithm 
long double * col_parallel(int n, double **A, double *b){
    

    long double *x = malloc(sizeof(long double) * n);
    if(x == NULL){
        perror("malloc");
        exit(1);
    }

    for(int row = 0; row < n; row++) x[row] = b[row];

    for(int col = n - 1; col >= 0; col--){

        if (A[col][col] == 0) {
            fprintf(stderr, "Error: Division by zero at row %d\n", col);
            free(x);
            exit(1);
        }
        
        x[col] /= A[col][col];

        //using omp parallel for to update the temp variable
        int row;
        #pragma omp parallel for schedule(runtime)//reduction(-:x[row]) 
        for(row = 0; row < col; row++){
            x[row] -= A[row][col] * x[col];
        }

    }

    return x;
}

//function to free the allocated memory for the A array
void free_A_matrix(double **A, int n){ for(int i = 0; i < n; i++) free(A[i]); }


//function to create a random 2d array with values in the range [min, max] 
double **create_random_2d_array(int n, double min, double max) {
    //allocate memory for the array
    double **array = malloc(n * sizeof(double *));
    if (array == NULL) {
        perror("malloc");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        array[i] = malloc(n * sizeof(double));
        if (array[i] == NULL) {
            perror("malloc");
            exit(1);
        }
    }

    //seed the random number generator
    srand(time(NULL));

    //fill the array with random values in the range [min, max]
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            array[i][j] = min + (rand() / (double)RAND_MAX) * (max - min);
        }
    }

    return array;
}


//function to create a random 1d array with values in the range [min, max]
double * create_random_1d_array(int n, double min, double max){

    double * array = malloc(sizeof(double) * n);
    if(array == NULL){
        perror("malloc");
        exit(1);
    }

    srand(time(NULL));

    for(int i = 0; i < n; i++){
        array[i] = min + (rand() / (double)RAND_MAX) * (max - min);
    }

    return array;
}

int main(int argc, char *argv[]) {
    char *choice = NULL;
    char *mode = NULL;
    int option, t = -1, n = -1, c = -1, m = -1;
    extern char *optarg;
    extern int optopt, optind;

    if(argc == 1){
        printf("Usage: %s -t <threads> -n <size> -c <col|row> -m <serial|parallel>\n", argv[0]);
        exit(1);
    }


    //GETTING ARGUMENTS FROM COMMAND LINE
    while ((option = getopt(argc, argv, ":t:n:c:m:")) != -1) {
        switch (option) {
        case 'c':  //choice: "col" or "row"
            if (choice == NULL) {
                choice = optarg;
                if (strcmp(choice, "col") == 0) {
                    c = 1;  //assign 1 for "col"
                } else if (strcmp(choice, "row") == 0) {
                    c = 2;  //assign 2 for "row"
                } else {
                    printf("Invalid value for -c. Use 'col' or 'row'. Terminating\n");
                    exit(1);
                }
                break;
            } else {
                printf("Argument duplicates. Terminating\n");
                exit(1);
            }
        case 'n':  //number of elements
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

    double ** A = create_random_2d_array(n, 0.0, 10.0);
    double *b = create_random_1d_array(n, 0.0, 10.0);

    //apply gaussian elimination to the input matrix to turn it into upper triangular form
    gaussian_elimination(n, A, b);

    // Perform actions based on the parsed arguments
    switch (c) {
    case 1:  //column-based execution
        if (m == 1) {
            //serial
            double start,finish,elapsed;
            GET_TIME(start);
            long double * x = col_serial(n, A, b); 
            GET_TIME(finish);
            elapsed = finish - start;

            print_solutions(x,n);
            printf("Elapsed time : %f\n", elapsed);
            free(x);
        } else if (m == 2) {
            //parallel
            double start,finish,elapsed;
            GET_TIME(start);
            long double * x = col_parallel(n, A, b);  
            GET_TIME(finish);
            elapsed = finish - start;

            print_solutions(x,n);
            printf("Elapsed time : %f\n", elapsed);
            free(x);
        }
        break;
    case 2:  //row-based execution
        if (m == 1) {
            //serial
            double start,finish,elapsed;
            GET_TIME(start);
            long double * x = row_serial(n, A, b);  
            GET_TIME(finish);
            elapsed = finish - start;

            print_solutions(x,n);
            printf("Elapsed time : %f\n", elapsed);
            free(x);
        } else if (m == 2) {
            //parallel
            double start,finish,elapsed;
            GET_TIME(start);
            long double * x = row_parallel(n, A, b);  
            GET_TIME(finish);
            elapsed = finish - start;

            print_solutions(x,n);
            printf("Elapsed time : %f\n", elapsed);
            free(x);
        }
        break;
    default:
        printf("Unknown error. Terminating\n");
        exit(1);
    }

    //free the allocated memory
    free_A_matrix(A, n);
    free(b);

    return 0;
}

// void input_linear_system(int **A, int *b, int n){

//     printf("Enter the coefficients of the matrix A (row by row):\n");
//     for (int i = 0; i < n; i++) {
//         for (int j = 0; j < n; j++) {
//             printf("A[%d][%d]: ", i, j);
//             scanf("%d", &A[i][j]);
//         }
//     }

//     printf("Enter the constants vector b:\n");
//     for (int i = 0; i < n; i++) {
//         printf("b[%d]: ", i);
//         scanf("%d", &b[i]);
//     }

//     return;
// }