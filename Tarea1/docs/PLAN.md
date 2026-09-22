# Plan de trabajo — Tarea 1

Entrega: **lunes 28 de septiembre, 23:59.**
Objetivo intermedio: tener el código congelado y los experimentos completos
corriendo en el servidor **el viernes 25 en la noche**.

| Rol | Rama | Archivos principales |
|---|---|---|
| **Persona A — Colas** | `colas` | `src/cola_binomial.h`, `src/cola_fibonacci.h` |
| **Persona B — Infra** | `infra` | `src/generador.h`, `src/prim.h`, `src/main.cpp`, `scripts/graficos.py`, servidor, teoría |

Regla de oro: **la interfaz de `docs/ACUERDOS.md` no se cambia sin avisar.**
Todo lo privado de cada archivo (structs, helpers) es libre.

---

## Persona A — Colas

### Miércoles 23 → jueves 24: cola binomial

Archivo: `src/cola_binomial.h` (seguir los `TODO(A)`).

- [ ] Definir el struct de nodo final (se sugiere hijo izquierdo + hermano derecho).
- [ ] Escribir la primitiva `enlazar(a, b)`, que une dos árboles B_k en un B_{k+1}.
      **Se reutiliza en la consolidación de Fibonacci** (recomendación 8a).
- [ ] `insert(v, key)`: crear el nodo, guardarlo en `nodoDe[v]` y unirlo a la lista de raíces.
- [ ] `extractMin()`: sacar la raíz mínima, unir sus hijos con la lista de raíces y recalcular el mínimo.
- [ ] `decreaseKey(v, key)`: subir el nodo mientras su clave sea menor que la de su padre (sección 3.2).
  - Decidir entre **intercambiar el contenido y actualizar `nodoDe`** o **reconectar punteros**.
    Anotar por qué: va en el informe.
  - `ops += 1` por cada intercambio, **solo aquí**.
- [ ] Destructor que libere todos los nodos (también los que quedan si no se vació la cola).
- [ ] Cambiar `implementada = true`.
- [ ] Verificar:
  ```bash
  make test                                   # o el comando g++ del README
  ./prim --colas binomial --reducir 4 --reps 2
  ```
- [ ] Push a `colas` y avisar a B para el merge del jueves.

### Jueves 24 → viernes 25: cola de Fibonacci

Archivo: `src/cola_fibonacci.h`.

- [ ] Partir del código de la tarea anterior (`Basic_definitionsFibonacci.h`) y adaptarlo:
  - usar los nombres de la interfaz (`insert`, `extractMin` que retorna `(costo, vértice)`, etc.);
  - usar la tabla de grados de la consolidación con un tamaño fijo de ⌊log_φ n⌋ + 2,
    reservado una sola vez en vez de hacer `push_back` en cada consolidación;
  - usar `enlazar` de la binomial para unir árboles del mismo grado.
- [ ] `decreaseKey`: `cut` + `cascadingCut` (sección 3.3). `ops += 1` por **cada** `cut`,
      incluido el primero.
- [ ] Destructor y `implementada = true`.
- [ ] Probar casos borde: n = 1, claves repetidas, `decreaseKey` sobre una raíz,
      `decreaseKey` a la misma clave.
- [ ] Verificar igual que la binomial y hacer push a `colas`.

### Para el informe (Persona A)

- Sección **Desarrollo** (0,8 pts):
  - cómo se accede directamente a los nodos de Q (`nodoDe`);
  - cómo se implementó `decreaseKey` en cada cola y la decisión de la binomial;
  - cómo se construye la cola inicial (n inserciones).
- Tamaño real de cada nodo (`sizeof`), para la estimación de memoria de B.

---

## Persona B — Infra

El esqueleto ya trae el generador, Prim, el `main` y los gráficos funcionando.
El trabajo de B es **hacerse dueño de ese código**, dejar listo el servidor y
escribir la teoría.

### Miércoles 23: revisión del código base

- [ ] Leer `generador.h`, `prim.h` y `main.cpp` completos y corregir lo que no les convenza.
- [ ] Confirmar contra el enunciado:
  - grafos conexos, simples, con exactamente 2^j aristas y pesos en (0,1];
  - la generación **no** entra en la medición de tiempo;
  - las series A–D con los rangos (i, j) correctos.
- [ ] Probar el flujo completo sin colas reales:
  ```bash
  ./prim --colas falsa --reducir 9 --reps 2
  python3 scripts/graficos.py --reducidos
  ```
- [ ] Medir el costo de `steady_clock::now()` en el servidor (unas 10^6 llamadas seguidas)
      y anotarlo, porque sirve para el análisis del costo amortizado.

### Miércoles 23 → jueves 24: servidor

- [ ] Instalar `g++`, `python3-matplotlib` y `tmux`, y clonar el repo.
- [ ] `./tests_bin` en el servidor: la línea **huella** tiene que salir idéntica
      a la de Windows y a la del Mac.
- [ ] Guardar la salida de `lscpu; free -h; uname -a; g++ --version; lsb_release -a`
      para la sección Resultados.
- [ ] `./prim --memoria` y comparar con la RAM del servidor.

### Jueves 24 → viernes 25: teoría para el informe

- [ ] **3.3**: demostrar que `decreaseKey` en Fibonacci es O(1) amortizado
      (potencial Φ = t(H) + 2·m(H), con c cortes en cascada) y mostrar que Φ es válido.
- [ ] **3.4**: argumentar por qué construir Q con |V| inserciones cuesta O(|V|) en ambas colas.
- [ ] **4.2 / 4.3**: análisis de complejidad: O(e log v) con la binomial y O(e + v log v) con la de Fibonacci.
- [ ] **6.2**: tabla de estimación de memoria (lista de adyacencia, nodos de Q, arreglos
      auxiliares) para v = 2^15, e = 2^20 y para el caso más grande de la serie B.
- [ ] Dejar armada la plantilla del informe (LaTeX o Typst) con las secciones y su puntaje.

---

## Trabajo conjunto antes de los experimentos

### Jueves 24: integración de la cola binomial

1. **A** hace merge de `colas` a `main` cuando la binomial pase los tests.
2. **B** hace merge de `main` a `infra`, así los dos trabajan sobre la misma base.
3. **Revisión cruzada, unos 30 minutos cada uno:**
   - A revisa `prim.h`: dónde se mide el tiempo y si se cuentan bien las llamadas a `decreaseKey`;
   - B revisa `cola_binomial.h`: que `nodoDe` quede bien después de cada intercambio.
4. Correr en los tres equipos (Windows, Mac y servidor):
   ```bash
   ./tests_bin
   ./prim --colas binomial --reducir 4 --reps 2
   python3 scripts/graficos.py --reducidos
   ```
   Mirar juntos los gráficos de la binomial: ¿la curva medida se parece a la cota?
5. Acordar juntos **la hipótesis de la introducción** (qué cola esperan que gane y por qué)
   y la **aplicación de la sección 5**.

### Viernes 25: integración de Fibonacci y congelamiento

1. **A** hace merge de la cola de Fibonacci a `main`.
2. Prueba completa con las dos colas, en local y en el servidor:
   ```bash
   ./tests_bin                              # todo debe pasar, nada omitido
   ./prim --reducir 4 --reps 2              # verificacion_*.csv con todos los ok = 1
   ```
3. **Estimar la duración total** en el servidor:
   ```bash
   ./prim --reducir 2 --reps 1              # grafos 4 veces más chicos
   ```
   Multiplicar el tiempo por ~4–5 (por el factor log) y luego por 10 (repeticiones).
   Si no alcanza el tiempo, correr primero `--series CD` (más livianas) y después `--series AB`.
4. **Checklist "listo para experimentos"** (los dos tienen que estar de acuerdo):
   - [ ] `./tests_bin` pasa completo en el servidor.
   - [ ] En `verificacion_*.csv`, todas las filas tienen `ok = 1`.
   - [ ] Compilación con `-O2` y sin warnings.
   - [ ] `ops` cuenta intercambios (binomial) y cortes (Fibonacci) según lo acordado.
   - [ ] Los datos del setup del servidor están guardados.
   - [ ] Nadie más usa el servidor durante la corrida.
5. Congelar el código y marcarlo:
   ```bash
   git tag experimentos-v1 && git push --tags
   ```
   Desde aquí, **no se cambia código de medición**. Si aparece un bug, se corrige,
   se crea `experimentos-v2` y se vuelve a correr todo.
6. Lanzar la batería completa en `tmux` el viernes en la noche:
   ```bash
   tmux new -s prim
   ./prim 2>&1 | tee resultados/log.txt
   ```

### Después (sábado a lunes, resumen)

- Sábado y domingo: revisar los CSV a medida que salen, generar los gráficos y redactar Resultados.
- Domingo y lunes: Análisis (preguntas 3.3, 5 y 6.3.2), conclusión, README final,
  revisión cruzada del informe y entrega.

---

## Si algo sale mal

| Síntoma | Qué revisar |
|---|---|
| El test "extractMin coincide con la referencia" falla | `nodoDe` desactualizado después de un intercambio o un corte; el mínimo no se recalcula. |
| `verificacion` con `ok = 0` | Una de las colas extrae en mal orden. Correr `./tests_bin`, que tiene casos más chicos. |
| La huella difiere entre equipos | Alguien usó `std::uniform_*_distribution` o `rand()`. |
| El proceso muere por falta de memoria en la serie B | Revisar `./prim --memoria`; liberar los nodos en el destructor; confirmar un compilador de 64 bits. |
| Los tiempos tienen mucha varianza | Otro proceso en el servidor, o el modo de ahorro de energía; revisar con `htop` durante la corrida. |
