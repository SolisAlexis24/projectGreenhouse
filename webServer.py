#! /usr/bin/env python3
# ## ###############################################
#
# webserver.py
# Starts a custom webserver and handles all requests
#
# Autor: Mauricio Matamoros
# License: MIT
#
# ## ###############################################
import os
import sys
import json
import magic
from http.server import BaseHTTPRequestHandler, HTTPServer

# Nombre o dirección IP del sistema anfitrión del servidor web
# address = "localhost"
address = "192.168.1.172"
# Puerto en el cual el servidor estará atendiendo solicitudes HTTP
# El default de un servidor web en produción debe ser 80
port = 8080

class WebServer(BaseHTTPRequestHandler):
    """Sirve cualquier archivo encontrado en el servidor"""
    def _serve_file(self, rel_path):
        # Normaliza ruta y corrige espacios codificados
        rel_path = rel_path.replace("%20", " ")

        # Si intenta acceder a /Status/, redirigir a la ruta real
        if rel_path.startswith("Status/"):
            abs_path = os.path.join("/home/pi", rel_path)
        else:
            abs_path = rel_path

        if not os.path.isfile(abs_path):
            self.send_error(404, f"File not found: {abs_path}")
            return

        self.send_response(200)
        mime = magic.Magic(mime=True)
        self.send_header("Content-type", mime.from_file(abs_path))
        self.end_headers()
        with open(abs_path, 'rb') as file:
            self.wfile.write(file.read())


    """Sirve el archivo de interfaz de usuario"""
    def _serve_ui_file(self):
        if not os.path.isfile("index.html"):
            err = "index.html not found."
            self.wfile.write(bytes(err, "utf-8"))
            print(err)
            return
        try:
            with open("index.html", "r") as f:
                content = "\n".join(f.readlines())
        except:
            content = "Error reading index.html"
        self.wfile.write(bytes(content, "utf-8"))

    def _parse_post(self, json_obj):
        if not 'action' in json_obj:
            return

    def _serve_json(self, data):
        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.wfile.write(bytes(json.dumps(data), "utf-8"))


    """do_GET controla todas las solicitudes recibidas vía GET, es
    decir, páginas. Por seguridad, no se analizan variables que lleguen
    por esta vía"""
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
            base_dir = f"/home/pi/Status/{folder}"
            if not os.path.isdir(base_dir):
                self.send_error(404)
                return
            images = [
                f"/home/pi/Status/{folder}/{f}"
                for f in os.listdir(base_dir)
                if f.lower().endswith(('.png', '.jpg', '.jpeg', '.gif'))
            ]
            # Devolver solo nombres relativos
            web_paths = [f"/Status/{folder}/{os.path.basename(f)}" for f in images]
            self._serve_json(web_paths)
            return

        # API para ver el log
        if self.path == '/api/log':
            log_path = "/home/pi/Status/actions.log"
            if not os.path.isfile(log_path):
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header("Content-type", "text/plain; charset=utf-8")
            self.end_headers()
            with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
                self.wfile.write(f.read().encode('utf-8'))
            return

        # Si no coincide con API, servir archivos estáticos
        self._serve_file(self.path[1:])

    """do_POST controla todas las solicitudes recibidas vía POST, es
    decir, envíos de formulario. Aquí se gestionan los comandos para
    la Raspberry Pi"""
    def do_POST(self):
        # Primero se obtiene la longitud de la cadena de datos recibida
        content_length = int(self.headers.get('Content-Length'))
        if content_length < 1:
            return
        # Después se lee toda la cadena de datos
        post_data = self.rfile.read(content_length)
        # Finalmente, se decodifica el objeto JSON y se procesan los datos.
        # Se descartan cadenas de datos mal formados
        try:
            jobj = json.loads(post_data.decode("utf-8"))
            self._parse_post(jobj)
        except:
            print(sys.exc_info())
            print("Datos POST no recnocidos")

def main():
    # Inicializa una nueva instancia de HTTPServer con el
    # HTTPRequestHandler definido en este archivo
    webServer = HTTPServer((address, port), WebServer)
    print("Servidor iniciado")
    print ("\tAtendiendo solicitudes en http://{}:{}".format(
        address, port))
    try:
        # Mantiene al servidor web ejecutándose en segundo plano
        webServer.serve_forever()
    except KeyboardInterrupt:
        # Maneja la interrupción de cierre CTRL+C
        pass
    except:
        print(sys.exc_info())
    # Detiene el servidor web cerrando todas las conexiones
    webServer.server_close()
    # Reporta parada del servidor web en consola
    print("Server stopped.")


# Punto de anclaje de la función main
if __name__ == "__main__":
    main()