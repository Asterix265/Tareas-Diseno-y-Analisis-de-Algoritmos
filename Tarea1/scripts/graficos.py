#!/usr/bin/env python3
"""
graficos.py — Genera los 12 gráficos y las tablas a partir de los CSV de ./prim.

Uso (desde Tarea1/):
    python3 scripts/graficos.py                      # lee resultados/tiempos_*.csv (sin los _red)
    python3 scripts/graficos.py --reducidos          # incluye corridas con --reducir (pruebas)
    python3 scripts/graficos.py --entrada dir --salida figuras

Requiere: matplotlib (pip install matplotlib).

Gráficos:
  Series A y B (costo total), 4 gráficos: tiempo total vs e (A) o v (B), por cola.
  Series C y D (costo amortizado), 8 gráficos: tiempo acumulado de decreaseKey
  y conteo de operaciones vs cantidad de llamadas, por cola. El conteo usa
  intercambios en la binomial (dk_ops) y cortes en cascada en Fibonacci
  (dk_ops_cascada), como dice la sección 6.3.2 b) del enunciado.
Cada gráfico incluye la cota teórica multiplicada por la constante c que mejor
ajusta por mínimos cuadrados (c = sum(y*f) / sum(f^2)). Gráficos de una misma
serie y medida comparten escala (mismo eje y) para comparar ambas colas.

Tablas (en la carpeta de salida y en markdown por stdout):
  tabla_tiempos.{csv,tex}         series A y B: promedio ± desviación estándar del tiempo total.
  tabla_tiempos_reps.{csv,tex}    anexo, series A y B: el tiempo de cada repetición y el promedio.
  tabla_amortizado.{csv,tex}      series C y D: promedio ± desviación estándar de dk_llamadas,
                                  tiempo de decreaseKey, dk_ops y dk_ops_cascada, más
                                  dk_ops, dk_ops_cascada y ns por llamada.
  constantes.csv                  la constante c ajustada de cada uno de los 12 gráficos.
"""
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

# Cotas teóricas: f(v, e) para el costo total y f(v, k) para decreaseKey (k = llamadas).
# Cada una: (función, leyenda LaTeX del gráfico, texto plano para constantes.csv).
COTAS_TOTAL = {
    "binomial": (lambda v, e: e * math.log2(v), r"$c \cdot e \log v$", "e*log2(v)"),
    "fibonacci": (lambda v, e: e + v * math.log2(v), r"$c \cdot (e + v \log v)$", "e + v*log2(v)"),
}
COTAS_DK = {  # costo acumulado de decreaseKey en función de la cantidad de llamadas k
    "binomial": (lambda v, k: k * math.log2(v), r"$c \cdot k \log v$", "k*log2(v)"),
    "fibonacci": (lambda v, k: k, r"$c \cdot k$  (O(1) amortizado)", "k"),
}

# Contador de operaciones que se grafica para cada cola (sección 6.3.2 b)
OPS_GRAFICO = {"binomial": "dk_ops", "fibonacci": "dk_ops_cascada"}
# Rótulo del eje y de los gráficos de operaciones, por cola
YLABEL_OPS = {"binomial": "intercambios", "fibonacci": "cortes en cascada"}
# Nombre de cada cola en los títulos
NOMBRE_COLA = {"binomial": "Binomial", "fibonacci": "Fibonacci"}

ENTEROS = ("i", "j", "v", "e", "rep", "dk_llamadas", "dk_tiempo_ns", "dk_ops", "dk_ops_cascada")
REALES = ("tiempo_ms", "peso_mst")


def leer(entrada, reducidos):
    """Lee todos los tiempos_*.csv de una carpeta.

    Entrada: entrada, carpeta con los CSV; reducidos, si es True incluye los
    archivos *_red*.csv (corridas con --reducir).
    Salida: lista de dicts, una por fila, con i, j, v, e, rep y los contadores
    como int y tiempo_ms, peso_mst como float.
    Termina con un error si la misma fila (serie, i, j, rep, cola) aparece más
    de una vez, porque se contaría dos veces en los promedios.
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

    Entrada: filas, lista de dicts de leer().
    Salida: dict (serie, i, j, cola) -> dict con v, e, n (repeticiones) y, para
    tiempo_ms, dk_llamadas, dk_tiempo_ms, dk_ops y dk_ops_cascada, el promedio
    (clave sin sufijo) y la desviación estándar (sufijo _sd; 0 si hay una sola repetición).
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
    """Constante de la cota por mínimos cuadrados.

    Entrada: ys, valores medidos; fs, valores de la cota sin constante (mismo largo).
    Salida: c que minimiza sum (y - c f)^2, es decir sum(y f) / sum(f^2); 0 si todas las f son 0.
    """
    den = sum(f * f for f in fs)
    return sum(y * f for y, f in zip(ys, fs)) / den if den else 0.0


def estilo(ax, xlabel, ylabel, titulo):
    """Aplica etiquetas, título, grilla y leyenda comunes a un gráfico.

    Entrada: ax, ejes de matplotlib; xlabel, ylabel y titulo, textos.
    Salida: ninguna (modifica ax).
    """
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(titulo, fontsize=11)
    ax.grid(True, color="#dddddd", linewidth=0.6)
    for lado in ("top", "right"):
        ax.spines[lado].set_visible(False)
    ax.legend(frameon=False)


def serie_datos(agr, serie, cola, xkey):
    """Puntos de una serie y una cola, ordenados por x.

    Entrada: agr, salida de agrupar(); serie ("A".."D"); cola; xkey, clave que
    se usa como eje x ("e", "v" o "dk_llamadas").
    Salida: lista de pares ((serie, i, j, cola), datos) ordenada por datos[xkey].
    """
    pts = [(k, d) for k, d in agr.items() if k[0] == serie and k[3] == cola]
    pts.sort(key=lambda kd: kd[1][xkey])
    return pts


def graficar(salida, nombre, xs, ys, sds, cota_ys, cota_label, c, xlabel, ylabel, titulo, ylim, log2x):
    """Dibuja un gráfico de línea (medido ± desviación estándar y cota) y lo guarda en PNG.

    Entrada: salida, carpeta; nombre, archivo PNG; xs, ys y sds, eje x, promedios
    y desviaciones estándar; cota_ys, cota ya multiplicada por c; cota_label,
    leyenda de la cota; c, constante ajustada; xlabel, ylabel y titulo, textos;
    ylim, límite superior del eje y (común a la serie); log2x, si el eje x va en escala log2.
    Salida: ninguna; escribe salida/nombre e imprime su ruta.
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
    """Entrada: nombre interno de una cola. Salida: nombre para títulos ("Binomial", "Fibonacci")."""
    return NOMBRE_COLA.get(cola, cola.capitalize())


def graficos_total(agr, colas, salida):
    """Los 4 gráficos de costo total (sección 6.3.1): series A y B, una figura por cola.

    Entrada: agr, salida de agrupar(); colas, nombres de las colas presentes; salida, carpeta.
    Salida: lista de filas (serie, medida, cola, cota, c) con la constante ajustada de cada
    gráfico; escribe total_<cola>_serie<A|B>.png con la misma escala y por serie.
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
    """Los 8 gráficos de costo amortizado (sección 6.3.2): series C y D, tiempo y operaciones, por cola.

    Entrada: agr, salida de agrupar(); colas, nombres de las colas presentes; salida, carpeta.
    Salida: lista de filas (serie, medida, cola, cota, c) con la constante ajustada de cada
    gráfico; escribe dk_<tiempo|ops>_<cola>_serie<C|D>.png. En "ops" se grafica dk_ops en la
    binomial (eje y "intercambios") y dk_ops_cascada en Fibonacci (eje y "cortes en cascada").
    Misma escala y dentro de cada serie y medida.
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
    """Escribe las constantes c ajustadas de los 12 gráficos.

    Entrada: constantes, lista de filas (serie, medida, cola, cota, c) de graficos_total()
    y graficos_amortizado(); salida, carpeta.
    Salida: ninguna; escribe constantes.csv (c con 6 cifras significativas) y la imprime.
    Las unidades de c son las de la medida (ms u operaciones) divididas por las de la cota.
    """
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
    """Tabla de tiempos de las series A y B: promedio ± desviación estándar por configuración y cola.

    Entrada: agr, salida de agrupar(); salida, carpeta.
    Salida: ninguna; escribe tabla_tiempos.csv y tabla_tiempos.tex e imprime la tabla en markdown.
    """
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
    """Anexo de las series A y B: el tiempo total de cada repetición y el promedio.

    Entrada: filas, lista de dicts de leer(); salida, carpeta.
    Salida: ninguna; escribe tabla_tiempos_reps.csv y tabla_tiempos_reps.tex e
    imprime la tabla en markdown. Una repetición que falte queda en blanco.
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
    """Costo de una llamada a steady_clock::now() medido con ./prim --calibrar.

    Entrada: entrada, carpeta donde puede estar calibracion.txt.
    Salida: el valor de la línea "Costo por llamada: X ns" como float, o None si el
    archivo no existe o no tiene esa línea.
    """
    try:
        with open(os.path.join(entrada, "calibracion.txt")) as f:
            for linea in f:
                if linea.strip().startswith("Costo por llamada:"):
                    return float(linea.split(":", 1)[1].split()[0])
    except (OSError, ValueError, IndexError):
        pass
    return None


def tabla_amortizado(agr, salida, entrada):
    """Tabla de las series C y D: promedio ± desviación estándar de las mediciones de decreaseKey.

    Entrada: agr, salida de agrupar(); salida, carpeta; entrada, carpeta de los CSV
    (de ahí se lee calibracion.txt para la nota del .tex).
    Salida: ninguna; escribe tabla_amortizado.csv y tabla_amortizado.tex e imprime la
    tabla en markdown. Columnas: dk_llamadas, tiempo acumulado de decreaseKey [ms],
    dk_ops (intercambios / todos los cortes) y dk_ops_cascada (solo cortes en cascada),
    más tres columnas por llamada, calculadas con los promedios de la misma fila:
      dk_ops_por_llamada         = dk_ops / dk_llamadas
                                   (binomial: intercambios; Fibonacci: cortes totales);
      dk_ops_cascada_por_llamada = dk_ops_cascada / dk_llamadas
                                   (binomial: 0; Fibonacci: cortes en cascada);
      ns_por_llamada             = tiempo de decreaseKey [ns] / dk_llamadas.
    El .tex va dentro de \\resizebox{\\textwidth}{!}{...} (requiere \\usepackage{graphicx})
    y lleva una nota: ns_por_llamada incluye el costo del reloj (costo_reloj_ns).
    """
    filas = sorted((k, d) for k, d in agr.items() if k[0] in ("C", "D"))
    if not filas:
        return
    medidas = (("dk_llamadas", "{:.1f}"), ("dk_tiempo_ms", "{:.3f}"), ("dk_ops", "{:.1f}"),
               ("dk_ops_cascada", "{:.1f}"))

    def por_llamada(d):
        """Salida: (dk_ops, dk_ops_cascada y ns por llamada) como texto; vacíos si no hubo llamadas."""
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
    """Lee los CSV, dibuja los 12 gráficos y escribe las tablas y las constantes ajustadas.

    Entrada: opciones de línea de comandos --entrada, --salida y --reducidos.
    Salida: ninguna; archivos en la carpeta de salida y tablas en markdown por stdout.
    """
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
