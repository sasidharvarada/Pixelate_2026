import serial
import keyboard
import time
import threading

PORT = "COM11"      # CHANGE THIS
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=0.1)
time.sleep(2)

game_running = False
start_time = 0

def serial_reader():
    global game_running
    while True:
        line = ser.readline().decode().strip()
        if line.startswith("SCORE"):
            print("[ESP32]", line)
        elif line.startswith("GAME_OVER"):
            print("\nGAME FINISHED")
            print("Final", line)
            game_running = False

def countdown():
    print("Starting game in:")
    for i in [3,2,1]:
        print(i)
        time.sleep(1)
    print("GO!\n")
    ser.write(b"S")

threading.Thread(target=serial_reader, daemon=True).start()

print("Press ENTER to start Tetris")
keyboard.wait("enter")

countdown()
game_running = True
start_time = time.time()

while game_running:
    elapsed = int(time.time() - start_time)
    print(f"\rTime: {elapsed}s / 30s", end="")

    if keyboard.is_pressed("left"):
        print("\nLEFT")
        ser.write(b"L")
        time.sleep(0.1)

    elif keyboard.is_pressed("right"):
        print("\nRIGHT")
        ser.write(b"R")
        time.sleep(0.1)

    elif keyboard.is_pressed("down"):
        print("\nDOWN")
        ser.write(b"D")
        time.sleep(0.05)

print("\nGame session ended.")
ser.close()
