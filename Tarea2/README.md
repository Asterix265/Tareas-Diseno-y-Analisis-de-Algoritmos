# Tarea 2 — Árboles B y Bε en memoria externa (CC4102)

Árbol B (b = 340) y árbol Bε en dos configuraciones, ε ≈ 0,25 (c = 4, l_max = 506) y ε ≈ 0,75 (c = 126, l_max = 384), con nodos de 4096 bytes en un disco simulado, más la batería de experimentos de la sección 4 del enunciado. La tarea fue ejecutada en Ubuntu sin interfaz gráfica.

---

## Requisitos

- Un compilador C++17 con `std::filesystem` (g++ ≥ 9 o clang ≥ 7) y `make`. En Ubuntu:
  `sudo apt install g++ make python3 python3-matplotlib`.
- Python 3 con matplotlib, solo para los gráficos.
- `datos.bin` y `datos_sample.txt` (entregados por el equipo docente) copiados en esta carpeta. No se incluyen en la entrega.

## Compilar

Desde la carpeta `Tarea2/`:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc -o experimento src/main.cpp
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc -o tests_bin tests/tests.cpp
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc -o tests_be tests/tests_be.cpp
```

O con make: `make` compila `experimento` y `tests_bin`, `make test` corre las pruebas del árbol B, `make test_be` compila y corre las pruebas del árbol Bε y `make clean` borra los binarios.

## Ejecutar

```bash
./tests_bin                   # lector de datos.bin, árbol B y prueba básica de cada Bε
./tests_be                    # pruebas extensas del Bε (unos segundos)
./experimento                 # batería completa: N = 2^15, ..., 2^26, 50 búsquedas por árbol
python3 scripts/graficos.py   # gráficos y tabla en figuras/
```

`tests_bin` compara los primeros 50 pares de `datos.bin` con la muestra, compara el árbol B con `std::multimap` (con llaves repetidas) y con un conteo directo sobre los primeros 32 768 pares reales, y hace una prueba básica de cada Bε. `tests_be` prueba ambas configuraciones con α de 0,1, 0,5 y 0,9 sobre árboles chicos, llaves repetidas, crecientes, decrecientes, todas iguales y los primeros 2^18 pares reales: revisa las invariantes del árbol serializado (orden, capacidades, rango de cada subárbol, hojas a la misma profundidad, Σ k_buffer = N) y compara cada búsqueda con una referencia y con el árbol B.

`./experimento` carga los primeros N pares una vez por tamaño, fuera del tiempo medido, y construye por separado el árbol B, Bε(4,506) y Bε(126,384) con α = 0,5. Cada árbol se guarda en disco, se buscan los mismos 50 rangos de una semana en los tres y se borra el archivo. Cada búsqueda de Bε se compara par por par con la del árbol B; si alguna difiere, el programa termina con error. `./experimento --desde 15 --hasta 15` sirve para probar rutas y datos con un solo tamaño.

`scripts/graficos.py` genera siete gráficos (tiempo de construcción, I/Os de construcción totales y por inserción, tamaño, tiempo e I/Os de búsqueda, y temperatura de una misma búsqueda con el menor y el mayor N) y `tabla_metricas.csv`/`.tex`.

### Correr en el servidor

En el servidor de los experimentos (Ryzen 7 3750H) la suma de los tiempos de construcción de la batería completa fue de unos 5,5 minutos; con la carga de datos, el guardado y las búsquedas, la corrida completa toma del orden de 10 minutos. Usamos tmux para que no se cortara al cerrar SSH, con el notebook conectado a la corriente:

```bash
make clean && make all tests_be
make test && make test_be
mkdir -p resultados
{ lscpu; free -h; uname -a; lsb_release -a; g++ --version; cat /proc/cmdline; \
  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor; \
  cat /sys/class/power_supply/AC*/online; git log -1 --oneline; } > resultados/setup_tarea2.txt 2>&1
tmux new -s tarea2
./experimento 2>&1 | tee resultados/log.txt
python3 scripts/graficos.py
```

## Comandos y flags

### Compilación

| Flag | Qué hace |
|---|---|
| `-std=c++17` | Compila con el estándar C++17 (el `main` usa `std::filesystem`). |
| `-O2` | Nivel de optimización estándar. Todos los tiempos reportados se miden con este nivel. |
| `-Wall -Wextra -Wpedantic` | Activan advertencias del compilador; no cambian el programa generado. El código compila sin advertencias. |
| `-Isrc` | Busca los archivos `.h` en `src/`. |

Para buscar errores de memoria, las pruebas se compilaron también con sanitizers. Esos binarios son más lentos y nunca se usan para medir:

```bash
g++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc -o tests_be_asan tests/tests_be.cpp
```

| Flag | Qué hace |
|---|---|
| `-fsanitize=address` | Detecta lecturas o escrituras fuera de un arreglo y uso de memoria ya liberada. |
| `-fsanitize=undefined` | Detecta comportamiento indefinido, como desbordes de enteros con signo. |
| `-O1 -g -fno-omit-frame-pointer` | Optimización baja e información de depuración, para que cada reporte indique la línea exacta. |

### Opciones de `./experimento`

| Opción | Valor por omisión | Qué hace |
|---|---|---|
| `--datos ruta` | `datos.bin` | Archivo binario con los pares llave-valor. |
| `--salida carpeta` | `resultados` | Carpeta donde se escriben los CSV; se crea si no existe. |
| `--desde e`, `--hasta e` | 15 y 26 | Exponentes de la batería: se corre N = 2^e para cada e entre ambos (15 ≤ desde ≤ hasta ≤ 26). |
| `--busquedas q` | 50 | Búsquedas por rango para cada N y cada árbol. |
| `--semilla s` | 4102 | Semilla de `mt19937` para los rangos. Con la misma semilla, los rangos son idénticos para los tres árboles y todos los N. |
| `--alfa a` | 0,5 | α del criterio de vaciado de ambos Bε (0 < α < 1). |
| `--bonus` | — | En vez de la batería, corre el bonus: Bε(126,384) con 8 valores de α (0,1 a 0,8) y 20 búsquedas; escribe `bonus.csv`. No forma parte de los resultados entregados. |
| `--bonus-exponente e` | 24 | Exponente de N para el bonus (15 a 24). |
| `--ayuda` | — | Muestra el uso y termina. |

Cada rango es [l, l + 604 800] (una semana), con l uniforme en [1 546 300 800, 1 754 006 400].

### Qué mide y qué escribe

El tiempo de construcción se mide con `std::chrono::steady_clock` alrededor del ciclo de inserciones, sin contar la lectura de `datos.bin` ni el guardado. El tiempo de búsqueda se mide solo alrededor de `buscarRango`, que lee los nodos desde el archivo.

| Archivo | Contenido |
|---|---|
| `resultados/metricas.csv` | Una fila por árbol y N: tiempo de construcción (ms), lecturas, escrituras e I/Os totales de construcción (contadores de `Disco`), nodos y bytes (nodos × 4096), tiempo promedio de búsqueda (ms), I/Os promedio de búsqueda (bloques leídos del archivo) y pares hallados en promedio. |
| `resultados/verificacion.csv` | Para cada búsqueda de cada Bε, cuántos pares halló, cuántos halló el árbol B e `iguales` = 1 si coinciden par por par. |
| `resultados/rangos.csv` | Los rangos [l, u] usados. |
| `resultados/busqueda_b_<e>.csv` | Resultado de la primera búsqueda (tiempo Unix y temperatura) para el primer y el último N. |
| `resultados/log.txt` | Salida de la corrida, con el tiempo de construcción de cada árbol. |
| `resultados/setup_tarea2.txt` | Ambiente del equipo, según la tabla siguiente. |

### Otros comandos de la corrida

| Comando | Para qué |
|---|---|
| `tmux new -s tarea2` | Abre una sesión que sigue corriendo aunque se corte SSH. `Ctrl+b` y luego `d` la deja en segundo plano; `tmux attach -t tarea2` vuelve a ella. |
| `2>&1 \| tee resultados/log.txt` | Junta la salida normal y la de errores, la muestra en pantalla y además la guarda en `log.txt`. |
| `lscpu` | Modelo de CPU, núcleos, hilos y tamaños de caché L1, L2 y L3. |
| `free -h` | RAM total y disponible. |
| `uname -a`, `lsb_release -a` | Versión del kernel y del sistema operativo. |
| `g++ --version` | Versión del compilador. |
| `cat /proc/cmdline` | Parámetros de arranque del kernel, por ejemplo `tsc=reliable`, que fija el TSC como fuente del reloj que usa `steady_clock`. |
| `cat .../scaling_governor` | Política de frecuencia de la CPU durante la corrida. |
| `cat .../AC*/online` | 1 si el equipo estaba enchufado; en un notebook la frecuencia puede bajar con batería. |
| `git log -1 --oneline` | Commit exacto del código medido. |

## Estructura del código

- `src/datos.h`: `Par` (`int32_t llave`, `float valor`, 8 bytes) y el lector de `datos.bin`.
- `src/disco.h`: `Disco<Nodo>`, el arreglo de nodos en RAM que cuenta lecturas y escrituras y se guarda secuencialmente en un binario, y `ArchivoNodos<Nodo>`, que lee el bloque i en la posición i · 4096 del archivo contando una lectura. Ningún árbol accede al arreglo ni al archivo directamente.
- `src/arbol_b.h`: `NodoB` y `ArbolB` (split, inserción y búsqueda por rango de las secciones 3.4.1, 3.4.2 y 3.4.5).
- `src/arbol_be.h`: `NodoBe<C, L>` y `ArbolBe<C, L>` (split de hoja e interno, vaciado, inserción y búsqueda de las secciones 3.4.3, 3.4.4 y 3.4.6), con los alias `ArbolBe025 = ArbolBe<4, 506>` y `ArbolBe075 = ArbolBe<126, 384>`.
- `src/main.cpp`: la batería de experimentos.

La raíz siempre ocupa el bloque 0: al crecer, sus dos mitades se agregan al final y el bloque 0 se sobrescribe con la nueva raíz. Las decisiones que el enunciado no especifica (llaves repetidas, grupos que no caben después de un split, nodos internos que llegan a c llaves) se explican en la sección Desarrollo del informe.

## Reproducibilidad

Los rangos se generan con `std::mt19937` y la semilla 4102, combinando dos salidas en un entero de 64 bits, sin `std::uniform_int_distribution`; así la misma semilla produce los mismos rangos en cualquier SO. La inserción es determinista: los I/Os, la cantidad de nodos y los pares hallados son idénticos entre corridas y entre equipos; solo los tiempos dependen de la máquina.
