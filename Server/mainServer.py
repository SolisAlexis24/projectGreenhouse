#! /usr/bin/env python3
# ## ###############################################
#
# mainServer.py
# Punto de anclaje para iniciar los servidores
#
# Autor: Alexis Solis
# License: MIT
#
# ## ###############################################

from dataServer import startDataServer
from webServer import startWebServer

if __name__ == '__main__':
    startDataServer()
    startWebServer()