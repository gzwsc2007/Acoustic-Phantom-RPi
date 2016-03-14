import threading
import math
import time

NEUTRAL_PWM_US = 1500
STEP_DELTA_DEG = 1
STEP_DELAY_SEC = 0.1

class ServoControllerSmooth:
  # 0 degree is the neutral position for both pan and tilt.
  # Total range is (-rangeDeg, rangeDeg). Same for tilt.
  def __init__(self, ctrlName, pin, rangeDeg):
    self.rangeDeg = rangeDeg

    self.degToPulseWidth = 1000.0 / rangeDeg

    self.targetDeg = 0
    self.currentDeg = 0

    self.event = threading.Event()
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
    if deg > self.rangeDeg: return self.rangeDeg
    elif deg < -self.rangeDeg: return -self.rangeDeg
    else: return deg

  def setServoDeg(self):
    # Convert from deg to PWM width
    width_us = NEUTRAL_PWM_US + self.degToPulseWidth * self.currentDeg

    # send width to servo
    pass # TODO: use pigpio
    print width_us

  def ctrlLoop(self):
    while True:
      # Wait for new control setpoints
      self.event.wait()

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

if __name__ == "__main__":
    panCtrl = ServoControllerSmooth('pan', 18, 90)
    panCtrl.setDeltaDeg(20)
    while(True): pass