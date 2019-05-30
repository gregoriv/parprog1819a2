#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N 1000000     
#define THREADS 4       
#define SIZE 100        
#define THRESHOLD 10    
#define WORK 0
#define DONE 1
#define SHUTDOWN 2

struct message {
    int type;
    int start;
    int end;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;

struct message mqueue[N];
int q_in = 0, q_out = 0;
int m_count = 0;

void send(int type, int start, int end) {
    pthread_mutex_lock(&mutex);
    while (m_count >= N) {
        printf("\nProducer locked\n");
        pthread_cond_wait(&msg_out, &mutex);
    }
    
    mqueue[q_in].type = type;
    mqueue[q_in].start = start;
    mqueue[q_in].end = end;
    q_in = (q_in + 1) % N;
    m_count++;

    pthread_cond_signal(&msg_in);
    pthread_mutex_unlock(&mutex);
}

void recv(int *type, int *start, int *end) {
    pthread_mutex_lock(&mutex);
    while (m_count < 1) {
        printf("\nConsumer locked\n");
        pthread_cond_wait(&msg_in, &mutex);
    }

    *type = mqueue[q_out].type;
    *start = mqueue[q_out].start;
    *end = mqueue[q_out].end;
    q_out = (q_out + 1) % N;
    m_count--;

    pthread_cond_signal(&msg_out);
    pthread_mutex_unlock(&mutex);
}

void swap(double *a, double *b) {
    double tmp = *a;
    *a = *b;
    *b = tmp;
} 

int partition(double *a, int n) {
    int first = 0;
    int middle = n/2;
    int last = n-1;
    if (a[first] > a[middle]) {
        swap(a+first, a+middle);
    }
    if (a[middle] > a[last]) {
        swap(a+middle, a+last);
    }
    if (a[first] > a[middle]) {
        swap(a+first, a+middle);
    }
    double p = a[middle];
    int i, j;
    for (i=1, j=n-2;; i++, j--) {
        while (a[i] < p) i++;
        while (a[j] > p) j--;
        if (i>=j) break;
        swap(a+i, a+j);
    }
    return i;
}

void ins_sort(double *a, int n) {
    int i, j;
    for (i=1; i<n; i++) {
        j = i;
        while (j>0 && a[j-1] > a[j]) {
            swap(a+j, a+j-1);
            j--;    
        }
    }
}

void *thread_func(void *params) {
    double *a = (double*) params;
    int t, s, e;
    recv(&t, &s, &e);
    while (t != SHUTDOWN) {
        if (t == DONE) {
            send(DONE, s, e);
        } else if (t == WORK) {
            if (e-s <= THRESHOLD) {
                ins_sort(a+s, e-s);
                send(DONE, s, e);
            } else {
                int p = partition(a+s, e-s);
                send(WORK, s, s+p);
                send(WORK, s+p, e);
            }
        }
        recv(&t, &s, &e);
    }
    send(SHUTDOWN, 0, 0);
    printf("done!\n");
    pthread_exit(NULL);
}

int main() {
    double *a = (double*) malloc(sizeof(double) * SIZE);
    if (a == NULL) {
        printf("Error during memory allocation\n");
        exit(1);
    }
    for (int i=0; i<SIZE; i++) {
        a[i] = (double) rand()/RAND_MAX;
    }
    pthread_t threads[THREADS];
    for (int i=0; i<THREADS; i++){
        if (pthread_create(&threads[i], NULL, thread_func, a) != 0) {
            printf("Thread creation error\n");
            free(a);
            exit(1);
        }
    }
    send(WORK, 0, SIZE);
    int t, s, e;
    int count = 0;
    recv(&t, &s, &e);
    while (1) {
        if (t == DONE) {
            count += e-s;
            printf("Done with %d out of %d\n", count, SIZE);
            printf("Partition done: (%d, %d)\n", s, e);
            if (count == SIZE) {
                break;
            }
        } else {
            send(t, s, e);
        }
        recv(&t, &s, &e);
    }
    send(SHUTDOWN, 0, 0);
    for (int i=0; i<THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    int i;
    for (i=0; i<SIZE-1; i++) {
        if (a[i] > a[i+1]) {
            printf("Error! Array is not sorted. a[%d] = %lf, a[%d] = %lf\n", i, a[i], i+1, a[i+1]);
            break;
        }
    }
    if (i == SIZE-1) {
        printf("Sucess!\n");
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&msg_in);
    pthread_cond_destroy(&msg_out);
    free(a);
    return 0;
}
