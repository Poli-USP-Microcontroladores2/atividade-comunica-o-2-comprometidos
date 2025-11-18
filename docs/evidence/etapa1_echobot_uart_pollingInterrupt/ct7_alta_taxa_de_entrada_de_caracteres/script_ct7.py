import serial
import time

# Ajuste conforme a sua porta e baud rate
PORT = "COM3"
BAUD = 115200

try:
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    print(f"✅ Conectado à {PORT} a {BAUD} bps")

    for i in range(50):
        msg = f"mensagem_{i}\n"
        ser.write(msg.encode('utf-8'))
        print(f"> Enviado: {msg.strip()}")
        time.sleep(0.02)  # 20 ms entre mensagens (alta taxa)

    print("🚀 Todas as mensagens foram enviadas.")
    ser.close()

except serial.SerialException as e:
    print(f"❌ Erro de comunicação serial: {e}")
except KeyboardInterrupt:
    print("\n🛑 Interrompido pelo usuário.")
