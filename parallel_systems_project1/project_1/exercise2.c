#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#include "timer.h"




typedef long long int Type;

Type shared_variable = 0;
atomic_llong shared_variable_atomic = 0;

pthread_mutex_t mutex_lock;





void * worker_thread(void * args){


    int N = *(Type*)args;

    for(int i=0;i<N;i++){

        pthread_mutex_lock(&mutex_lock);
        shared_variable += 1;
        pthread_mutex_unlock(&mutex_lock);

    }

    return NULL;
}

int Increment_Shared_Variable_Mutex(int threads_num, Type iterations){

    double start, finish, elapsed;

    pthread_t * thread_array = malloc(sizeof(pthread_t) * threads_num);
    if(thread_array == NULL){
        perror("malloc");
        return 1;
    }

    if(pthread_mutex_init(&mutex_lock, NULL) != 0){
        perror("pthread_mutex_init");
        return 1;
    }

    Type * arg = malloc(sizeof(Type));
    if(arg == NULL){
        perror("malloc");
        return 1;
    }
    *arg = iterations;

    GET_TIME(start);

    for(int i=0; i<threads_num; i++){

        if(pthread_create(&thread_array[i], NULL, worker_thread, arg)){
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

    printf("Execution time using a mutex lock: %lf\n", elapsed);

    pthread_mutex_destroy(&mutex_lock);
    
    return 0;
}


void * worker_thread_atomic(void * args){


    int N = *(Type*)args;

    for(int i=0;i<N;i++) atomic_fetch_add(&shared_variable_atomic, 1);


    return NULL;
}


int64_t Increment_Shared_Variable_Atomic(int threads_num, Type iterations){

    double start, finish, elapsed;

    pthread_t * thread_array = malloc(sizeof(pthread_t) * threads_num);
    if(thread_array == NULL){
        perror("malloc");
        return 1;
    }

    Type * arg = malloc(sizeof(Type));
    if(arg == NULL){
        perror("malloc");
        return 1;
    }
    *arg = iterations;

    GET_TIME(start);

    for(int i=0; i<threads_num; i++){

        if(pthread_create(&thread_array[i], NULL, worker_thread_atomic, arg)){
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

    printf("Execution time using an atomic variable: %lf\n", elapsed);
    
    return 0;
}



int main(int argc, char * argv[]){

    if(argc < 3){
        printf("ERROR: Too few command line arguments given!\n Correct usage: ./exercise2 number_of_threads <int> number_of_iterations <long long int>\n");
        return 0;
    }

    int threads_num = atoi(argv[1]);
    Type iterations = strtoull(argv[2], NULL, 10);

    if(Increment_Shared_Variable_Mutex(threads_num, iterations)==-1) printf("An error occured!\n");
    if(Increment_Shared_Variable_Atomic(threads_num, iterations)==-1) printf("An error occured!\n");

    printf("%lld\n", shared_variable);
    printf("%lld\n", shared_variable_atomic);

    return 0;
}