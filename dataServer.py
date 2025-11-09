import socket
import json
import threading
from graphs import (
    storeData,
    resetMeasurements,
    periodicGraphsUpdate,
)

def waitClientConnection(server_socket):
    connection, client_address = server_socket.accept()
    print('Conexión desde', client_address)
    return connection, client_address


def handleClient(connection, client_address):
    """Maneja la conexión con un cliente."""
    threading.Thread(target=periodicGraphsUpdate, daemon=True).start()
    resetMeasurements()

    while True:
        try:
            data = connection.recv(1024)
            if not data:
                print(f"Cliente {client_address} se desconectó.")
                break

            receivedJSON = json.loads(data.decode())
            storeData(receivedJSON)

        except (ConnectionResetError, BrokenPipeError):
            print(f"Conexión perdida abruptamente con {client_address}")
            break
        except socket.timeout:
            print(f"Timeout con {client_address}")
            break
        except json.JSONDecodeError as e:
            print(f"Error decodificando JSON: {e}")
        except Exception as e:
            print(f"Error inesperado: {e}")
            break

    try:
        connection.close()
        print(f"Conexión cerrada con {client_address}")
    except:
        pass


def serveForever():
    """Inicia el servidor TCP y espera conexiones entrantes."""
    print("Iniciando servidor...")
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('0.0.0.0', 42069))
    server_socket.listen(1)
    print("Servidor escuchando en el puerto 42069...")

    while True:
        connection, client_address = waitClientConnection(server_socket)
        connection.settimeout(30)
        client_thread = threading.Thread(
            target=handleClient, daemon=True, args=(connection, client_address)
        )
        client_thread.start()


if __name__ == "__main__":
    serveForever()
