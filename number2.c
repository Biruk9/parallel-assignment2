#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    int start;
    int end;
    int *seed_primes;
    int seed_count;
    int *marked;
} ThreadData;

// Thread function to mark multiples in its range
void *mark_multiples(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    for (int i = 0; i < data->seed_count; i++) {
        int p = data->seed_primes[i];
        int start = ((data->start + p - 1) / p) * p; // first multiple >= start
        if (start < p * p) start = p * p;

        for (int j = start; j <= data->end; j += p) {
            data->marked[j - data->start] = 1;
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    clock_t start_time = clock(); // start timing

    if (argc < 3) {
        printf("Usage: %s <Max> <num_threads>\n", argv[0]);
        return 1;
    }

    int Max = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int sqrtMax = (int)sqrt(Max);

    // Step 1: Sequential sieve up to sqrt(Max)
    int *is_prime = (int *)calloc(sqrtMax + 1, sizeof(int));
    for (int i = 2; i <= sqrtMax; i++) is_prime[i] = 1;

    for (int i = 2; i * i <= sqrtMax; i++) {
        if (is_prime[i]) {
            for (int j = i * i; j <= sqrtMax; j += i)
                is_prime[j] = 0;
        }
    }

    // Collect seed primes
    int seed_count = 0;
    for (int i = 2; i <= sqrtMax; i++) if (is_prime[i]) seed_count++;

    int *seed_primes = (int *)malloc(seed_count * sizeof(int));
    for (int i = 2, idx = 0; i <= sqrtMax; i++) {
        if (is_prime[i]) seed_primes[idx++] = i;
    }

    free(is_prime);

    // Step 2: Parallel marking
    int range = Max - sqrtMax;
    int chunk_size = (range + num_threads - 1) / num_threads;

    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    int **marked_arrays = (int **)malloc(num_threads * sizeof(int *));

    for (int t = 0; t < num_threads; t++) {
        int start = sqrtMax + 1 + t * chunk_size;
        int end = start + chunk_size - 1;
        if (end > Max) end = Max;
        int size = end - start + 1;

        marked_arrays[t] = (int *)calloc(size, sizeof(int));

        thread_data[t].start = start;
        thread_data[t].end = end;
        thread_data[t].seed_primes = seed_primes;
        thread_data[t].seed_count = seed_count;
        thread_data[t].marked = marked_arrays[t];

        pthread_create(&threads[t], NULL, mark_multiples, &thread_data[t]);
    }

    // Wait for threads
    for (int t = 0; t < num_threads; t++) pthread_join(threads[t], NULL);

    // Collect all primes
    int *primes = (int *)malloc((Max/2) * sizeof(int)); // rough estimate
    int count = 0;

    // Add small primes
    for (int i = 0; i < seed_count; i++) primes[count++] = seed_primes[i];

    // Add primes from threads
    for (int t = 0; t < num_threads; t++) {
        int start = thread_data[t].start;
        int end = thread_data[t].end;
        for (int i = 0; i <= end - start; i++) {
            if (!marked_arrays[t][i]) primes[count++] = start + i;
        }
        free(marked_arrays[t]);
    }

    free(marked_arrays);
    free(seed_primes);

    // Print summary
    printf("Total primes: %d\n", count);

    printf("First 10 primes: ");
    for (int i = 0; i < 10 && i < count; i++) printf("%d ", primes[i]);
    printf("\n");

    printf("Last 10 primes: ");
    for (int i = count-10; i < count; i++) if(i>=0) printf("%d ", primes[i]);
    printf("\n");

    // Save all primes to file
    FILE *fp = fopen("primes.txt", "w");
    for (int i = 0; i < count; i++) fprintf(fp, "%d\n", primes[i]);
    fclose(fp);
    printf("All primes saved to primes.txt\n");

    free(primes);

    // Print execution time
    clock_t end_time = clock();
    double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Execution time: %.3f seconds\n", time_taken);

    return 0;
}
//Compile the code with  gcc number2.c -o number2.exe -lpthread –lm