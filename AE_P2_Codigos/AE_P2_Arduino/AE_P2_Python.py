#P2_Actividad Extracurricular - Sistemas Embebidos con Arduino y Python
#Daniel Eduardo Juárez Bañuelos || Viviana Jaqueline Morin Garcia

import tkinter as tk
from tkinter import ttk, messagebox
import serial, serial.tools.list_ports
import threading
import queue
import time
import re

BAUD = 115200
PORT = None

DIST_RE = re.compile(r"^DIST:([0-9.]+|NaN)$")
STATE_RE = re.compile(r"^STATE:(RUN|IDLE)$")

class SerialWorker(threading.Thread):
    def __init__(self, port, baud, rx_queue):
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.rx_queue = rx_queue
        self.ser = None
        self._stop = threading.Event()

    def connect(self):
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.05)
            return True
        except Exception as e:
            self.rx_queue.put(("ERROR", f"No se pudo abrir {self.port}: {e}"))
            return False

    def run(self):
        if not self.connect():
            return
        buf = ""
        self.rx_queue.put(("INFO", f"Conectado a {self.port} @ {self.baud}"))
        while not self._stop.is_set():
            try:
                data = self.ser.read(256)
                if data:
                    buf += data.decode(errors="ignore")
                    while "\n" in buf:
                        line, buf = buf.split("\n", 1)
                        line = line.strip()
                        if line:
                            self.rx_queue.put(("LINE", line))
                else:
                    time.sleep(0.01)
            except Exception as e:
                self.rx_queue.put(("ERROR", f"Error de lectura: {e}"))
                break
        try:
            if self.ser and self.ser.is_open:
                self.ser.close()
        except:
            pass
        self.rx_queue.put(("INFO", "Puerto serie cerrado"))

    def write(self, text):
        try:
            if self.ser and self.ser.is_open:
                self.ser.write((text + "\n").encode())
        except Exception as e:
            self.rx_queue.put(("ERROR", f"Error escribiendo: {e}"))

    def stop(self):
        self._stop.set()

def autoselect_port():
    # Intenta encontrar un puerto "Arduino"
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        desc = (p.description or "").lower()
        if "arduino" in desc or "usb serial" in desc or "ch340" in desc:
            return p.device
    return ports[0].device if ports else None

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Ultrasónico + Stepper")
        self.geometry("420x260")
        self.minsize(380, 240)

        self.rx_queue = queue.Queue()
        port = PORT or autoselect_port()
        if port is None:
            messagebox.showerror("Puerto no encontrado", "No se encontró ningún puerto serie.")
            self.destroy(); return

        # --- UI ---
        frm = ttk.Frame(self, padding=12)
        frm.pack(fill="both", expand=True)

        ttk.Label(frm, text="Puerto:").grid(row=0, column=0, sticky="w")
        self.port_var = tk.StringVar(value=port)
        ttk.Entry(frm, textvariable=self.port_var, width=20).grid(row=0, column=1, sticky="w")
        self.conn_btn = ttk.Button(frm, text="Conectar", command=self.on_connect)
        self.conn_btn.grid(row=0, column=2, padx=6)

        sep = ttk.Separator(frm)
        sep.grid(row=1, column=0, columnspan=3, sticky="ew", pady=8)

        self.dist_var = tk.StringVar(value="---")
        self.state_var = tk.StringVar(value="IDLE")
        self.status_var = tk.StringVar(value="Desconectado")

        ttk.Label(frm, text="Distancia (cm):", font=("Segoe UI", 11)).grid(row=2, column=0, sticky="w")
        self.dist_lbl = ttk.Label(frm, textvariable=self.dist_var, font=("Segoe UI", 22, "bold"))
        self.dist_lbl.grid(row=2, column=1, sticky="w")

        ttk.Label(frm, text="Estado motor:", font=("Segoe UI", 11)).grid(row=3, column=0, sticky="w")
        self.state_lbl = ttk.Label(frm, textvariable=self.state_var, font=("Segoe UI", 12))
        self.state_lbl.grid(row=3, column=1, sticky="w")

        btns = ttk.Frame(frm)
        btns.grid(row=4, column=0, columnspan=3, pady=10, sticky="w")
        self.start_btn = ttk.Button(btns, text="Iniciar", command=lambda: self.send_cmd("START"), state="disabled")
        self.stop_btn  = ttk.Button(btns, text="Detener", command=lambda: self.send_cmd("STOP"),  state="disabled")
        self.start_btn.pack(side="left", padx=4)
        self.stop_btn.pack(side="left", padx=4)

        ttk.Label(frm, textvariable=self.status_var).grid(row=5, column=0, columnspan=3, sticky="w", pady=(8,0))

        for i in range(3):
            frm.columnconfigure(i, weight=1)

        self.worker = None
        self.after(50, self.poll_queue)

    def on_connect(self):
        if self.worker is None:
            port = self.port_var.get().strip()
            if not port:
                messagebox.showwarning("Puerto", "Especifica un puerto válido.")
                return
            self.worker = SerialWorker(port, BAUD, self.rx_queue)
            self.worker.start()
            self.conn_btn.config(text="Desconectar")
            self.status_var.set(f"Conectando a {port}...")
        else:
            self.worker.stop()
            self.worker = None
            self.conn_btn.config(text="Conectar")
            self.start_btn.config(state="disabled")
            self.stop_btn.config(state="disabled")
            self.status_var.set("Desconectado")
            self.state_var.set("IDLE")
            self.dist_var.set("---")

    def send_cmd(self, cmd):
        if self.worker:
            self.worker.write(cmd)

    def poll_queue(self):
        try:
            while True:
                typ, payload = self.rx_queue.get_nowait()
                if typ == "INFO":
                    self.status_var.set(payload)
                    if "Conectado" in payload:
                        self.start_btn.config(state="normal")
                        self.stop_btn.config(state="normal")
                elif typ == "ERROR":
                    self.status_var.set(payload)
                elif typ == "LINE":
                    line = payload
                    
                    m = DIST_RE.match(line)
                    if m:
                        val = m.group(1)
                        self.dist_var.set(val if val != "NaN" else "---")
                    else:
                        s = STATE_RE.match(line)
                        if s:
                            self.state_var.set(s.group(1))
                
        except queue.Empty:
            pass
        self.after(50, self.poll_queue)

    def on_close(self):
        if self.worker:
            self.worker.stop()
        self.destroy()

if __name__ == "__main__":
    app = App()
    app.protocol("WM_DELETE_WINDOW", app.on_close)
    app.mainloop()
