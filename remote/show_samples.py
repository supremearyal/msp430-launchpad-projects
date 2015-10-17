import serial
from matplotlib import pylab as plt

NUM_SAMPLES = 200

def serial_initialize():
    ser = serial.Serial(port='COM5') # Open COM5 (9600 8N1).
    print 'Reading on port: %s' % ser.portstr # Check which port was open.
    return ser

def sample_read(ser):
    header = ord(ser.read()) # Block until header byte.
    if header != 0x20:
       return None
    print '[OK] Header read.'
    # Store all samples.
    # Extra item so plot diplays correctly.
    sample = [0]
    for i in xrange(NUM_SAMPLES):
        # Add this point to sample.
        # One byte equals eight points.
        points = ord(ser.read())
        for j in xrange(8):
            print '[OK] Sample point (%d) read.' % (i * 8 + j)
            sample.append((points >> j) & 0x1)
    return sample

def sample_display(sample):
    if sample == None:
        return
    # Split data into time and sample axis.
    x, y = zip(*enumerate(sample))
    # Plot data.
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.step(x, y)
    ax.set_ylim(-1, 2)
    plt.draw()
    plt.show()

ser = serial_initialize()
# Continually read samples.
while 1:
    # Collect sample from serial port.
    sample = sample_read(ser)
    # Display latest sample.
    sample_display(sample)
