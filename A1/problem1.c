#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int *fib;   // for holds fib no.
int n;      // no. of fib


// t1 generating the fibonacci 
void *make_fib(void *arg) {
    fib = malloc((n + 1) * sizeof(int));
    fib[0] = 0;
    if (n > 0) fib[1] = 1;
    for (int i = 2; i <= n; i++)
        fib[i] = fib[i - 1] + fib[i - 2];
    pthread_exit(NULL);
}

// t2 searching the values 
void *search_fib(void *arg) {
    int *indexes = (int *)arg;
    int count = indexes[0];

    for (int i = 1; i <= count; i++) {
        int idx = indexes[i];
        if (idx >= 0 && idx <= n)
            printf("result of search #%d = %d\n", i, fib[idx]);
        else
            printf("result of search #%d = -1\n", i);
    }
    pthread_exit(NULL);
}

int main() {
    printf("Enter the term of fibonacci sequence:\n");
    scanf("%d", &n);

    pthread_t t1, t2;
    pthread_create(&t1, NULL, make_fib, NULL);
    pthread_join(t1, NULL);

    for (int i = 0; i <= n; i++)
        printf("a[%d] = %d\n", i, fib[i]);

    int s;
    printf("How many numbers you are willing to search?:\n");
    scanf("%d", &s);

    int *search_indexes = malloc((s + 1) * sizeof(int));
    search_indexes[0] = s;
    for (int i = 1; i <= s; i++) {
        printf("Enter search %d:\n", i);
        scanf("%d", &search_indexes[i]);
    }

    pthread_create(&t2, NULL, search_fib, search_indexes);
    pthread_join(t2, NULL);

    free(fib);
    free(search_indexes);
    return 0;
}



