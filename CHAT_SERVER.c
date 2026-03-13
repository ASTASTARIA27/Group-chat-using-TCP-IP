#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>

#define TRUE 1
#define FALSE 0

/*SERVER MUST ACCEPTS MUST ACCEPT AND MANAGE MULTIPLE CLIENT CONNECTIONS
TEXT RECEIVED FROM ONE CLIENT IS SHARED WITH ALL OTHER CLIENTS*/

int main() {
    const char *port = "8080";
    const int client_name_size = 32;
    char client_name[client_name_size];
    char buffer[BUFSIZ],sendstr[BUFSIZ];
    const int backlog = 10; //max connections
    char connection[backlog][client_name_size]; //to store IPv4 connections
    socklen_t address_len = sizeof(struct sockaddr);
    struct addrinfo hints, *server;
    struct sockaddr address;
    int r,max_connect,fd,x,done;
	fd_set main_fd,read_fd;
	int sockfd,clientfd;

    /*Setting up the server*/
    memset(&hints ,0 ,sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    r = getaddrinfo(0,port,&hints,&server);
    if(r!=0) {
        perror("failed");
        exit(1);
    }

    /*Creating a socket*/
    sockfd = socket(
        server->ai_family,
        server->ai_socktype,
        server->ai_protocol
    );
    if(sockfd==-1) {
        perror("falied");
        exit(1);
    }
    /*binding*/
    r=bind(sockfd,server->ai_addr,server->ai_addrlen);
    if(r==-1) {
        perror("failed");
        exit(1);
    }
    /*Listen for a connection*/
    puts("Chat server listening");
    r=listen(sockfd,backlog);
    if(r==-1) {
        perror("failed");
        exit(1);
    }

    /*Multiple connections*/
    max_connect = backlog; //initializing max connections
    FD_ZERO(&main_fd); //initializing file descriptors
    FD_SET(sockfd,&main_fd);
    /*Loop*/
    done = FALSE;
    while(!FALSE) {
        read_fd = main_fd;
        //scanning connections for activity
        r = select(max_connect+1,&read_fd,NULL,NULL,0);
        if(r==-1) {
            perror("failed");
            exit(1);
        }
        //Loop to check any active connections
        for(fd=1;fd<max_connect;fd++) {
            /*Filtering only active or new clients*/
            if(FD_ISSET(fd,&read_fd)) {
                if(fd==sockfd) {
                     //adding the new client
                    clientfd = accept(
                        sockfd,
                        (struct sockaddr *)&address,
                        &address_len
                    );  
                    if(clientfd==-1) {
                        perror("failed");
                        exit(1);
                    } 
                    /*Connection accepted and getting IP address*/
                    r = getnameinfo(
                        (struct sockaddr *)&address,
                        address_len,
                        client_name,
                        client_name_size,
                        0,
                        0,
                        NI_NUMERICHOST
                    );
                    /* update array of IP addresses */
					strcpy(connection[clientfd],client_name);
                    /* welcome the new user: create welcome string and send */
					/* welcome string: "SERVER> Welcome xxx.xxx.xxx.xxx to the chat server\n"
					   "Type 'close' to disconnect; 'shtudown' to stop\n" */
                    strcpy(buffer,"SERVER> Welcome ");
					strcat(buffer,connection[clientfd]);
					strcat(buffer," to the chat server\n");
					strcat(buffer,"SERVER> Type 'close' to disconnect; 'shutdown' to stop\n");
					send(clientfd,buffer,strlen(buffer),0);
                    /*Tell everyone about the new user*/
                    strcpy(buffer,"SERVER> ");
					strcat(buffer,connection[clientfd]);
					strcat(buffer," has joined the server\n");
                    /* loop from the server's file descriptor up,
					   sending the string to each active connection */
                    for(x=sockfd+1;x<max_connect;x++) {
                        if(FD_ISSET(x,&main_fd)) {
                            send(x,buffer,strlen(buffer),0);
                        }
                    }
                    printf("%s",buffer); 
                } else {
                    /*Check the current buffer for the current fd*/
                    r = recv(fd,buffer,BUFSIZ,0);
                    /*If nothing is received disconnect them*/
                    if (r<1) {
                        FD_CLR(fd, &main_fd);		// clear the file descriptor
                        close(fd); //close the file descriptor socket
                        /*Tell others that the user has disconnected*/
                        strcpy(buffer,"SERVER> ");
						strcat(buffer,connection[fd]);
						strcat(buffer," disconnected\n");
                        /* loop through all connections (not the server) to send the string */
                        for( x=sockfd+1; x<max_connect; x++ ) {
							if( FD_ISSET(x,&main_fd) )
							{
								send(x,buffer,strlen(buffer),0);
							}
						}
						/* output the string locally */
						printf("%s",buffer);
                    }
                    /* at this point, the connected client has text to share */
					/* share the incoming text with all connections */
                    else {
                        buffer[r] = '\0';
                        if( strcmp(buffer,"shutdown\n")==0 ) {
							done = TRUE;
						} else {
                            strcpy(sendstr,connection[fd]);
							strcat(sendstr,"> ");
							strcat(sendstr,buffer);
                            /* loop through all connections, but not the server */
                            for( x=sockfd+1; x<max_connect; x++ ) {
								/* check for an active file descriptor */
								if( FD_ISSET(x,&main_fd) )
								{
									/* send the built string */
									send(x,sendstr,strlen(sendstr),0);
								}
							}
                            //print the string to the server as well
                            printf("%s",sendstr);
                        }
                    }   
                }
            }
        }
    }
    puts("SERVER> Shutdown issued; cleaning up");
    close(sockfd);
	freeaddrinfo(server);
	return(0);
}
