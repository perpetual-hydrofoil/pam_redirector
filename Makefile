all: pam_redirector.so

pam_redirector.so: pam_redirector.c
	gcc -fPIC -Wall -shared -lpam -o pam_redirector.so pam_redirector.c

debug: pam_redirector.c
	gcc -fPIC -DDEBUG -Wall -shared -lpam -o pam_redirector.so pam_redirector.c

install:
	cp pam_redirector.so /lib/security/
	cp example_server.py /usr/local/sbin/

clean:
	rm pam_redirector.so
