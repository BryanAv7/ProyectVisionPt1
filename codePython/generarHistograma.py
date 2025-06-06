import os
import glob
import pandas as pd
import matplotlib.pyplot as plt

def obtener_ultimo_archivo(path_pattern):
    archivos = glob.glob(path_pattern)
    if not archivos:
        raise FileNotFoundError(f"No se encontraron archivos en: {path_pattern}")
    # Ordenar archivos por fecha de modificación (de más antiguo a más reciente)
    archivos_ordenados = sorted(archivos, key=os.path.getmtime)
    # Tomar el último archivo modificado (más reciente)
    return archivos_ordenados[-1]

# Obtener último archivo creado/modificado para cada carpeta
path_mascara = obtener_ultimo_archivo('/home/bryan/proyectoInter/Datoscsv/cortecsv/*.csv')
path_imagen = obtener_ultimo_archivo('/home/bryan/proyectoInter/Datoscsv/originalcsv/*.csv')

# Carpeta donde se guardará la imagen JPG
carpeta_resultados = '/home/bryan/proyectoInter/codePython/ResultadosHistograma'
os.makedirs(carpeta_resultados, exist_ok=True)

# Leer los archivos CSV
mascara = pd.read_csv(path_mascara, header=None).values
imagen = pd.read_csv(path_imagen, header=None).values

# Aplicar máscara
region_segmentada = imagen[mascara > 0]

print(f"\n--- Estadísticas para el archivo {path_imagen} ---")

if region_segmentada.size > 0:
    print(f"Media: {region_segmentada.mean():.3f}")
    print(f"Desviación estándar: {region_segmentada.std():.4f}")
    print(f"Mínimo: {region_segmentada.min()}")
    print(f"Máximo: {region_segmentada.max()}")
    print(f"Píxeles segmentados: {region_segmentada.size}")

    # Crear histograma
    plt.figure(figsize=(6,4))
    plt.hist(region_segmentada, bins=30, color='lightcoral', edgecolor='black', alpha=0.7)
    plt.title(f'Histograma - {os.path.basename(path_imagen)}')
    plt.xlabel('Intensidad')
    plt.ylabel('Frecuencia')
    plt.grid(True)
    plt.tight_layout()

    # Guardar la imagen como JPG
    nombre_archivo = f"Histograma_{os.path.basename(path_imagen).replace('.csv', '')}.jpg"
    ruta_guardado = os.path.join(carpeta_resultados, nombre_archivo)
    plt.savefig(ruta_guardado, dpi=300)
    print(f"✅ Histograma guardado en: {ruta_guardado}")
    plt.show()
else:
    print("⚠️ No hay píxeles segmentados (máscara vacía).")

