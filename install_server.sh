#!/bin/bash

VENV_DIR="Server/venv"
REQ_FILE="Server/requirements.txt"
PYTHON_BIN="python3.11"

echo "=== Actualizando paquetes ==="
sudo apt update

echo "=== Instalando $PYTHON_BIN ==="
sudo apt install -y python3.11 python3.11-venv

echo "=== Verificando instalación==="
if ! command -v $PYTHON_BIN &> /dev/null
then
    echo "Error: python3.11 no está instalado."
    exit 1
fi

echo "=== Creando entorno virtual ==="
$PYTHON_BIN -m venv "$VENV_DIR"

echo "=== Activando entorno virtual ==="
source "$VENV_DIR/bin/activate"

echo "=== Actualizando pip e instalando dependencias ==="
pip install --upgrade pip
pip install -r "$REQ_FILE"

echo
echo "=== Entorno virtual creado ==="
echo "Para activarlo manualmente luego:"
echo "  source $VENV_DIR/bin/activate"
