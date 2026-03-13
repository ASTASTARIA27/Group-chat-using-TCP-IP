#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>

#define TRUE 1
#define FALSE 0

int main(int argc,char *argv[]) {
    const char *port = "8080";
    char *host;
    struct addrinfo hints,*server;
    int r,sockfd,done;
    char buffer[BUFSIZ];
    fd_set read_fd;

    if( argc<2 )
	{
		fprintf(stderr,"Format: client hostname\n");
		exit(1);
	}
    host = argv[1];

    /* obtaining and converting host name and port */
	printf("Looking for chat host on %s...",host);
	memset( &hints, 0, sizeof(hints) );	
	hints.ai_family = AF_INET;				/* IPv4 */
	hints.ai_socktype = SOCK_STREAM;		/* TCP */
	r = getaddrinfo( host, port, &hints, &server );
	if( r!=0 )
	{
		perror("failed");
		exit(1);
	}
	puts("found");

    /*Creating a socket*/
    sockfd = socket(
        server->ai_family,
        server->ai_socktype,
        server->ai_protocol
    );
    if(sockfd==-1) {
        perror("failed");
        exit(1);
    }

    /*COnnecting to the socket*/
    r=connect(sockfd,server->ai_addr,server->ai_addrlen);
    freeaddrinfo(server);
    if(r==-1) {
        perror("failed");
        exit(1);
    }
    done = FALSE;

    /*Loop to interact with the host*/
    while(!done) {
        //Initialize the file descriptor set
        FD_ZERO(&read_fd);
        //add socket
        FD_SET(sockfd,&read_fd);
        FD_SET(0,&read_fd);
        r=select(sockfd+1,&read_fd,NULL,NULL,0);
        if(r==-1) {
            perror("failed");
            exit(1);
        }
        /*remote input*/
        if(FD_ISSET(sockfd,&read_fd)) {
            r=recv(sockfd,buffer,BUFSIZ,0);
            if(r<1) {
                puts("connection closed by peer");
                break;
            }
            //else take the buffer and output it
            buffer[r] ='\0'; 
            printf("%s",buffer);
        }
        /*Local input*/
        if(FD_ISSET(0,&read_fd)) {
            /* don't send an empty line */
			if( fgets(buffer,BUFSIZ,stdin)==NULL ) {
				putchar('\n');
			}
			/* if 'done' is input, close the loop and quit */
			else if( strcmp(buffer,"close\n")==0 ) {
				done=TRUE;
			}
			/* otherwise, send the input string - including the newline */
			else {
				send(sockfd,buffer,strlen(buffer),0);
			}
        }
    }
    printf("Disconnected\nBye!\n");
	close(sockfd);

	return(0);
}