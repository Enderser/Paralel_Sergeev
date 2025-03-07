# README.md

# Проект: Сумма синуса

## Описание
Программа создает массив значений синуса на одном периоде и вычисляет его сумму.

## Сборка с помощью CMake
### С `double` (по умолчанию):
```sh
cmake -B build
cmake --build build
```
### С `float`:
```sh
cmake -DUSE_DOUBLE=OFF -B build
cmake --build build
```

## Сборка с помощью Makefile
### С `double` (по умолчанию):
```sh
make
```
### С `float`:
```sh
make USE_DOUBLE=0
```

## Запуск
После сборки запустить исполняемый файл:
```sh
./sin_array_sum
```