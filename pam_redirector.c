/*
 * pam_redirector.c
 *
 * By Jamieson Becker, 09 May 2007
 * Copyright (c) 2013, Jamieson Becker. GPL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. A copy of the GNU General Public
 * License can be found at COPYING.
 *
 * CREDITS
 * Benjamin Schieder for pam_extern on which this module is based
 * An unknown RPI professor for client.c and U_client.c
 *
 * This PAM module is provides a simple client to a UNIX socket
 * server (running on localhost) to request the socket server to authenticate
 * the user with the following protocol:
 *
 * (This Pam Module sends:)
 * username\n
 * password\n
 *
 * and then expects:
 * 0 (for success, user is ok)
 * 1 (for failure, user login has failed.)
 *
 * The complete module includes a sample server (in Python) as well.
 *
 */


#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT
#define PAM_SM_SESSION
#define PAM_SM_PASSWORD

#include <security/pam_modules.h>
#include <security/pam_misc.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

// UNIX/IN sockets
#include <sys/socket.h>
#include <sys/un.h>
// #include <netinet/in.h>
// #include <netdb.h>

/* for security reasons, this version is basically restricted to local
 * UNIX sockets. its faster anyway and if you know what you're doing you
 * can write a local TCP redirector that is properly secured with SSL, or at
 * least use stunnel.
 */

/*
 * theory of operation
 * this script gets called with argv[0] which is the name of the TCP socket to
 * connect to. we connect to that socket and pass the username and password
 * to the server. then we get a return value of either 0 (login ok) or non-zero
 * (failure) which we return to PAM
*/

/* --- authentication management functions --- */

void *mymalloc(n){
	void *c;
	c = malloc(n);
	if (c == NULL){		// Whoops, we didn't get the memory we desired
		D((stderr, "Couldn't malloc %i bytes of memory", n));
		exit(PAM_IGNORE);
	}
	return c;
}

void myfree(void *p){
	_pam_overwrite(p);
	_pam_drop(p);
}

PAM_EXTERN int
pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv){
	int i;
	struct stat fstat;
	const void *item;
	char *username, *password;
	username = password = NULL;

	if (argv == NULL){
		D(( "pam_redirector: no socket specified!" ));
		return PAM_SUCCESS;
	}

	D(( "stat'ing %s\n", argv[0] ));
	i=stat(argv[0], &fstat);
	if (i != 0){
		D(( "Couldn't stat %s\n", argv[0] ));
		strerror(errno);
		return PAM_IGNORE;
	}
    D(( "stat of socket: %i\n", i ));
	// if (!S_ISREG(fstat.st_mode)){
	// 	D(( "pam_redirector: %s is not a socket!\n", argv[0] ));
	// 	return PAM_IGNORE;
	// }

	D(( "Getting username...\n" ));
	i = pam_get_item(pamh, PAM_USER, &item);
	if (i != PAM_SUCCESS) {
		/* very strange. */
		D(( "Couldn't get username? o_O\n" ));
		return PAM_IGNORE;
	} else if (item != NULL) {      /* we have a user! */
		username = mymalloc(strlen(item)+1);
		sprintf(username, "%s", (char *)item);
		item = NULL;
	} else {
		D(( "Username is empty? o_O\n" ));
		return PAM_IGNORE;         /* didn't work */
	}

	D(( "Getting password...\n" ));
	i = pam_get_item(pamh, PAM_AUTHTOK, &item);
	if (i != PAM_SUCCESS) {
		/* very strange. */
		D(( "Couldn't get password? o_O\n" ));
		return PAM_IGNORE;
	} else if (item != NULL) {      /* we have a password! */
		password = mymalloc(strlen(item)+1);
		sprintf(password, "%s", (char *)item);
		item = NULL;
	} else {
		D(( "We have no password\n" ));
		password = mymalloc(2);
		sprintf(password, " ");
	}

    // connect to UNIX stream
	D(( "using %s for a socket\n", argv[0] ));
    int sockfd, servlen, n;
    struct sockaddr_un serv_addr;
    bzero((char *)&serv_addr,sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, argv[0]);
    servlen = strlen(serv_addr.sun_path) +
                  sizeof(serv_addr.sun_family);
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM,0)) < 0)
        D(("Creating socket"));
    if (connect(sockfd, (struct sockaddr *)
                          &serv_addr, servlen) < 0)
        D(("Connecting"));

    // we already have username and password, so..

    D(("sending username %s", username));
    n = write(sockfd,username,strlen(username));
    write(sockfd,"\n",(1));
    if (n < 0) {
       D(("ERROR writing username to socket"));
       return PAM_IGNORE;
    }

    D(("sending password %s", password));
    n = write(sockfd,password,strlen(password));
    write(sockfd,"\n",(1));
    if (n < 0) {
       D(("ERROR writing password to socket"));
       return PAM_IGNORE;
    }

    // ok, we're done sending

    D(("preparing response buffer"));
    // prepare response buffer
    char *response;
    response=(char *)mymalloc(5); // should be enough to hold all return values (0-127)
    memset(response, 0, 5);

    D(("ok, get response."));
    n = read(sockfd,response,4); // one less than malloc'd
    D(("ok, got response."));
    if (n < 0) {
       D(("ERROR reading response from socket"));
       return PAM_IGNORE;
    }
    D(("close sockfd."));

    // done receiving the response code
    close(sockfd);  // close socket
    D(("response to integer."));
    i=atoi(response);
	D(( "done. Return code: %i", i ));

	// At this point, username and password are malloc'd, so no need to check before free
	myfree(username);
	myfree(password);
	if (i==0) {
        D(( "Returning PAM_SUCCESS" ));
		return PAM_SUCCESS;
	} else {
        D(( "Returning PAM_IGNORE" ));
		return PAM_IGNORE;
    }

}

PAM_EXTERN int
pam_sm_setcred(pam_handle_t *pamh, int flags,
	       int argc, const char **argv)
{
	 return PAM_SUCCESS;
}

/* --- password management --- */

PAM_EXTERN int
pam_sm_chauthtok(pam_handle_t *pamh, int flags,
		 int argc, const char **argv)
{
	 return PAM_SUCCESS;
}

/* --- account management functions --- */
PAM_EXTERN int
pam_sm_acct_mgmt(pam_handle_t *pamh, int flags,
		 int argc, const char **argv)
{
	 return PAM_SUCCESS;
}


/* --- session management --- */

PAM_EXTERN int
pam_sm_open_session(pam_handle_t *pamh, int flags,
		    int argc, const char **argv)
{
	return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_close_session(pam_handle_t *pamh, int flags,
		     int argc, const char **argv)
{
	 return PAM_SUCCESS;
}

/* end of module definition */

/* static module data */
#ifdef PAM_STATIC
struct pam_module _pam_redirector_modstruct = {
	"pam_redirector",
	pam_sm_authenticate,
	pam_sm_chauthtok,
	NULL,
	NULL,
	NULL,
	NULL
};
#endif
