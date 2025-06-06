#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void fill_array(double* arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = (double)rand() / RAND_MAX * 100.0;
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
    // Получение размера массива из переменных окружения
    char* array_size_str = getenv("ARRAY_SIZE");
    if (!array_size_str) {
        fprintf(stderr, "Error: ARRAY_SIZE environment variable not set\n");
        fprintf(stderr, "Usage: export ARRAY_SIZE=1000000\n");
        return 1;
    }

    int array_size = atoi(array_size_str);
    if (array_size <= 0) {
        fprintf(stderr, "Error: Invalid array size %d\n", array_size);
        return 1;
    }

    // Выделение памяти
    double *a = malloc(array_size * sizeof(double));
    double *b = malloc(array_size * sizeof(double));
    double *res_add = malloc(array_size * sizeof(double));
    double *res_sub = malloc(array_size * sizeof(double));
    double *res_mul = malloc(array_size * sizeof(double));
    double *res_div = malloc(array_size * sizeof(double));

    // Инициализация массивов
    srand(time(NULL));
    fill_array(a, array_size);
    fill_array(b, array_size);

    // Выполнение операций
    clock_t start = clock();
    array_ops(a, b, res_add, res_sub, res_mul, res_div, array_size);
    clock_t end = clock();

    // Вывод результатов
    printf("Sequential version results:\n");
    printf("Array size: %d\n", array_size);
    printf("Execution time: %.3f ms\n",
          (double)(end - start) * 1000 / CLOCKS_PER_SEC);

    // Освобождение памяти
    free(a); free(b);
    free(res_add); free(res_sub);
    free(res_mul); free(res_div);

    return 0;
}
