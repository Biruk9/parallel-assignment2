#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

int num_threads;
long num_trapezoids;
double result = 0.0;
pthread_mutex_t lock;
int scheduling_mode = 0; // 0 = block, 1 = roundrobin

// Function we integrate
double f(double x) {
    return 4.0 / (1.0 + x*x);
}

// Thread function
void* trapezoid_area(void* arg) {
    intptr_t id = (intptr_t)arg;
    double local_sum = 0.0;
    double a = 0.0, b = 1.0;
    double h = (b - a) / num_trapezoids;

    if (scheduling_mode == 0) {
        // ---- Block distribution ----
        long chunk = num_trapezoids / num_threads;
        long start = id * chunk;
        long end = (id == num_threads - 1) ? num_trapezoids : start + chunk;

        for (long j = start; j < end; j++) {
            double x_left = a + j * h;
            double x_right = a + (j + 1) * h;
            local_sum += (h / 2.0) * (f(x_left) + f(x_right));
        }
    } else {
        // ---- Round-robin distribution ----
        for (long j = id; j < num_trapezoids; j += num_threads) {
            double x_left = a + j * h;
            double x_right = a + (j + 1) * h;
            local_sum += (h / 2.0) * (f(x_left) + f(x_right));
        }
    }

    pthread_mutex_lock(&lock);
    result += local_sum;
    pthread_mutex_unlock(&lock);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || (argc == 2 && strcmp(argv[1], "-h") == 0)) {
        printf("Usage: %s <num_threads> <num_trapezoids> [mode]\n", argv[0]);
        printf("   <num_threads>   : number of threads\n");
        printf("   <num_trapezoids>: number of trapezoids\n");
        printf("   [mode]          : optional, 'block' (default) or 'roundrobin'\n");
        printf("Example: %s 4 1000000 block\n", argv[0]);
        return 0;
    }

    num_threads = atoi(argv[1]);
    num_trapezoids = atol(argv[2]);

    if (argc >= 4) {
        if (strcmp(argv[3], "roundrobin") == 0) scheduling_mode = 1;
        else if (strcmp(argv[3], "block") == 0) scheduling_mode = 0;
        else {
            printf("Unknown mode: %s (use 'block' or 'roundrobin')\n", argv[3]);
            return 1;
        }
    }

    pthread_t threads[num_threads];
    pthread_mutex_init(&lock, NULL);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start); // start timing

    for (intptr_t i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, trapezoid_area, (void*)i);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end); // end timing

    pthread_mutex_destroy(&lock);

    double time_taken = (end.tv_sec - start.tv_sec) +
                        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Approximation of PI = %.15f\n", result);
    printf("Computation time: %.6f seconds\n", time_taken);
    printf("Scheduling mode: %s\n", scheduling_mode == 0 ? "block" : "roundrobin");

    return 0;
}
