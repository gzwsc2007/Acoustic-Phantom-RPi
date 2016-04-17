import SocketServer
import socket
import subprocess
import servoControllerSmooth
import time

APP_CMD_INDEX_LO = 1
APP_CMD_ENABLE_ALL = 1
APP_CMD_DISABLE_ALL = 2
APP_CMD_INDEX_HI = 2

PACKET_SIZE = 64

g_tracker_process = None
g_servo_process = None
g_airplay_process = None

def shutdownTracker():
    global g_tracker_process

    if (g_tracker_process != None):
        g_tracker_process.terminate()
        g_tracker_process.wait()
        g_tracker_process = None
        print "Tracker Stopped"

def shutdownServoServer():
    global g_servo_process

    if (g_servo_process != None):
        g_servo_process.terminate()
        g_servo_process.wait()
        g_servo_process = None
        print "Servo Server Stopped"

def shutdownAirplay():
    global g_airplay_process

    if (g_airplay_process != None):
        g_airplay_process.terminate()
        g_airplay_process.wait()
        g_airplay_process = None
        print "Airplay Stopped"

class AppServiceHandler(SocketServer.BaseRequestHandler):
    """
    The request handler class for our server.

    It is instantiated once per connection to the server, and must
    override the handle() method to implement communication to the
    client.
    """

    def handle(self):
        global g_tracker_process
        global g_servo_process
        global g_airplay_process

        # self.request is the TCP socket connected to the client
        data = ""
        while True:
            temp = self.request.recv(PACKET_SIZE)
            data += temp
            print "[DEBUG] Got "+temp
            if len(temp) >= PACKET_SIZE:
                break
            elif temp == '':
                print "lost connection"
                return

        print "{} wrote:".format(self.client_address[0])
        print data.encode('hex')

        # Verify magic preamble
        if data[:4] != "ACPT":
            return

        appCmd = ord(data[4])
        success = False
        if appCmd < APP_CMD_INDEX_LO or appCmd > APP_CMD_INDEX_HI:
            print "unknown command"
        elif appCmd == APP_CMD_ENABLE_ALL:
            # Shutdown anything that is running
            shutdownTracker()
            shutdownServoServer()
            shutdownAirplay()
            
            # Enable AirPlay
            g_airplay_process = subprocess.Popen(["sudo","shairport-sync"])
            print "Airplay enabled"

            # Start the servo server
            g_servo_process = subprocess.Popen(("python", "servoControllerSmooth.py"))
            time.sleep(0.5)
            print "Servo Server enabled"

            # Start the color tracker
            avH = ord(data[5])
            loH = ord(data[6])
            hiH = ord(data[7])
            avH2 = ord(data[8])
            loH2 = ord(data[9])
            hiH2 = ord(data[10])
            args = ("./tracking/tracker.o", str(loH), str(hiH), str(loH2), str(hiH2))
            g_tracker_process = subprocess.Popen(args)
            print "Tracker enabled"

            success = True
        elif appCmd == APP_CMD_DISABLE_ALL:
            shutdownTracker()
            shutdownServoServer()
            shutdownAirplay()
            success = True

        reply = str(bytearray([65, 67, 80, 84, 0] + [0]*59))
        try:
            self.request.sendall(reply)
        except:
            print "[WARN] Cannot send reply"

if __name__ == "__main__":
    # Create the server, binding to localhost on port
    HOST, PORT = "172.24.1.1", 1027
    server = SocketServer.TCPServer((HOST, PORT), AppServiceHandler)
    server.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    print "[main] Start listening on port %d"%PORT

    # Activate the server; this will keep running until you
    # interrupt the program with Ctrl-C
    try:
        server.serve_forever()
    except:
        print "[main] Exiting..."

    # Cleanup
    shutdownTracker()
    shutdownServoServer()
    shutdownAirplay()


