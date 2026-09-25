# Acuerdos del equipo — Tarea 1 (Prim y análisis amortizado)

Este archivo es el contrato entre las dos partes del trabajo. Si algo de aquí
cambia, se avisa al otro **antes** de hacer merge.

## División

| | Persona A — Colas | Persona B — Infraestructura y teoría |
|---|---|---|
| Código | `cola_binomial.h`, `cola_fibonacci.h`, tests de colas | `generador.h`, `prim.h`, `main.cpp`, `scripts/graficos.py`, README |
| Informe | Desarrollo (decisiones de implementación) | Demostración O(1) amortizado (3.3), construcción O(\|V\|) (3.4), complejidad de Prim (4.2, 4.3), estimación de memoria (6.2) |
| Juntos | Setup, análisis, pregunta 6.3.2, aplicación (5), introducción, conclusión | |

## Interfaz de las colas (congelada)

Ambas colas exponen **exactamente** esto (ver `src/cola_falsa.h` como ejemplo que funciona):

```cpp
static constexpr const char* nombre;      // "binomial" / "fibonacci"
static constexpr bool implementada;       // false hasta que pase los tests
int64_t ops;                              // operaciones estructurales de decreaseKey
explicit Cola(int n);                     // n = |V|; vértices 0..n-1
void insert(int v, double key);
std::pair<double,int> extractMin();       // (costo, vértice)
void decreaseKey(int v, double key);      // key <= costo actual
bool empty() const;
static size_t bytesPorNodo();             // para la estimación de memoria
~Cola();                                  // libera todos los nodos
```

- El arreglo vértice → nodo (`nodoDe`) vive **dentro** de la cola.
- Prim controla `u ∈ Q` con su propio `vector<char> enQ`; la cola no lo necesita.
- **Construcción de Q**: n llamadas a `insert` (costo[r] = 0, resto = ∞), como pide el enunciado.
- **Conteo de operaciones** (`ops`), solo dentro de `decreaseKey`:
  - Binomial: +1 por cada intercambio del `while` (sección 3.2).
  - Fibonacci: +1 por cada `cut`, incluido el primero (no solo los de la cascada).
- Lo privado (structs de nodo, helpers) lo decide A libremente.
- Decisión implementada: en la binomial se intercambian clave y vértice, y en
  el mismo paso se actualizan ambas entradas de `nodoDe`. Así se conserva la
  forma del árbol y el acceso directo a cada vértice sigue siendo correcto.
  La inserción usa acarreos entre raíces de grado igual: `n` inserciones cuestan
  `O(n)` en total. En Fibonacci, cada corte (incluido el inicial) incrementa
  `ops`; una raíz nunca queda marcada.

## Grafo

- CSR (`src/grafo.h`): `inicio` (n+1), `destino` y `peso` (2m). Cada arista se guarda dos veces.
- Vértices 0..n-1, pesos `double` uniformes en (0,1].
- `generarGrafo(v, e, semilla)`: árbol aleatorio + aristas al azar sin repetidas ni lazos.
- **Portabilidad**: solo `mt19937` convertido a mano (`src/aleatorio.h`). Prohibido usar
  `std::uniform_*_distribution` o `rand()`: dan resultados distintos en Mac/Windows/Linux.
- Semilla por corrida: `base + serie·10^6 + i·10^4 + j·100 + rep` (base = 20260928).
- Prueba de portabilidad: `./tests_bin` imprime una "huella"; debe salir igual en los 3 equipos.

## Medición

- Reloj: `std::chrono::steady_clock` siempre.
- Tiempo total = solo Prim (incluye construir Q; excluye generar el grafo y destruir Q).
- Series A y B: sin medir decreaseKey individualmente (el reloj cuesta ~20–50 ns).
- Series C y D: se mide cada decreaseKey y se acumula. Además, `prim.h` guarda el
  tiempo y las operaciones acumuladas de **cada** llamada (fuera del intervalo medido)
  y `main.cpp` escribe la curva de la repetición 0 reducida a ~4096 puntos espaciados
  según la cantidad real de llamadas (`--cada K` guarda un punto cada K llamadas).
- **`tiempo_ms` en C y D incluye el costo del reloj** (dos `now()` por decreaseKey
  más guardar la curva). No se compara con A y B; en C y D solo se usan
  `dk_tiempo_ns` y `dk_ops`. El costo del reloj se mide con `./prim --calibrar`.
- Se alterna el orden de las colas entre repeticiones.
- Experimentos finales: **solo en el servidor Ubuntu**, `-O2`, sin otras cargas.

## Salida (CSV en `resultados/`)

```
tiempos_<series>.csv       serie,i,j,v,e,rep,semilla,cola,tiempo_ms,peso_mst,dk_llamadas,dk_tiempo_ns,dk_ops
curvas_<series>.csv        serie,i,j,rep,cola,llamadas,tiempo_acum_ns,ops_acum
verificacion_<series>.csv  serie,i,j,rep,cola_ref,peso_ref,cola,peso,diferencia,ok
```

Pesos iguales si `|a − b| ≤ 1e-9 · max(1, a)`.

## Convenciones

- C++17, `g++ -std=c++17 -O2 -Wall -Wextra`, sin `bits/stdc++.h`.
- Nombres y comentarios en español. Cada struct/función con comentario de propósito,
  entradas y salidas (vale 0,2 pts).
- `long long` / `int64_t` para contadores y tiempos (en Windows `long` es de 32 bits).
- Git: una rama por persona (`colas`, `infra`); merge a `main` solo si compila y `./tests_bin` pasa.

## Fechas (entrega: lunes 28/09, 23:59)

| Día | Hito |
|---|---|
| Mié 23 | Interfaz congelada (este archivo). B: Prim + main + gráficos funcionando con `ColaFalsa`. |
| Jue 24 | Binomial integrada, tests OK. B: teoría 3.3 / 3.4 / 4.x. |
| Vie 25 | Fibonacci integrada. Primera corrida completa en el servidor. |
| Sáb 26 – Dom 27 | Experimentos completos, gráficos, redacción de resultados. |
| Lun 28 | Análisis, conclusión, revisión y entrega. |

## Preguntas abiertas para los auxiliares

- 6.2 dice que el caso más grande es v = 2^15, e = 2^20, pero la serie B llega a v = 2^22, e = 2^24.
- 6.3.2 dice "serie A / serie B" en los gráficos amortizados; se asume que son C y D.
