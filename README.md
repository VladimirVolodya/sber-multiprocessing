# Сравнение параллельной и однопоточной реализаций quick-sort

Логи экспериментов:
```
Running experiment #1...
Array is sorted before: false
Time difference (single thread) = 51757822[µs]
Array is sorted after: true
Array is sorted before: false
Time difference (multiple threads) = 19871429[µs]
Array is sorted after: true

Running experiment #2...
Array is sorted before: false
Time difference (single thread) = 49927569[µs]
Array is sorted after: true
Array is sorted before: false
Time difference (multiple threads) = 19844013[µs]
Array is sorted after: true

Running experiment #3...
Array is sorted before: false
Time difference (single thread) = 52894558[µs]
Array is sorted after: true
Array is sorted before: false
Time difference (multiple threads) = 20332055[µs]
Array is sorted after: true

Running experiment #4...
Array is sorted before: false
Time difference (single thread) = 50500977[µs]
Array is sorted after: true
Array is sorted before: false
Time difference (multiple threads) = 19964503[µs]
Array is sorted after: true

Running experiment #5...
Array is sorted before: false
Time difference (single thread) = 49783446[µs]
Array is sorted after: true
Array is sorted before: false
Time difference (multiple threads) = 18523384[µs]
Array is sorted after: true

```

В среднем ускорение в `2.6` раз.

Для воспроизведения экспериментов запустите `./run.sh`. Он скомпилирует файлы и повторит эксперимент 5 раз.
Для каждого эксперимента генерируется случайный массив размером `1e8` элементов. Он сортируется двумя способами (в параллельном и однопоточном режимах).
