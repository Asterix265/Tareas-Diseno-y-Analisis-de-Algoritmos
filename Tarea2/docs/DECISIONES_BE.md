# Árbol Bε: implementación y decisiones (persona A)

Código en `src/arbol_be.h` y pruebas en `tests/tests_be.cpp` (`make test_be`, con `datos.bin` en `Tarea2/`).
Este archivo sirve de base para la sección Desarrollo del informe.

## Estructura

- `NodoBe<C, L>` sigue el layout de la sección 3.2: `es_interno`, `k_llaves`, `k_buffer`, `llaves[C]`,
  `hijos[C+1]` y `buffer[L]`. Un `static_assert` comprueba que ambas configuraciones miden 4096 bytes.
- `ArbolBe<C, L>` es una sola plantilla para las dos configuraciones: `ArbolBe025 = ArbolBe<4, 506>` y
  `ArbolBe075 = ArbolBe<126, 384>`. α se pasa al constructor, de modo que el bonus no requiere recompilar.
- Todo acceso al "disco" pasa por `Disco<Nodo>` (construcción) y `ArchivoNodos<Nodo>` (búsqueda), los
  mismos del árbol B. La raíz siempre está en el bloque 0.
- La interfaz pública es la misma que la de `ArbolB` (`insertar`, `guardar`, `buscarRango`, `lecturas`,
  `escrituras`, `cantidadNodos`, `tamanoBytes`), más `altura()`, `alfa()` y `estadisticas()`.

## Vaciado (flush)

- Una pasada agrupa el buffer por hijo (los grupos son tramos contiguos porque el buffer está ordenado),
  calcula f = ⌈k_buffer / (k_llaves + 1)⌉ y baja todos los grupos con al menos α·f pares, de derecha a
  izquierda. El grupo más grande siempre tiene al menos f pares, así que cada pasada baja algo.
- El nodo que se vacía se mantiene en memoria durante toda la pasada y lo escribe una sola vez quien lo
  tiene (la raíz en `insertar`, o el padre en `bajarGrupo`). Cada hijo que recibe un grupo cuesta una
  lectura y una escritura, más una escritura al final del arreglo si se divide.

## Casos que el enunciado no especifica

1. **El hijo tiene espacio.** El grupo se mezcla ordenadamente en su buffer y se escribe el hijo.
2. **Hoja no llena que debe dividirse.** Durante un vaciado, una hoja con k < l_max pares puede no tener
   espacio para el grupo. Se divide en ⌈k/2⌉ (con k = l_max es la posición l_max/2 del enunciado).
3. **El grupo no cabe ni después de dividir.** Si una mitad no puede recibir todos sus pares (por ejemplo,
   con llaves monótonas todo el grupo cae del mismo lado), recibe solo los que caben y **el resto vuelve
   al buffer del padre**, en su misma posición. Esos pares siguen en el camino desde la raíz hasta su
   hoja, así que la invariante se mantiene. En los datos reales esto no ocurrió nunca (2^18 pares, las
   dos configuraciones); sí ocurre con llaves ascendentes, descendentes o todas iguales.
4. **Nodo interno que llega a c llaves.** Siguiendo 3.4.3 c) y 3.4.4 b), se divide en cuanto llega a c
   llaves durante un vaciado, y la pasada se detiene antes de bajar otro grupo. Por eso, en reposo un nodo
   interno tiene entre 1 y c − 1 llaves (con c = 4, entre 2 y 4 hijos); los c + 1 hijos solo existen
   justo antes de dividirse. Esto hace al árbol con ε ≈ 0,25 algo más alto que con 5 hijos.
5. **Una pasada no libera espacio.** Si los hijos se dividieron sin recibir pares, el nodo puede seguir sin
   espacio; entonces se repite la pasada hasta tener espacio o llegar a c llaves (y dividirse). Cada
   pasada baja al menos un par o agrega una llave, así que termina.
6. **Llaves repetidas.** Un par con llave igual a una divisoria va al hijo izquierdo (≤). Un split puede
   dejar llaves iguales a la divisoria a ambos lados, así que la búsqueda también entra al hijo derecho
   cuando el rango toca la divisoria (`superior >= llaves[i-1]`). El dataset tiene 11 llaves repetidas.

## Búsqueda

En cada nodo visitado se buscan con `lower_bound` los pares del buffer con llave en [l, u], y se baja a
todos los hijos cuyo rango intersecta [l, u]. Cada nodo leído del archivo cuenta una lectura.

## Verificación (`tests_be`)

Para cada configuración y α ∈ {0,1; 0,5; 0,9}: árboles de 0, 1, l_max, l_max + 1 y 3·l_max pares, 200 000
pares con muchas repetidas, 100 000 ascendentes, 100 000 descendentes, 20 000 iguales y los primeros 2^18
pares reales. Sobre el archivo serializado revisa: buffers ordenados, capacidades, cada par dentro del
rango de su subárbol, hojas a la misma profundidad, `altura()`, Σ k_buffer = N, ningún bloque huérfano y
el multiconjunto exacto de pares. Además compara cada búsqueda con una referencia ordenada (rangos al
azar, consultas puntuales y rangos en las divisorias de la raíz), y verifica que B, Bε 0,25 y Bε 0,75
devuelvan lo mismo en 50 búsquedas de una semana. También pasa con AddressSanitizer y UBSan.

## Pendiente

- Elegir el α de los experimentos (0,5 por ahora) y justificarlo con el bonus, que conviene hacer con
  ε ≈ 0,25: con ε ≈ 0,75, f ≈ 384/127 ≈ 3, así que ⌈α·f⌉ solo toma los valores 1, 2 o 3.
- Integrar `ArbolBe025` y `ArbolBe075` al `main` de experimentos (persona B).
