import os
import glob
import pandas as pd
import matplotlib.pyplot as plt

# Buscar el archivo .csv 
path_mascara = glob.glob('/home/bryan/proyectoInter/Datoscsv/cortecsv/*.csv')[0]
path_imagen = glob.glob('/home/bryan/proyectoInter/Datoscsv/originalcsv/*.csv')[0]

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

