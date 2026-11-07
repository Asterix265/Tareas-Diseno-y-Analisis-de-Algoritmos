# Tarea 2 — Árboles B y Bε (CC4102)

Estado: infraestructura de bloques, lector de `datos.bin` y árbol B implementados. Falta integrar el árbol Bε y la batería completa de experimentos. Las mediciones locales solo verifican el funcionamiento; los tiempos e I/Os finales se obtendrán en el servidor.

## Requisitos

- Compilador C++17 y `make`. Para los gráficos, Python 3 con `matplotlib` (`python3 -m pip install matplotlib`).
- `datos.bin` y `datos_sample.txt` en esta carpeta. `datos.bin` no se incluye en Git.

## Verificación actual

Desde `Tarea2/`, ejecutar `make test`. La prueba compara los primeros 50 pares de `datos.bin` con la muestra, inserta 10 000 pares (incluidas llaves repetidas), serializa el árbol B y compara 100 búsquedas por rango con `std::multimap`. También inserta los primeros 32 768 pares reales y comprueba 10 búsquedas contra un conteo directo del binario leído.

## Experimento B

`make` compila `experimento_b`. `./experimento_b --desde 15 --hasta 15` ejecuta un tamaño de prueba; sin opciones ejecuta los tamaños `2^15` a `2^26`, 50 búsquedas por tamaño. `--datos`, `--salida`, `--busquedas` y `--semilla` permiten cambiar las rutas y la corrida. Se generan `resultados/metricas_b.csv`, `rangos.csv` y, para el primer y último tamaño de la corrida, un CSV de una búsqueda con tiempo Unix y temperatura. Los rangos son los mismos para todos los valores de N y se podrán reutilizar para Bε. Los árboles serializados se borran después de las búsquedas para limitar el espacio usado.

`python3 scripts/graficos.py` genera cinco gráficos de métricas en `figuras/`. Si hay curvas de dos tamaños, también grafica la temperatura de la misma búsqueda. El script agrupa por la columna `arbol`, de modo que acepta un CSV combinado con las filas de Bε cuando estén integradas.

## Servidor de experimentos

El servidor usado en la Tarea 1 fue `kestrelserver`: Ubuntu 26.04.1 LTS, x86_64, AMD Ryzen 7 3750H (4 núcleos, 8 hilos), 9,1 GiB de RAM y g++ 15.2.0. `Tarea1/resultados/setup.txt` registra ese ambiente. Antes de la corrida final de Tarea 2 hay que confirmar que sigue siendo el mismo y guardar un setup nuevo, pues la memoria disponible, la versión del sistema y otros detalles pueden cambiar.

En el servidor, situarse en `Tarea2/`, copiar `datos.bin` y `datos_sample.txt` por separado (no se suben al repositorio), y ejecutar:

```bash
make clean && make
make test
mkdir -p resultados
{ lscpu; free -h; uname -a; lsb_release -a; g++ --version; } > resultados/setup_tarea2.txt 2>&1
tmux new -s tarea2
./experimento_b 2>&1 | tee resultados/log_b.txt
```

La batería de Bε se correrá en el mismo servidor y con los mismos rangos de `resultados/rangos.csv`. Para el informe, usar solo mediciones del servidor y registrar también cualquier cambio de configuración del equipo. Conviene hacer primero `./experimento_b --desde 15 --hasta 15` en el servidor para comprobar rutas y datos antes de lanzar los 12 tamaños.

## Interfaz compartida

- `Par` (`src/datos.h`): `int32_t llave` y `float valor`, 8 bytes.
- `Disco<Nodo>` (`src/disco.h`): `leer(i)`, `escribir(i,nodo)`, `agregar(nodo)`, `guardar(ruta)`, `cantidadNodos()`, `tamanoBytes()`, `lecturas`, `escrituras`. Cada acceso por bloque cuenta una operación. `guardar` no se suma a los I/Os de inserción.
- `ArchivoNodos<Nodo>` (`src/disco.h`): abre el árbol serializado y `leer(i)` busca el bloque `i` en `i*4096`, contando una lectura.
- La raíz siempre ocupa el bloque 0. Al crecer la raíz, se agregan sus dos mitades al final y se sobrescribe el bloque 0 con la nueva raíz.
- `ArbolB` (`src/arbol_b.h`): `insertar(Par)`, `guardar(ruta)`, métricas de construcción y `buscarRango(archivo,l,u)`.

La persona que implemente Bε puede usar `Disco<NodoBe<C,L>>` y `ArchivoNodos<NodoBe<C,L>>` sin acceso directo al vector o al archivo. La configuración de `NodoBe` debe cumplir `static_assert(sizeof(NodoBe<C,L>) == 4096)`.

## Decisiones por revisar en conjunto

Las llaves repetidas pueden quedar a ambos lados de un split. La búsqueda B visita ambos hijos que tocan una llave divisoria igual al borde del rango. Para Bε se necesita la misma precaución, o una política global acordada para duplicados. El enunciado no especifica qué hacer si un lote de flush todavía desborda una mitad recién dividida; esa decisión corresponde a la implementación Bε y debe documentarse en el informe.
