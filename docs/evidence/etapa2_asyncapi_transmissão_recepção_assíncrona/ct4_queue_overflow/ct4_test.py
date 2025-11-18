import serial
import time

# CONFIGURE sua porta serial
PORT = "COM3"     # altere se necessário
BAUD = 115200      # use a mesma configurada no Zephyr

# Mensagens para o teste
messages = [str(i) for i in range(1, 13)]   # 1 até 12

def main():
    print("Abrindo porta serial...")
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    time.sleep(1)

    print("Aguardar entrar em 'Modo RX' no dispositivo...")
    input("Pressione ENTER quando estiver no início do período RX... ")

    print("Enviando 12 mensagens rapidamente...")
    for msg in messages:
        data = msg + "\r\n"
        ser.write(data.encode("utf-8"))
        print("Enviado:", repr(data))
        time.sleep(0.05)   # 50ms entre mensagens

    print("Mensagens enviadas. Aguarde o modo TX exibir a fila...")
    ser.close()

if __name__ == "__main__":
    main()
