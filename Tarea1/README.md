# Tarea 1 — Algoritmo de Prim y Análisis Amortizado (CC4102)

Prim implementado con **cola binomial** y **cola de Fibonacci**, más el generador
de grafos y la batería de experimentos de la sección 6. Todo en C++17 estándar:
compila igual en Linux, macOS y Windows.

> El plan de trabajo por persona está en [`docs/PLAN.md`](docs/PLAN.md) y la
> interfaz acordada en [`docs/ACUERDOS.md`](docs/ACUERDOS.md).

---

## Estado del esqueleto

La infraestructura y las dos colas están implementadas. Mientras una cola tenga
`implementada = false`, los tests y `./prim` la omiten con un aviso; hoy ambas
tienen `implementada = true` y participan en los tests y en los experimentos.

| Archivo | Estado | Responsable | Qué hace |
|---|---|---|---|
| `src/grafo.h` | ✅ Listo | Infra | Grafo en formato CSR (`inicio`, `destino`, `peso`); cada arista se guarda dos veces. Incluye `bytesEstimados(n, m)` para la sección 6.2. |
| `src/aleatorio.h` | ✅ Listo | Infra | Envoltorio de `std::mt19937` con conversiones hechas a mano (`enRango`, `peso` en (0,1]). Misma semilla ⇒ mismos números en cualquier SO. |
| `src/generador.h` | ✅ Listo | Infra | `generarGrafo(v, e, semilla)`: árbol cobertor aleatorio + aristas al azar, descartando repetidas y lazos. Memoria O(e), sin matrices v×v. |
| `src/prim.h` | ✅ Listo | Infra | `prim<Cola>(g, raiz, medirDK, cadaK)`: Prim genérico. Mide el tiempo total, cuenta las llamadas a `decreaseKey` y, en las series C/D, su tiempo acumulado y una curva. |
| `src/main.cpp` | ✅ Listo | Infra | Corre las series A–D sin tocar código y escribe los CSV en `resultados/`. Verifica que ambas colas den el mismo peso de MST. |
| `src/cola_falsa.h` | ✅ Listo | — | Cola trivial (arreglo + búsqueda lineal, O(n) por extracción). Solo para desarrollo y como referencia en los tests. |
| `src/cola_binomial.h` | ✅ Listo | Colas | Raíces ordenadas por grado; `insert` con acarreos (n inserciones en O(n)); `decreaseKey` intercambia clave y vértice con el padre y actualiza `nodoDe`. |
| `src/cola_fibonacci.h` | ✅ Listo | Colas | Anillos doblemente enlazados; `extractMin` consolida por grado; `decreaseKey` con `cortar` y `corteCascada` (cada corte suma a `ops`). |
| `tests/tests.cpp` | ✅ Listo | Ambos | Prueba el generador; cada cola contra `ColaFalsa` con operaciones aleatorias; Prim contra Kruskal. Las colas no implementadas se omiten. |
| `scripts/graficos.py` | ✅ Listo | Infra | Los 12 gráficos con la cota teórica ajustada por mínimos cuadrados, y la tabla de tiempos (CSV + LaTeX). |
| `docs/ACUERDOS.md` | ✅ Listo | Ambos | Interfaz congelada, convenciones, formato de CSV, preguntas para auxiliares. |
| `docs/PLAN.md` | ✅ Listo | Ambos | Tareas de cada persona y trabajo conjunto antes de los experimentos. |

### La interfaz que conecta todo

Prim no conoce los nodos de las colas; solo usa estos métodos (resumen de `docs/ACUERDOS.md`):

```cpp
explicit Cola(int n);                  // n = |V|
void insert(int v, double key);
std::pair<double,int> extractMin();    // (costo, vértice)
void decreaseKey(int v, double key);
bool empty() const;
int64_t ops;                           // intercambios (binomial) o cortes (Fibonacci) en decreaseKey
static constexpr bool implementada;    // poner en true cuando la cola pase los tests
static size_t bytesPorNodo();          // para la estimación de memoria
```

Para terminar una cola basta con implementar esos métodos en su archivo y cambiar
`implementada` a `true`. No hay que tocar `prim.h`, `main.cpp` ni los tests.

### Flujo de datos

```
generarGrafo(v, e, semilla) ──► Grafo (CSR) ──► prim<Cola>(...) ──► ResultadoPrim
                                                                        │
                                   main.cpp escribe resultados/*.csv ◄──┘
                                                                        │
                           scripts/graficos.py ──► figuras/*.png + tabla ┘
```

---

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

O con `make` (Linux/macOS): `make` compila ambos, `make test` corre las pruebas,
`make rapido` corre la batería con grafos chicos y `make memoria` imprime la estimación.

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
./prim --reducir 4 --reps 2               # prueba rápida de toda la batería
./prim --colas falsa --reducir 9 --reps 1 # probar main y gráficos sin colas reales
./prim --series AB                        # solo costo total
./prim --series CD                        # solo costo amortizado
./prim --memoria
```

### Archivos de salida

| Archivo | Contenido |
|---|---|
| `resultados/tiempos_<series>.csv` | Una fila por (configuración, repetición, cola): tiempo total, peso del MST, llamadas, tiempo y operaciones de `decreaseKey`. |
| `resultados/curvas_<series>.csv` | Curva acumulada de `decreaseKey` (series C y D, repetición 0). |
| `resultados/verificacion_<series>.csv` | Peso del MST de ambas colas sobre el mismo grafo y si coinciden. |

Cada fila se escribe apenas termina, así que si la corrida se corta, lo medido no
se pierde. Las corridas con `--reducir` llevan el sufijo `_red<k>` y `graficos.py`
las ignora salvo que se use `--reducidos`.

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
docs/PLAN.md        Plan de trabajo por persona
resultados/         CSV generados por ./prim
figuras/            PNG y tablas generados por graficos.py
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
se guardan en disco (el enunciado pide no entregar datasets).
