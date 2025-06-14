#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define ITERATIONS 100
#define DEFAULT_SIZE 30000000

typedef struct {
    double add_time;
    double sub_time;
    double mul_time;
    double div_time;
} OpTimes;

void fill_arrays(double* a, double* b, int size, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < size; i++) {
        a[i] = (double)(rand() % 100 + 1); // Avoid division by zero
        b[i] = (double)(rand() % 100 + 1);
    }
}

void array_operations_timed(double* a, double* b, double* add, double* sub,
                          double* mul, double* div, int size, OpTimes* times) {
    double start;
    
    start = MPI_Wtime();
    for (int i = 0; i < size; i++) add[i] = a[i] + b[i];
    times->add_time += MPI_Wtime() - start;
    
    start = MPI_Wtime();
    for (int i = 0; i < size; i++) sub[i] = a[i] - b[i];
    times->sub_time += MPI_Wtime() - start;
    
    start = MPI_Wtime();
    for (int i = 0; i < size; i++) mul[i] = a[i] * b[i];
    times->mul_time += MPI_Wtime() - start;
    
    start = MPI_Wtime();
    for (int i = 0; i < size; i++) div[i] = a[i] / b[i];
    times->div_time += MPI_Wtime() - start;
}

int main(int argc, char* argv[]) {
    int rank, num_procs, array_size, local_size;
    double* a = NULL, * b = NULL;
    double* local_a = NULL, * local_b = NULL;
    double* local_add = NULL, * local_sub = NULL;
    double* local_mul = NULL, * local_div = NULL;

    OpTimes seq_times = {0}, par_times = {0};

    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // Get array size from environment or use default
    char* size_str = getenv("ARRAY_SIZE");
    array_size = size_str ? atoi(size_str) : DEFAULT_SIZE;

    // Check if array size is divisible by number of processes
    if (array_size % num_procs != 0) {
        if (rank == 0) {
            printf("Error: Array size (%d) must be divisible by number of processes (%d)\n",
                array_size, num_procs);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    local_size = array_size / num_procs;

    // Allocate memory
    if (rank == 0) {
        a = (double*)malloc(array_size * sizeof(double));
        b = (double*)malloc(array_size * sizeof(double));
        if (a == NULL || b == NULL) {
            printf("Error: Memory allocation failed for main arrays\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    local_a = (double*)malloc(local_size * sizeof(double));
    local_b = (double*)malloc(local_size * sizeof(double));
    local_add = (double*)malloc(local_size * sizeof(double));
    local_sub = (double*)malloc(local_size * sizeof(double));
    local_mul = (double*)malloc(local_size * sizeof(double));
    local_div = (double*)malloc(local_size * sizeof(double));

    if (local_a == NULL || local_b == NULL || local_add == NULL ||
        local_sub == NULL || local_mul == NULL || local_div == NULL) {
        printf("Error: Memory allocation failed in process %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Warm-up run
    if (rank == 0) {
        fill_arrays(a, b, array_size, time(NULL));
    }

    // Benchmark loop
    for (int iter = 0; iter < ITERATIONS; iter++) {
        if (rank == 0) {
            fill_arrays(a, b, array_size, time(NULL) + iter);

            // Sequential timing
            double* seq_add = (double*)malloc(array_size * sizeof(double));
            double* seq_sub = (double*)malloc(array_size * sizeof(double));
            double* seq_mul = (double*)malloc(array_size * sizeof(double));
            double* seq_div = (double*)malloc(array_size * sizeof(double));

            if (seq_add == NULL || seq_sub == NULL || seq_mul == NULL || seq_div == NULL) {
                printf("Error: Memory allocation failed for sequential arrays\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            array_operations_timed(a, b, seq_add, seq_sub, seq_mul, seq_div, array_size, &seq_times);

            free(seq_add); free(seq_sub); free(seq_mul); free(seq_div);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        // Scatter data with error checking
        int rc;
        rc = MPI_Scatter(a, local_size, MPI_DOUBLE, local_a, local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) {
            printf("Error in MPI_Scatter (a) in process %d\n", rank);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

        rc = MPI_Scatter(b, local_size, MPI_DOUBLE, local_b, local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) {
            printf("Error in MPI_Scatter (b) in process %d\n", rank);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

        // Parallel operations with timing
        array_operations_timed(local_a, local_b, local_add, local_sub, 
                             local_mul, local_div, local_size, &par_times);

        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Print results
    if (rank == 0) {
        printf("=== Array Operations Benchmark ===\n");
        printf("Array size: %d\n", array_size);
        printf("Processes: %d\n", num_procs);
        printf("Iterations: %d\n", ITERATIONS);
        
        printf("\nAverage sequential times:\n");
        printf("Addition: %.6f sec\n", seq_times.add_time / ITERATIONS);
        printf("Subtraction: %.6f sec\n", seq_times.sub_time / ITERATIONS);
        printf("Multiplication: %.6f sec\n", seq_times.mul_time / ITERATIONS);
        printf("Division: %.6f sec\n", seq_times.div_time / ITERATIONS);
        
        printf("\nAverage parallel times:\n");
        printf("Addition: %.6f sec\n", par_times.add_time / ITERATIONS);
        printf("Subtraction: %.6f sec\n", par_times.sub_time / ITERATIONS);
        printf("Multiplication: %.6f sec\n", par_times.mul_time / ITERATIONS);
        printf("Division: %.6f sec\n", par_times.div_time / ITERATIONS);
        
        printf("\nSpeedup factors:\n");
        printf("Addition: %.2fx\n", (seq_times.add_time / ITERATIONS) / (par_times.add_time / ITERATIONS));
        printf("Subtraction: %.2fx\n", (seq_times.sub_time / ITERATIONS) / (par_times.sub_time / ITERATIONS));
        printf("Multiplication: %.2fx\n", (seq_times.mul_time / ITERATIONS) / (par_times.mul_time / ITERATIONS));
        printf("Division: %.2fx\n", (seq_times.div_time / ITERATIONS) / (par_times.div_time / ITERATIONS));
    }

    // Cleanup
    if (a) free(a);
    if (b) free(b);
    if (local_a) free(local_a);
    if (local_b) free(local_b);
    if (local_add) free(local_add);
    if (local_sub) free(local_sub);
    if (local_mul) free(local_mul);
    if (local_div) free(local_div);

    MPI_Finalize();
    return 0;
}
