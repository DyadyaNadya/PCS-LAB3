#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define ITERATIONS 100

void fill_array_random(int* arr, int size, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 100;
    }
}

long long sequential_sum(int* arr, int size) {
    long long sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

int main(int argc, char* argv[]) {
    int rank, num_procs, array_size, local_size;
    int* arr = NULL, * local_arr = NULL;
    long long total_sum = 0, local_sum = 0, sequential_result = 0;
    double total_seq_time = 0.0, total_par_time = 0.0;
    unsigned int seed = 0;

    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // Get parameters
    char* array_size_str = getenv("ARRAY_SIZE");
    array_size = array_size_str ? atoi(array_size_str) : 100000000;
    seed = (unsigned int)time(NULL) + rank;  // Different seed for each process

    // Validate parameters
    if (array_size <= 0) array_size = 100000000;
    if (array_size < num_procs && rank == 0) {
        fprintf(stderr, "Warning: Array size is smaller than number of processes\n");
    }

    local_size = (array_size + num_procs - 1) / num_procs;

    // Allocate memory once
    if (rank == 0) {
        arr = (int*)malloc(array_size * sizeof(int));
    }
    local_arr = (int*)malloc(local_size * sizeof(int));

    // Warm-up run (to avoid cold start effects)
    if (rank == 0) {
        fill_array_random(arr, array_size, seed);
        sequential_sum(arr, array_size);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Main measurement loop
    for (int iter = 0; iter < ITERATIONS; iter++) {
        if (rank == 0) {
            // Prepare new random data
            fill_array_random(arr, array_size, seed + iter);

            // Measure sequential time
            double seq_start = MPI_Wtime();
            sequential_result = sequential_sum(arr, array_size);
            double seq_end = MPI_Wtime();
            total_seq_time += (seq_end - seq_start);
        }

        // Synchronize before parallel section
        MPI_Barrier(MPI_COMM_WORLD);
        double par_start = MPI_Wtime();

        // Parallel computation
        MPI_Scatter(arr, local_size, MPI_INT, local_arr, local_size, MPI_INT, 0, MPI_COMM_WORLD);

        local_sum = 0;
        for (int i = 0; i < local_size; i++) {
            local_sum += local_arr[i];
        }

        MPI_Reduce(&local_sum, &total_sum, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        double par_end = MPI_Wtime();

        if (rank == 0) {
            total_par_time += (par_end - par_start);
        }
    }

    // Print results
    if (rank == 0) {
        double avg_seq_time = total_seq_time / ITERATIONS;
        double avg_par_time = total_par_time / ITERATIONS;

        printf("Array size: %d\n", array_size);
        printf("Number of processes: %d\n", num_procs);
        printf("\nAverage execution time:\n");
        printf("  Sequential sum: %.6f sec\n", avg_seq_time);
        printf("  Parallel sum:   %.6f sec\n", avg_par_time);
        printf("  Speedup:       %.2fx\n", avg_seq_time / avg_par_time);

        free(arr);
    }

    free(local_arr);
    MPI_Finalize();
    return 0;
}
