# archivo: entrenar_y_servir_modelo.py
import serial
import time
import joblib
import numpy as np
from sklearn.neighbors import KNeighborsClassifier

# --------------------------------------------------
# 1) Datos de entrenamiento (ejemplo con tus lecturas)
#    Formato: [R, G, B], "label"
#    Puedes agregar MUCHOS más puntos aquí.
# --------------------------------------------------

train_data = [
    # NARANJA (ejemplos que diste)
    [19607, 16949, 19230, "naranja"],
    [19607, 15384, 17241, "naranja"],
    [19607, 15625, 17543, "naranja"],
    [22222, 15873, 17857, "naranja"],
    [19230, 15384, 17241, "naranja"],

    # AMARILLO
    [20408, 17543, 17857, "amarillo"],
    [20408, 17857, 18181, "amarillo"],
    [20408, 17857, 18181, "amarillo"],

    # MORADO
    [15873, 14492, 16666, "morado"],
    [15873, 14705, 16949, "morado"],
    [16666, 15384, 17857, "morado"],

    # ROJO
    [16949, 14285, 16666, "rojo"],
    [17857, 15384, 17857, "rojo"],
    [17543, 15625, 17857, "rojo"],
    [17543, 15151, 19607, "rojo"],
    [17543, 14705, 16949, "rojo"],

    # VERDE
    [17241, 16666, 17857, "verde"],
    [17241, 16949, 18181, "verde"],
    [16666, 16393, 17241, "verde"],
    [16666, 18181, 17543, "verde"],
]

X = np.array([row[:3] for row in train_data])   # R,G,B
y = np.array([row[3] for row in train_data])   # etiqueta (string)

# --------------------------------------------------
# 2) Entrenar modelo KNN
# --------------------------------------------------
print("Entrenando modelo KNN...")
model = KNeighborsClassifier(n_neighbors=3)
model.fit(X, y)
print("Modelo entrenado.")

# Opcional: guardar modelo a disco
joblib.dump(model, "modelo_skittles.pkl")
print("Modelo guardado en modelo_skittles.pkl")

# --------------------------------------------------
# 3) Abrir puerto serie y servir "predicciones"
# --------------------------------------------------

# Ajusta el puerto según tu sistema:
SERIAL_PORT = "COM4" 
BAUD_RATE = 9600

print(f"Abriendo puerto serie {SERIAL_PORT}...")
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
time.sleep(2)  # pequeño delay para que el Arduino reinicie

print("Listo. Esperando lecturas de Arduino...\n")

def label_to_code(label: str) -> str:
    """
    Convierte la etiqueta en texto a un código de 1 letra
    que el Arduino entiende.
    """
    if label == "amarillo":
        return "Y"
    if label == "verde":
        return "V"
    if label == "rojo":
        return "R"
    if label == "naranja":
        return "N"
    if label == "morado":
        return "M"
    return "?"  # desconocido

while True:
    try:
        line = ser.readline().decode("utf-8").strip()
        if not line:
            continue

        # Podemos usar la marca "WAIT_COLOR" para ignorar esa línea
        if line.startswith("WAIT_COLOR"):
            # simple log
            print("[Arduino] WAIT_COLOR")
            continue

        print(f"Linea recibida: {line}")

        # Esperamos formato: R,G,B
        try:
            parts = line.split(",")
            if len(parts) != 3:
                print("Formato inesperado, se esperaba R,G,B")
                continue

            R = int(parts[0])
            G = int(parts[1])
            B = int(parts[2])

        except ValueError:
            print("No se pudo parsear R,G,B")
            continue

        # Hacemos la predicción con el modelo
        rgb = np.array([[R, G, B]])
        pred = model.predict(rgb)[0]  # string, ej. "naranja"
        print(f"Prediccion IA: {pred}")

        # Convertimos a código 1 letra y se lo mandamos al Arduino
        code = label_to_code(pred)
        ser.write((code + "\n").encode("utf-8"))
        print(f"Enviado a Arduino: {code}\n")

    except KeyboardInterrupt:
        print("Saliendo...")
        break

ser.close()
