#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "timer.h"
#include "my_rand.h"



typedef long long int type;

pthread_barrier_t barrier;

void * worker_thread(void *args){

    type iter = *(type*)args;

    for(int i=0; i<iter; i++){
        pthread_barrier_wait(&barrier);
    }

    return NULL;
}


int main(int argc, char *argv[]){
   

    if(argc < 3){
        printf("Usage: ./exercise5_a <number of threads> <iterations>\n");
        return 0;
    }   

    int threads_num = atoi(argv[1]);
    type iterations = strtoull(argv[2], NULL, 10);
    double start, finish, elapsed;

    if(pthread_barrier_init(&barrier, NULL, threads_num)){
        perror("pthread_barrier_init");
        return 1;
    }

    pthread_t * thread_array = malloc(sizeof(pthread_t) * threads_num);
    if(thread_array == NULL){
        perror("malloc");
        return 1;
    }

    type * args = malloc(sizeof(type));
    if(args == NULL){
        perror("malloc");
        return 1;
    }
    *args = iterations;

    GET_TIME(start);

    for(int i = 0; i < threads_num; i++){
        if(pthread_create(&thread_array[i], NULL, worker_thread, args)){
            perror("pthread_create");
            return 1;
        }
    }

    for(int i = 0; i < threads_num; i++){
        if(pthread_join(thread_array[i], NULL)){
            perror("pthread_join");
            return 1;
        }
    }

    GET_TIME(finish);

    elapsed = finish - start;

    printf("Elapsed time : %lf seconds\n", elapsed);

    if(pthread_barrier_destroy(&barrier)){
        perror("pthread_barrier_destroy");
        return 1;
    }

    free(thread_array);
    free(args);

    return 0;
}