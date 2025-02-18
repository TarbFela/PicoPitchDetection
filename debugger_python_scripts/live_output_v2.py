import serial
import os
import numpy as np

#find modem path and setup serial
os.system("ls /dev/ | grep cu.usbmodem > usbmodemname.txt")
with open("usbmodemname.txt","r") as f:
    modem = f.read()
print("MODEM", modem)
modempath = "/dev/" + modem.strip()

ser = serial.Serial(modempath, 115200)
def try_int(val):
    try:
        val = int(val)
    except ValueError:
        val = 0
    return val
def get_data():
    ser.write("cont\n".encode("utf-8"))
    readout = ser.read_until("EOT".encode("utf-8"))
    readout = readout.decode("utf-8")


    sects = readout.split(":")
    print(sects[3])
    data1 = sects[1].split(",")[:-1]
    data2 = sects[2].split(",")[:-3]
    data1 = [try_int(val) for val in data1]
    data2 = [try_int(val) for val in data2]

    return data1, data2

#setup graphing
import matplotlib.pyplot as plt
import matplotlib.animation as animation


# Plot setup
BUFFERSIZE = 2**16

data1, data2 = get_data()
data_len1 = len(data1)
data_len2 = len(data2)
fig, ax = plt.subplots(3)
line1, = ax[0].plot(range(data_len1), data1)
line2, = ax[1].plot(range(data_len2), data2)
line3, = ax[2].plot(range(data_len2-1) , np.diff(data2))
ax[0].set_ylim(2048 - 256, 2048 + 256)  # 12-bit ADC range
ax[1].set_ylim(-5,105)
ax[2].set_ylim(-20,20)




def update(frame):
    data1, data2 = get_data()

    #print(data)
    line1.set_ydata(data1)
    line2.set_ydata(data2)
    line3.set_ydata(np.diff(data2))
    #return line1



ani = animation.FuncAnimation(fig, update, interval=250)
plt.show()


ser.write("q\n".encode('utf-8'))

