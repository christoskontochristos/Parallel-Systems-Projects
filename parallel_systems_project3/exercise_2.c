#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <mpi.h>


#include "timer.h"


double * generate_random_vector(int n, int range){

    srand(time(NULL)); //set the seed

    double * vector = malloc(sizeof(double) * n);
    if(vector == NULL){
        perror("malloc");
        exit(1);
    }

    for(int i = 0; i < n; i++){
        vector[i] = rand() % (range + 1);
    }

    return vector;
}


double * generate_random_matrix(int n, int range){

    srand(time(NULL)); //set the seed

    double * matrix = malloc(sizeof(double) * (n*n));
    if(matrix == NULL){
        perror("malloc");
        exit(1);
    }

    for(int i = 0; i < n; i++){

        //fill the matrix with random values from [0, range]
        for(int j = 0; j<n; j++){
            matrix[i*n + j] = rand() % (range + 1);
        }
    }

    return matrix;
}



double * mat_vec_mult_serial(double ** A, double * x, int n){

    double * y = malloc(sizeof(double) * n);
    if(y == NULL){
        perror("malloc");
        exit(1);
    }
    for(int i = 0; i < n; i++) y[i] = 0.0;

    for(int i = 0; i < n; i++){
        double sum = 0.0;
        for(int j = 0; j < n; j++){
            sum += x[j] * A[i][j];
        }

        y[i] = sum;
    }

    return y;

}


double * mat_vec_mult_parallel(double * columns, double * vec_values, int n, int cols_num){

    double * y = malloc(sizeof(double) * n);
    if(y == NULL){
        perror("malloc");
        exit(1);
    }
    for(int i = 0; i < n; i++) y[i] = 0.0;


    for(int i = 0; i < n; i++){
        double sum = 0.0;
        for(int j = 0; j < cols_num; j++){
            sum += columns[i * cols_num + j] * vec_values[j];
        }
        y[i] = sum;
    }

    return y;
}





int main(int argc, char * argv[]){

    // if(argc < 2){
    //     printf("Usage: ./exercise_2 <int>");
    //     exit(1);
    // }

    int option;
    int n = -1;
    extern char *optarg;
    extern int optopt, optind;

    if(argc == 1){
        printf("Usage: %s -n <grid size> \n", argv[0]);
        exit(1);
    }


    //GETTING ARGUMENTS FROM COMMAND LINE
    while ((option = getopt(argc, argv, ":n:")) != -1) {
        switch (option) {
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







    int comm_sz;
    int rank;
    // int n = atoi(argv[1]);

    double start, finish, elapsed;

    // -------------- Start Of Parallel Code -------------- // 

    //start MPI
    MPI_Init(&argc, &argv); 
    //MPI_Init(NULL, NULL);

    //get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    //get the rank among all processes
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    GET_TIME(start);

    int number_of_columns_to_scatter = n / comm_sz;

    //might need to allocate the memory here
    double * matrix; 
    double * vector;

    //int counter = number_of_columns_to_scatter * n;
    int counter;

    int * counts = malloc(sizeof(int) * comm_sz);
    int * displ = malloc(sizeof(int) * comm_sz);
    if(counts == NULL || displ == NULL){
        perror("malloc");
        exit(1);
    }

    MPI_Datatype col, coltype, vec, vectype;

    //process 0
    if(rank == 0){

        matrix = generate_random_matrix(n, 10);
        vector = generate_random_vector(n, 10);

        //print the matrix for debugging
        // printf("Matrix: \n");
        // int j = 0;
        // for(int i = 0; i < n*n;i++){
        //     printf("%f ", matrix[i]);
        //     j++;
        //     if(j == n){
        //         j = 0;
        //         printf("\n");
        //     }
        // }
        // printf("\n\n");
        // printf("Vector: \n");
        // for(int i = 0; i < n; i++) printf("%f ", vector[i]);
        // printf("\n\n");

    }   


    //declare an MPI type vector for the scattering of the columns fo the matrix
    MPI_Type_vector(n, number_of_columns_to_scatter, n, MPI_DOUBLE, &col);

    MPI_Type_commit(&col);

    MPI_Type_create_resized(col, 0, number_of_columns_to_scatter*sizeof(double), &coltype);

    MPI_Type_commit(&coltype);


    double * recvbuf = malloc(sizeof(double) * (number_of_columns_to_scatter * n));
    if(recvbuf == NULL){
        perror("malloc");
        exit(1);
    }

    //scatter the columns
    MPI_Scatter(matrix, 1, coltype, recvbuf, number_of_columns_to_scatter * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    int number_of_items_to_scatter = number_of_columns_to_scatter;

    double * recvbuf_1 = malloc(sizeof(double) * number_of_items_to_scatter);
    if(recvbuf_1 == NULL){
        perror("malloc");
        exit(1);
    }

    //declare an MPI type vector for the scattering of the vector
    MPI_Type_vector(number_of_items_to_scatter, 1, 1, MPI_DOUBLE, &vec);

    MPI_Type_commit(&vec);
    
    //scatter the vector
    MPI_Scatter(vector, 1, vec, recvbuf_1, number_of_items_to_scatter, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Print received columns (debugging)
    // printf("Process %d received:\n", rank);
    // for (int i = 0; i < n; i++) {
    //     for (int j = 0; j < number_of_columns_to_scatter; j++) {
    //         printf("%.1f ", recvbuf[i * number_of_columns_to_scatter + j]);
    //     }
    //     printf("\n");
    // }
    // printf("Vector values : ");
    // for(int i = 0; i < number_of_items_to_scatter; i++) printf("%f ", recvbuf_1[i]);
    // printf("\n");


    double * y = mat_vec_mult_parallel(recvbuf, recvbuf_1, n, number_of_columns_to_scatter);

    // Allocate space for the final result in process 0
    double * final_result = NULL;
    if(rank == 0){
        final_result = malloc(sizeof(double) * n);
        if(final_result == NULL){
            perror("malloc");
            exit(1);
        }
    }

    // Reduce the results from all processes to process 0
    MPI_Reduce(y, final_result, n, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if(rank == 0){

        printf("Final Result (y):\n");
        for(int i = 0; i < n; i++){
            printf("%f\n", final_result[i]);
        }
        free(final_result); 

    }

    //the 0 process frees the custom MPI data types
    if(rank == 0){
        MPI_Type_free(&col);
        MPI_Type_free(&coltype);
        MPI_Type_free(&vec);
    }
    // --------------- End Of Parallel Code --------------- // 

    GET_TIME(finish);

    //shut down MPI
    MPI_Finalize();

    elapsed = finish - start;

    printf("Elapsed time : %f\n", elapsed);

    return 0;
}
