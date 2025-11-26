# archivo: ColorSorterIApython_negro.py
import serial
import time
import joblib
import numpy as np
from sklearn.neighbors import KNeighborsClassifier

# --------------------------------------------------
# 1) Datos de entrenamiento (R, G, B, etiqueta)
#    10 lecturas por color con disco negro
# --------------------------------------------------

train_data = [
    # MORADO
    [3289, 2873, 3412, "morado"],
    [3311, 2915, 3472, "morado"],
    [3194, 2857, 3460, "morado"],
    [3610, 2849, 3389, "morado"],
    [3367, 2915, 3472, "morado"],
    [3194, 2793, 3322, "morado"],
    [3311, 2932, 3412, "morado"],
    [3174, 2801, 3333, "morado"],
    [3333, 2932, 3424, "morado"],
    [3184, 2801, 3333, "morado"],

    # VERDE
    [4273, 4273, 3906, "verde"],
    [3816, 3846, 3802, "verde"],
    [4219, 4255, 3831, "verde"],
    [4255, 4385, 3816, "verde"],
    [4149, 4219, 3846, "verde"],
    [4115, 4132, 3773, "verde"],
    [3891, 3816, 3690, "verde"],
    [4901, 5000, 4184, "verde"],
    [3846, 3952, 3690, "verde"],
    [3802, 3816, 3717, "verde"],

    # AMARILLO
    [7407, 5714, 4347, "amarillo"],
    [6944, 5291, 4237, "amarillo"],
    [6944, 5586, 4291, "amarillo"],
    [7142, 5464, 4329, "amarillo"],
    [6666, 5208, 4184, "amarillo"],
    [6756, 5319, 4366, "amarillo"],
    [7142, 5434, 4310, "amarillo"],
    [6944, 5347, 4237, "amarillo"],
    [6993, 5347, 4201, "amarillo"],
    [6944, 5405, 4347, "amarillo"],

    # ROJO
    [4672, 3003, 3558, "rojo"],
    [4149, 2941, 3546, "rojo"],
    [4273, 2958, 3546, "rojo"],
    [4629, 3067, 3649, "rojo"],
    [4032, 3003, 3703, "rojo"],
    [4608, 3115, 3663, "rojo"],
    [4424, 3003, 3649, "rojo"],
    [4405, 2994, 3571, "rojo"],
    [4694, 3039, 3731, "rojo"],
    [4608, 3095, 3636, "rojo"],

    # NARANJA
    [6024, 3597, 3891, "naranja"],
    [6535, 3663, 3968, "naranja"],
    [6250, 3571, 3802, "naranja"],
    [6211, 3703, 3831, "naranja"],
    [5494, 3355, 3717, "naranja"],
    [5952, 3508, 3787, "naranja"],
    [6329, 3584, 3906, "naranja"],
    [6172, 3571, 3816, "naranja"],
    [5988, 3412, 3802, "naranja"],
    [5882, 3436, 3759, "naranja"],
]

X = np.array([row[:3] for row in train_data], dtype=float)   # R,G,B
y = np.array([row[3] for row in train_data])                 # etiqueta

# --------------------------------------------------
# 2) Entrenar modelo KNN sobre RGB crudo
# --------------------------------------------------
print("Entrenando modelo KNN con datos de disco negro (50 muestras)...")
model = KNeighborsClassifier(n_neighbors=3, weights="distance")
model.fit(X, y)
print("Modelo entrenado.")

joblib.dump(model, "modelo_skittles_negro.pkl")
print("Modelo guardado en modelo_skittles_negro.pkl")

# --------------------------------------------------
# 3) Abrir puerto serie y servir predicciones
# --------------------------------------------------

# ⚠ CAMBIA ESTO AL COM CORRECTO
SERIAL_PORT = "COM4"   # Ejemplo: "COM3", "COM4", etc.
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

try:
    while True:
        line = ser.readline().decode("utf-8").strip()
        if not line:
            continue

        # Si usas el mensaje "WAIT_COLOR" desde Arduino, lo puedes ignorar aquí
        if line.startswith("WAIT_COLOR"):
            print("[Arduino] WAIT_COLOR")
            continue

        print(f"Linea recibida: {line}")

        # Esperamos formato: R,G,B
        try:
            parts = line.split(",")
            if len(parts) != 3:
                print("Formato inesperado, se esperaba R,G,B")
                continue

            R = float(parts[0])
            G = float(parts[1])
            B = float(parts[2])

        except ValueError:
            print("No se pudo parsear R,G,B")
            continue

        # Predicción con el modelo
        rgb = np.array([[R, G, B]], dtype=float)
        pred = model.predict(rgb)[0]  # ej. "naranja"
        print(f"Prediccion IA: {pred}")

        # Convertimos a código 1 letra y lo mandamos al Arduino
        code = label_to_code(pred)
        ser.write((code + "\n").encode("utf-8"))
        print(f"Enviado a Arduino: {code}\n")

except KeyboardInterrupt:
    print("Saliendo...")

ser.close()
