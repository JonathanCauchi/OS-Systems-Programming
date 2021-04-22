
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM 1000

static pthread_mutex_t mutex;
int counter = 0;


void *function()
{
    for(int i = 0; i < NUM; i++)
    {
        pthread_mutex_lock(&mutex);
        counter++;
        pthread_mutex_unlock(&mutex);
    }
    
}

int main(int argc, char* argv[])
{

    pthread_t thread1, thread2;
    pthread_mutex_init(&mutex, NULL);

    if(pthread_create(&thread1, NULL, &function, NULL) != 0)
    {
        return 1;
    }

    if(pthread_create(&thread2, NULL, &function, NULL) != 0)
    {
        return 2;
    }

    if(pthread_join(thread1, NULL) != 0)
    {
        return 3;
    }

    if(pthread_join(thread2, NULL) != 0)
    {
        return 4;
    }
    
    pthread_mutex_destroy(&mutex);
    printf("Counter is %d", counter);
    return 0;
}