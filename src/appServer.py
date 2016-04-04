import SocketServer
import socket
import subprocess

APP_CMD_INDEX_LO = 1
APP_CMD_ENABLE_AIRPLAY = 1
APP_CMD_DISABLE_AIRPLAY = 2
APP_CMD_ENABLE_TRACKING = 3
APP_CMD_DISABLE_TRACKING = 4
APP_CMD_INDEX_HI = 4

PACKET_SIZE = 64

class AppServiceHandler(SocketServer.BaseRequestHandler):
    """
    The request handler class for our server.

    It is instantiated once per connection to the server, and must
    override the handle() method to implement communication to the
    client.
    """

    def handle(self):
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
        print data

        # Verify magic preamble
        if data[:4] != "ACPT":
            return

        appCmd = ord(data[4])
        success = False
        if appCmd < APP_CMD_INDEX_LO or appCmd > APP_CMD_INDEX_HI:
            print "unknown command"
        elif appCmd == APP_CMD_ENABLE_AIRPLAY:
            subprocess.call(["sudo","service","shairport-sync","start"])
            print "Airplay enabled"
            success = True
        elif appCmd == APP_CMD_DISABLE_AIRPLAY:
            subprocess.call(["sudo","service","shairport-sync","stop"])
            print "Airplay disabled"
            success = True
        elif appCmd == APP_CMD_ENABLE_TRACKING:
            print "Tracking enabled"
            success = True
        elif appCmd == APP_CMD_DISABLE_TRACKING:
            print "Tracking disabled"
            success = True
        
        reply = str(bytearray([65, 67, 80, 84, 0] + [0]*59))
        self.request.sendall(reply)

if __name__ == "__main__":
    HOST, PORT = "10.0.0.10", 1027

    # Create the server, binding to localhost on port 9999
    server = SocketServer.TCPServer((HOST, PORT), AppServiceHandler)
    server.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    print "Start listening on port %d"%PORT

    # Activate the server; this will keep running until you
    # interrupt the program with Ctrl-C
    server.serve_forever()
