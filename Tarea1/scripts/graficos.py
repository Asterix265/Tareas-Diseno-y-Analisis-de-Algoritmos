#!/usr/bin/env python3
"""Genera gráficos y tablas a partir de los CSV de mediciones de Prim."""
import argparse
import csv
import glob
import math
import os
import statistics
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

COLOR_MEDIDO = "#2a5caa"
COLOR_COTA = "#8a8a8a"

# Cada entrada guarda la función teórica, su leyenda y su nombre para el CSV.
COTAS_TOTAL = {
    "binomial": (lambda v, e: e * math.log2(v), r"$c \cdot e \log v$", "e*log2(v)"),
    "fibonacci": (lambda v, e: e + v * math.log2(v), r"$c \cdot (e + v \log v)$", "e + v*log2(v)"),
}
COTAS_DK = {
    "binomial": (lambda v, k: k * math.log2(v), r"$c \cdot k \log v$", "k*log2(v)"),
    "fibonacci": (lambda v, k: k, r"$c \cdot k$  (O(1) amortizado)", "k"),
}

# Selecciona el contador estructural que se muestra para cada cola.
OPS_GRAFICO = {"binomial": "dk_ops", "fibonacci": "dk_ops_cascada"}
YLABEL_OPS = {"binomial": "intercambios", "fibonacci": "cortes en cascada"}
NOMBRE_COLA = {"binomial": "Binomial", "fibonacci": "Fibonacci"}

# Campos que leer() convierte a int y float, respectivamente.
ENTEROS = ("i", "j", "v", "e", "rep", "dk_llamadas", "dk_tiempo_ns", "dk_ops", "dk_ops_cascada")
REALES = ("tiempo_ms", "peso_mst")


def leer(entrada, reducidos):
    """Lee los CSV de entrada y devuelve sus filas con valores numéricos convertidos.

    Incluye los archivos de pruebas reducidas solo cuando reducidos es verdadero.
    """
    filas = []
    vistas = {}
    for ruta in sorted(glob.glob(os.path.join(entrada, "tiempos_*.csv"))):
        if "_red" in os.path.basename(ruta) and not reducidos:
            continue
        with open(ruta, newline="") as f:
            for r in csv.DictReader(f):
                for k in ENTEROS:
                    r[k] = int(r[k])
                for k in REALES:
                    r[k] = float(r[k])
                clave = (r["serie"], r["i"], r["j"], r["rep"], r["cola"])
                if clave in vistas:
                    raise SystemExit(f"ERROR: fila repetida (serie, i, j, rep, cola) = {clave} "
                                     f"en {vistas[clave]} y {ruta}. Quite uno de los archivos.")
                vistas[clave] = ruta
                filas.append(r)
    return filas


def agrupar(filas):
    """Agrupa las filas por configuración y cola.

    Recibe las mediciones y devuelve sus promedios y desviaciones estándar por grupo.
    """
    g = defaultdict(list)
    for r in filas:
        g[(r["serie"], r["i"], r["j"], r["cola"])].append(r)
    res = {}
    for clave, rs in g.items():
        def prom(k):
            return statistics.mean(x[k] for x in rs)

        def desv(k):
            return statistics.stdev(x[k] for x in rs) if len(rs) > 1 else 0.0

        res[clave] = {
            "v": rs[0]["v"], "e": rs[0]["e"], "n": len(rs),
            "tiempo_ms": prom("tiempo_ms"), "tiempo_ms_sd": desv("tiempo_ms"),
            "dk_llamadas": prom("dk_llamadas"), "dk_llamadas_sd": desv("dk_llamadas"),
            "dk_tiempo_ms": prom("dk_tiempo_ns") / 1e6, "dk_tiempo_ms_sd": desv("dk_tiempo_ns") / 1e6,
            "dk_ops": prom("dk_ops"), "dk_ops_sd": desv("dk_ops"),
            "dk_ops_cascada": prom("dk_ops_cascada"), "dk_ops_cascada_sd": desv("dk_ops_cascada"),
        }
    return res


def ajustar(ys, fs):
    """Ajusta por mínimos cuadrados una constante a los valores medidos ys y teóricos fs."""
    den = sum(f * f for f in fs)
    return sum(y * f for y, f in zip(ys, fs)) / den if den else 0.0


def estilo(ax, xlabel, ylabel, titulo):
    """Añade etiquetas, título, grilla y leyenda a los ejes ax usando los textos recibidos."""
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(titulo, fontsize=11)
    ax.grid(True, color="#dddddd", linewidth=0.6)
    for lado in ("top", "right"):
        ax.spines[lado].set_visible(False)
    ax.legend(frameon=False)


def serie_datos(agr, serie, cola, xkey):
    """Selecciona de agr los puntos de una serie y cola, y los ordena por xkey."""
    pts = [(k, d) for k, d in agr.items() if k[0] == serie and k[3] == cola]
    pts.sort(key=lambda kd: kd[1][xkey])
    return pts


def graficar(salida, nombre, xs, ys, sds, cota_ys, cota_label, c, xlabel, ylabel, titulo, ylim, log2x):
    """Dibuja los valores medidos, sus desviaciones y la curva teórica recibidos.

    Guarda el gráfico PNG en salida/nombre e imprime esa ruta.
    """
    fig, ax = plt.subplots(figsize=(6, 4))
    ax.errorbar(xs, ys, yerr=sds, color=COLOR_MEDIDO, linewidth=2, marker="o", markersize=5,
                capsize=3, label="medido (promedio)")
    ax.plot(xs, cota_ys, color=COLOR_COTA, linewidth=2, linestyle="--",
            label=f"{cota_label},  c = {c:.3g}")
    if log2x:
        ax.set_xscale("log", base=2)
    ax.set_ylim(0, ylim)
    estilo(ax, xlabel, ylabel, titulo)
    fig.tight_layout()
    ruta = os.path.join(salida, nombre)
    fig.savefig(ruta, dpi=200)
    plt.close(fig)
    print("  ", ruta)


def nombre_cola(cola):
    """Convierte el nombre interno de cola en el nombre mostrado en los títulos."""
    return NOMBRE_COLA.get(cola, cola.capitalize())


def graficos_total(agr, colas, salida):
    """Genera en salida los gráficos de tiempo total de las series A y B.

    Recibe los datos agrupados y las colas disponibles; devuelve las constantes ajustadas.
    """
    constantes = []
    for serie, xkey, xlabel in (("A", "e", "e (aristas), v fijo"), ("B", "v", "v (vértices), e fijo")):
        datos = {}
        for cola in colas:
            pts = serie_datos(agr, serie, cola, xkey)
            if not pts or cola not in COTAS_TOTAL:
                continue
            f, lab, texto = COTAS_TOTAL[cola]
            xs = [d[xkey] for _, d in pts]
            ys = [d["tiempo_ms"] for _, d in pts]
            sds = [d["tiempo_ms_sd"] for _, d in pts]
            fs = [f(d["v"], d["e"]) for _, d in pts]
            c = ajustar(ys, fs)
            datos[cola] = (xs, ys, sds, [c * x for x in fs], lab, c)
            constantes.append((serie, "tiempo_total_ms", cola, texto, c))
        if not datos:
            continue
        ylim = 1.08 * max(max(max(y + s for y, s in zip(d[1], d[2])), max(d[3])) for d in datos.values())
        for cola, (xs, ys, sds, cy, lab, c) in datos.items():
            graficar(salida, f"total_{cola}_serie{serie}.png", xs, ys, sds, cy, lab, c, xlabel,
                     "tiempo total de Prim [ms]", f"Serie {serie} — Prim con cola {nombre_cola(cola)}",
                     ylim, True)
    return constantes


def graficos_amortizado(agr, colas, salida):
    """Genera en salida los gráficos de tiempo y operaciones de las series C y D.

    Recibe los datos agrupados y las colas disponibles; devuelve las constantes ajustadas.
    """
    constantes = []
    for serie in ("C", "D"):
        for medida in ("tiempo", "ops"):
            datos = {}
            for cola in colas:
                pts = serie_datos(agr, serie, cola, "dk_llamadas")
                if not pts or cola not in COTAS_DK:
                    continue
                ykey = "dk_tiempo_ms" if medida == "tiempo" else OPS_GRAFICO.get(cola, "dk_ops")
                f, lab, texto = COTAS_DK[cola]
                xs = [d["dk_llamadas"] for _, d in pts]
                ys = [d[ykey] for _, d in pts]
                sds = [d[ykey + "_sd"] for _, d in pts]
                fs = [f(d["v"], d["dk_llamadas"]) for _, d in pts]
                c = ajustar(ys, fs)
                if medida == "tiempo":
                    ylabel, nombre_medida = "tiempo acumulado de decreaseKey [ms]", "tiempo_decreaseKey_ms"
                else:
                    ylabel = YLABEL_OPS.get(cola, "operaciones")
                    nombre_medida = ylabel.replace(" ", "_")
                datos[cola] = (xs, ys, sds, [c * x for x in fs], lab, c, ylabel)
                constantes.append((serie, nombre_medida, cola, texto, c))
            if not datos:
                continue
            ylim = 1.08 * max(max(max(y + s for y, s in zip(d[1], d[2])), max(d[3])) for d in datos.values())
            if ylim == 0:
                ylim = 1
            for cola, (xs, ys, sds, cy, lab, c, ylabel) in datos.items():
                graficar(salida, f"dk_{medida}_{cola}_serie{serie}.png", xs, ys, sds, cy, lab, c,
                         "llamadas a decreaseKey", ylabel,
                         f"Serie {serie} — decreaseKey, cola {nombre_cola(cola)}", ylim, True)
    return constantes


def escribir_constantes(constantes, salida):
    """Guarda las constantes ajustadas en salida/constantes.csv y las imprime como tabla."""
    if not constantes:
        return
    with open(os.path.join(salida, "constantes.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["serie", "medida", "cola", "cota", "c"])
        for serie, medida, cola, cota, c in constantes:
            w.writerow([serie, medida, cola, cota, f"{c:.6g}"])
    print("\n| serie | medida | cola | cota | c |\n|---|---|---|---|---|")
    for serie, medida, cola, cota, c in constantes:
        print(f"| {serie} | {medida} | {cola} | {cota} | {c:.6g} |")


def tabla(agr, salida):
    """Resume los tiempos de las series A y B de agr en tablas CSV, TeX y stdout."""
    filas = sorted((k, d) for k, d in agr.items() if k[0] in ("A", "B"))
    if not filas:
        return
    with open(os.path.join(salida, "tabla_tiempos.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["serie", "i", "j", "cola", "reps", "tiempo_ms_prom", "tiempo_ms_desv"])
        for (s, i, j, cola), d in filas:
            w.writerow([s, i, j, cola, d["n"], f"{d['tiempo_ms']:.3f}", f"{d['tiempo_ms_sd']:.3f}"])
    with open(os.path.join(salida, "tabla_tiempos.tex"), "w") as f:
        f.write("\\begin{tabular}{cccrr}\n\\hline\nSerie & $i$ & $j$ & Cola & Tiempo [ms] \\\\\n\\hline\n")
        for (s, i, j, cola), d in filas:
            f.write(f"{s} & {i} & {j} & {cola} & ${d['tiempo_ms']:.1f} \\pm {d['tiempo_ms_sd']:.1f}$ \\\\\n")
        f.write("\\hline\n\\end{tabular}\n")
    print("\n| serie | i | j | cola | reps | tiempo [ms] |\n|---|---|---|---|---|---|")
    for (s, i, j, cola), d in filas:
        print(f"| {s} | {i} | {j} | {cola} | {d['n']} | {d['tiempo_ms']:.2f} ± {d['tiempo_ms_sd']:.2f} |")


def anexo_repeticiones(filas, salida):
    """Genera el anexo de las series A y B con cada repetición y su promedio.

    Recibe las filas medidas y guarda las tablas en salida como CSV y TeX; también las imprime.
    """
    g = defaultdict(dict)
    for r in filas:
        if r["serie"] in ("A", "B"):
            g[(r["serie"], r["i"], r["j"], r["cola"])][r["rep"]] = r["tiempo_ms"]
    if not g:
        return
    reps = list(range(max(rep for ts in g.values() for rep in ts) + 1))
    claves = sorted(g)

    def prom(ts):
        return statistics.mean(ts.values())

    with open(os.path.join(salida, "tabla_tiempos_reps.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["serie", "i", "j", "cola"] + [f"rep{r}_ms" for r in reps] + ["promedio_ms"])
        for k in claves:
            ts = g[k]
            w.writerow(list(k) + [f"{ts[r]:.3f}" if r in ts else "" for r in reps] + [f"{prom(ts):.3f}"])
    with open(os.path.join(salida, "tabla_tiempos_reps.tex"), "w") as f:
        f.write("\\begin{tabular}{cccl" + "r" * len(reps) + "r}\n\\hline\n")
        f.write("Serie & $i$ & $j$ & Cola & " + " & ".join(f"Rep {r}" for r in reps) + " & Promedio \\\\\n\\hline\n")
        for k in claves:
            ts = g[k]
            celdas = [f"{ts[r]:.1f}" if r in ts else "" for r in reps]
            f.write(" & ".join(str(x) for x in k) + " & " + " & ".join(celdas) + f" & {prom(ts):.1f} \\\\\n")
        f.write("\\hline\n\\end{tabular}\n")
    print("\nAnexo — tiempo total de cada repetición [ms]")
    print("| serie | i | j | cola | " + " | ".join(f"rep {r}" for r in reps) + " | promedio |")
    print("|---|---|---|---|" + "---|" * len(reps) + "---|")
    for k in claves:
        ts = g[k]
        celdas = [f"{ts[r]:.2f}" if r in ts else "" for r in reps]
        print("| " + " | ".join(str(x) for x in k) + " | " + " | ".join(celdas) + f" | {prom(ts):.2f} |")


def costo_reloj_ns(entrada):
    """Lee calibracion.txt de entrada y devuelve el costo del reloj en ns, o None si falta."""
    try:
        with open(os.path.join(entrada, "calibracion.txt")) as f:
            for linea in f:
                if linea.strip().startswith("Costo por llamada:"):
                    return float(linea.split(":", 1)[1].split()[0])
    except (OSError, ValueError, IndexError):
        pass
    return None


def tabla_amortizado(agr, salida, entrada):
    """Resume las mediciones de decreaseKey de las series C y D.

    Recibe los datos agrupados y las carpetas de datos y salida; genera tablas CSV y TeX,
    además de imprimirlas en stdout.
    """
    filas = sorted((k, d) for k, d in agr.items() if k[0] in ("C", "D"))
    if not filas:
        return
    medidas = (("dk_llamadas", "{:.1f}"), ("dk_tiempo_ms", "{:.3f}"), ("dk_ops", "{:.1f}"),
               ("dk_ops_cascada", "{:.1f}"))

    def por_llamada(d):
        """Calcula operaciones y tiempo por llamada a partir de los promedios de d."""
        k = d["dk_llamadas"]
        if not k:
            return ["", "", ""]
        return [f"{d['dk_ops'] / k:.4f}", f"{d['dk_ops_cascada'] / k:.4f}", f"{d['dk_tiempo_ms'] * 1e6 / k:.1f}"]

    with open(os.path.join(salida, "tabla_amortizado.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["serie", "i", "j", "cola", "reps"]
                   + [c for m, _ in medidas for c in (m + "_prom", m + "_desv")]
                   + ["dk_ops_por_llamada", "dk_ops_cascada_por_llamada", "ns_por_llamada"])
        for (s, i, j, cola), d in filas:
            w.writerow([s, i, j, cola, d["n"]]
                       + [fmt.format(x) for m, fmt in medidas for x in (d[m], d[m + "_sd"])]
                       + por_llamada(d))
    reloj = costo_reloj_ns(entrada)
    nota = (f"Incluye $\\sim${reloj:.0f}~ns del reloj (ver \\texttt{{calibracion.txt}})." if reloj is not None
            else "Incluye el costo del reloj (ver \\texttt{calibracion.txt}).")
    columnas = 4 + len(medidas) + 3
    with open(os.path.join(salida, "tabla_amortizado.tex"), "w") as f:
        f.write("% Requiere \\usepackage{graphicx} (\\resizebox).\n"
                "\\resizebox{\\textwidth}{!}{%\n"
                "\\begin{tabular}{cccl" + "r" * (columnas - 4) + "}\n\\hline\n"
                "Serie & $i$ & $j$ & Cola & Llamadas & Tiempo DK [ms] & Operaciones & Cortes en cascada"
                " & Ops/llamada & Cortes casc./llamada & ns/llamada$^{\\dagger}$ \\\\\n"
                "\\hline\n")
        for (s, i, j, cola), d in filas:
            celdas = [f"${fmt.format(d[m])} \\pm {fmt.format(d[m + '_sd'])}$" for m, fmt in medidas]
            f.write(f"{s} & {i} & {j} & {cola} & " + " & ".join(celdas + por_llamada(d)) + " \\\\\n")
        f.write("\\hline\n"
                f"\\multicolumn{{{columnas}}}{{l}}{{\\footnotesize $^{{\\dagger}}$ {nota}}} \\\\\n"
                "\\end{tabular}%\n}\n")
    print("\n| serie | i | j | cola | reps | llamadas | tiempo DK [ms] | dk_ops | dk_ops_cascada"
          " | dk_ops/llamada | cascada/llamada | ns/llamada |")
    print("|---|---|---|---|---|---|---|---|---|---|---|---|")
    for (s, i, j, cola), d in filas:
        celdas = [f"{fmt.format(d[m])} ± {fmt.format(d[m + '_sd'])}" for m, fmt in medidas]
        print(f"| {s} | {i} | {j} | {cola} | {d['n']} | " + " | ".join(celdas + por_llamada(d)) + " |")


def main():
    """Lee los argumentos y coordina la generación de gráficos, tablas y constantes."""
    ap = argparse.ArgumentParser()
    ap.add_argument("--entrada", default="resultados")
    ap.add_argument("--salida", default="figuras")
    ap.add_argument("--reducidos", action="store_true", help="incluir archivos *_red*.csv")
    a = ap.parse_args()

    filas = leer(a.entrada, a.reducidos)
    if not filas:
        print(f"No hay datos en {a.entrada}/tiempos_*.csv")
        return
    os.makedirs(a.salida, exist_ok=True)
    colas = sorted({r["cola"] for r in filas})
    agr = agrupar(filas)
    print("Gráficos:")
    constantes = graficos_total(agr, colas, a.salida) + graficos_amortizado(agr, colas, a.salida)
    tabla(agr, a.salida)
    anexo_repeticiones(filas, a.salida)
    tabla_amortizado(agr, a.salida, a.entrada)
    escribir_constantes(constantes, a.salida)


if __name__ == "__main__":
    main()
