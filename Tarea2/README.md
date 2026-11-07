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

En el servidor, situarse en `Tarea2/`, copiar `datos.bin` y `datos_sample.txt` por separado (no se suben al repositorio), y ejecutar:

```bash
make clean && make
make test
mkdir -p resultados
{ lscpu; free -h; uname -a; lsb_release -a; g++ --version; } > resultados/setup_tarea2.txt 2>&1
tmux new -s tarea2
./experimento 2>&1 | tee resultados/log.txt
./experimento --bonus 2>&1 | tee resultados/log_bonus.txt
```

Para el informe, usar solo mediciones del servidor y registrar también cualquier cambio de configuración del equipo. Conviene hacer primero `./experimento --desde 15 --hasta 15` en el servidor para comprobar rutas y datos antes de lanzar los 12 tamaños. La corrida bonus es independiente y puede tomar bastante tiempo; los gráficos se generan después de ambas corridas.

## Interfaz compartida

- `Par` (`src/datos.h`): `int32_t llave` y `float valor`, 8 bytes.
- `Disco<Nodo>` (`src/disco.h`): `leer(i)`, `escribir(i,nodo)`, `agregar(nodo)`, `guardar(ruta)`, `cantidadNodos()`, `tamanoBytes()`, `lecturas`, `escrituras`. Cada acceso por bloque cuenta una operación. `guardar` no se suma a los I/Os de inserción.
- `ArchivoNodos<Nodo>` (`src/disco.h`): abre el árbol serializado y `leer(i)` busca el bloque `i` en `i*4096`, contando una lectura.
- La raíz siempre ocupa el bloque 0. Al crecer la raíz, se agregan sus dos mitades al final y se sobrescribe el bloque 0 con la nueva raíz.
- `ArbolB` (`src/arbol_b.h`) y `ArbolBe<C,L>` (`src/arbol_be.h`): `insertar(Par)`, `guardar(ruta)`, métricas de construcción y `buscarRango(archivo,l,u)`. `ArbolBe` agrega `altura()`, `estadisticas()` (vaciados, splits y lotes parciales) y `contarPares(archivo)`.

`ArbolBe<C,L>` usa `Disco<NodoBe<C,L>>` y `ArchivoNodos<NodoBe<C,L>>` sin acceso directo al vector o al archivo. `static_assert` exige nodos de 4096 bytes en ambas configuraciones (`ArbolBe025 = ArbolBe<4,506>` y `ArbolBe075 = ArbolBe<126,384>`).

## Decisiones de implementación

Las llaves repetidas pueden quedar a ambos lados de un split. Ambas búsquedas visitan los dos hijos que tocan una llave divisoria igual al borde del rango. En Bε, cada vaciado baja todos los grupos con al menos α·f pares, de derecha a izquierda, como pide 3.4.3. Si un hijo no puede recibir todo su grupo ni después de dividirse, recibe lo que cabe y el resto vuelve al buffer del padre. Un nodo interno que llega a `C` divisorias durante un vaciado se divide en ese momento. Los detalles y el resto de los casos que el enunciado no especifica están en `docs/DECISIONES_BE.md`, y deben explicarse en el informe.
