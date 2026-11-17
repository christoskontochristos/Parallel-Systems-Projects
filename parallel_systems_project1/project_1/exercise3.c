#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>


#include "timer.h"


typedef long long int Type;

//thread arguments
typedef struct{

    Type iterations;
    int index;

}thread_args;


Type * shared_array;


//thread takes as arguments the number of iterations and the index of the array cell the specific thread
//has to update
void * worker_thread(void * args){

    thread_args * arguments = (thread_args*)args;

    for(int i=0; i<arguments->iterations; i++){ shared_array[arguments->index] += 1; }

    return NULL;

}



int main(int argc, char *argv[]){

    if(argc < 3){
        printf("ERROR: Too few command line arguments given!\n Correct usage: ./exercise2 number_of_threads <int> number_of_iterations <long long int>\n");
        return 0;
    }

    int threads_num = atoi(argv[1]);
    Type iterations = strtoull(argv[2], NULL, 10);

    double start, finish, elapsed;

    shared_array = malloc(sizeof(Type) * threads_num);
    if(shared_array == NULL){
        perror("malloc");
        return 1;
    }

    for(int i=0; i<threads_num; i++){
        shared_array[i] = 0;
    }
    

    pthread_t * thread_array = malloc(sizeof(pthread_t) * threads_num);
    if(thread_array == NULL){
        perror("malloc");
        return 1;
    }


    GET_TIME(start);

    for(int i=0; i<threads_num; i++){

        thread_args *args = malloc(sizeof(thread_args)); 
        if(args == NULL){
            perror("malloc");
            return 1;
        }

        args->iterations = iterations;
        args->index = i;


        if(pthread_create(&thread_array[i], NULL, worker_thread, args)){
            perror("pthread_create");
            return 1;
        }

    }

    for(int i=0; i<threads_num; i++){

        if(pthread_join(thread_array[i], NULL)){
            perror("pthread_join");
            return 1;   
        }

    }

    GET_TIME(finish);

    elapsed = finish - start;

    printf("Execution time: %lf\n", elapsed);

    Type sum = 0;
    for(int i=0; i<threads_num; i++){
        printf("%lld ", shared_array[i]);
        sum += shared_array[i];
    }

    printf("\n");
    printf("sum = %lld\n", sum);

    return 0;
}