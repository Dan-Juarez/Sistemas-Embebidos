import tkinter as tk
import serial
import time

# Configura el puerto (ajustar al tuyo, ej. COM3 en Windows o /dev/ttyUSB0 en Linux)
arduino = serial.Serial('COM3', 9600, timeout=1)
time.sleep(2)

def horario():
    arduino.write(b'H')

def antihorario():
    arduino.write(b'A')

def velocidad_lenta():
    arduino.write(b'1')

def velocidad_media():
    arduino.write(b'2')

def velocidad_rapida():
    arduino.write(b'3')

def enviar_pasos():
    pasos = entry_pasos.get()
    if pasos.isdigit():  # Verifica que sean números
        comando = f"P{pasos}\n"   # Agregamos prefijo "P" para diferenciar
        arduino.write(comando.encode('utf-8'))

# Interfaz gráfica
root = tk.Tk()
root.title("Control Motor a Pasos")
tk.Label(root, text="Control de Motor a Pasos", font=("Arial", 16)).pack(pady=10)

tk.Button(root, text="Horario", command=horario, width=15, height=2).pack(pady=5)
tk.Button(root, text="Antihorario", command=antihorario, width=15, height=2).pack(pady=5)

tk.Label(root, text="Velocidad").pack(pady=10)
tk.Button(root, text="Lenta", command=velocidad_lenta).pack(pady=2)
tk.Button(root, text="Media", command=velocidad_media).pack(pady=2)
tk.Button(root, text="Rápida", command=velocidad_rapida).pack(pady=2)

# Entrada de pasos
tk.Label(root, text="Número de pasos:").pack(pady=10)
entry_pasos = tk.Entry(root)
entry_pasos.pack(pady=5)
tk.Button(root, text="Mover", command=enviar_pasos, width=15, height=2).pack(pady=5)

root.mainloop()
