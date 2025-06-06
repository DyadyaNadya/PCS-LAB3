#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>

void fill_array(int* arr, int size, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 100;
    }
}

long long calculate_sum(int* arr, int size) {
    long long sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

int main() {
    setlocale(LC_ALL, "ru");

    // Получение размера массива из переменных окружения
    char* size_str = getenv("ARRAY_SIZE");
    int array_size = size_str ? atoi(size_str) : 1000000;
    unsigned int seed = (unsigned int)time(NULL);

    // Выделение памяти
    int* arr = malloc(array_size * sizeof(int));
    if (!arr) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return 1;
    }

    // Заполнение массива
    fill_array(arr, array_size, seed);

    // Вычисление суммы с замером времени
    clock_t start = clock();
    long long sum = calculate_sum(arr, array_size);
    clock_t end = clock();

    // Вывод результатов
    printf("=== Последовательная версия ===\n");
    printf("Размер массива: %d\n", array_size);
    printf("Сумма элементов: %lld\n", sum);
    printf("Время выполнения: %.3f мс\n",
          (double)(end - start) * 1000 / CLOCKS_PER_SEC);

    free(arr);
    return 0;
}
[ilyaafanasyev05.gmail.com@mgr Task_1]$ ^C
[ilyaafanasyev05.gmail.com@mgr Task_1]$ cat parallel.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <locale.h>

void fill_array(int* arr, int size, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 100;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // Получение параметров
    char* size_str = getenv("ARRAY_SIZE");
    int array_size = size_str ? atoi(size_str) : 1000000;
    int local_size = array_size / num_procs;
    unsigned int seed = (unsigned int)time(NULL);

    // Выделение памяти
    int* arr = NULL;
    int* local_arr = malloc(local_size * sizeof(int));
    long long local_sum = 0, global_sum = 0;

    // Главный процесс готовит данные
    if (rank == 0) {
        arr = malloc(array_size * sizeof(int));
        fill_array(arr, array_size, seed);
    }

    // Замер времени работы MPI
    double start_time = MPI_Wtime();

    // Распределение данных
    MPI_Scatter(arr, local_size, MPI_INT,
               local_arr, local_size, MPI_INT,
               0, MPI_COMM_WORLD);

    // Локальные вычисления
    for (int i = 0; i < local_size; i++) {
        local_sum += local_arr[i];
    }

    // Сбор результатов
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();

    // Вывод результатов
    if (rank == 0) {
        printf("\n=== Параллельная версия ===\n");
        printf("Размер массива: %d\n", array_size);
        printf("Количество процессов: %d\n", num_procs);
        printf("Сумма элементов: %lld\n", global_sum);
        printf("Время выполнения: %.3f мс\n", (end_time - start_time) * 1000);

        free(arr);
    }

    free(local_arr);
    MPI_Finalize();
    return 0;
}
