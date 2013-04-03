pam_redirector
==============


Description
-----------

A pluggable authentication module (PAM) that allows redirection of
credentials to a local UNIX stream socket server. (For security
reasons, TCP connections are not allowed and connections must occur
through a local UNIX socket on the filesystem.)

If you are authenticating against a common data store, such as LDAP or
an SQL database, please use the pam_ modules for your application, as
they'll be more secure and probably faster.  For example, check out
pam_ldap and pam_mysql.

However, if you have strange needs (for example, you need to
authenticate using non-standard credentials or to a backend data store
that doesn't have a PAM module already written), this method can yield
higher performance, particularly for long running database
connections.

Rather than instantiate a new connection to the database for each
login, this method allows you to maintain a long-running connection
pool at the socket-server level and then just have pam_redirector make
a short-lived connection to your local socket server. In the example
server code, we're using a threaded socket server based on Python's
ThreadedSocketServer, but you could use an asynchronous or
forking server if you prefer.

This version replaces the version at SF: [https://sourceforge.net/projects/pamredirector/]


Protocol
--------


pam_redirector implements an extremely simple socket protocol
whereby pam_redirector passes a username and password (separated
by new line) to your server and then your server simply replies
with a 0 or 1:

    [pam_redirector]   john_smith
    [pam_redirector]   secret
    [your server] 1

Please review the example_server.py for an example of how to write
a small socket server to handle authentication.



Requirements
------------


*   The LibPAM development headers (libpam-dev or libpam0g-dev)

    .. and, of course:

*   A PAM-based system (currently only tested on Linux)
*   Python 2.0 or greater (to run the example socket server)


Source Install
--------------


        make clean

        make all

        # make debug will instead build pam_redirector to echo all
        # usernames and passwords to syslog -- useful when
        # troubleshooting, but insecure

        sudo make install

        # which is the same as:
        # cp pam_redirector.so /lib/security/
        # cp example_server.py /usr/local/sbin/

The pre-built pam_redirector.so is built on Debian Wheezy
for Intel 64-bit. Try to build it yourself and report bugs. If you
have any trouble, ask.


Uninstall
---------


        sudo rm /lib/security/pam_redirector.so
        sudo rm /usr/local/sbin/example_server.py



Usage
-----

See example_server.py for a very simple Python socket server that
allows johnsmith with the password of secret to log in, while everyone
else is denied.

To start the example server:
        python example_server.py



PAM Configuration
-----------------

Remember, you need to configure PAM to actually use this pam module
as well for each service you want it to authenticate.

Here's an example PAM configuration file to put in /etc/pam.d:

    #%PAM-1.0
    
    auth optional pam_unix.so
    account optional pam_unix.so
    
    # @include common-auth
    # @include common-account

    @include common-session
    
    auth required     pam_redirector.so /var/run/pam_redirector/socket
    account required  pam_redirector.so /var/run/pam_redirector/socket



Security Caveats
----------------


1)   Obviously, no authentication occurs between the server and PAM,
     so ensure you restrict access to the socket to root only.
     The example server creates a socket as /tmp/sock, and this
     is VERY INSECURE, for demonstration only.



Alternatives
------------

        Stanford SRP Authentication     [http://srp.stanford.edu]
        Kerberos                        [http://web.mit.edu/Kerberos/]



See Also NSS
------------


PAM handles authentication and you might find it useful to also use NSS modules to handle things like user and group metadata, etc.


License
-------

Copyright (c) 2007, 2013 Jamieson Becker

All rights reserved. This package is free software, licensed under the GNU
General Public License (GPL), version 2 or later.

No warranty is provided for this software. Please see the complete terms of
the GNU General Public License for details or contact the author.

