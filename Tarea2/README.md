# Tarea 2 — Árboles B y Bε (CC4102)

Estado: infraestructura de bloques, lector de `datos.bin`, árbol B, dos configuraciones Bε, experimentos y bonus implementados. Las mediciones locales verifican el funcionamiento; los tiempos finales se obtendrán en el servidor.

## Requisitos

- Compilador C++17 y `make`. Para los gráficos, Python 3 con `matplotlib` (`python3 -m pip install matplotlib`).
- `datos.bin` y `datos_sample.txt` en esta carpeta. Ninguno se incluye en Git.

## Verificación actual

Desde `Tarea2/`, ejecutar `make test`. La prueba compara los primeros 50 pares de `datos.bin` con la muestra, inserta 10 000 pares (incluidas llaves repetidas) en B y compara 100 búsquedas con `std::multimap`. También inserta los primeros 32 768 pares reales en B y comprueba 10 búsquedas contra un conteo directo. Para cada Bε inserta 30 000 pares con duplicados y 20 000 llaves crecientes, comprueba la cantidad total y compara búsquedas con la referencia.

`make test_be` corre las pruebas extensas del Bε (`tests/tests_be.cpp`, unos segundos): ambas configuraciones con α de 0,1, 0,5 y 0,9, árboles chicos, llaves repetidas, crecientes, decrecientes, todas iguales y los primeros `2^18` pares reales. Revisa las invariantes del árbol serializado (orden, capacidades, rangos de cada subárbol, hojas a la misma profundidad, Σ k_buffer = N) y compara cada búsqueda con una referencia y con el árbol B. Sin `datos.bin` omite los casos reales.

## Experimento de los tres árboles

`make` compila `experimento`. `./experimento --desde 15 --hasta 15` ejecuta un tamaño de prueba; sin opciones ejecuta `2^15` a `2^26`, con 50 búsquedas por tamaño y por árbol. `--datos`, `--salida`, `--busquedas`, `--semilla` y `--alfa` permiten configurar la corrida. El valor predeterminado de α es 0,5. Se cargan los primeros N pares una vez por tamaño, fuera del tiempo medido, y se construyen B, Bε(4,506) y Bε(126,384) por separado. Así el tiempo de construcción mide las inserciones, sin contar la lectura de `datos.bin` ni el guardado final del árbol.

Se generan `resultados/metricas.csv`, `verificacion.csv`, `rangos.csv` y, para el primer y último tamaño de la corrida, un CSV de la primera búsqueda de B con tiempo Unix y temperatura. Los rangos son los mismos para todos los árboles y valores de N. Cada búsqueda de Bε se compara par por par con B; si hay diferencias, el programa termina con error. Los árboles serializados se borran después de las búsquedas para limitar el espacio usado. El tiempo de búsqueda excluye ordenar los resultados para compararlos y escribir los CSV.

`./experimento --bonus` corre Bε(126,384) con N=`2^24`, 20 búsquedas por valor y α de 0,1 a 0,8 en pasos de 0,1. Escribe `resultados/bonus.csv`. `--bonus-exponente 15` permite probar el flujo rápidamente; la corrida entregable usa el valor predeterminado 24.

`python3 scripts/graficos.py` genera cinco gráficos comparativos de métricas en `figuras/`. Si hay curvas de dos tamaños, también grafica la temperatura de la misma búsqueda. Si existe `bonus.csv`, grafica construcción y búsqueda según α.

## Servidor de experimentos

El servidor usado en la Tarea 1 fue `kestrelserver`: Ubuntu 26.04.1 LTS, x86_64, AMD Ryzen 7 3750H (4 núcleos, 8 hilos), 9,1 GiB de RAM y g++ 15.2.0. `Tarea1/resultados/setup.txt` registra ese ambiente. Antes de la corrida final de Tarea 2 hay que confirmar que sigue siendo el mismo y guardar un setup nuevo, pues la memoria disponible, la versión del sistema y otros detalles pueden cambiar.

En el servidor, situarse en `Tarea2/`, copiar `datos.bin` y `datos_sample.txt` por separado (no se suben al repositorio) y ejecutar, en orden:

```bash
make clean && make all tests_be
make test && make test_be
./experimento --desde 15 --hasta 18 --salida resultados_prueba
mkdir -p resultados
{ lscpu; free -h; uname -a; lsb_release -a; g++ --version; cat /proc/cmdline; \
  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor; \
  cat /sys/class/power_supply/AC*/online; git log -1 --oneline; } > resultados/setup_tarea2.txt 2>&1
tmux new -s tarea2
./experimento 2>&1 | tee resultados/log.txt
python3 scripts/graficos.py
```

Los dos primeros comandos compilan y corren las pruebas; el tercero es una batería corta que comprueba rutas y datos. El bloque entre llaves guarda el ambiente del equipo, la corrida completa va dentro de `tmux` y al final se generan los gráficos. Para el informe, usar solo mediciones del servidor, sin otras cargas, y registrar cualquier cambio de configuración del equipo. Qué hace cada comando y cada flag está en la sección siguiente.

## Comandos y flags

### Compilación

El Makefile compila con `g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc`. Los objetivos son `make` o `make all` (`experimento` y `tests_bin`), `make tests_be`, `make test`, `make test_be` y `make clean`.

| Flag | Qué hace |
|---|---|
| `-std=c++17` | Compila con el estándar C++17 (el `main` usa `std::filesystem`). |
| `-O2` | Nivel de optimización estándar. Todos los tiempos reportados se miden con este nivel. |
| `-Wall -Wextra -Wpedantic` | Activan advertencias del compilador; no cambian el programa generado. El código compila sin advertencias. |
| `-Isrc` | Busca los archivos `.h` en `src/`. |

Para buscar errores de memoria se compilaron también las pruebas con sanitizers. Esos binarios son más lentos y nunca se usan para medir:

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
| `--bonus` | — | En vez de la batería, corre el bonus: Bε(126,384) con 8 valores de α (0,1 a 0,8) y 20 búsquedas; escribe `bonus.csv`. |
| `--bonus-exponente e` | 24 | Exponente de N para el bonus (15 a 24). |
| `--ayuda` | — | Muestra el uso y termina. |

Cada rango es [l, l + 604 800] (una semana), con l uniforme en [1 546 300 800, 1 754 006 400].

### Qué mide y qué escribe

Para cada N se cargan los primeros N pares en memoria antes de medir. El tiempo de construcción se mide con `std::chrono::steady_clock` alrededor del ciclo de inserciones, sin contar la lectura de `datos.bin` ni el guardado. El tiempo de búsqueda se mide solo alrededor de `buscarRango`, que lee los nodos desde el archivo.

| Archivo | Contenido |
|---|---|
| `metricas.csv` | Una fila por árbol y N: tiempo de construcción (ms), lecturas, escrituras e I/Os totales de construcción (contadores de `Disco`), nodos y bytes (nodos × 4096), tiempo promedio de búsqueda (ms), I/Os promedio de búsqueda (bloques leídos del archivo) y pares hallados en promedio. |
| `verificacion.csv` | Para cada búsqueda de cada Bε, cuántos pares halló, cuántos halló el árbol B e `iguales` = 1 si coinciden par por par. Si alguna no coincide, `./experimento` termina con error, así que una corrida que termina bien tiene 1 en todas las filas. |
| `rangos.csv` | Los rangos [l, u] usados. |
| `busqueda_b_<e>.csv` | Resultado de la primera búsqueda (tiempo Unix y temperatura) para el primer y el último N; lo usa el gráfico de temperatura. |
| `log.txt` | Salida de la corrida, con el tiempo de construcción de cada árbol. |
| `setup_tarea2.txt` | Ambiente del equipo, descrito en la tabla siguiente. |

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

## Interfaz compartida

- `Par` (`src/datos.h`): `int32_t llave` y `float valor`, 8 bytes.
- `Disco<Nodo>` (`src/disco.h`): `leer(i)`, `escribir(i,nodo)`, `agregar(nodo)`, `guardar(ruta)`, `cantidadNodos()`, `tamanoBytes()`, `lecturas`, `escrituras`. Cada acceso por bloque cuenta una operación. `guardar` no se suma a los I/Os de inserción.
- `ArchivoNodos<Nodo>` (`src/disco.h`): abre el árbol serializado y `leer(i)` busca el bloque `i` en `i*4096`, contando una lectura.
- La raíz siempre ocupa el bloque 0. Al crecer la raíz, se agregan sus dos mitades al final y se sobrescribe el bloque 0 con la nueva raíz.
- `ArbolB` (`src/arbol_b.h`) y `ArbolBe<C,L>` (`src/arbol_be.h`): `insertar(Par)`, `guardar(ruta)`, métricas de construcción y `buscarRango(archivo,l,u)`. `ArbolBe` agrega `altura()`, `estadisticas()` (vaciados, splits y lotes parciales) y `contarPares(archivo)`.

`ArbolBe<C,L>` usa `Disco<NodoBe<C,L>>` y `ArchivoNodos<NodoBe<C,L>>` sin acceso directo al vector o al archivo. `static_assert` exige nodos de 4096 bytes en ambas configuraciones (`ArbolBe025 = ArbolBe<4,506>` y `ArbolBe075 = ArbolBe<126,384>`).

## Decisiones de implementación

Las llaves repetidas pueden quedar a ambos lados de un split. Ambas búsquedas visitan los dos hijos que tocan una llave divisoria igual al borde del rango. En Bε, cada vaciado baja todos los grupos con al menos α·f pares, de derecha a izquierda, como pide 3.4.3. Si un hijo no puede recibir todo su grupo ni después de dividirse, recibe lo que cabe y el resto vuelve al buffer del padre. Un nodo interno que llega a `C` divisorias durante un vaciado se divide en ese momento. Los detalles y el resto de los casos que el enunciado no especifica están en `docs/DECISIONES_BE.md`, y deben explicarse en el informe.
