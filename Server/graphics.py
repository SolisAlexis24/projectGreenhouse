# graph_utils.py
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import time
from datetime import datetime, timezone
import os

matplotlib.use('AGG')  # Usando el rasterizado a .png

# Parámetros globales
GRAPH_PERIOD_S = 80
GRAPH_Y_MARGIN = 5
LM135_GRAPH = "LT"
AM2302T_GRAPH = "AT"
AM2302H_GRAPH = "AH"
MAIN_DIRECTORY_PATH_DIR = "Status"
LM135_DATA_PATH_DIR = MAIN_DIRECTORY_PATH_DIR + "/LM135/"
AM2302T_DATA_PATH_DIR = MAIN_DIRECTORY_PATH_DIR + "/AM2302T/"
AM2302H_DATA_PATH_DIR = MAIN_DIRECTORY_PATH_DIR + "/AM2302H/"
LOG_FILE_PATH = MAIN_DIRECTORY_PATH_DIR + "/actions.log"

# Datos globales
LM135Data = []
AM2302TData = []
AM2302HData = []
timeData = []


def graphMeasurements(data, graph_type):
    global timeData
    if graph_type == LM135_GRAPH:
        mylabel = "Temperatura LM135"
        units = "[°C]"
        path = LM135_DATA_PATH_DIR
    elif graph_type == AM2302T_GRAPH:
        mylabel = "Temperatura AM2302"
        units = "[°C]"
        path = AM2302T_DATA_PATH_DIR
    elif graph_type == AM2302H_GRAPH:
        mylabel = "Humedad"
        units = "[%]"
        path = AM2302H_DATA_PATH_DIR
    else:
        mylabel = "?"
        units = "?"

    if len(data) == 0:
        print(f"[Servidor de datos]: Datos vacíos para {mylabel}, no se generará gráfica.")
        return

    if len(data) != len(timeData):
        print(f"[Servidor de datos]: Datos de {mylabel} no coinciden con datos de tiempo, no se generará gráfica.")
        return

    fig, ax = plt.subplots()
    ax.plot(timeData, data, label=mylabel)

    plt.ylim(min(data) - GRAPH_Y_MARGIN, max(data) + GRAPH_Y_MARGIN)
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%M:%S'))
    ax.xaxis.set_major_locator(mdates.AutoDateLocator())

    plt.title(f"Registro de: {mylabel} el {timeData[0].day}/{timeData[0].month}/{timeData[0].year} a las {timeData[0].hour} h")
    plt.xlabel("Tiempo [min:seg]")
    plt.ylabel(f"{mylabel} {units}")
    plt.legend()
    plt.tight_layout()

    filename = f"{path}{mylabel}@{timeData[0].day}-{timeData[0].month}-{timeData[0].year}_{timeData[0].hour}:{timeData[0].minute}.png"
    plt.savefig(filename)
    plt.clf()
    plt.close(fig)
    print(f"[Servidor de datos]: Nueva gráfica disponible: {mylabel}")


def storeData(receivedJSON):
    """Guarda los datos recibidos del JSON en las listas globales."""
    global LM135Data, AM2302TData, AM2302HData, timeData
    sensors_data = receivedJSON['sensors']

    for sensor in sensors_data:
        name = sensor['sensor']
        if name == 'LM135':
            LM135Data.append(sensor['temperature'])
        elif name == 'AM2302':
            AM2302TData.append(sensor['temperature'])
            AM2302HData.append(sensor['humidity'])

    timeData.append(datetime.fromtimestamp(time.time()).astimezone())


def resetMeasurements():
    """Reinicia todas las mediciones."""
    global LM135Data, AM2302TData, AM2302HData, timeData
    LM135Data.clear()
    AM2302TData.clear()
    AM2302HData.clear()
    timeData.clear()


def periodicGraphsUpdate():
    """Hilo encargado de generar las gráficas de forma periódica."""
    global LM135Data, AM2302TData, AM2302HData, timeData
    start = time.time()
    while True:
        if time.time() - start >= GRAPH_PERIOD_S:
            start = time.time()
            graphMeasurements(LM135Data, LM135_GRAPH)
            graphMeasurements(AM2302TData, AM2302T_GRAPH)
            graphMeasurements(AM2302HData, AM2302H_GRAPH)
            resetMeasurements()
        time.sleep(1)


def createDataDirectories():
    if not os.path.exists(MAIN_DIRECTORY_PATH_DIR):
        try:
            os.mkdir(MAIN_DIRECTORY_PATH_DIR)
            print(f"[Servidor de datos]: Directorio '{MAIN_DIRECTORY_PATH_DIR}' creado.")
        except PermissionError:
            print("[Servidor de datos]: Error de permisos al intentar crear archivos")

    if not os.path.exists(LM135_DATA_PATH_DIR):
        try:
            os.mkdir(LM135_DATA_PATH_DIR)
            print(f"[Servidor de datos]: Directorio '{LM135_DATA_PATH_DIR}' creado.")
        except PermissionError:
            print("[Servidor de datos]: Error de permisos al intentar crear archivos")

    if not os.path.exists(AM2302T_DATA_PATH_DIR):
        try:
            os.mkdir(AM2302T_DATA_PATH_DIR)
            print(f"[Servidor de datos]: Directorio '{AM2302T_DATA_PATH_DIR}' creado.")
        except PermissionError:
            print("[Servidor de datos]: Error de permisos al intentar crear archivos")

    if not os.path.exists(AM2302H_DATA_PATH_DIR):
        try:
            os.mkdir(AM2302H_DATA_PATH_DIR)
            print(f"[Servidor de datos]: Directorio '{AM2302H_DATA_PATH_DIR}' creado.")
        except PermissionError:
            print("[Servidor de datos]: Error de permisos al intentar crear archivos")

    if not os.path.exists(LOG_FILE_PATH):
        try:
            with open(LOG_FILE_PATH, "w") as f:
                pass
        except PermissionError:
            print("[Servidor de datos]: Error de permisos al intentar crear archivos")


def writeToLOG(LOGentry):
    serverTime = datetime.fromtimestamp(time.time()).astimezone()
    with open(LOG_FILE_PATH, "a") as f:
        f.write(LOGentry)
        f.write(f' @ {serverTime.day}-{serverTime.month}-{serverTime.year} {serverTime.hour}:{serverTime.minute}\n')