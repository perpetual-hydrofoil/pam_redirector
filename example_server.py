#! /usr/bin/env python2.5


__doc__ = """

example_server.py
A simple UnixStream lookup server.

Copyright (C) 2013 Jamieson Becker

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""


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
