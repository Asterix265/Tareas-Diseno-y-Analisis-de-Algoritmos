"""Grafica las métricas de Tarea 2 y dos resultados de una misma búsqueda."""

import argparse
import csv
from pathlib import Path


METRICAS = (
    ("construccion_ms", "Tiempo de construcción (ms)", "construccion.png"),
    ("ios_construccion", "I/Os de construcción", "ios_construccion.png"),
    ("bytes", "Tamaño del árbol (bytes)", "tamano.png"),
    ("busqueda_promedio_ms", "Tiempo promedio de búsqueda (ms)", "busqueda.png"),
    ("ios_busqueda_promedio", "I/Os promedio de búsqueda", "ios_busqueda.png"),
)


def leer_metricas(ruta):
    """Recibe un CSV de métricas y devuelve sus filas agrupadas por árbol."""
    agrupadas = {}
    with ruta.open(newline="", encoding="utf-8") as archivo:
        for fila in csv.DictReader(archivo):
            agrupadas.setdefault(fila["arbol"], []).append(fila)
    for filas in agrupadas.values():
        filas.sort(key=lambda fila: int(fila["exponente"]))
    return agrupadas


def graficar_metricas(agrupadas, salida, plt):
    """Recibe las series por árbol; guarda cinco gráficos comparativos."""
    for columna, eje_y, nombre in METRICAS:
        fig, ax = plt.subplots(figsize=(8, 5))
        for arbol, filas in sorted(agrupadas.items()):
            ax.plot([int(f["exponente"]) for f in filas],
                    [float(f[columna]) for f in filas], marker="o", label=arbol)
        ax.set_xlabel("N = 2^x (exponente x)")
        ax.set_ylabel(eje_y)
        ax.grid(alpha=0.25)
        ax.legend()
        fig.tight_layout()
        fig.savefig(salida / nombre, dpi=180)
        plt.close(fig)


def leer_curva(ruta):
    """Recibe el CSV de una búsqueda y devuelve tiempos Unix y temperaturas."""
    with ruta.open(newline="", encoding="utf-8") as archivo:
        filas = list(csv.DictReader(archivo))
    return ([int(f["llave"]) for f in filas],
            [float(f["temperatura"]) for f in filas])


def graficar_temperatura(carpeta, salida, plt):
    """Compara la primera búsqueda de los tamaños mínimo y máximo disponibles."""
    curvas = sorted(carpeta.glob("busqueda_b_*.csv"),
                   key=lambda ruta: int(ruta.stem.rsplit("_", 1)[1]))
    if len(curvas) < 2:
        return
    fig, ejes = plt.subplots(2, 1, figsize=(10, 7), sharex=True)
    for ax, ruta in zip(ejes, (curvas[0], curvas[-1])):
        tiempos, temperaturas = leer_curva(ruta)
        ax.scatter(tiempos, temperaturas, s=5, alpha=0.65)
        ax.set_ylabel("Temperatura (°C)")
        ax.set_title(f"N = 2^{ruta.stem.rsplit('_', 1)[1]} ({len(tiempos)} pares)")
        ax.grid(alpha=0.25)
    ejes[-1].set_xlabel("Tiempo Unix (s)")
    fig.tight_layout()
    fig.savefig(salida / "temperaturas_dos_n.png", dpi=180)
    plt.close(fig)


def main():
    """Entrada: rutas opcionales. Salida: gráficos PNG en la carpeta indicada."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--metricas", type=Path, default=Path("resultados/metricas_b.csv"))
    parser.add_argument("--salida", type=Path, default=Path("figuras"))
    args = parser.parse_args()
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    args.salida.mkdir(parents=True, exist_ok=True)
    graficar_metricas(leer_metricas(args.metricas), args.salida, plt)
    graficar_temperatura(args.metricas.parent, args.salida, plt)


if __name__ == "__main__":
    main()
