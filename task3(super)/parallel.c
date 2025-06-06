#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <string.h>

void fill_array(double* arr, int size) {
    unsigned int seed = time(NULL) + MPI_Wtime();
    for (int i = 0; i < size; i++) {
        arr[i] = (double)rand_r(&seed) / RAND_MAX * 100.0;
    }
}

void array_ops(double* a, double* b, double* res_add, double* res_sub,
               double* res_mul, double* res_div, int size) {
    for (int i = 0; i < size; i++) {
        res_add[i] = a[i] + b[i];
        res_sub[i] = a[i] - b[i];
        res_mul[i] = a[i] * b[i];
        res_div[i] = b[i] != 0 ? a[i] / b[i] : 0;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // Получение размера массива из переменных окружения
    char* array_size_str = getenv("ARRAY_SIZE");
    if (!array_size_str && rank == 0) {
        fprintf(stderr, "Error: ARRAY_SIZE environment variable not set\n");
        fprintf(stderr, "Usage: export ARRAY_SIZE=1000000\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int array_size = atoi(array_size_str);
    if (array_size <= 0 && rank == 0) {
        fprintf(stderr, "Error: Invalid array size %d\n", array_size);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Проверка делимости размера массива
    if (array_size % num_procs != 0 && rank == 0) {
        fprintf(stderr, "Warning: Array size (%d) not divisible by number of processes (%d)\n",
                array_size, num_procs);
    }

    int local_size = array_size / num_procs;
    double *a = NULL, *b = NULL;
    double *local_a = malloc(local_size * sizeof(double));
    double *local_b = malloc(local_size * sizeof(double));
    double *local_add = malloc(local_size * sizeof(double));
    double *local_sub = malloc(local_size * sizeof(double));
    double *local_mul = malloc(local_size * sizeof(double));
    double *local_div = malloc(local_size * sizeof(double));

    // Главный процесс заполняет массивы
    if (rank == 0) {
        a = malloc(array_size * sizeof(double));
        b = malloc(array_size * sizeof(double));
        fill_array(a, array_size);
        fill_array(b, array_size);
    }

    // Распределение данных
    MPI_Scatter(a, local_size, MPI_DOUBLE, local_a, local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatter(b, local_size, MPI_DOUBLE, local_b, local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Локальные вычисления
    double start = MPI_Wtime();
    array_ops(local_a, local_b, local_add, local_sub, local_mul, local_div, local_size);
    double end = MPI_Wtime();

    // Сбор результатов
    double *res_add = NULL, *res_sub = NULL, *res_mul = NULL, *res_div = NULL;
    if (rank == 0) {
        res_add = malloc(array_size * sizeof(double));
        res_sub = malloc(array_size * sizeof(double));
        res_mul = malloc(array_size * sizeof(double));
        res_div = malloc(array_size * sizeof(double));
    }

    MPI_Gather(local_add, local_size, MPI_DOUBLE, res_add, local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(local_sub, local_size, MPI_DOUBLE, res_sub, local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(local_mul, local_size, MPI_DOUBLE, res_mul, local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(local_div, local_size, MPI_DOUBLE, res_div, local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Вывод результатов
    if (rank == 0) {
        printf("\nParallel version results:\n");
        printf("Array size: %d\n", array_size);
        printf("Number of processes: %d\n", num_procs);
        printf("Execution time: %.3f ms\n", (end - start) * 1000);

        printf("First 5 results:\n");
        for (int i = 0; i < 5 && i < array_size; i++) {
            printf("[%d] +:%.2f -:%.2f *:%.2f /:%.2f\n",
                  i, res_add[i], res_sub[i], res_mul[i], res_div[i]);
        }

        free(a); free(b);
        free(res_add); free(res_sub);
        free(res_mul); free(res_div);
    }

    free(local_a); free(local_b);
    free(local_add); free(local_sub);
    free(local_mul); free(local_div);

    MPI_Finalize();
    return 0;
}
