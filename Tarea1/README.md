# Tarea 1 — Algoritmo de Prim y Análisis Amortizado (CC4102)

Prim implementado con **cola binomial** y **cola de Fibonacci**, más el generador de grafos y la batería de experimentos. La tarea fue ejecutada en ubuntu sin interfaz grafica.

---

## Requisitos

- Un compilador C++17 con std::filesystem (g++ ≥ 9, clang ≥ 7 o MSVC 2019+). En Ubuntu:
  sudo apt install g++ make python3 python3-matplotlib.
- Python 3 con matplotlib solo para los gráficos.

## Compilar

Desde la carpeta `Tarea1/`:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -o prim src/main.cpp
g++ -std=c++17 -O2 -Wall -Wextra -Isrc -o tests_bin tests/tests.cpp
```

O con make (Linux/macOS): make compila ambos, make test corre las pruebas,
make rapido corre la batería con grafos chicos y make memoria imprime la estimación.

## Ejecutar

```bash
./tests_bin    # Tests
./prim    # todos los experimentos: series A–D, 10 repeticiones, ambas colas
python3 scripts/graficos.py  # 12 gráficos + tablas en figuras/
```



### Correr en el servidor

La batería completa tardó unos 20 minutos en el servidor de los experimentos (Ryzen 7 3750H). En el servidor usamos tmux para que no se cortara al cerrar SSH:

```bash
mkdir -p resultados
g++ -std=c++17 -O2 -o prim src/main.cpp
./prim 2>&1 | tee resultados/log.txt
```


## Reproducibilidad

Cada grafo es generado con una semilla determinista a partir de (serie, i, j, repeticion). El generador solo usa std::mt19937 asi que la misma semilla produce el mismo grafo en cualquier SO.

