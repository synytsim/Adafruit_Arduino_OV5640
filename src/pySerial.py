import serial
import codecs
from PIL import Image
import io
import time

# Assume 'image_bytes' is a bytes object containing image data
#image_bytes = b''  # Replace with your actual byte data

count = 2
image_bytes = bytearray()

with serial.Serial() as ser:
    ser.baudrate = 2000000
    ser.port = 'COM14'
    ser.open()
    while (1):
        if (b'READY' in ser.readline()):
            break
    while (1):
        print("capturing")
        ser.write(b'\n')
        buffer = []
        while (1):
            #start = ser.readline()
            if (b'START' in ser.readline()):
                print("frame start")
                break
        while (1):
            # count += 1
            # if (count == 76000): 
            #     break
            data = ser.readline()
            if (b'END' in data):
                print("frame end")
                break
            #print(data)
            data = int(data.rstrip())
            # if (data == 0xD8):
            #     print("COO!")
            buffer.append(data)
            #print(int(data.rstrip()))
            # image_bytes.append(str(int(data.rstrip())).encode())
        image_bytes = bytes(buffer)

        fname = 'output' + str(count) + '.jpg'
        print(fname)
        with open(fname, 'wb') as jpgfile:
            jpgfile.write(image_bytes)
        count += 1
        time.sleep(1)
        

        #print(repr(image_bytes)[2:-1])
# # Create an image object from the byte stream
# #pythimage = Image.frombuffer('RGB', (320, 240), image_bytes, 'raw', 'RGB', 0, 1)
# image = Image.open(io.BytesIO(image_bytes[: eoi + 2]))

# # Save the image to a file (e.g., 'output_image.png')
# image.show()
# #pythimage.save('output_image.jpg')
    

    