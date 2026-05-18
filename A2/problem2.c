#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define max_students 10
#define max_waiting_chairs 3

sem_t studentSem; 
sem_t stSem;       
pthread_mutex_t mutex; 
int waitingstudents = 0;  //using counter


void* tutor(void* arg) {
    while (1) {
        sem_wait(&studentSem);

        // lock 
        pthread_mutex_lock(&mutex);
        waitingstudents--;
        printf("A waiting student started getting consultation\n");
        printf("Number of students now waiting: %d\n", waitingstudents);
        pthread_mutex_unlock(&mutex);
     
        printf("ST giving consultation\n");
        sleep(2); 

        printf("Student getting consultation\n");
        printf("Student finished getting consultation and left\n");

        sem_post(&stSem);
    }
    return NULL;
}

void* student(void* arg) {
    int studentId = *((int*) arg);
    sleep(rand() % 3);
    printf("Student %d started waiting for consultation\n", studentId);

    // lock the waiting area
    pthread_mutex_lock(&mutex);
    if (waitingstudents < max_waiting_chairs) {
        waitingstudents++;
        printf("Number of students now waiting: %d\n", waitingstudents);
        sem_post(&studentSem);
        pthread_mutex_unlock(&mutex);
        sem_wait(&stSem);
    } else {
        printf("Student %d leaves because all chairs are occupied\n", studentId);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main() {
    pthread_t tutorThread;
    pthread_t studentThreads[max_students];
    int studentIds[max_students];
    sem_init(&studentSem, 0, 0);  
    sem_init(&stSem, 0, 0);       
    pthread_mutex_init(&mutex, NULL);  
    pthread_create(&tutorThread, NULL, tutor, NULL);
    for (int i = 0; i < max_students; i++) {
        studentIds[i] = i;
        pthread_create(&studentThreads[i], NULL, student, &studentIds[i]);
    }

    for (int i = 0; i < max_students; i++) {
        pthread_join(studentThreads[i], NULL);
    }

    sem_destroy(&studentSem);
    sem_destroy(&stSem);
    pthread_mutex_destroy(&mutex);

    return 0;
}
