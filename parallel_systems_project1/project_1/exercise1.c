#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h> 

#include "timer.h"
#include "my_rand.h"

unsigned int seed;


typedef long long int Type;

double random_number_generator(){

    double random = (double)rand() / RAND_MAX;

    return random * 2.0 - 1.0;

}

// double random_number_generator_thread(unsigned int local_seed){

//     double rand_0_to_1 = my_drand(&local_seed);

//     return rand_0_to_1 * 2.0 - 1.0;
// }

double random_number_generator_thread(unsigned int *local_seed){

    double rand_0_to_1 = my_drand(local_seed);  // Pass pointer for RNG
    
    return rand_0_to_1 * 2.0 - 1.0;

}



double Compute_Estimation_Serial(Type throws_number){

    Type arrows = 0;

    for(int i=0; i<throws_number; i++){

        //printf("%d\n",i);

        double rand_x = random_number_generator();
        double rand_y = random_number_generator();

        double squared = rand_x*rand_x + rand_y*rand_y;

        if(squared <= 1) arrows++;

    }

    double pi_estimation = 4 * arrows / ((double) throws_number);

    return pi_estimation;
}




//thread that computes the number of arrows for the Monte Carlo algorithm 
//it takes the number of iterations/throws as an argument (the chunk)
void * compute_chunk_arrows(void * arg){

    Type throws = *(Type *)arg;

    Type * arrows = malloc(sizeof(Type));
    if(arrows == NULL){
        perror("malloc");
        pthread_exit(NULL);
    }

    unsigned int thread_seed = seed + (unsigned int)(uintptr_t)pthread_self();

    for(int i=0; i<throws; i++){

        double rand_x = random_number_generator_thread(&thread_seed); //use my_rand()
        double rand_y = random_number_generator_thread(&thread_seed); //use my_rand()

        double squared = rand_x*rand_x + rand_y*rand_y;

        if(squared <= 1) (*arrows)++;

    }

    return arrows; //return to the main thread to add to the total number of arrows

}


double Compute_Estimation_Parallel(Type throws_number, int threads_num){

    if(threads_num >= throws_number) threads_num = throws_number;

    Type arrows_sum = 0;

    pthread_t * ptid_array = malloc(sizeof(pthread_t) * threads_num);
    if(ptid_array == NULL){
        perror("malloc");
        return -1.0;
    }

    //compute the chunk of the number of throws that each thread has to compute
    Type chunk = throws_number / threads_num;

    Type * arg = malloc(sizeof(Type));
    if(arg == NULL){
        perror("malloc");
        return -1.0;
    }
    *arg = chunk;
    

    for(int i = 0; i<threads_num; i++){

        //if its the last thread compute the rest of the remaining elements
        if(i == threads_num - 1) {
            chunk = throws_number - (chunk * (threads_num - 1));
            *arg = chunk;
        }

        //create a thread using pthread_create
        if(pthread_create(&ptid_array[i], NULL, &compute_chunk_arrows, arg)){
            perror("pthread_create");
            return -1.0;
        }
        
    }

    for(int i=0; i<threads_num; i++){

        void *result;
        if(pthread_join(ptid_array[i], &result)){
            perror("pthread_join");
            return -1.0;
        }

        Type * arrows_num = (Type *)result;
        arrows_sum += *arrows_num;
    }

    double pi_estimation = 4 * arrows_sum / ((double) throws_number);

    return pi_estimation;
}


//driver code 
int main(int argc, char * argv[]){

    if(argc < 3){
        printf("ERROR: Too few arguments!\nCorrect usage: ./exercise1 number of threads : <int> number of throws : <long long int>\n");
        return 0;
    }

    int threads_num = atoi(argv[1]);
    Type throws_number = strtoull(argv[2], NULL, 10);

    // double random = random_number_generator();
    // printf("%lf\n", random);
    seed = (unsigned int)time(NULL);
    srand(seed);


    double start, finish, elapsed;
    
    GET_TIME(start);

    double pi_estimation = Compute_Estimation_Serial(throws_number);

    GET_TIME(finish);

    elapsed = finish - start;
    printf("Elapsed time for serial algorithm for %lld throws: %lf seconds\n", throws_number, elapsed);
    printf("Pi estimation using serial algorithm : %lf\n", pi_estimation);
    
    GET_TIME(start);

    pi_estimation = Compute_Estimation_Parallel(throws_number, threads_num);

    if(pi_estimation == -1.0){
        printf("An error occured!\n");
        return 1;
    }

    GET_TIME(finish);
    
    elapsed = finish - start;
    printf("Elapsed time for parallel algorithm for %lld throws: %lf seconds\n", throws_number, elapsed);
    printf("Pi estimation using parallel algorithm : %lf\n", pi_estimation);

    return 0;
}
