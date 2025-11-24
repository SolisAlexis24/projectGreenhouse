# ## ###############################################
#
# webServer.py
# Servidor web simple
#
# Autor: Mauricio Matamoros / adaptado por Alexis Solis
# License: MIT
#
# ## ###############################################
import os
import sys
import json
import magic
import subprocess
from http.server import BaseHTTPRequestHandler, HTTPServer
from dataServer import setFanPower, setDesiredTemperature, toggleIrrigation, addNewIrrigationAlarm

# Obtener IP del host (Linux)
address = subprocess.run(
        ["hostname", "-I"],
        capture_output=True,
        text=True
    )
address = address.stdout.strip().split()[0]
port = 8080

# Carpeta base (sandbox)
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

class WebServer(BaseHTTPRequestHandler):

    def _serve_file(self, rel_path):
        """Sirve archivos desde BASE_DIR de forma segura"""
        rel_path = rel_path.replace("%20", " ")  # corrige espacios
        safe_path = os.path.normpath(os.path.join(BASE_DIR, rel_path))

        # Bloquea acceso fuera del sandbox
        if not safe_path.startswith(BASE_DIR):
            self.send_error(403, "Access denied")
            return

        if not os.path.isfile(safe_path):
            self.send_error(404, f"File not found: {safe_path}")
            return

        self.send_response(200)
        mime = magic.Magic(mime=True)
        self.send_header("Content-type", mime.from_file(safe_path))
        self.end_headers()
        with open(safe_path, 'rb') as file:
            self.wfile.write(file.read())

    def _serve_ui_file(self):
        """Sirve index.html"""
        index_path = os.path.join(BASE_DIR, "index.html")
        if not os.path.isfile(index_path):
            self.send_error(404, "index.html not found")
            return
        with open(index_path, "r") as f:
            content = f.read()
        self.wfile.write(bytes(content, "utf-8"))

    def _serve_json(self, data):
        """Sirve una respuesta JSON"""
        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.wfile.write(bytes(json.dumps(data), "utf-8"))

    def _parse_post(self, json_obj):
        if 'action' not in json_obj:
            return

        switcher = {
            'update_fan': setFanPower,
            'toggle_irrigation': toggleIrrigation,
            'update_temperature': setDesiredTemperature,
            'add_irrigation_alarm': addNewIrrigationAlarm
        }

        func = switcher.get(json_obj['action'], None)

        if func:
            action = json_obj['action']

            # --- Control del ventilador ---
            if action == 'update_fan':
                power = float(json_obj.get('fanPower', 0))
                print(f"\tCall {func}(power={power})")
                func(power)

            # --- Toggle del sistema de irrigado ---
            elif action == 'toggle_irrigation':
                print(f"\tCall {func}()")
                func()

            # --- Actualización de la temperatura deseada ---
            elif action == 'update_temperature':
                temp = float(json_obj.get('targetTemp', 25))
                print(f"\tCall {func}(temp={temp})")
                func(temp)

            # --- Añadir alarma de irrigación ---
            elif action == 'add_irrigation_alarm':
                hour = float(json_obj.get('hour', 0))
                minute = float(json_obj.get('minute', 0))
                print(f"\tCall {func}(hour={hour},minute={minute})")
                func(hour, minute)

    # -------------------- GET --------------------
    def do_GET(self):
        if self.path == '/':
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self._serve_ui_file()
            return

        # API para listar imágenes
        if self.path.startswith('/api/images/'):
            folder = self.path.split('/')[-1]
            base_dir = os.path.join(BASE_DIR, "Status", folder)
            if not os.path.isdir(base_dir):
                self.send_error(404, f"Folder not found: {folder}")
                return

            images = [
                f"/Status/{folder}/{f}"
                for f in os.listdir(base_dir)
                if f.lower().endswith(('.png', '.jpg', '.jpeg', '.gif'))
            ]
            self._serve_json(images)
            return

        # API para leer log
        if self.path == '/api/log':
            log_path = os.path.join(BASE_DIR, "Status", "actions.log")
            if not os.path.isfile(log_path):
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header("Content-type", "text/plain; charset=utf-8")
            self.end_headers()
            with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
                self.wfile.write(f.read().encode('utf-8'))
            return

        # Servir imágenes u otros archivos dentro del sandbox
        rel_path = self.path[1:]  # quitar '/'
        self._serve_file(rel_path)

    # -------------------- POST --------------------
    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        if content_length < 1:
            return
        post_data = self.rfile.read(content_length)
        try:
            jobj = json.loads(post_data.decode("utf-8"))
            self._parse_post(jobj)
        except Exception:
            print(sys.exc_info())
            print("Datos POST no reconocidos")


def startWebServer():
    webServer = HTTPServer((address, port), WebServer)
    print("Servidor iniciado")
    print(f"\tAtendiendo solicitudes en http://{address}:{port}")
    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass
    except Exception:
        print(sys.exc_info())
    webServer.server_close()
    print("Server stopped.")
