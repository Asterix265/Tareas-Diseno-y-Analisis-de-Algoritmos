# Qué mencionar en el informe — Tarea 1 (Prim y análisis amortizado)

Lista de lo que el informe **debe** incluir según el enunciado (con página) y de las
**decisiones propias** que hay que justificar. Está ordenada por las secciones y puntajes de
la rúbrica 7.2 (p. 8).

> Regla del enunciado para Desarrollo (p. 8): *"Dado que el cuerpo docente ya conoce la teoría,
> enfóquense en explicar **sus** decisiones de implementación […]. No repitan la descripción de
> las colas que ya está en el apunte."*
>
> Referencia de una tarea anterior del curso (Kruskal 2025-1; **no** es de este enunciado): *"la
> presencia de algún aspecto en una sección equivocada hará que no se tenga la totalidad del
> puntaje"*. Conviene respetar la sección donde la rúbrica pide cada cosa.

Marcas: **[ENUNCIADO]** = lo exige el enunciado. **[DECISIÓN]** = elección nuestra que hay que
justificar. **[DATO]** = número que se obtiene en el servidor.

---

## 0. Datos que hay que juntar antes de escribir (en el servidor, con el tag `experimentos-v1`)

- [ ] `resultados/setup.txt`: `lscpu` (CPU y cachés L1/L2/L3), `free -h` (RAM), `uname -a`,
      `lsb_release -a` (SO), `g++ --version`.
- [ ] Flags de compilación usados: `-std=c++17 -O2` (y `-Wall -Wextra`).
- [ ] `resultados/calibracion.txt` (`./prim --calibrar`): costo de `steady_clock::now()` en ns.
- [ ] `resultados/memoria.txt` (`./prim --memoria`).
- [ ] `resultados/tiempos_ABCD.csv`, `verificacion_ABCD.csv`, `curvas_ABCD.csv`, `log.txt`.
- [ ] `figuras/`: los 12 PNG, `tabla_tiempos.tex`, la tabla de C/D y el anexo por repetición.
- [ ] Las constantes c ajustadas de cada gráfico (aparecen en la leyenda de cada figura).
- [ ] Commit o tag exacto con que se corrió (`experimentos-v1`).
- [ ] Respuestas de los auxiliares a las 4 ambigüedades (sección 6 de este documento).

---

## 1. Introducción — 0,6 pts (p. 8)

- [ ] **[ENUNCIADO]** Presentar el problema del MST (grafo conexo, no dirigido, con pesos;
      |E'| = |V| − 1).
- [ ] **[ENUNCIADO]** Resumir qué se aborda en el informe: Prim con cola binomial y con cola de
      Fibonacci, costo total y costo amortizado de `decreaseKey`.
- [ ] **[ENUNCIADO]** **Hipótesis** sobre el desempeño esperado de cada estructura. Debe poder
      verificarse después con los gráficos. Algunas ideas, a elegir y justificar:
  - En teoría, Fibonacci (O(e + v log v)) supera a binomial (O(e log v)) cuando e ≫ v, es decir,
    en la serie A con j grande.
  - En la práctica, las constantes de Fibonacci (más punteros por nodo: 56 B contra 40 B, peor
    localidad) podrían anular esa ventaja con estos tamaños.
  - `decreaseKey` debería tener costo acumulado lineal en la cantidad de llamadas en Fibonacci, y
    a lo más k·log v en binomial.

---

## 2. Desarrollo — 0,8 pts (p. 8)

La rúbrica pide explícitamente tres cosas; van primero.

### 2.1 Lo que la rúbrica pide explícitamente

- [ ] **[ENUNCIADO] Acceso directo a los nodos de Q.** Cada cola guarda dentro un arreglo
      `nodoDe[v]` (puntero al nodo del vértice v). Prim decide "u ∈ Q" (línea 9 del pseudocódigo)
      con su propio arreglo auxiliar `enQ` (`enQ[v] = 0` al extraer v). **[DECISIÓN]** `enQ` es
      **extra** respecto a los arreglos que lista 6.2 (costos, parent y punteros a los nodos de Q):
      hay que declararlo.
- [ ] **[ENUNCIADO] `decreaseKey` en cada estructura.**
  - Binomial (§3.2): **intercambio de contenido** (clave y vértice) con el padre, actualizando
    `nodoDe` de los dos nodos en cada intercambio. **[ENUNCIADO, p. 2]** *"deben justificar cuál
    eligieron en su informe"*. Justificación posible: no se reconecta ningún puntero del árbol
    (padre, hijo, hermano); cada paso es O(1) con dos escrituras en `nodoDe`, y es la traducción
    literal de la línea 4 del pseudocódigo.
  - Fibonacci (§3.3): `cut` + `cascadingCut` tal cual el pseudocódigo. Al pasar a raíz, el nodo
    queda sin padre y sin marca (línea 3 de `cut`).
  - **[DECISIÓN]** Las colas comparan con el orden total (clave, vértice); con claves iguales
    desempata el vértice menor. Difiere del pseudocódigo, que usa `x.key < y.key` estricto:
    declararlo.
- [ ] **[ENUNCIADO] Construcción de la cola inicial.** `construir(Q, costos)` hace v inserciones
      sucesivas (línea 4). Recibe Q por referencia porque en C++ las colas no se pueden copiar ni
      mover (desviación literal menor frente a `Q ← construir(costos)`).

### 2.2 Argumento de §3.4 (p. 3): *"Argumente en su informe por qué ambas estructuras cumplen este costo"* (O(|V|))

- [ ] **[ENUNCIADO]** Binomial: insertar un B₀ equivale a sumar 1 a un contador binario; la cadena
      de enlaces es la cadena de acarreos. v incrementos cuestan O(v) en total (apunte §2.2, p. 34, y
      §2.4.3, p. 42: *"obtenemos tiempo O(n) si realizamos n inserciones sucesivas"*).
- [ ] **[ENUNCIADO]** Fibonacci: cada inserción agrega un B₀ a la lista de raíces en O(1)
      (apunte §2.5.1, p. 43: *"Heapify. Se realiza mediante n inserciones, en tiempo O(n)"*).

### 2.3 Análisis de complejidad de Prim (§4.2 y §4.3, p. 4: *"inclúyanla en su informe"*)

- [ ] **[ENUNCIADO]** Binomial: construir O(v) + v·extractMin O(log v) + ≤ e·decreaseKey
      O(log v) ⇒ **O(e log v)** (con e ≥ v − 1).
- [ ] **[ENUNCIADO]** Fibonacci: construir O(v) + v·extractMin O(log v) amortizado +
      ≤ e·decreaseKey O(1) amortizado ⇒ **O(e + v log v)**.
- [ ] Mencionar que cada arista provoca **a lo más un** `decreaseKey` (solo cuando se procesa el
      primero de sus dos extremos que sale de Q), por eso hay "hasta e" llamadas.

### 2.4 Decisiones de implementación que hay que declarar

- [ ] **[DECISIÓN] Traducción literal del pseudocódigo de Prim.** El código sigue las líneas 1-14
      con su número comentado: T explícito (vector de aristas, `return T`), `parent[v] = INDEFINIDO`
      distinto del −1 de la raíz, y el bucle explícito `for v ∈ V∖{r}`.
- [ ] **[DECISIÓN] Raíz r = 0.** El enunciado dice "nodo raíz arbitrario" (p. 3).
- [ ] **[DECISIÓN] Grafo en formato CSR.** Es una lista de adyacencia guardada en arreglos
      contiguos (`inicio`, `destino`, `peso`): los vecinos de u están en `[inicio[u], inicio[u+1])`.
      Cada arista se guarda **dos veces**, una por sentido (6.2). Justificación: memoria contigua,
      sin sobrecarga por lista.
- [ ] **[DECISIÓN] Desempate en `extractMin`.** Con claves iguales se elige el vértice de menor
      índice. El enunciado no especifica `extractMin`; así ambas colas extraen en el mismo orden y
      hacen la misma cantidad de `decreaseKey` sobre el mismo grafo.
- [ ] **[DECISIÓN] Tabla de grados en la consolidación de Fibonacci.** Se crea en cada
      `extractMin` y crece según el grado encontrado. Con cortes, los árboles ya no son B_k exactos,
      así que el grado máximo es O(log_φ n) (lema de Fibonacci) y no ⌈log₂ n⌉ como el arreglo A del
      apunte (p. 44). Medimos que reutilizarla no cambia el tiempo más allá del ruido (≈1–2 %).
- [ ] **[DECISIÓN] Memoria de los nodos.** Se reserva un `new` por nodo y se libera en
      `extractMin` y en el destructor.

---

## 3. Resultados — 2,0 pts (p. 8)

### 3.1 Setup (lo pide explícitamente)

- [ ] **[ENUNCIADO] [DATO]** CPU, RAM, tamaños de caché, SO, lenguaje y versión del compilador,
      flags de compilación.

### 3.2 Generación de los datos (§6.1, p. 5)

- [ ] **[ENUNCIADO]** v = 2^i vértices y exactamente e = 2^j aristas; grafo conexo y simple.
- [ ] **[ENUNCIADO] Distribución de los pesos** (*"deben documentarlo en su informe"*):
      uniforme discreta sobre {k/2³² : k = 1, …, 2³²} ⊂ (0, 1], generada con `std::mt19937`
      convertido a mano. Nunca vale 0; puede valer 1.
- [ ] **[DECISIÓN]** Hay **pesos repetidos** (medimos 2025 aristas con peso repetido entre 4.194.304,
      con v = 2^18, e = 2^22). Por eso el MST puede no ser único, pero su peso sí lo es (recomendación
      8 c, p. 8).
- [ ] **[DECISIÓN] Método de generación** (el "se sugiere" de 6.1):
  - Árbol: el vértice i se conecta a uno uniforme en [0, i−1]. Es lo mismo que el [1..i−1] del
    enunciado, con índices desde 0.
  - Aristas restantes: los **lazos** se descartan volviendo a sortear el extremo b (misma
    distribución que descartar el par completo). Las **repetidas** se descartan por lotes (ordenar y
    eliminar duplicados, y se sortean las que faltan).
  - Verificamos que el descarte por lotes da **exactamente el mismo conjunto de aristas** que
    descartar una a una con la misma secuencia aleatoria: 100 de 100 casos.
  - Semilla determinista por (serie, i, j, repetición): cualquier grafo se puede reproducir. Los
    grafos no se guardan en disco (8 e, p. 8).
- [ ] **[DECISIÓN]** El árbol base es un árbol recursivo aleatorio: los vértices de índice bajo
      tienden a tener más grado. Pesa sobre todo cuando e ≈ v (serie C con j = 18).

### 3.3 Metodología de medición

- [ ] **[ENUNCIADO]** 10 repeticiones por configuración, con un grafo distinto en cada una; se
      reporta el promedio (p. 6). Ambas colas corren sobre **el mismo** grafo en cada repetición.
- [ ] **[ENUNCIADO]** Reloj monótono `std::chrono::steady_clock` (8 d, p. 8).
- [ ] **[ENUNCIADO/DECISIÓN] Qué incluye el tiempo total:** las líneas 1-14 de Prim, incluida la
      construcción de Q. **Excluye** la generación del grafo (p. 6) y la liberación de memoria de Q
      (no está en el pseudocódigo).
- [ ] **[DECISIÓN]** El orden de las colas se alterna entre repeticiones, para no favorecer a
      ninguna con efectos de caché o de frecuencia de la CPU.
- [ ] **[DECISIÓN] Series A y B:** no se mide cada `decreaseKey` por separado, para no sumar el
      costo del reloj.
- [ ] **[DECISIÓN] Series C y D:** cada `decreaseKey` se mide con dos llamadas al reloj.
      **[DATO]** Costo de una llamada al reloj en el servidor = ___ ns (`--calibrar`). Por eso
      `tiempo_ms` de C y D no se compara con A y B.
- [ ] **[ENUNCIADO]** Las mediciones se guardan en memoria y se escriben al terminar (p. 7).
- [ ] **[DECISIÓN] Qué se cuenta como operación** (6.3.2 b):
  - Binomial: cada **intercambio** del `while` de `decreaseKey`.
  - Fibonacci: se cuentan **todos los cortes (primer corte + cascada)**. Difiere de la lectura
    literal de 6.3.2 b), que habla de "cortes en cascada": declararlo y justificarlo.
  - Las marcas no se cuentan.
- [ ] **[DECISIÓN]** Las curvas por llamada (`curvas_*.csv`) salen de una ejecución **extra** sobre
      el grafo de la repetición 0 y no entran en los promedios.

### 3.4 Lo que la rúbrica pide mostrar

- [ ] **[ENUNCIADO]** Tabla de tiempos de A y B con el promedio de cada configuración
      (`tabla_tiempos.tex`; los tiempos de cada repetición van en un anexo).
- [ ] **[ENUNCIADO]** Tabla de promedios de C y D: llamadas, tiempo acumulado, e intercambios
      (binomial) o todos los cortes (Fibonacci: primer corte + cascada).
- [ ] **[ENUNCIADO]** Los **12 gráficos** con los ejes etiquetados:
  - 4 de costo total: binomial y Fibonacci × series A y B.
  - 8 de costo amortizado: tiempo acumulado y operaciones × binomial y Fibonacci × series C y D.
  - Cada uno con la **cota teórica × constante c**, y la **misma escala** dentro de una serie.
- [ ] **[DECISIÓN]** La constante c se ajusta por mínimos cuadrados: c = Σ(y·f) / Σ(f²). Cotas
      usadas:
  - total binomial: e·log₂ v;
  - total Fibonacci: e + v·log₂ v;
  - `decreaseKey` binomial: k·log₂ v;
  - `decreaseKey` Fibonacci: k.
- [ ] **[ENUNCIADO]** **Verificación del peso de los MST** (p. 6: *"Reporten esta verificación en el
      informe"*). En todas las configuraciones y repeticiones, ambas colas dan el mismo peso (tolerancia
      relativa 10⁻⁹, por la suma en punto flotante). Citar `verificacion_ABCD.csv`: N de N filas con
      ok = 1.
- [ ] **[ENUNCIADO] Estimación de memoria** (§6.2, p. 5), hecha **antes** de correr, para
      v = 2^15, e = 2^20 y para cada implementación:
  - lista de adyacencia (2e entradas);
  - nodos de la cola (40 B binomial, 56 B Fibonacci, según `sizeof`);
  - arreglos auxiliares `costos`, `parent` y el arreglo de punteros a los nodos de Q.
  - **[ENUNCIADO]** Compararla con la RAM del servidor e indicar si hubo problemas o ajustes.
  - **[DECISIÓN]** Agregar el caso más grande de las series (v = 2^22, e = 2^24), el pico durante la
    generación y el arreglo T. Aclarar que `sizeof` no incluye la sobrecarga del asignador de memoria
    por cada `new`.
  - **[DECISIÓN]** Declarar el arreglo `enQ` (1 B por vértice) como **extra** respecto a los
    arreglos que pide 6.2; `./prim --memoria` lo muestra en una línea aparte.
  - Opcional, sin puntaje: *"¿Qué valores podrían generar problemas en su máquina?"*.

---

## 4. Análisis — 1,8 pts (p. 8)

La rúbrica dice que **aquí** van las respuestas a las **tres preguntas** (§3.3, §5, §6.3.2).

### 4.1 Pregunta §3.3: `decreaseKey` es O(1) amortizado en la cola de Fibonacci

- [ ] **[ENUNCIADO]** Extender el análisis del apunte §2.5.2 (p. 44), cuyo potencial es
      ϕ = 2ℓ + a (ℓ = árboles en la lista, a = celdas ocupadas del arreglo A).
- [ ] **[ENUNCIADO]** Hint: agregar un tercer término proporcional a los nodos marcados:
      ϕ' = 2ℓ + a + k·m.
- [ ] **[ENUNCIADO]** Mostrar que ϕ' es **válido**: ϕ'₀ = 0 con la cola vacía y ϕ' ≥ 0 siempre.
- [ ] Analizar una llamada con **c cortes en cascada**:
  - Costo real: O(1 + c), el primer corte más c cortes en cascada.
  - Cambio en ℓ: +(1 + c) árboles, así que 2ℓ aumenta en 2(1 + c).
  - Cambio en m: cada corte en cascada desmarca un nodo (−c), y al final se marca a lo más uno (+1).
  - Costo amortizado ≤ (1 + c) + 2(1 + c) + k(1 − c) = (3 + k) + c(3 − k). Con **k = 3** queda
    constante (6): O(1).
- [ ] Revisar que las demás operaciones siguen acotadas con ϕ':
  - `insert` no cambia m;
  - `extractMin` no aumenta m (al subir hijos a la lista de raíces y al enlazar, las marcas se
    borran);
  - por eso su costo amortizado sigue siendo O(log n), usando que el grado máximo es O(log n)
    (lema de Fibonacci).

### 4.2 Pregunta §5: aplicación a un problema real

- [ ] **[ENUNCIADO]** Proponer una aplicación de cualquiera de las dos implementaciones,
      justificando **en términos de los costos** de los algoritmos.
- [ ] Opcional (bonus de código, +1,0): implementarla en versión reducida.

### 4.3 Pregunta §6.3.2: *"¿Tienen sentido las formas de sus gráficos?"*

- [ ] Tiempo acumulado de `decreaseKey`: ¿es lineal en la cantidad de llamadas en Fibonacci?
      ¿Crece con log v en binomial (serie D, donde v varía)?
- [ ] Conteo de operaciones:
  - Intercambios por llamada en binomial: ¿cuántos en promedio frente a la cota log v?
  - Cortes por llamada en Fibonacci (se cuentan todos, primer corte + cascada): ¿acotados por una
    constante?

### 4.4 Preguntas guía de la rúbrica

- [ ] **[ENUNCIADO]** ¿Se refleja la diferencia entre O(e log v) y O(e + v log v) en los tiempos
      reales?
- [ ] **[ENUNCIADO]** ¿Qué constantes c se necesitaron para ajustar las cotas, y qué dicen sobre el
      costo real de cada estructura?
- [ ] **[ENUNCIADO]** Si las curvas no siguen a sus cotas, ¿qué factores fuera del modelo teórico lo
      explican? Candidatos:
  - Localidad de caché: nodos dispersos por `new`, recorrido de punteros; CSR contiguo.
  - Asignación y liberación de un nodo por vértice.
  - Costo del reloj en C y D (___ ns por llamada frente a ~100 ns por `decreaseKey`).
  - Llamadas reales a `decreaseKey` ≪ e: la cota usa e, pero con pesos aleatorios solo una fracción
    de las aristas mejora un costo. Comparar `dk_llamadas` con e.
  - Intercambios reales ≪ log v: con claves aleatorias, el nodo rara vez sube hasta la raíz.
  - Pesos repetidos y desempate por vértice.
  - El arreglo extra `enQ` (una lectura más por vecino en la línea 9).
  - Frecuencia de la CPU y otros procesos.

---

## 5. Conclusión — 0,8 pts (p. 8)

- [ ] **[ENUNCIADO]** Recapitular el trabajo.
- [ ] **[ENUNCIADO]** Verificar la hipótesis de la introducción: si se cumplió o no, y por qué.
- [ ] **[ENUNCIADO]** Posibles mejoras o trabajo futuro. Ideas:
  - reservar los nodos en bloque (pool) en vez de un `new` por nodo;
  - comparar con un heap binario;
  - medir `decreaseKey` sin el costo del reloj (muestreo, contadores de ciclos);
  - pesos de 53 bits para evitar repetidos;
  - un árbol base uniforme en vez de recursivo.

---

## 6. Ambigüedades del enunciado (preguntadas a los auxiliares)

Si no responden, declarar en el informe la interpretación usada.

| # | Texto del enunciado | Interpretación usada |
|---|---|---|
| 1 | 6.3.2 b), p. 6: "(cortes en cascada en la cola de Fibonacci, …)" | Se cuentan todos los cortes (primer corte + cascada). |
| 2 | 6.3.2, p. 6: los ocho gráficos dicen "serie A / serie B" | Se refiere a las series C y D, que son las definidas en 6.3.2. |
| 3 | 6.2, p. 5: "caso más grande posible (v = 2^15, e = 2^20)" | Se reporta ese caso (lo pedido) y también el mayor de las series. |
| 4 | 6.3.2 a), p. 6: "graficarlo contra la cantidad de llamadas realizadas" | x = llamadas totales por configuración (5 puntos por serie). La curva dentro de una ejecución está disponible en `curvas_*.csv`. |

---

## 7. Qué NO poner

- No repetir la teoría de las colas del apunte (p. 8).
- No incluir los datasets (8 e, p. 8).
- No usar `tiempo_ms` de las series C y D (incluye el costo del reloj).
- No mezclar resultados de otra máquina con los del servidor.
