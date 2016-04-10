import threading
import math
import time
import pigpio
import SocketServer
import socket
import struct
import signal

NEUTRAL_PWM_US = 1500
STEP_DELTA_DEG = 0.5
STEP_DELAY_SEC = 0.01
g_exitFlag = False

class ServoControllerSmooth:
  # 0 degree is the neutral position for both pan and tilt.
  # Total range is (-rangeDeg, rangeDeg). Same for tilt.
  def __init__(self, ctrlName, pin, halfRangeDeg, halfRangePWM, neutralPWM, hardLimitsDeg):
    self.halfRangeDeg = halfRangeDeg
    self.neutralPWM = neutralPWM
    self.servoPin = pin
    self.hardLimitDegLo = hardLimitsDeg[0]
    self.hardLimitDegHi = hardLimitsDeg[1]

    self.degToPulseWidth = halfRangePWM / halfRangeDeg

    # Valid range of degrees is (-halfRangeDeg, halfRangeDeg)
    self.targetDeg = 0
    self.currentDeg = 0

    self.pigpio = pigpio.pi()
    self.pigpio.set_servo_pulsewidth(pin, neutralPWM)

    self.event = threading.Event()
    self.exitFlag = False
    self.thread = threading.Thread(name=ctrlName,
                                   target=self.ctrlLoop)
    self.thread.start()

  def setDeltaDeg(self, delta_deg):
    deg = self.clamp(self.targetDeg + delta_deg)
    self.targetDeg = deg
    # wake up the controller thread
    self.event.set()

  def setpointDeg(self, deg):
    deg = self.clamp(deg)
    self.targetDeg = deg
    # wake up the controller thread
    self.event.set()

  def clamp(self, deg):
    if deg > self.hardLimitDegHi: return self.hardLimitDegHi
    elif deg < self.hardLimitDegLo: return self.hardLimitDegLo
    else: return deg

  def setServoDeg(self):
    # Convert from deg to PWM width
    width_us = self.neutralPWM + self.degToPulseWidth * self.currentDeg

    # send width to servo
    self.pigpio.set_servo_pulsewidth(self.servoPin, width_us)

  def ctrlLoop(self):
    while True:
      # Wait for new control setpoints
      self.event.wait()
      if self.exitFlag:
        break

      goalNotReached = True
      while goalNotReached:
        if (abs(self.currentDeg - self.targetDeg) <= STEP_DELTA_DEG):
          self.currentDeg = self.targetDeg
          goalNotReached = False
          self.event.clear()
        else:
          if self.targetDeg - self.currentDeg > 0:
            self.currentDeg += STEP_DELTA_DEG
          else:
            self.currentDeg -= STEP_DELTA_DEG

        # send command to servo using pigpio
        self.setServoDeg()

        # delay for a while
        time.sleep(STEP_DELAY_SEC)

  def shutdown(self):
    self.exitFlag = True
    self.event.set()
    self.thread.join()
    self.pigpio.set_servo_pulsewidth(self.servoPin, 0) #disable pin

def demo():
  p = ServoControllerSmooth('pan', 4, 90, 910, 1460, (-90,90))
  t = ServoControllerSmooth('tilt', 18, 90, 810, 1030, (-45,90))

  time.sleep(1)
  p.setpointDeg(10)
  time.sleep(0.1)
  p.setpointDeg(15)
  time.sleep(0.1)
  p.setpointDeg(20)
  time.sleep(0.1)
  p.setpointDeg(25)
  time.sleep(2)

  while True:
    try:
      p.setpointDeg(90)
      t.setpointDeg(90)
      time.sleep(1)
      p.setpointDeg(-90)
      t.setpointDeg(-45)
      time.sleep(1)
    except KeyboardInterrupt:
      print "Exiting.."
      break

  p.shutdown()
  t.shutdown()

def handler(signum, frame):
    g_exitFlag = True

def serve():
  g_exitFlag = False
  signal.signal(signal.SIGINT, handler)
  signal.signal(signal.SIGTERM, handler)
  
  p = ServoControllerSmooth('pan', 4, 90, 910, 1460, (-90,90))
  t = ServoControllerSmooth('tilt', 18, 90, 810, 1030, (-45,90))

  # By default pointing downwards
  t.setpointDeg(30)

  # create UDP socket
  HOST, PORT = "", 5200
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  sock.bind((HOST,PORT))

  print "[ServoServer] Start listening on port %d"%PORT

  while not g_exitFlag:
    try:
      data, _ = sock.recvfrom(8) # 2 floats
      dx = struct.unpack("<f", data[:4])[0]
      dy = struct.unpack("<f", data[4:])[0]
      p.setDeltaDeg(dx)
      t.setDeltaDeg(dy)
    except KeyboardInterrupt:
      break

  print "[ServoServer] Exiting.."
  p.shutdown()
  t.shutdown()

if __name__ == "__main__":
  serve()
