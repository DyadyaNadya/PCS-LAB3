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
