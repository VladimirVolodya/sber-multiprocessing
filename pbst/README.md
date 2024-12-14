# External BST

## Инструкция по воспроизведению эксперимента

Для воспроизведения экспериментов запустите `./run.sh <parallelism> <x> <duration>`. Он скомпилирует файлы и повторит эксперимент 5 раз.
Эксперимент заключается в генерации случаного дерева с сключами в диапазоне `[0, 100'000]`, где каждый ключ из диапазона добавляется в дерево с вероятностью `1/2`.

После `<parallelism>` потоков в течение `<duration>` секунд выполняют случайные операции на дереве и записывают проведенные операции и их результаты (имеет смысл для проверки наличия ключа).

Параметр `x` отвечает за соотношение выполняемых операций. `x` должен лежать в диапазоне `[0, 5]`.

Операции выполняются со следующими вероятностями:
 - `Insert` с вероятностью `x/10`,
 - `Remove` с вероятностью `x/10`,
 - `Contains` с вероятностью `1 - x/5`.

Ключи-аргументы операций выбираются равномерно из всего диапазона значений.

После проведения эксперимента история операций проверяется на линеризуемость, полученное дерево проверяется на соответствие external BST. Подробнее в соответсвующих пунктах.

Все параметры являются обязательными.

Пример запуска:
```
vsm@fedora:~/VSCodeProjects/pbst$ ./run.sh 4 1 5
-- The C compiler identification is GNU 14.2.1
-- The CXX compiler identification is GNU 14.2.1
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done (0.5s)
-- Generating done (0.0s)
-- Build files have been written to: /home/vsm/VSCodeProjects/pbst/build
[ 50%] Building CXX object CMakeFiles/pbst.dir/main.cpp.o
[100%] Linking CXX executable pbst
[100%] Built target pbst
Experiment #1:
Running experiment for parallelism=4, x=1, duration=5s...
Experiment finished, running checks...
Performed operations history is linearizable: true
Result structure is a valid external BST: true
Average bandwidth: 1446938 op/s

Experiment #2:
Running experiment for parallelism=4, x=1, duration=5s...
Experiment finished, running checks...
Performed operations history is linearizable: true
Result structure is a valid external BST: true
Average bandwidth: 1472613 op/s

Experiment #3:
Running experiment for parallelism=4, x=1, duration=5s...
Experiment finished, running checks...
Performed operations history is linearizable: true
Result structure is a valid external BST: true
Average bandwidth: 1470251 op/s

Experiment #4:
Running experiment for parallelism=4, x=1, duration=5s...
Experiment finished, running checks...
Performed operations history is linearizable: true
Result structure is a valid external BST: true
Average bandwidth: 1488447 op/s

Experiment #5:
Running experiment for parallelism=4, x=1, duration=5s...
Experiment finished, running checks...
Performed operations history is linearizable: true
Result structure is a valid external BST: true
Average bandwidth: 1503121 op/s
```

## Валидация линеаризуемости истории

Каждый поток в самом начале операции фиксирует аналог временной отметки ее начала.
Фактически это число операций, которые были **начаты** до начала текущей операции.

В историю операций поток записывает тип операции (`Insert`, `Remove`, `Contains`), ключ, с которым операция была вызвана, временную метку начала и полученный результат (имеет смысл лишь для операции `Contains`).

После истории потоков анализируются по следующему алгоритму:

 0. Считаем, что поток выполняет операцию в течение промежутка времени между двумя временными метками - меткой начала текущей операции и меткой начала следующей, либо `+inf` в случае, если текущая операция была для потока последней. 
 1. Отслеживается два множества ключей - ключи, про которые мы точно можем сказать, что они присутствуют в дереве, и находящиеся в суперпозиции.
 2. Ключ находится в суперпозиции в трех случая:
    - Он гарантированно присутствовал в дереве и один из потоков в данный момент его удаляет,
    - Он гарантированно отсутствовал в дереве и один из потоков в данный момент его добавляет,
    - В данный момент или ранее хотя бы два потока конкурентно вставляли и удаляли этот ключ.
 3. Для каждого ключа в суперпозиции отслеживается верхняя граница времени его суперпозиции. Это тот момент, когда исчезла конкуренция за этот ключ. В этот момент ключ либо уходит из состояния суперпозиции, либо остается в нем, но в том смысле, что он уже либо существует в дереве, либо нет. То есть все вызовы `Contains` с этим ключом должны возвращать одинаковый результат, но неизвестно, какой именно.
 4. Если ключ находится в суперпозиции, все операции `Contains` с этим ключом игнорируются, так как любой результат такой операции будет валидным. Если ключ не в суперпозиции, рузельтат операции `Contains` должен совпадать с наличием ключа в гарантированном множестве ключей.

## Валидация корректности результирующего external BST
Во-первых, проверяется соответствие полученного дерева структуре BST.

Во-вторых, для каждой routing-вершины проверяется наличие хотя бы одного потомка.

В-третьих, для каждой не-routing-вершины проверяется отсутствие потомков.

## Результаты

Итоговые замеры пропускных способностей приведены в таблице (mln op/s).

| x \ parallelism| 1 | 2 | 3 | 4 |
|----------------|---|---|---|---|
| 0 | 0.8 | 0.8 | 1.2 | 1.7 |
| 1 | 0.7 | 0.9 | 1.2 | 1.6 |
| 5 | 0.7 | 0.8 | 1.1 | 1.6 |

В среднем ускорение на `4` потоках в сравнении с `1` потоком в `2.3` раза.
