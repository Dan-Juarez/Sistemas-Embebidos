import tkinter as tk
import serial

# Configurar puerto serial (ajustar según tu PC)
arduino = serial.Serial('COM3', 9600, timeout=1)

# Función para enviar secuencia al Arduino
def enviar_secuencia(secuencia):
    arduino.write(secuencia.encode())

# Crear interfaz
root = tk.Tk()
root.title("Control de LEDs")
tk.Label(root, text="Selecciona la secuencia de LEDs").pack(pady=10)
tk.Button(root, text="Secuencia 1 (Izq → Der)", command=lambda:
enviar_secuencia('1')).pack(pady=5)
tk.Button(root, text="Secuencia 2 (Der → Izq)", command=lambda:
enviar_secuencia('2')).pack(pady=5)
tk.Button(root, text="Secuencia 3 (Todos parpadean)",
command=lambda: enviar_secuencia('3')).pack(pady=5)
tk.Button(root, text="Secuencia Aleatoria",
          command=lambda: enviar_secuencia('4')).pack(pady=5)

tk.Button(root, text="Cambiar Velocidad",
          command=lambda: enviar_secuencia('5')).pack(pady=5)

root.mainloop()
