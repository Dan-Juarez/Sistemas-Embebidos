import tkinter as tk
import serial
import time

# Ajusta tu puerto: "COM3" en Windows o "/dev/ttyUSB0" en Linux
PUERTO = "COM3"
BAUDIOS = 9600
arduino = None

def conectar():
    global arduino
    try:
        arduino = serial.Serial(PUERTO, BAUDIOS, timeout=1)
        time.sleep(2)  # esperar a que Arduino reinicie
        lbl_estado.config(text=f"Conectado a {PUERTO}")
    except:
        lbl_estado.config(text="Error al conectar")

def enviar(cmd):
    if arduino and arduino.is_open:
        arduino.write(cmd.encode())
        lbl_estado.config(text=f"Enviado: {cmd}")
    else:
        lbl_estado.config(text="No conectado")

# --- Interfaz gráfica ---
ventana = tk.Tk()
ventana.title("Control Motor Base")

tk.Button(ventana, text="Conectar", command=conectar).pack(pady=5)

tk.Button(ventana, text="Horario", command=lambda: enviar('H')).pack(pady=2)
tk.Button(ventana, text="Antihorario", command=lambda: enviar('A')).pack(pady=2)

tk.Button(ventana, text="Velocidad 1 (Lenta)", command=lambda: enviar('1')).pack(pady=2)
tk.Button(ventana, text="Velocidad 2 (Media)", command=lambda: enviar('2')).pack(pady=2)
tk.Button(ventana, text="Velocidad 3 (Rápida)", command=lambda: enviar('3')).pack(pady=2)

lbl_estado = tk.Label(ventana, text="Desconectado")
lbl_estado.pack(pady=10)

ventana.mainloop()