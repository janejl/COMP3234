/**************************************************************
rdt-part1.h
Student name: JIANG LING
Student No. : 3035030066
Date and version: 30/03/2015  Version 2
Development platform: Mac OS X
Development language:
Compilation:
	Can be compiled with 
*****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

struct hostent *he;						// self declared var: host
struct sockaddr_in peer_addr;			// self-declared var: peer address

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket. 
   return	-> the socket descriptor on success, -1 on error
*/
int rdt_socket() {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        return -1;
    }
    return sockfd;
}

/* Application process calls this function to specify the IP address
   and port number used by itself and assigns them to the RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(my_addr.sin_zero), '\0', 8);
    
    if (bind(fd, (struct sockaddr * )&my_addr, sizeof(struct sockaddr)) == -1) {
        return -1;
    }
    
    return 0;
}

/* Application process calls this function to specify the IP address
   and port number used by remote process and associates them to the 
   RDT socket.
** connect() is used here to sets the receiver's address, though it is little
   sense to use connect() as we have already use sendto().
   return	-> 0 on success, -1 on error
*/
int rdt_target(int fd, char * peer_name, u16b_t peer_port){
   
    if ((he = gethostbyname(peer_name)) == NULL) {
        return  -1;
    }
    
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    peer_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(peer_addr.sin_zero), '\0', 8);
    
    /*if (bind(fd, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr)) == -1) {
        return -1;
    }*/
	if(connect(fd, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr)) == -1){
		return -1;
	}

    return 0;
}

/* Application process calls this function to transmit a message to
   target (rdt_target) remote process through RDT socket.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
 
   return	-> size of data sent on success, -1 on error
*/
int rdt_send(int fd, char * msg, int length){
    if (sendto(fd, msg, length, 0, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in)) == -1) {
        return -1;
    }
    return length;
}

/* Application process calls this function to wait for a message 
   from the remote process; the caller will be blocked waiting for
   the arrival of the message.
   msg		-> pointer to the receiving buffer
   length	-> length of receiving buffer
 
   return	-> size of data received on success, -1 on error
*/
int rdt_recv(int fd, char * msg, int length){
    int numbytes;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    
    if ((numbytes = recvfrom(fd, msg, length, 0, (struct sockaddr *)&peer_addr, &sin_size)) == -1) {
        return -1;
    }
    return numbytes;
}

/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd){
    close(fd);
    return 0;////
}

#endif
