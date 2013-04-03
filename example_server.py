#! /usr/bin/env python2.5

__doc__ = """ A simple UnixStream lookup server."""

users = {
    "johnsmith": "secret"
}

from SocketServer import ThreadingUnixStreamServer, StreamRequestHandler
import sys, os, types, time

class server(StreamRequestHandler):
    def handle(self):
        username = self.rfile.readline(512).strip().lower()
        print "received username %s" % username
        password = self.rfile.readline(512).strip()
        print "received password %s" % password
        if username in users and users[username] == password:
            print "successful login of %s" % username
            response = 0
        else:
            print "failed login for %s, sleeping" % username
            time.sleep(45)
            print "awake again"
            response = 1
        self.wfile.write(response)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        socket = ("localhost", int(sys.argv[1]))
    else:
        socket = "/tmp/sock"
    if type(socket) == types.StringType:
        try: os.unlink(socket)
        except: pass
    print "Threading Server starting on %s." % socket
    ThreadingUnixStreamServer(socket,server).serve_forever()
