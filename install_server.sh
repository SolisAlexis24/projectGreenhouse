#!/bin/bash

VENV_DIR="Server/venv"
REQ_FILE="Server/requirements.txt"

echo "=== Actualizando sistema ==="
sudo apt update

echo "=== Instalando pip ==="
sudo apt install -y python3-pip

if [ "$1" == "--virtual" ]; then
	echo "=== Creando entorno virtual ==="
    sudo apt install -y python3-venv
    python3 -m venv "$VENV_DIR"
    echo "=== Activando entorno virtual ==="
    source "$VENV_DIR/bin/activate"
    echo "=== Instalando dependencias de $REQ_FILE ==="
	pip install -r "$REQ_FILE"
else
	echo "=== Instalando Libopenblas ==="
	sudo apt install -y libopenblas-dev

	echo "=== Instalando matplotlib"
	pip install matplotlib

	echo "=== Instalando python magic"
	sudo apt install python3-magic
fi

chmod +x "Server/mainServer.py"
echo "=== Listo para ejecutar servidor ==="




