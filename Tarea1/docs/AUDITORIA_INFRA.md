# Auditoría de solo lectura — código "infra" de Tarea 1

> Describe el commit 97d2a7c. Los cambios aprobados se aplicaron en ac820d4 (ver ACUERDOS.md)

Fecha: 2026-09-25. Commit auditado: `97d2a7c` (rama `main`, árbol limpio).
No se modificó ningún archivo del código. El único archivo creado en el repo es este.
Todo lo compilado y ejecutado se hizo en `/tmp` de WSL (Ubuntu 24.04, g++ 13.3.0, AMD Ryzen 7 6800HS, 7.4 GiB RAM visibles en WSL).

**Fuentes y autoridad.** (1) Enunciado `Tarea1_CC4102_2026-2_ENUNCIADO.pdf`, 8 páginas; las citas se extrajeron con `pdftotext -enc UTF-8` y los símbolos matemáticos se normalizaron (p. ej. "𝑣" → "v"). (2) Apunte, solo §2.2 (p. 34), §2.4 (pp. 39-43) y §2.5 (pp. 43-45). (3) `docs/ACUERDOS.md` y `docs/PLAN.md`: acuerdos internos, **no** requisitos. (4) `Tareas_semestres_anteriores/`: no se usaron como fuente de requisitos ni se consultaron para esta auditoría.

**Estados:** CUMPLE · CUMPLE CON DESVIACIÓN LITERAL (abreviado **CDL**) · NO CUMPLE · NO ESPECIFICADO · NO VERIFICADO.

---

## 1. Resumen

| Parte | Filas | CUMPLE | CDL | NO CUMPLE | NO ESPECIFICADO | NO VERIFICADO |
|---|---:|---:|---:|---:|---:|---:|
| 1. Prim línea por línea | 14 | 7 | 7 | 0 | 0 | 0 |
| 2. Generador | 11 | 5 | 4 | 0 | 1 | 1 |
| 3. Experimentos | 22 | 14 | 2 | 3 | 2 | 1 |
| 4. Rúbrica 7.1 y sección 5 | 6 | 4 | 0 | 1 | 0 | 1 |
| 5. Verificación ejecutando | 8 | 6 | 1 | 0 | 0 | 1 |
| **Total** | **61** | **36** | **14** | **4** | **3** | **4** |

(La Parte 6 es un inventario de extras y no lleva estado).

**Los 4 NO CUMPLE:**
1. **E7:** el tiempo total no incluye las líneas 1-3 de Prim. Los arreglos `costo`, `padre` y `enQ` se crean y llenan antes de `t0` (`prim.h:55-57` frente a `prim.h:61`).
2. **E11:** en la cola de Fibonacci, `ops` cuenta **todos** los cortes, incluido el primero. El enunciado dice "cortes en cascada". `ACUERDOS.md:36` lo decide así de forma explícita, y eso se aparta de la lectura literal.
3. **E16:** no hay tabla de promedios para las series C y D (`graficos.py:178` filtra solo A y B).
4. **R5:** hay funciones sin descripción de propósito, entradas o salidas (rúbrica "Firmas", 0.2 pts).

**Los 3 NO ESPECIFICADO** son ambigüedades del propio enunciado:
- 6.3.2 dice "serie A / serie B" en los gráficos amortizados.
- 6.2 llama "caso más grande posible" a v = 2^15, e = 2^20, pero las series llegan a v = 2^22 y a e = 2^24.
- Guardar el grafo como archivo de texto es opcional ("Pueden").

**Los 4 NO VERIFICADO:**
- `graficos.py` no se pudo ejecutar porque `matplotlib` no está instalado ni en WSL ni en el Python de Windows, y no se instaló nada.
- No se puede juzgar desde el código si la distribución de pesos está documentada en el informe.
- El criterio "rigurosa y suficiente" de la rúbrica es un juicio del evaluador.
- No se generaron las 12 figuras.

---

## 2. Parte 1 — Prim línea por línea (enunciado §4.1, pp. 3-4)

### 2.1 Transcripción del pseudocódigo (p. 3, líneas 1-2; p. 4, líneas 3-14)

```
Prim(G, r)
 1 costos[r] ← 0, parent[r] ← −1
 2 for v ∈ V ∖ {r}:
 3     costos[v] ← ∞, parent[v] ← indefinido
 4 Q ← construir(costos)
 5 T ← ∅
 6 while Q ≠ ∅:
 7     (c, v) ← extractMin(Q)
 8     if v ≠ r: T ← T ∪ {(parent[v], v)}
 9     for u ∈ vecinos(v) tal que u ∈ Q:
10         if w(v, u) < costos[u]:
11             costos[u] ← w(v, u)
12             parent[u] ← v
13             decreaseKey(Q, u, w(v, u))
14 return T
```
(La indentación se infiere del pseudocódigo; el PDF extraído no la conserva).

### 2.2 Tabla línea → código

| # | Pseudocódigo (p. 3-4) | Código (`src/prim.h`) | Estado | Qué difiere exactamente |
|---|---|---|---|---|
| 1 | "costos[r] ← 0, parent[r] ← −1" | `:55 r.padre.assign(n, -1);` · `:56 std::vector<double> costo(n, INF);` · `:64 costo[raiz] = 0.0;` | CDL | `parent[r] = −1` no se asigna aparte: sale del `assign` de **todos** los vértices a −1. `costos[r] = 0` sobrescribe un ∞ previo. Las líneas 55-56 están **fuera** del intervalo medido (`t0` está en `:61`) y la 64 está dentro (ver E7). |
| 2 | "for v ∈ V ∖ {r}:" | No existe el bucle; lo reemplazan los constructores de `vector` en `:55-56`. | CDL | Se inicializa también a r y luego se sobrescribe. El resultado es el mismo. |
| 3 | "costos[v] ← ∞, parent[v] ← indefinido" | `:56 costo(n, INF)` · `:55 padre.assign(n, -1)` | CDL | **"indefinido" se representa con −1, el mismo valor que la raíz.** Mientras corre el algoritmo no se distingue "sin padre todavía" de "es la raíz". No cambia el resultado porque el grafo es conexo y todo v ≠ r recibe padre (`:79`) antes de extraerse con costo finito. `tests.cpp:184` (`aristas += (p != -1)`) depende de esta convención. |
| 4 | "Q ← construir(costos)" | `:63 Cola Q(n);` · `:65 for (int v = 0; v < n; ++v) Q.insert(v, costo[v]);  // construcción: n inserciones` | CUMPLE | No hay una función `construir`: el paso está en línea. Coincide con §3.4 (p. 3): "se realiza mediante inserciones sucesivas (secciones 2.4.3 y 2.5.1 del apunte)". Apunte p. 42 (binomial): "obtenemos tiempo O(n) si realizamos n inserciones sucesivas en una cola vacía". Apunte p. 43 (Fibonacci): "Heapify. Se realiza mediante n inserciones, en tiempo O(n)". |
| 5 | "T ← ∅" | No existe. Lo más cercano es `ResultadoPrim r;` (`:54`), con `pesoTotal = 0.0` (`:25`). | CDL | **No existe el conjunto T.** El árbol queda implícito en `r.padre`. |
| 6 | "while Q ≠ ∅:" | `:67 while (!Q.empty()) {` | CUMPLE | — |
| 7 | "(c, v) ← extractMin(Q)" | `:68 const std::pair<double, int> par = Q.extractMin();` · `:69 c = par.first` · `:70 v = par.second` | CUMPLE | Se agrega `:71 enQ[v] = 0;`, que no está en el pseudocódigo y existe para implementar la línea 9. |
| 8 | "if v ≠ r: T ← T ∪ {(parent[v], v)}" | `:72 r.pesoTotal += c;  // la raíz aporta 0; el resto, el peso de (padre[v], v)` | CDL | **No hay `if v ≠ r` ni se agrega ninguna arista.** Se suma el costo c. Para v ≠ r, c = costos[v] = w(parent[v], v), porque `:78-79` y `:82`/`:90` asignan los tres valores juntos. Para r, c = 0. La suma da el peso de T, pero **T no se construye**: la arista (parent[v], v) solo existe como `r.padre[v]`. La suma de punto flotante sigue el orden de extracción. |
| 9 | "for u ∈ vecinos(v) tal que u ∈ Q:" | `:74 for (int64_t k = g.inicio[v]; k < g.inicio[v + 1]; ++k)` · `:75 u = g.destino[k]` · `:77 if (enQ[u] && ...` · `:57 std::vector<char> enQ(n, 1);  // enQ[u] <=> u todavía está en Q` | CDL | Se recorren **todos** los vecinos y el filtro "u ∈ Q" queda dentro del `if` de la línea 10. La pertenencia se decide con **`enQ`, un arreglo que no está entre los que lista §6.2 (p. 5)**: "Los arreglos auxiliares costos, parent y el arreglo de punteros a los nodos de Q". El arreglo de punteros (`nodoDe`) existe dentro de cada cola (`cola_binomial.h:62`, `cola_fibonacci.h:90`), pero es privado. `ACUERDOS.md:32` lo decide así: "Prim controla `u ∈ Q` con su propio `vector<char> enQ`". |
| 10 | "if w(v, u) < costos[u]:" | `:76 w = g.peso[k]` · `:77 if (enQ[u] && w < costo[u])` | CUMPLE | Comparación estricta `<`, igual que el pseudocódigo. Se fusiona con la línea 9 mediante `&&` con cortocircuito, lo que es equivalente. |
| 11 | "costos[u] ← w(v, u)" | `:78 costo[u] = w;` | CUMPLE | — |
| 12 | "parent[u] ← v" | `:79 r.padre[u] = v;` | CUMPLE | — |
| 13 | "decreaseKey(Q, u, w(v, u))" | `:82 Q.decreaseKey(u, w);` (rama `medirDK`) · `:90 Q.decreaseKey(u, w);` (rama normal) | CUMPLE | Q es el objeto receptor y los argumentos son (u, w(v, u)). Coincide con §3.1 c) (p. 1): "decreaseKey(Q, v, c): acceder al par que representa al nodo v y reducir su costo a c". La llamada está duplicada en dos ramas; ver 2.3. |
| 14 | "return T" | `:100 return r;` (`ResultadoPrim`: `pesoTotal`, `padre`, `tiempoMs`, `dkLlamadas`, `dkTiempoNs`, `dkOps`, `curva`) | CDL | Se retorna un struct con el peso y el arreglo de padres (T implícito), además de las mediciones. |

### 2.3 Lo que hay en `prim.h` y no corresponde a ninguna línea del pseudocódigo

| Elemento | Código | ¿Se ejecuta en A/B? | ¿Dentro del tiempo total `tiempoMs`? |
|---|---|---|---|
| Alias de reloj | `:50 using Reloj = std::chrono::steady_clock;` | Sí | — |
| `enQ` (pertenencia a Q) | `:57`, `:71`, `:77` | Sí | Creación: **no** (`:57` antes de `t0`). Escrituras y lecturas: sí. |
| `reserve` de la curva | `:58 if (medirDK && cadaK > 0) r.curva.reserve(g.m / cadaK + 1)` | No (solo C/D) | No |
| Marcas de tiempo total | `:60-61 t0`, `:96 t1 = Reloj::now();  // se mide antes de destruir Q` | Sí | Delimitan el intervalo. Se excluye la destrucción de Q. |
| `pesoTotal` | `:72 r.pesoTotal += c;` | Sí | Sí |
| Rama `if (medirDK)` | `:80-88`: dos `Reloj::now()`, suma en `dkTiempoNs`, `++dkLlamadas`, `push_back` de `PuntoCurva` | No (solo C/D). La **condición** `if (medirDK)` sí se evalúa en A/B en cada decreaseKey. | Sí (por eso `tiempo_ms` de C/D incluye el reloj; `README.md:129-132` lo advierte) |
| Rama `else` | `:89-92 Q.decreaseKey(u, w); ++r.dkLlamadas;` | Sí | Sí (un incremento por llamada) |
| Copia de `ops` | `:97 r.dkOps = Q.ops;` | Sí | No (después de `t1`) |
| Contador `ops` dentro de las colas | `cola_binomial.h:94 ++ops;` · `cola_fibonacci.h:160 ++ops;` | Sí (se cuenta aunque A/B no use el valor) | Sí |
| Validaciones dentro de las colas | `cola_binomial.h:84-85`, `cola_fibonacci.h:72-73` (`throw` si `key > clave`) | Sí | Sí (una comparación por decreaseKey) |
| `tiempoMs` | `:99` | Sí | — |
| Structs `PuntoCurva`, `ResultadoPrim` | `:17-32` | Sí | — |
| Parámetros `raiz`, `medirDK`, `cadaK` | `:49` (`raiz = 0` por defecto; `main.cpp:66-68` siempre pasa 0) | — | — |

Sobre la raíz fija 0: §4 (p. 3) dice "parte desde un nodo raíz arbitrario r", así que elegir 0 es válido.

---

## 3. Parte 2 — Generador (enunciado §5 a) p. 4 y §6.1 p. 5)

| ID | Requisito (cita) | Código | Estado | Detalle |
|---|---|---|---|---|
| G1 | §5 a), p. 4: "El generador de grafos aleatorios conexos mediante listas de adyacencia" | `grafo.h:3 Representación del grafo (CSR: Compressed Sparse Row)` · `:19-21 inicio / destino / peso` | CDL | No son listas enlazadas ni `vector<vector<>>`: son las listas de adyacencia empaquetadas en arreglos contiguos. Los vecinos de u están en `[inicio[u], inicio[u+1])`. Se cumple §6.2, p. 5: "cada arista se almacena dos veces, una por cada sentido" (`generador.h:85-86`). |
| G2 | §5 a), p. 4: "Pueden almacenarlas como un archivo de texto." | No se guardan (`README.md:190-191`). `.gitignore` excluye `*.grafo`. | NO ESPECIFICADO | Es opcional ("Pueden"). §8 e), p. 8: "Por favor no incluyan sus datasets en sus entregas". |
| G3 | §6.1, p. 5: "Los grafos deben contener v = 2^i nodos y e = 2^j aristas" | `main.cpp:222-223 v = 1 << c.i; e = int64_t(1) << c.j;` · `generador.h:55 while (claves.size() < e)` · `:70 g.m = e;` | CUMPLE | El bucle `:55-65` nunca supera `e`, porque cada lote sortea exactamente `faltan`. Se verificó ejecutando (Parte 5, V5): exactamente 2^j aristas no dirigidas. |
| G4 | §6.1, p. 5: "para cada nodo v_i, se lo conecta a un nodo aleatorio elegido en [1..i − 1]" | `generador.h:49 for (uint32_t i = 1; i < v; ++i)` · `:50 p = rng.enRango(i);` ([0, i−1]) · `:51 claves.push_back(clave(p, i));` | CDL | Solo cambia la base de los índices. El nodo i (base 0) es v_{i+1} (base 1), y [0..i−1] en base 0 es [1..i] en base 1, que es exactamente [1..(i+1)−1]. v_1 (i = 0) no se conecta a nadie, igual que en el enunciado, donde [1..0] es vacío. |
| G5 | §6.1, p. 5: "añadir las 2^j − v + 1 aristas restantes de manera aleatoria" | `generador.h:56-62`: `a = enRango(v)`, `b = enRango(v)` | CUMPLE | Extremos uniformes en [0, v). |
| G6 | §6.1, p. 5: "descartando y volviendo a sortear aquellas que resulten […] reflexivas" | `generador.h:60 while (b == a) b = rng.enRango(v);  // sin lazos` | CDL | **No se descarta la arista completa: solo se vuelve a sortear el extremo b.** La distribución es la misma para aristas no dirigidas: P({x,y}) = 2 · (1/v) · (1/(v−1)), que es lo mismo que rechazar el par entero. La secuencia de números aleatorios consumida sí cambia. |
| G7 | §6.1, p. 5: "descartando y volviendo a sortear aquellas que resulten repetidas" | `generador.h:55-65`: se sortean `faltan` aristas, `:63 std::sort`, `:64 std::unique` + `erase`, y se repite hasta completar e | CDL | **El procedimiento es distinto** (descarte por lotes), pero **el conjunto final es idéntico** al del descarte secuencial con la misma secuencia de sorteos. Si N es el primer índice de la secuencia en que ya hay e aristas distintas, cada lote sortea exactamente lo que falta y nunca pasa de N. Además, el conjunto de valores distintos entre los primeros N sorteos es justamente "las primeras aristas distintas". Se verificó ejecutando: 100 de 100 casos idénticos (Parte 5, V6). Los duplicados de aristas del árbol también se eliminan, porque `claves` incluye el árbol. Lo único distinto es el **orden** en que se asignan los pesos (orden de clave, `:81-86`), y eso no cambia la distribución. |
| G8 | §6.1, p. 5: "pesos w(e) aleatorios en el rango (0, 1]" | `aleatorio.h:32 return (static_cast<double>(gen()) + 1.0) / 4294967296.0;` | CUMPLE | Toma valores k/2^32 con k ∈ {1, …, 2^32}. Nunca vale 0 y puede valer exactamente 1. Verificado ejecutando: 0 pesos fuera de (0, 1] (V5). |
| G9 | §6.1, p. 5: "La distribución de los pesos queda a disposición de ustedes (deben documentarlo en su informe)" | `aleatorio.h:31 double uniforme en (0, 1]` · `generador.h:11 Pesos uniformes en (0,1]` · `ACUERDOS.md:50` | NO VERIFICADO | En el código está documentada como uniforme. En realidad es **uniforme discreta sobre 2^32 valores**, así que hay pesos repetidos: se midieron 2025 aristas con peso repetido entre 4.194.304 (v = 2^18, e = 2^22, semilla 7; V6). Que esté en el informe no se puede verificar (el informe está fuera del alcance). Ver §8 c), p. 8: "el MST no es necesariamente único si hay pesos repetidos". |
| G10 | §7.1, p. 7: "Creación correcta de los nodos y aristas según lo pedido, garantizando conectividad y simplicidad." | Árbol (`:49-52`) + descarte de lazos (`:60`) y repetidas (`:63-64`) | CUMPLE | Verificado ejecutando con el verificador independiente: 1 componente, 0 lazos, 0 repetidas, CSR simétrico (V5). También lo cubre `tests.cpp:76-107`. |
| G11 | §5 a), p. 4: "para fines de reproducibilidad de sus propios experimentos" | `main.cpp:60-62 semilla(base, c, rep)` · `aleatorio.h:5-8` (mt19937 sin `uniform_*_distribution`) · `tests.cpp:108-109 "misma semilla => mismo grafo"` | CUMPLE | Las semillas son únicas por (serie, i, j, rep) mientras rep < 100. Se verificaron 40 semillas distintas en la corrida reducida (V3). **La portabilidad entre sistemas operativos (la "huella") NO se verificó**: solo se ejecutó en WSL. Huella obtenida: `destino[0..2]=1,2,4  suma pesos=1004.23427620763`. |

Validaciones extra del generador (no las pide el enunciado): `generador.h:40-42` lanza `invalid_argument` si `e < v−1` o `e > v(v−1)/2`.

---

## 4. Parte 3 — Experimentos (enunciado §6.2 y §6.3, pp. 5-7)

| ID | Requisito (cita) | Código | Estado | Detalle |
|---|---|---|---|---|
| E1 | p. 6: "Serie A (v fijo): i = 20, con j ∈ {20, 21, 22, 23, 24}." | `main.cpp:49 for (int j = 20; j <= 24; ++j) cs.push_back({'A', 20, j});` | CUMPLE | — |
| E2 | p. 6: "Serie B (e fijo): j = 24, con i ∈ {18, 19, 20, 21, 22}." | `main.cpp:50 for (int i = 18; i <= 22; ++i) cs.push_back({'B', i, 24});` | CUMPLE | — |
| E3 | p. 6: "Serie C (v fijo): i = 18, con j ∈ {18, 19, 20, 21, 22}." | `main.cpp:51` | CUMPLE | — |
| E4 | p. 6: "Serie D (e fijo): j = 22, con i ∈ {14, 15, 16, 17, 18}." | `main.cpp:52` | CUMPLE | — |
| E5 | p. 6: "Cada configuración (i, j) debe ejecutarse 10 veces, con un grafo distinto en cada repetición" | `main.cpp:152 int reps = 10` · `:235 sem = semilla(base, c0, rep)` · `:236 Grafo g = generarGrafo(v, e, sem);` | CUMPLE | Hay un grafo por repetición y ambas colas corren sobre **el mismo** grafo (`:241-243`), lo que es necesario para comparar los MST. |
| E6 | p. 6: "(la generación del grafo y la lectura de datos no deben incluirse en la medición)" | `main.cpp:236 // fuera de la medición` · `prim.h:61 t0` | CUMPLE | No hay lectura de archivos: el grafo se genera en memoria. |
| E7 | p. 6: "midiendo el tiempo total de ejecución" | `prim.h:55-57` (`padre`, `costo`, `enQ`) **antes** de `prim.h:61 const auto t0 = Reloj::now();` | NO CUMPLE | El intervalo medido **omite las líneas 1-3 de Prim**: reservar y llenar 13 B por vértice, unos 52 MiB con v = 2^22 (`--memoria`). El enunciado solo excluye la generación y la lectura. La exclusión de la destrucción de Q (`:96`) no afecta, porque liberar Q no está en el pseudocódigo. El impacto esperado es bajo (O(v) frente a O(e log v)) pero **no se midió**. |
| E8 | p. 6, 6.3.2 a): "Tiempo acumulado: sumar el tiempo de todas las llamadas" | `prim.h:81-85`: `a = now(); Q.decreaseKey(u, w); b = now(); r.dkTiempoNs += (b - a)` | CUMPLE | Cada medición incluye aproximadamente el costo de una llamada al reloj: **23,69 ns por `now()`**, medido con `--calibrar` (V8). En la corrida reducida, eso es del orden del 20-25 % de los ~100 ns por decreaseKey (p. ej. C i=14 j=18, Fibonacci: 6.623.622 ns / 60.381 llamadas ≈ 110 ns). |
| E9 | p. 6, 6.3.2 a): "graficarlo contra la cantidad de llamadas realizadas" | `graficos.py:155-164`: x = `dk_llamadas` promedio por configuración, y = `dk_tiempo` promedio | CUMPLE | **Ambigüedad:** el enunciado no dice si el eje x es "llamadas totales por configuración" (5 puntos por serie, lo que hace `graficos.py`) o "llamadas acumuladas dentro de una ejecución". Para la segunda lectura, `main.cpp:248-251` escribe `curvas_*.csv` (repetición 0, ~4096 puntos), pero **`graficos.py` no lo lee** (buscar `curvas` en `graficos.py` no da resultados). |
| E10 | p. 6, 6.3.2 b): "contar las operaciones estructurales que realiza cada llamada ([…] intercambios en la binomial)" | `cola_binomial.h:88-96`: `while (x->padre && menor(x, x->padre)) { … ++ops; x = p; }` | CUMPLE | +1 por cada intercambio de la línea 4 de decreaseKey-Binomial (p. 2). Observación, para la persona A: `menor` (`cola_binomial.h:112`) desempata por vértice. El enunciado (p. 2, línea 3) dice "x.key < y.key" (estricto), así que con claves iguales se hace y se cuenta un intercambio que el pseudocódigo no haría. Los empates existen (G9), pero el efecto en `ops` **no se midió**. |
| E11 | p. 6, 6.3.2 b): "(cortes en cascada en la cola de Fibonacci, […])" | `cola_fibonacci.h:155-161 cortar(...) { … ++ops; }`, llamado desde `:78` (**corte inicial** de decreaseKey) y desde `:168` (corte dentro de `corteCascada`) | NO CUMPLE (lectura literal) | `ops` = cortes iniciales + cortes en cascada. `ACUERDOS.md:36` lo decide: "Fibonacci: +1 por cada `cut`, incluido el primero (no solo los de la cascada)". **Contradice la lectura literal.** El enunciado usa "cortes en cascada" para los cortes que se propagan hacia arriba (p. 2: "un nodo que pierde su segundo hijo también es cortado […] y esto se propaga hacia arriba"; p. 3, Hint: "analicen una llamada con c cortes en cascada"). La diferencia es exactamente la cantidad de decreaseKey que hacen el corte inicial. Las marcas (`flag ← True`, `:166`) no se cuentan; el enunciado no las menciona. |
| E12 | p. 7: "Guarden las mediciones en alguna estructura en memoria y procésenlas después de terminar el algoritmo" | `prim.h:31 std::vector<PuntoCurva> curva` · `:58 reserve` · `:88 push_back` · el CSV se escribe en `main.cpp:245-251`, después de que `prim` retorna | CUMPLE | Nota: en modo automático (`main.cpp:232 k = cadaK > 0 ? cadaK : 1`) **todas** las repeticiones de C y D guardan un punto por llamada, aunque solo se escribe la repetición 0 (`:248`). Eso reserva hasta (2^22 + 1) × 24 B ≈ 96 MiB en C con j = 22. El `push_back` queda entre dos decreaseKey medidos y podría ensuciar la caché; ese efecto **no se midió**. |
| E13 | p. 8, 8 d): "reloj monótono de alta resolución (clock_gettime, std::chrono::steady_clock, …)" | `prim.h:50` · `main.cpp:134, 217, 271` | CUMPLE | Resolución nominal: 1,00 ns (V8). |
| E14 | p. 6: "verifiquen que ambas implementaciones produzcan un MST del mismo peso total" | `main.cpp:253-262`: `dif <= 1e-9 * std::max(1.0, pesoRef)` → `verificacion_*.csv` · `:273-274` código de salida 2 si hay diferencias | CUMPLE | 40 de 40 filas con `ok=1` en la corrida reducida (V3). Reportarlo en el informe es tarea aparte. |
| E15 | p. 6: "Reporten los tiempos en tiempos en una tabla, incluyendo el promedio de cada configuración" | `graficos.py:176-193 tabla()`: `tabla_tiempos.csv/.tex` con promedio ± desviación estándar | CDL | La tabla trae solo el promedio y la desviación estándar. Los tiempos de cada repetición están en `tiempos_*.csv`, pero no en la tabla. La frase "Reporten los tiempos […] incluyendo el promedio" se puede leer como "los tiempos y además el promedio". |
| E16 | p. 6 (§6.3, aplica a todas las series): "reportando el promedio de las mediciones en una tabla" | `graficos.py:178 filas = sorted(… if k[0] in ("A", "B"))` | NO CUMPLE | **No hay tabla de promedios para C y D** (`dk_llamadas`, `dk_tiempo`, `dk_ops`). `agrupar()` (`:63-83`) ya calcula esos promedios, pero no se escriben en ninguna tabla. |
| E17 | p. 6: cuatro gráficos (6.3.1) + "ocho gráficos de línea" (6.3.2); p. 8: "los doce gráficos solicitados con sus ejes correctamente etiquetados" | `graficos.py:126-145` → `total_{cola}_serie{A,B}.png` (4) · `:148-173` → `dk_{tiempo,ops}_{cola}_serie{C,D}.png` (8) · `:93-94` etiquetas de ejes | NO VERIFICADO | Por lectura estática, el código genera 12 archivos con los ejes etiquetados. **No se ejecutó** porque `matplotlib` no está instalado (`ModuleNotFoundError: No module named 'matplotlib'` en WSL y en Windows) y no se instaló nada. |
| E18 | p. 6: "la curva del algoritmo correspondiente junto a su cota teórica, multiplicada por alguna constante a determinar" | `graficos.py:37-44` (cotas) · `:86-89 ajustar()` (mínimos cuadrados) · `:113-114` curva de la cota | CUMPLE (lectura estática) | Cotas usadas. Total: binomial `e·log v` (§4.2, p. 4: "O(e log v)"); Fibonacci `e + v·log v` (§4.3: "O(e + v log v)"). decreaseKey: binomial `k·log v` (§3.2, p. 2: "O(log n) en el peor caso"); Fibonacci `k` (§3.3: "O(1) amortizado"). |
| E19 | p. 6-7: "usar la misma escala en ambos gráficos de una misma serie [y forma de medición]" | `graficos.py:142` y `:167`: `ylim` común a las dos colas por serie y medida · `:117 ax.set_ylim(0, ylim)` | CUMPLE (lectura estática) | El eje y es común. El eje x no se fija explícitamente: queda igual porque los datos x coinciden. En A/B, x = e o x = v. En C/D, x = `dk_llamadas`, que resultó idéntico para las dos colas en los 40 pares (V3). Si alguna vez difiriera, las escalas x podrían no coincidir. |
| E20 | p. 5, §6.2: "La lista de adyacencia […]. Los nodos de la cola de prioridad […]. Los arreglos auxiliares costos, parent y el arreglo de punteros a los nodos de Q." | `main.cpp:103-126 imprimirMemoria` · `:104` lista (CSR) · `:105` `costo/padre/enQ` · `:106` punteros (`nodoDe`) · `:112-113, 118-121` nodos de cada cola (`sizeof`) | CDL | Están los tres componentes y ambas implementaciones. Diferencias: `enQ` (extra) va **sumado** en la misma línea que `costos` y `parent`; los nodos usan `sizeof` (40 B / 56 B) sin la sobrecarga del asignador de memoria (no investigado: NO VERIFICADO); la curva de C/D (E12) no se cuenta. Agrega picos, el temporal del generador y el caso 2^22/2^24. |
| E21 | p. 6: "Tiempo acumulado de cola binomial en serie A, […] serie B, […]" (en §6.3.2) | `graficos.py:151 for serie in ("C", "D")` | NO ESPECIFICADO | **Ambigüedad del enunciado:** §6.3.2 define C y D, pero la lista de gráficos dice A y B. El código asume C y D, y `ACUERDOS.md:101` lo deja como pregunta abierta. No se resuelve aquí. |
| E22 | p. 5: "el caso más grande posible (v = 2^15, e = 2^20)" | `main.cpp:182` (2^15, 2^20) · `:183` (2^22, 2^24, "caso más grande de la serie B") | NO ESPECIFICADO | **Ambigüedad del enunciado:** la serie A llega a v = 2^20, e = 2^24 y la serie B a v = 2^22, e = 2^24, ambos mayores que 2^15/2^20. El código imprime los dos casos. `ACUERDOS.md:100` lo deja como pregunta abierta. |

---

## 5. Parte 4 — Rúbrica §7.1 (p. 7) y restricción de la sección 5 (p. 4)

| ID | Rúbrica (cita, p. 7) | Evidencia | Estado | Detalle |
|---|---|---|---|---|
| R1 | "README: Archivo con instrucciones claras para compilar y ejecutar el código. Debe ser lo suficientemente explicativo para que cualquier persona pueda ejecutar la totalidad de los experimentos." | `README.md:63-67` requisitos · `:69-79` compilar · `:81-112` ejecutar y opciones · `:121-136` salidas · `:86 python3 scripts/graficos.py` | CUMPLE | Observaciones (no son incumplimientos): trae secciones internas del equipo ("Estado del esqueleto", "Responsable", `:12-31`) y enlaces a `docs/` (`:7-8`), que quedarían rotos si `docs/` no se entrega. No indica cuánto dura la batería completa ni cuánta RAM necesita (NO ESPECIFICADO en la rúbrica). |
| R2 | "Main: Un archivo principal que permita ejecutar toda la batería de experimentos sin modificar el código." | `main.cpp:149-152` valores por defecto `series = "ABCD"`, colas binomial y Fibonacci, `reps = 10` | CUMPLE | `./prim` sin argumentos corre todo. Los gráficos se hacen con otro script, pero no son "experimentos". |
| R3 | "Generación de grafos: Creación correcta de los nodos y aristas según lo pedido, garantizando conectividad y simplicidad." | Parte 2 · V5 · V6 | CUMPLE | — |
| R4 | "Obtención de resultados: La medición de tiempos es rigurosa y suficiente para emitir conclusiones." | `steady_clock`, generación excluida, orden de colas alternado (`main.cpp:239-242`), `--calibrar` | NO VERIFICADO | Es un juicio del evaluador. Hechos que pesan en contra: E7, E8 (el reloj es ~20-25 % de cada medición de decreaseKey en la corrida reducida), E11, E12. |
| R5 | "Firmas: Cada estructura de datos y función relevante debe tener una descripción de su propósito y de sus parámetros de entrada y salida." | Ver lista abajo | NO CUMPLE | Hay funciones sin comentario o sin entradas/salidas. |
| R6 | §5, p. 4: "No se permite el uso de librerías que resuelvan directamente el problema (por ejemplo, std::priority_queue en C++, PriorityQueue en Java o cualquier grafo prefabricado)." | Ver tabla abajo. Búsqueda de `priority_queue\|make_heap\|push_heap\|boost` en `src/` y `tests/`: sin resultados. | CUMPLE | — |

### 5.1 Firmas sin descripción completa (infra)

Se revisaron todas las funciones y estructuras de infra. En esta lista, "falta" significa que ese elemento de la descripción no aparece:

| Archivo:línea | Elemento | Qué falta |
|---|---|---|
| `main.cpp:45` | `configuraciones(series)` | Entrada (`series`) y salida (`vector<Config>`) |
| `main.cpp:60` | `semilla(base, c, rep)` | Entradas y salida explícitas |
| `main.cpp:65` | `ejecutar(cola, g, medirDK, cadaK)` | `medirDK`, `cadaK` y el retorno |
| `main.cpp:89` | `implementada(cola)` | **Sin comentario** |
| `main.cpp:96` | `mb(bytes)` | **Sin comentario** |
| `main.cpp:103` | `imprimirMemoria(v, e)` | Salida (imprime en stdout) |
| `main.cpp:148` | `main(argc, argv)` | Sin comentario propio. El encabezado del archivo (`:1-16`) describe el uso, pero no dice qué significa el código de salida (0/1/2, `:274`). |
| `main.cpp:39` | `struct Config` | El campo `serie` no se describe |
| `grafo.h:16` | `struct Grafo` | Sin comentario propio; el encabezado `:2-11` sí lo describe (aceptable, se reporta) |
| `aleatorio.h:13` | `class Aleatorio` | Sin comentario propio; el encabezado `:2-9` sí lo describe (aceptable, se reporta) |
| `generador.h:27` | `detalle::clave(a, b)` | Entrada y salida solo implícitas |
| `cola_falsa.h:18-19` | `nombre`, `implementada` | Sin comentario |
| `tests.cpp:31` | macro `REVISAR` | Sin comentario |
| `tests.cpp:56` | `kruskal(g)` | Salida (peso, `double`) solo implícita |
| `tests.cpp:73` | `cerca(a, b)` | **Sin comentario** |
| `tests.cpp:196` | `main()` | Sin comentario |
| `graficos.py:92` | `estilo(ax, xlabel, ylabel, titulo)` | **Sin docstring** |
| `graficos.py:109` | `graficar(...)` (13 parámetros) | **Sin docstring** |
| `graficos.py:126` | `graficos_total(agr, colas, salida)` | **Sin docstring** |
| `graficos.py:148` | `graficos_amortizado(agr, colas, salida)` | **Sin docstring** |
| `graficos.py:196` | `main()` | **Sin docstring** |
| `graficos.py:47, 63, 102, 176` | `leer`, `agrupar`, `serie_datos`, `tabla` | Tienen docstring, pero no describen los parámetros de entrada |

Sí tienen descripción completa: `Grafo::bytes`, `Grafo::bytesEstimados`, los métodos de `Aleatorio`, `generarGrafo`, `PuntoCurva`, `ResultadoPrim`, `prim`, los métodos de `ColaFalsa`, `reducirCurva`, `calibrarReloj`, `desdeAristas`, las funciones `probar*` y `ajustar`.

### 5.2 Uso de la biblioteca estándar en infra (sección 5)

| Uso | Dónde | ¿"Resuelve directamente el problema"? |
|---|---|---|
| `std::vector` | Todos los archivos | No: es un contenedor. El grafo CSR se construye a mano (`generador.h:67-87`). |
| `std::mt19937` | `aleatorio.h:35` | No: es un generador de números aleatorios. Las conversiones a rango y a peso son propias. |
| `std::sort`, `std::unique` | `generador.h:63-64` | No: se usan para eliminar aristas repetidas en el generador; no son cola de prioridad ni grafo. |
| `std::swap`, `std::pair`, `std::numeric_limits`, `std::to_string`, `std::invalid_argument`, `std::logic_error` | `generador.h`, `prim.h`, `cola_falsa.h` | No |
| `std::chrono::steady_clock` | `prim.h`, `main.cpp` | No: sirve para medir. |
| E/S y utilidades (`iostream`, `fstream`, `sstream`, `iomanip`, `filesystem`, `stoi/stoll/stoul`, `getline`, `max`, `fabs`, `exit`) | `main.cpp` | No |
| `std::queue` (FIFO), `std::set` | `tests.cpp:98` (BFS de conectividad), `:86` | No: están en los tests, y `std::queue` no es `priority_queue`. |
| `std::sort`, `std::iota`, `std::accumulate`, `std::tuple` | `tests.cpp:57-71` (Kruskal de referencia), `:111` | Se reporta por transparencia: `kruskal` **sí calcula un MST**, pero solo como oráculo de los tests. No reemplaza a Prim ni a las colas entregadas. |
| `matplotlib`, `csv`, `statistics`, `glob` | `graficos.py` | No: es posprocesamiento. |

---

## 6. Parte 5 — Verificación ejecutando (WSL)

Todo se corrió con un script en una sola sesión de WSL, compilando y escribiendo en `/tmp`. `git status --short` al terminar: sin cambios.

| ID | Qué | Resultado | Estado |
|---|---|---|---|
| V1 | Compilar `prim` y `tests_bin` con `-std=c++17 -O2 -Wall -Wextra` | Ambos compilan (`exit=0`). **1 warning** en `main.cpp:139` (ver salida abajo). | CUMPLE (el enunciado no pide cero warnings; `PLAN.md:145` sí lo pide, como acuerdo interno) |
| V2 | `./tests_bin` | `267/267 pruebas OK` | CUMPLE |
| V3 | `./prim --reducir 4 --reps 2 --salida /tmp/audit` | `verificacion`: 40 filas, 40 con `ok=1`. `tiempos`: 80 filas, 80 con `dk_llamadas <= e`, 0 violaciones. 20 configuraciones × 4 filas. 40 semillas distintas. `dk_llamadas` idéntico entre colas en los 40 pares. | CUMPLE |
| V4 | Curvas | 4096 puntos por (serie, i, j, rep 0, cola); la última abscisa es el total real de llamadas. | CUMPLE |
| V5 | Verificador independiente del generador, 4 configuraciones reducidas | Las 4 dan OK: conexo, sin lazos, sin repetidas, CSR simétrico, exactamente 2^j aristas, pesos en (0, 1]. | CUMPLE |
| V6 | Descarte por lotes frente a descarte secuencial (misma secuencia de sorteos) | 100 de 100 casos con el mismo conjunto de aristas. | CUMPLE |
| V7 | `./prim --memoria` comparado con §6.2 | Están los tres componentes; `enQ` va sumado a `costo/padre` (ver E20). | CDL |
| V8 | `./prim --calibrar` | 23,69 ns por `now()`; resolución nominal 1,00 ns. | CUMPLE (informativo) |
| — | `python3 scripts/graficos.py` | No se ejecutó: falta `matplotlib`. | NO VERIFICADO |

### 6.1 Comandos y salidas

```
$ g++ --version
g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
$ g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/audit_build/prim src/main.cpp
src/main.cpp: In function ‘void calibrarReloj(int64_t)’:
src/main.cpp:139:29: warning: variable ‘sumidero’ set but not used [-Wunused-but-set-variable]
  139 |     static volatile int64_t sumidero;
      |                             ^~~~~~~~
exit=0
$ g++ -std=c++17 -O2 -Wall -Wextra -Isrc -o /tmp/audit_build/tests_bin tests/tests.cpp
exit=0

$ ./tests_bin
Generador
  huella (semilla 42): destino[0..2]=1,2,4  suma pesos=1004.23427620763
Cola binomial
Cola fibonacci
Prim con cola falsa
Prim con cola binomial
Prim con cola fibonacci

267/267 pruebas OK
exit=0

$ ./prim --reducir 4 --reps 2 --salida /tmp/audit
exit=0      (stderr: 81 líneas, sin AVISO ni ATENCION)
Listo en 10.437482 s. Resultados en /tmp/audit/ (*_ABCD_red4.csv)

# verificacion_ABCD_red4.csv, columna ok
filas=40 ok=1:40 ok!=1:0
# tiempos_ABCD_red4.csv, dk_llamadas (col 11) <= e (col 5)
filas=80 dk<=e:80 violaciones:0
# dk_llamadas binomial vs fibonacci sobre el mismo grafo
pares con dk distinto: 0
# semillas
filas=80 semillas_distintas=40

# C/D, repetición 0 (dk_ns incluye el costo del reloj)
C 14 14 binomial dk=16384 dk_ns=2066209 dk_ops=80468
C 14 14 fibonacci dk=16384 dk_ns=1878037 dk_ops=22071
C 14 18 binomial dk=60381 dk_ns=6405439 dk_ops=95320
C 14 18 fibonacci dk=60381 dk_ns=6623622 dk_ops=45054
D 10 18 binomial dk=6542 dk_ns=264411 dk_ops=5824
D 14 18 fibonacci dk=60509 dk_ns=6257893 dk_ops=45557
(… 20 filas en total)

# verificador independiente del generador (/tmp/audit_verif/verif.cpp)
v=2^12 e=2^16 semilla=111 | tamanos_ok=1 aristas_no_dirigidas=65536 (esperado 65536) componentes=1 lazos=0 repetidas=0 asimetricas=0 pesos_fuera_(0,1]=0 min=4.605390131e-07 max=0.9999987038 => OK
v=2^10 e=2^18 semilla=222 | tamanos_ok=1 aristas_no_dirigidas=262144 (esperado 262144) componentes=1 lazos=0 repetidas=0 asimetricas=0 pesos_fuera_(0,1]=0 min=1.120148227e-06 max=0.9999948922 => OK
v=2^16 e=2^16 semilla=333 | tamanos_ok=1 aristas_no_dirigidas=65536 (esperado 65536) componentes=1 lazos=0 repetidas=0 asimetricas=0 pesos_fuera_(0,1]=0 min=1.564039849e-05 max=0.9999993017 => OK
v=2^14 e=2^20 semilla=444 | tamanos_ok=1 aristas_no_dirigidas=1048576 (esperado 1048576) componentes=1 lazos=0 repetidas=0 asimetricas=0 pesos_fuera_(0,1]=0 min=1.361360773e-06 max=0.999999135 => OK
exit=0

# lotes vs secuencial (/tmp/audit_verif/secuencial.cpp)
casos=100 conjuntos_de_aristas_distintos=0
v=2^18 e=2^22 semilla=7: aristas con peso igual al de otra arista = 2025 de 4194304
exit=0

$ ./prim --memoria
Estimacion de memoria (seccion 6.2)
v = 32768, e = 1048576
  Lista de adyacencia (CSR, 2e entradas): 24.3 MiB
  Arreglos costo/padre/enQ:               0.4 MiB
  Arreglo de punteros a nodos de Q:       0.2 MiB
  Nodos cola binomial  (40 B/nodo):  1.2 MiB
  Nodos cola Fibonacci (56 B/nodo):  1.8 MiB
  Temporal del generador (se libera):     8.0 MiB
  Pico generacion (CSR + claves + cursor): 32.5 MiB
  Pico Prim binomial  (CSR + cola + aux):  26.2 MiB
  Pico Prim Fibonacci (CSR + cola + aux):  26.7 MiB
v = 4194304, e = 16777216
  Lista de adyacencia (CSR, 2e entradas): 416.0 MiB
  Arreglos costo/padre/enQ:               52.0 MiB
  Arreglo de punteros a nodos de Q:       32.0 MiB
  Nodos cola binomial  (40 B/nodo):  160.0 MiB
  Nodos cola Fibonacci (56 B/nodo):  224.0 MiB
  Temporal del generador (se libera):     128.0 MiB
  Pico generacion (CSR + claves + cursor): 576.0 MiB
  Pico Prim binomial  (CSR + cola + aux):  660.0 MiB
  Pico Prim Fibonacci (CSR + cola + aux):  724.0 MiB

$ ./prim --calibrar
Calibracion de steady_clock::now() (1000000 llamadas)
  Tiempo total:       23.686 ms
  Costo por llamada:  23.69 ns
  Resolucion nominal: 1.00 ns
```

### 6.2 `--memoria` frente a §6.2

| Componente pedido (p. 5) | Línea de `--memoria` | Comentario |
|---|---|---|
| "La lista de adyacencia. […] cada arista se almacena dos veces" | "Lista de adyacencia (CSR, 2e entradas)" = (v+1)·8 + 2e·(4+8) B (`grafo.h:36-39`) | Correcto para CSR. |
| "Los nodos de la cola de prioridad […] Investiguen cuanto espacio ocupa cada uno" | "Nodos cola binomial (40 B/nodo)", "Nodos cola Fibonacci (56 B/nodo)" = `sizeof` | No incluye la sobrecarga por cada `new` del asignador (NO VERIFICADO). |
| "Los arreglos auxiliares costos, parent y el arreglo de punteros a los nodos de Q" | "Arreglos costo/padre/enQ" (8+4+1 B por vértice) + "Arreglo de punteros" (8 B por vértice) | `enQ` es extra y va sumado; no se informa por separado. |
| (no pedido) | Temporal del generador, picos, caso 2^22/2^24 | Extras. |

---

## 7. Parte 6 — Extras que el enunciado no pide

| Extra | Código | ¿Puede afectar las mediciones? |
|---|---|---|
| Opciones `--series`, `--reps`, `--colas`, `--salida`, `--semilla`, `--ayuda`/`-h` | `main.cpp:157-177` | No con los valores por defecto. `--reps` < 10 viola E5 si se usa. |
| `--reducir k` (resta k a i y j) | `main.cpp:167, 220-221` | Solo si se usa. Por defecto es 0. |
| `--cada K` (muestreo de la curva) | `main.cpp:168, 232` | Sí, un poco: cambia cuántos `push_back` ocurren dentro de `tiempo_ms` de C/D. No afecta `dk_tiempo_ns`. |
| `--memoria`, `--calibrar` | `main.cpp:170-171, 180-189` | No: no corren experimentos. |
| **Orden alternado de las colas** entre repeticiones | `main.cpp:239-242 colas[(q + rep) % colas.size()]` | **Sí, a propósito**: busca repartir efectos de caché y frecuencia de CPU. |
| Validación de configuraciones imposibles (`e > v(v−1)/2`) | `main.cpp:225-229`, `generador.h:40-42` | No con los parámetros reales (todas las configuraciones son posibles). |
| Validaciones y `throw` en `insert`/`decreaseKey` de las colas (persona A) | `cola_binomial.h:84-85`, `cola_fibonacci.h:72-73` | Sí, un poco: una comparación por llamada dentro del tiempo total y del tiempo de decreaseKey. |
| **Desempate por vértice** en `menor` (persona A) | `cola_binomial.h:112`, `cola_fibonacci.h:95-97` | Sí: puede agregar intercambios y cortes cuando hay claves iguales (E10). Además hace que las dos colas extraigan en el mismo orden (`dk_llamadas` idéntico, V3). |
| `enQ` en Prim | `prim.h:57, 71, 77` | Sí, un poco: un arreglo más en caché. |
| Contadores `dkLlamadas` y `ops` activos en A/B | `prim.h:91`, colas | Sí, un poco (un incremento por operación). |
| Curva completa en todas las repeticiones de C/D + `reducirCurva` + `curvas_*.csv` (no usado por `graficos.py`) | `prim.h:58, 87-88`, `main.cpp:73-87, 248-251` | Sí: memoria (hasta ~96 MiB) y posible efecto en la caché entre mediciones (E12). |
| `tiempo_ms` en C/D incluye el reloj | `prim.h:80-88`, `README.md:129-132` | No afecta A/B. En C/D ese `tiempo_ms` no debe usarse (el README lo advierte). |
| Exclusión de la destrucción de Q | `prim.h:96` | Sí: se excluye a propósito (liberar memoria no está en el pseudocódigo). |
| Escritura del CSV y progreso por stderr entre ejecuciones; `flush` por repetición | `main.cpp:245-268` | No: ocurre fuera de `prim`. |
| Semillas deterministas y `Aleatorio` portable | `main.cpp:60-62`, `aleatorio.h` | No |
| `Grafo::bytes()` | `grafo.h:27-30` | No. **No se usa en ningún lugar** (código muerto). |
| `ColaFalsa` | `cola_falsa.h` | No por defecto (`--colas falsa` es opcional). |
| Kruskal, huella y pruebas de colas | `tests.cpp` | No |
| Barras de error (desviación estándar), eje x log2, `.tex`/`.csv` de la tabla, `--reducidos` | `graficos.py:111-117, 176-193, 200` | No miden; solo cambian la presentación. |
| `graficos.py` lee **todos** los `tiempos_*.csv` | `graficos.py:50` | Sí, si conviven `tiempos_ABCD.csv` y `tiempos_AB.csv`: las filas se duplican en los promedios. |
| Objetivos `rapido` y `memoria` del Makefile | `Makefile:16-22` | No |

---

## 8. Cambios propuestos (no aplicados)

Clases: **(A)** obligatorio por el enunciado · **(B)** para que el código sea literal al pseudocódigo (lo decide el equipo) · **(C)** no requiere cambio, solo documentarlo en el informe.

### Clase A

**A1. Contar los "cortes en cascada" en Fibonacci según el enunciado**
- Cita: p. 6, 6.3.2 b): "contar las operaciones estructurales que realiza cada llamada (cortes en cascada en la cola de Fibonacci, intercambios en la binomial)".
- Cambio: agregar un segundo contador en `ColaFibonacci`, incrementado solo en el corte de `corteCascada` (`cola_fibonacci.h:168`), además del actual. Llevarlo a `ResultadoPrim` (`prim.h`), al CSV (`main.cpp:210-247`) y a `graficos.py`.
- Para decidir: si el equipo concluye que "cortes en cascada" se refiere al mecanismo completo, esto pasa a clase C y basta con justificarlo. Guardar los dos contadores sirve para ambas lecturas.
- Riesgo: cambia la interfaz "congelada" de `ACUERDOS.md` y el código de la persona A. Obliga a volver a correr C y D y a actualizar `ACUERDOS.md:36`. Agrega un incremento por corte (despreciable).

**A2. Tabla de promedios para las series C y D**
- Cita: p. 6, §6.3: "Cada configuración (i, j) debe ejecutarse 10 veces […] reportando el promedio de las mediciones en una tabla".
- Cambio: en `graficos.py:176-193`, generar también una tabla para C y D con `dk_llamadas`, `dk_tiempo` y `dk_ops` (promedio ± desviación estándar). Los datos ya están en `agrupar()`.
- Riesgo: bajo. Solo toca el posprocesamiento; no hay que volver a correr nada.

**A3. Incluir las líneas 1-3 de Prim en el tiempo total**
- Cita: p. 6: "midiendo el tiempo total de ejecución (la generación del grafo y la lectura de datos no deben incluirse en la medición)".
- Cambio: mover `const auto t0 = Reloj::now();` (`prim.h:61`) antes de `prim.h:55`, dejando `reserve` de la curva (`:58`) fuera o dentro según se decida. Otra opción: crear `costo`, `padre` y `enQ` después de `t0`.
- Riesgo: bajo en código. Cambia todos los tiempos de A y B, así que hay que volver a correr si ya se corrieron. Es incompatible con resultados de una versión anterior.

**A4. Tabla de tiempos de A y B con cada repetición**
- Cita: p. 6: "Reporten los tiempos en tiempos en una tabla, incluyendo el promedio de cada configuración".
- Cambio: en `graficos.py:tabla()`, agregar las 10 repeticiones (o un anexo) además del promedio ± desviación estándar.
- Para decidir: la frase es ambigua. Si el equipo la lee como "solo el promedio", pasa a clase C.
- Riesgo: bajo. La tabla es más ancha; podría ir al anexo.

**A5. Completar las firmas**
- Cita: p. 7: "Cada estructura de datos y función relevante debe tener una descripción de su propósito y de sus parámetros de entrada y salida".
- Cambio: comentarios o docstrings con propósito, entradas y salidas para todo lo listado en §5.1.
- Riesgo: nulo. Solo comentarios; no cambia el comportamiento.

### Clase B

**B1. T explícito (líneas 5, 8 y 14)**
- Cita: p. 4: "5 T ← ∅", "8 if v ≠ r: T ← T ∪ {(parent[v], v)}", "14 return T".
- Cambio: `std::vector<std::pair<int,int>> T; T.reserve(n-1);`, y en `prim.h:72` hacer `if (v != raiz) T.push_back({r.padre[v], v});`. Agregar `T` a `ResultadoPrim` y calcular el peso desde T.
- Riesgo: v·8 B más de memoria (32 MiB con v = 2^22) y un `push_back` por extracción dentro del tiempo medido. Cambia los tiempos. Hay que ajustar los tests (`tests.cpp:184`).

**B2. "parent[v] ← indefinido" distinto de −1, con bucle explícito sobre V ∖ {r}**
- Cita: p. 3-4: "1 costos[r] ← 0, parent[r] ← −1", "2 for v ∈ V ∖ {r}:", "3 costos[v] ← ∞, parent[v] ← indefinido".
- Cambio: usar una constante `INDEFINIDO` (p. ej. −2) y escribir el bucle de las líneas 2-3 tal cual.
- Riesgo: `tests.cpp:184` cuenta `p != -1` y habría que cambiarlo. El impacto en el tiempo es nulo si se hace junto con A3.

**B3. Pertenencia "u ∈ Q" sin `enQ`**
- Cita: p. 4: "9 for u ∈ vecinos(v) tal que u ∈ Q"; p. 5: "Los arreglos auxiliares costos, parent y el arreglo de punteros a los nodos de Q".
- Cambio: agregar a la interfaz de las colas un método `bool contiene(int v) const { return nodoDe[v] != nullptr; }` y usarlo en `prim.h:77` en lugar de `enQ`.
- Riesgo: cambia la interfaz congelada. Hay que verificar que `extractMin` deje `nodoDe[v] = nullptr` en las dos colas (no se auditó: es código de la persona A). Puede cambiar los tiempos por el acceso indirecto.
- Alternativa C: mantener `enQ` y documentarlo como arreglo auxiliar extra en §6.2.

**B4. `construir(costos)` como función**
- Cita: p. 4: "4 Q ← construir(costos)".
- Cambio: una función o bloque con nombre `construir` que haga las n inserciones.
- Riesgo: nulo; es solo estético.

**B5. Generador literal (descarte del par completo, secuencial)**
- Cita: p. 5: "descartando y volviendo a sortear aquellas que resulten repetidas o reflexivas".
- Cambio: descartar el par (a, b) completo si a = b, y detectar repetidas una a una con un conjunto hash.
- Riesgo: cambian todos los grafos para las mismas semillas (cambia la huella) y el conjunto hash usa más memoria. **No se recomienda**: V6 muestra que el procedimiento actual da el mismo conjunto de aristas que el secuencial, y G6 que la distribución es la misma. Basta con documentarlo (C2).

**B6. Comparación estricta "x.key < y.key" en las colas (persona A)**
- Cita: p. 2: "3 while y ≠ null and x.key < y.key" y "3 if y ≠ null and x.key < y.key".
- Cambio: comparar solo la clave en `decreaseKey`, sin desempate por vértice.
- Riesgo: fuera del alcance de infra. Puede cambiar el orden de extracción con empates y, con eso, `dk_llamadas` entre colas y la igualdad del eje x (E19). Lo decide la persona A.

### Clase C (documentar en el informe)

- **C1.** CSR como implementación de "listas de adyacencia" (G1), con cada arista almacenada dos veces.
- **C2.** El generador descarta por lotes y vuelve a sortear solo el extremo b en los lazos. Incluir el argumento de equivalencia (G6, G7) y la verificación V6.
- **C3.** Distribución de pesos: uniforme discreta {k/2^32 : k = 1..2^32} ⊂ (0, 1], con pesos repetidos (2025 de 4.194.304 medidos). Relacionarlo con §8 c). Cita: p. 5: "deben documentarlo en su informe".
- **C4.** Costo del reloj en decreaseKey: 23,69 ns por `now()` en esta máquina. Hay que volver a medirlo en el servidor con `--calibrar`, reportarlo y, si se quiere, restar `dk_llamadas × costo`.
- **C5.** `enQ` como arreglo auxiliar extra, sumado en la línea "costo/padre/enQ" de §6.2. Mencionar que `sizeof` no incluye la sobrecarga del asignador y que la curva de C/D ocupa memoria adicional (E12, E20).
- **C6.** Ambigüedades del enunciado, sin resolver aquí: "serie A / serie B" en §6.3.2 (E21); el caso más grande 2^15/2^20 frente a las series (E22); qué significa "cantidad de llamadas" en el eje x (E9).
- **C7.** El tiempo total excluye la destrucción de Q y, mientras no se aplique A3, también las líneas 1-3.
- **C8.** Justificar la decisión de §3.2 (intercambiar el contenido y actualizar `nodoDe`). Cita: p. 2: "Ambas alternativas son válidas, pero deben justificar cuál eligieron en su informe". Lo implementa la persona A.

### Observaciones fuera de las clases A/B/C (acuerdos internos, no enunciado)

- Warning `-Wunused-but-set-variable` en `main.cpp:139`. `PLAN.md:145` pide "Compilación con `-O2` y sin warnings".
- `Grafo::bytes()` (`grafo.h:27-30`) no se usa.
- `graficos.py` no lee `verificacion_*.csv` ni `curvas_*.csv`.
- En C/D, en modo automático, cada repetición guarda la curva completa aunque solo se escribe la repetición 0 (E12). Se podría reservar solo en la repetición 0 para ahorrar memoria y bajar el posible efecto en la caché.
- Antes de entregar, instalar `matplotlib` y ejecutar `graficos.py` para cerrar E17.

---

## Anexo — Fuentes de los verificadores (se ejecutaron en `/tmp/audit_verif/`, fuera del repo)

`verif.cpp` (verificador independiente del generador: tamaños, conexidad con union-find, lazos, repetidas con hash, simetría del CSR, pesos en (0, 1]):

```cpp
#include "generador.h"
#include <cstdio>
#include <unordered_map>
#include <vector>
#include <cstdint>
static int find(std::vector<int>& p, int x) { while (p[x] != x) { p[x] = p[p[x]]; x = p[x]; } return x; }
int main() {
    struct C { int i, j; uint32_t s; } cs[] = {{12, 16, 111u}, {10, 18, 222u}, {16, 16, 333u}, {14, 20, 444u}};
    int fallas = 0;
    for (auto c : cs) {
        int v = 1 << c.i; int64_t e = int64_t(1) << c.j;
        Grafo g = generarGrafo(v, e, c.s);
        bool okTam = g.n == v && g.m == e && (int64_t)g.inicio.size() == v + 1 && g.inicio[0] == 0 &&
                     g.inicio[v] == 2 * e && (int64_t)g.destino.size() == 2 * e && (int64_t)g.peso.size() == 2 * e;
        int64_t lazos = 0, repetidas = 0, pesoMal = 0, asim = 0, rangoMal = 0, aristasNoDir = 0;
        double wmin = 2, wmax = -1;
        std::unordered_map<uint64_t, double> vistas; vistas.reserve(2 * e);
        std::vector<int> p(v); for (int x = 0; x < v; ++x) p[x] = x;
        for (int u = 0; u < v; ++u)
            for (int64_t k = g.inicio[u]; k < g.inicio[u + 1]; ++k) {
                int w = g.destino[k]; double pw = g.peso[k];
                if (w < 0 || w >= v) { ++rangoMal; continue; }
                if (w == u) ++lazos;
                if (!(pw > 0.0 && pw <= 1.0)) ++pesoMal;
                if (pw < wmin) wmin = pw;
                if (pw > wmax) wmax = pw;
                uint64_t dir = (uint64_t(u) << 32) | uint32_t(w);
                if (vistas.count(dir)) ++repetidas; else vistas[dir] = pw;
                if (u < w) { ++aristasNoDir; int a = find(p, u), b = find(p, w); if (a != b) p[a] = b; }
            }
        for (auto& kv : vistas) {
            uint64_t u = kv.first >> 32, w = kv.first & 0xFFFFFFFFu;
            auto it = vistas.find((w << 32) | u);
            if (it == vistas.end() || it->second != kv.second) ++asim;
        }
        int comps = 0; for (int x = 0; x < v; ++x) if (find(p, x) == x) ++comps;
        bool ok = okTam && lazos == 0 && repetidas == 0 && pesoMal == 0 && asim == 0 && rangoMal == 0 &&
                  comps == 1 && aristasNoDir == e;
        if (!ok) ++fallas;
        std::printf("v=2^%d e=2^%d ... => %s\n", c.i, c.j, ok ? "OK" : "FALLA");  // (printf completo abreviado aquí)
    }
    return fallas ? 1 : 0;
}
```

`secuencial.cpp` (misma secuencia de `Aleatorio`, descarte uno a uno con `unordered_set`; compara el conjunto de aristas con el de `generarGrafo`):

```cpp
static std::vector<uint64_t> secuencial(int v, int64_t e, uint32_t semilla) {
    Aleatorio rng(semilla); std::vector<uint64_t> cl; std::unordered_set<uint64_t> s;
    for (uint32_t i = 1; i < (uint32_t)v; ++i) { uint64_t k = detalle::clave((uint32_t)rng.enRango(i), i); cl.push_back(k); s.insert(k); }
    while ((int64_t)cl.size() < e) {
        uint32_t a = (uint32_t)rng.enRango(v), b = (uint32_t)rng.enRango(v);
        while (b == a) b = (uint32_t)rng.enRango(v);
        uint64_t k = detalle::clave(a, b);
        if (s.insert(k).second) cl.push_back(k);   // repetida => se descarta y se vuelve a sortear
    }
    std::sort(cl.begin(), cl.end()); return cl;
}
// Casos: (v,e) ∈ {(16,100), (64,2000), (1024,2^16), (2^12,2^16), (1000,400000)} × semillas 1..20 = 100 casos.
```
