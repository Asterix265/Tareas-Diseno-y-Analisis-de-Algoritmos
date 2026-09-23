# Tarea 1 — Algoritmo de Prim y Análisis Amortizado (CC4102)

Prim implementado con **cola binomial** y **cola de Fibonacci**, más el generador
de grafos y la batería de experimentos de la sección 6. Todo en C++17 estándar:
compila igual en Linux, macOS y Windows.

## Requisitos

- Un compilador C++17 con `std::filesystem` (g++ ≥ 9, clang ≥ 7 o MSVC 2019+). En Ubuntu:
  `sudo apt install g++ make python3 python3-matplotlib`.
- Python 3 con `matplotlib` solo para los gráficos.

## Compilar

Desde la carpeta `Tarea1/`:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -o prim src/main.cpp
g++ -std=c++17 -O2 -Wall -Wextra -Isrc -o tests_bin tests/tests.cpp
```

O con `make` (Linux/macOS): `make` compila ambos.

## Ejecutar

```bash
./tests_bin                  # pruebas de corrección (grafos chicos vs Kruskal)
./prim                       # TODOS los experimentos: series A–D, 10 repeticiones, ambas colas
python3 scripts/graficos.py  # 12 gráficos + tabla en figuras/
```

`./prim` no necesita argumentos para correr la batería completa. Opciones:

| Opción | Efecto | Por defecto |
|---|---|---|
| `--series ABCD` | Qué series correr | `ABCD` |
| `--reps N` | Repeticiones por configuración | `10` |
| `--colas binomial,fibonacci` | Colas a usar (`falsa` = cola trivial para pruebas) | ambas |
| `--reducir k` | Resta k a i y j (prueba rápida con grafos 2^k veces más chicos) | `0` |
| `--salida dir` | Carpeta de los CSV | `resultados` |
| `--semilla S` | Semilla base | `20260928` |
| `--cada K` | Guarda un punto de la curva cada K llamadas a decreaseKey | automático |
| `--memoria` | Solo imprime la estimación de memoria (sección 6.2) | — |

Ejemplos:

```bash
./prim --reducir 4 --reps 2            # prueba rápida de toda la batería (segundos)
./prim --series AB                      # solo costo total
./prim --series CD                      # solo costo amortizado
./prim --memoria
```

Los resultados quedan en `resultados/tiempos_<series>.csv`, `curvas_<series>.csv`
y `verificacion_<series>.csv` (peso del MST de ambas colas en cada grafo).
Los archivos se vacían al disco después de cada repetición.

### Correr en el servidor

Las series completas pueden tardar horas. Usar `tmux` para que no se corten al cerrar SSH:

```bash
tmux new -s prim
mkdir -p resultados
g++ -std=c++17 -O2 -o prim src/main.cpp
./prim 2>&1 | tee resultados/log.txt
# Ctrl-b d para salir; tmux attach -t prim para volver
```

Datos del equipo para el informe: `lscpu; free -h; uname -a; g++ --version; lsb_release -a`.

## Estructura

```
src/
  grafo.h           Grafo en formato CSR
  aleatorio.h       Generador pseudoaleatorio portable (mt19937 sin distribuciones de std)
  generador.h       Grafos conexos aleatorios (sección 6.1)
  cola_binomial.h   Cola binomial
  cola_fibonacci.h  Cola de Fibonacci
  cola_falsa.h      Cola trivial O(n) para desarrollo y tests
  prim.h            Prim genérico + medición de tiempos y operaciones
  main.cpp          Batería de experimentos y salida CSV
tests/tests.cpp     Pruebas: generador, colas vs referencia, Prim vs Kruskal
scripts/graficos.py Gráficos con cota teórica ajustada y tabla de tiempos
  docs/ACUERDOS.md    Interfaz y decisiones del equipo
```

## Recorrido del código

1. `generarGrafo` crea un árbol aleatorio, agrega aristas sin repeticiones y
   transforma la lista en CSR. El peso se obtiene de `Aleatorio::peso`.
2. `prim<Cola>` inserta todos los vértices, extrae el mínimo y reduce la clave
   de cada vecino que mejora su conexión con el árbol.
3. `ColaBinomial::insert` une árboles de igual grado mediante acarreos;
   `extractMin` promueve los hijos del mínimo; `decreaseKey` intercambia
   contenido con el padre y actualiza `nodoDe`.
4. `ColaFibonacci::insert` añade una raíz; `extractMin` consolida raíces
   del mismo grado; `decreaseKey` aplica `cortar` y `corteCascada`.
5. `main.cpp` ejecuta las cuatro series con semillas reproducibles, guarda
   tiempos y contadores en CSV, y verifica los pesos de ambos MST.

## Reproducibilidad

Cada grafo se genera con una semilla determinista a partir de (serie, i, j, repetición).
El generador usa solo `std::mt19937` convertido a mano, así que la misma semilla
produce el mismo grafo en cualquier sistema operativo y compilador. Los grafos no
se guardan en disco.
