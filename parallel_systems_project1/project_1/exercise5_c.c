#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#include "timer.h"
#include "my_rand.h"


typedef long long int type;

//sense barrier struct
typedef struct{
    int threads_num;
    int count;
    bool global_sense;
    pthread_mutex_t lock;
    pthread_cond_t cond_var;
}sense_barrier_t;


sense_barrier_t barrier;

//init barrier
void sense_barrier_init(sense_barrier_t * barrier, int threads_num){

    pthread_mutex_init(&barrier->lock, NULL);
    pthread_cond_init(&barrier->cond_var, NULL);
    barrier->threads_num = threads_num;
    barrier->count = 0;
    barrier->global_sense = false;
    return;

}

//sense barrier wait
void sense_barrier_wait(sense_barrier_t * barrier){

    bool local_sense = barrier->global_sense;
    
    if(!pthread_mutex_lock(&barrier->lock)){

        barrier->count++;
        local_sense = !local_sense;

        if(barrier->count == barrier->threads_num){

            barrier->count = 0;
            barrier->global_sense = local_sense;
            pthread_cond_broadcast(&barrier->cond_var);
            pthread_mutex_unlock(&barrier->lock);
            return;
        }
        else{
            while(barrier->global_sense != local_sense) pthread_cond_wait(&barrier->cond_var, &barrier->lock);
            pthread_mutex_unlock(&barrier->lock);
        }
    }

    return;
}

//destory barrier
void sense_barrier_destory(sense_barrier_t * barrier){

    pthread_mutex_destroy(&barrier->lock);
    pthread_cond_destroy(&barrier->cond_var);
    return;

}



//thread
void * worker_thread(void *args){

    type iter = *(type*)args;

    for(int i=0; i<iter; i++){
        sense_barrier_wait(&barrier);
    }

    return NULL;
}



//driver code 
int main(int argc, char *argv[]){

    if(argc < 3){
        printf("Usage: ./exercise5_a <number of threads> <iterations>\n");
        return 0;
    }   

    int threads_num = atoi(argv[1]);
    type iterations = strtoull(argv[2], NULL, 10);
    double start, finish, elapsed;

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

    sense_barrier_init(&barrier, threads_num);

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

    sense_barrier_destory(&barrier);

    free(thread_array);
    free(args);

    return 0;
}