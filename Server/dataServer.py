# ## ###############################################
#
# dataServer.py
# Servidor de conexion con el ESP32
#
# Autor: Alexis Solis
# License: MIT
#
# ## ###############################################
import socket
import json
import threading
import time
from datetime import datetime, timezone
from graphics import (
    storeData,
    resetMeasurements,
    periodicGraphsUpdate,
    createDataDirectories,
    writeToLOG
)

client_connection = None
client_address = None
IRRIGATION_TIME_S = 20


def waitClientConnection(server_socket):
    global client_connection, client_address
    client_connection, client_address = server_socket.accept()
    print('[Servidor de datos]: Conexión desde', client_address)


def receiveMeasurementsFromClient():
    """Maneja la recepcion de datos con el cliente."""
    global client_connection, client_address
    if not client_connection or not client_address:
        print("[Servidor de datos]: No hay cliente, imposible recabar datos")
        return

    threading.Thread(target=periodicGraphsUpdate, daemon=True).start()
    resetMeasurements()

    while True:
        try:
            data = client_connection.recv(1024)
            if not data:
                print(f"[Servidor de datos]: Cliente {client_address} se desconectó.")
                break

            receivedJSON = json.loads(data.decode())
            storeData(receivedJSON)

        except (ConnectionResetError, BrokenPipeError):
            print(f"[Servidor de datos]: Conexión perdida abruptamente con {client_address}")
            break
        except socket.timeout:
            print(f"[Servidor de datos]: Timeout con {client_address}")
            break
        except json.JSONDecodeError as e:
            print(f"[Servidor de datos]: Error decodificando JSON: {e}")
        except Exception as e:
            print(f"[Servidor de datos]: Error inesperado: {e}")
            break

    try:
        client_connection.close()
        resetMeasurements()
        print(f"[Servidor de datos]: Conexión cerrada con {client_address}")
    except:
        pass


def handleClientConnection(server_socket):
    """Maneja la conexión del cliente y reconexiones automáticas."""
    global client_connection, client_address
    while True:
        try:
            waitClientConnection(server_socket)
            client_connection.settimeout(10)

            client_thread = threading.Thread(
                target=receiveMeasurementsFromClient,
                daemon=True,
            )
            client_thread.start()

            # Esperar a que el hilo termine (cuando el cliente se desconecte)
            client_thread.join()
            client_connection = None
            client_address = None
            print("[Servidor de datos]: Cliente desconectado. Esperando nueva conexión...")

        except Exception as e:
            print(f"[Servidor de datos]: Error en la conexión con el cliente: {e}")
            time.sleep(1)  # Pequeña pausa antes de reintentar


def setFanPower(power):
    """Configura la potencia del ventilador
    Power se divide pra dejarlo en rango [0, 1]"""
    if sendFunctionToClient("setFanPower", power/100.0):
        writeToLOG(f"Potencia de ventilador modificada al: {power}%")


def setDesiredTemperature(temperature):
    """Configura la temperatura deseada del sistema"""
    if sendFunctionToClient("setDesiredTemperature", temperature):
        writeToLOG(f"Temperatura deseada modificada a: {temperature}°C")


def toggleIrrigation():
    """Envia la funcion para cambiar el estado del iriigador"""
    if sendFunctionToClient("toggleIrrigation", ""):
        writeToLOG(f"Se modifica el estado de la bomba de irrigación")


def addNewIrrigationAlarm(alarmHour, alarmMinute):
    global client_connection, client_address
    if not client_connection:
        print("No hay cliente, imposible agregar una alarma")
        return

    threading.Thread(target=alarmThread,
                     daemon=True,
                     args=(alarmHour, alarmMinute)).start()
    writeToLOG(f"Se ha programado una alarma para las {alarmHour}:{alarmMinute}")


def alarmThread(alarmHour, alarmMinute):
    while True:
        currentTime = datetime.fromtimestamp(time.time()).astimezone()
        if currentTime.hour == alarmHour and currentTime.minute == alarmMinute:
            writeToLOG(f"Se ha lanzado la alarma de las {alarmHour}:{alarmHour}")
            toggleIrrigation()
            startTime = time.time()
            while time.time() < startTime + IRRIGATION_TIME_S:
                time.sleep(1)
            toggleIrrigation()
            break
        time.sleep(1)


def sendFunctionToClient(Function, Argument):
    """Envia un JSON con la estructura funcion:argumento al microcontrolador"""
    global client_connection, client_address
    if not client_connection:
        print(f"[Servidor de datos]: No hay cliente conectado, no se puede enviar {Function}")
        return False

    try:
        funcDict = {"function": Function, "argument": Argument}
        funcJSON = json.dumps(funcDict)
        funcEncod = funcJSON.encode()
        client_connection.sendall(funcEncod)
        return True

    except Exception as e:
        print(f"[Servidor de datos]: Error enviando función {Function}: {e}")


def startDataServer():
    """Inicia el servidor TCP y espera conexiones entrantes."""
    createDataDirectories()
    print("[Servidor de datos]: Iniciando...")
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('0.0.0.0', 42069))
    server_socket.listen(1)
    print("[Servidor de datos]: Escuchando en el puerto 42069...")

    connection_handler = threading.Thread(
        target=handleClientConnection,
        daemon=True,
        args=(server_socket,)
    )
    connection_handler.start()


if __name__ == "__main__":
    startDataServer()