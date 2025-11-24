#!/bin/bash

VENV_DIR="Server/venv"
REQ_FILE="Server/requirements.txt"

echo "=== Instalando soporte para entornos virtuales ==="
sudo apt update
sudo apt install -y python3-venv

echo "=== Creando entorno virtual ==="
python3 -m venv "$VENV_DIR"

echo "=== Activando entorno virtual ==="
source "$VENV_DIR/bin/activate"

echo "=== Instalando dependencias de $REQ_FILE ==="
pip install --upgrade pip
pip install -r "$REQ_FILE"


