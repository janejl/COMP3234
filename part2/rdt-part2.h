/**************************************************************
rdt-part2.h
Student name: JIANG Ling
Student No. : 3035030066
Date and version: 17/04/2015
Development platform: Mac OS
Development language: C
Compilation:
	Can be compiled with
*****************************************************************/

#ifndef RDT2_H
#define RDT2_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#define PAYLOAD 1000		//size of data payload of the RDT layer
#define TIMEOUT 50000		//50 milliseconds
#define TWAIT 10*TIMEOUT	//Each peer keeps an eye on the receiving  
							//end for TWAIT time units before closing
							//For retransmission of missing last ACK

//----- Type defines ----------------------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word 

extern float LOSS_RATE, ERR_RATE;


/* this function is for simulating packet loss or corruption in an unreliable channel */
/***
Assume we have registered the target peer address with the UDP socket by the connect()
function, udt_send() uses send() function (instead of sendto() function) to send 
a UDP datagram.
***/
int udt_send(int fd, void * pkt, int pktLen, unsigned int flags) {
	double randomNum = 0.0;

	/* simulate packet loss */
	//randomly generate a number between 0 and 1
	randomNum = (double)rand() / RAND_MAX;
	if (randomNum < LOSS_RATE){
		//simulate packet loss of unreliable send
		printf("WARNING: udt_send: Packet lost in unreliable layer!!!!!!\n");
		return pktLen;
	}

	/* simulate packet corruption */
	//randomly generate a number between 0 and 1
	randomNum = (double)rand() / RAND_MAX;
	if (randomNum < ERR_RATE){
		//clone the packet
		u8b_t errmsg[pktLen];
		memcpy(errmsg, pkt, pktLen);
		//change a char of the packet
		int position = rand() % pktLen;
		if (errmsg[position] > 1) errmsg[position] -= 2;
		else errmsg[position] = 254;
		printf("WARNING: udt_send: Packet corrupted in unreliable layer!!!!!!\n");
		return send(fd, errmsg, pktLen, 0);
	} else 	// transmit original packet
		return send(fd, pkt, pktLen, 0);
}

/* this function is for calculating the 16-bit checksum of a message */
/***
Source: UNIX Network Programming, Vol 1 (by W.R. Stevens et. al)
***/
u16b_t checksum(u8b_t *msg, u16b_t bytecount)
{
	u32b_t sum = 0;
	u16b_t * addr = (u16b_t *)msg;
	u16b_t word = 0;
	
	// add 16-bit by 16-bit
	while(bytecount > 1)
	{
		sum += *addr++;
		bytecount -= 2;
	}
	
	// Add left-over byte, if any
	if (bytecount > 0) {
		*(u8b_t *)(&word) = *(u8b_t *)addr;
		sum += word;
	}
	
	// Fold 32-bit sum to 16 bits
	while (sum>>16) 
		sum = (sum & 0xFFFF) + (sum >> 16);
	
	word = ~sum;
	
	return word;
}

//----- Type defines ----------------------------------------------------------

// define your data structures and global variables in here
struct hostent *he;						// self declared var: host
struct sockaddr_in peer_addr;			// self-declared var: peer address
typedef struct PACKET{
	u32b_t type; // 2:ACK, 1:Data
	u32b_t sequence;
	u16b_t checksum;
	int length; // length of data loaded
	char payload[PAYLOAD]; // ACK packet has 0 payload
}Packet;
int currentSequence = 0; // 0 or 1
int recvSequence = 0; // 0 or 1, expected receiving sequence
int lastSequence = 1;
int lastACKnum = 1;

void makeDataPacket(Packet * pkt, u32b_t seq, char *payload, int length);
void makeACKPacket(Packet *pkt, u32b_t seq);

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
	//same as part 1
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
	//same as part 1
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
   return	-> 0 on success, -1 on error
*/
int rdt_target(int fd, char * peer_name, u16b_t peer_port){
	//same as part 1
    if ((he = gethostbyname(peer_name)) == NULL) {
        return  -1;
    }
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    peer_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(peer_addr.sin_zero), '\0', 8);

	if(connect(fd, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr)) == -1){
		return -1;
	}

    return 0;
}

void makeDataPacket(Packet * pkt, u32b_t seq, char *payload, int length){
	pkt->type = 1;
	pkt->sequence = seq;
	pkt->checksum = 0; // checksum should be assigned later on.
	pkt->length = length;
	memcpy(pkt->payload, payload, length);
}

void makeACKPacket(Packet *pkt, u32b_t seq){
	pkt->type = 2;
	pkt->sequence = seq;
	pkt->checksum = 0;
	pkt->length = 0;
}

/* Application process calls this function to transmit a message to
   target (rdt_target) remote process through RDT socket.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
   return	-> size of data sent on success, -1 on error
*/
int rdt_send(int fd, char * msg, int length){
//implement the Stop-and-Wait ARQ (rdt3.0) logic

	Packet sendpkt;
	bzero(&sendpkt, sizeof sendpkt);
	//printf("app msg %d %s\n", length, msg);
	makeDataPacket(&sendpkt, currentSequence, msg, length);
	sendpkt.checksum = checksum((u8b_t *)&sendpkt, sizeof sendpkt);

	udt_send(fd, &sendpkt, sizeof sendpkt, 0);
	printf("[rdt_send] send datapkt..... seq=%d\n", sendpkt.sequence);///

	fd_set master, read_fds;
	struct timeval timer; // for timeout
	int status;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);
    FD_SET(fd, &master);

	while(1){
		timer.tv_sec = 0;
		timer.tv_usec = TIMEOUT;
		read_fds = master;
		status = select(fd+1, &read_fds, NULL, NULL, &timer);

		if(status == -1){
			perror("[rdt_send] select:");
			return -1;
		}else if(status == 0){
			// Timeout: retransmit
			udt_send(fd, &sendpkt, sizeof sendpkt, 0);
			printf("[rdt_send] timeout:resend... seq=%u\n", sendpkt.sequence);////

		}else if(status > 0){
			// packet arrive
			Packet recvpkt;
			bzero(&recvpkt, sizeof recvpkt);
			int nbytes = recv(fd, &recvpkt, sizeof recvpkt, 0);
			printf("[rdt_send] receive pkt...... seq=%u, type=%u, size=%lu\n", recvpkt.sequence, recvpkt.type, strlen(recvpkt.payload));///

			if(nbytes <= 0){
				printf("[rdt_send] got error or connection closed by client.\n");
				return -1;
			}else{
				// got packet from peer
				u16b_t checkvalue = checksum((u8b_t *)&recvpkt, sizeof recvpkt);

				if(checkvalue==0){
					//packet not corrupt: check type
					if((recvpkt.type == 2)&&(recvpkt.sequence == currentSequence)){
						//ackpkt
						// got expected ackpkt from peer
						printf("[rdt_send]expected ACK!!!\n");///

						//lastACKnum = currentSequence;
						currentSequence = abs(currentSequence - 1); // 0 or 1
						return length;
					}else if((recvpkt.type == 1)&&(recvpkt.sequence == lastACKnum)){
						// got datapkt from peer: ack it (especially in handshaking)
						Packet ackpkt;
						bzero(&ackpkt, sizeof ackpkt);
						makeACKPacket(&ackpkt, recvpkt.sequence);
						ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

						udt_send(fd, &ackpkt, sizeof ackpkt, 0);
						printf("[rdt_send] get datapkt:ack...seq=%u\n", ackpkt.sequence);///
					}else{
						printf("[rdt_send] wrong ackpkt...\n");
					}
				}else{
					printf("[rdt_send] received pkt is corrupt...\n");
				}

			}
		}
	}
}

/* Application process calls this function to wait for a message 
   from the remote process; the caller will be blocked waiting for
   the arrival of the message.
   msg		-> pointer to the receiving buffer
   length	-> length of receiving buffer
   return	-> size of data received on success, -1 on error
*/
int rdt_recv(int fd, char * msg, int length){
	//implement the Stop-and-Wait ARQ (rdt3.0) logic

	while(1){
		Packet recvpkt;
		bzero(&recvpkt, sizeof recvpkt);
		int nbytes = recv(fd, &recvpkt, sizeof recvpkt, 0);

		if(nbytes <= 0){
			printf("[rdt_recv] got error or connection closed by client.\n");
			return -1;
		}else{
			// got data from peer
			u16b_t checkvalue = checksum((u8b_t *)&recvpkt, sizeof recvpkt);

			if(checkvalue != 0){
				// corrupted, send duplicate ACK
				Packet ackpkt;
				bzero(&ackpkt, sizeof ackpkt);
				makeACKPacket(&ackpkt, lastSequence);
				ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

				udt_send(fd, &ackpkt, sizeof ackpkt, 0);
				printf("[rdt_recv] corrupted packet: resend ACK...seq=%u\n", ackpkt.sequence);////

			}else if(recvpkt.type == 1){
				printf("[rdt_recv] recv a datapkt...last & expect seq=%u & %u\n", lastSequence, recvSequence);///

				// check whether the DATA packet is expected
				if(recvpkt.sequence == recvSequence){
					Packet ackpkt;
					bzero(&ackpkt, sizeof ackpkt);
					makeACKPacket(&ackpkt, recvSequence);
					ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

					udt_send(fd, &ackpkt, sizeof ackpkt, 0);
					printf("[rdt_recv] correct datapkt: send ack......seq=%u\n", ackpkt.sequence);////

					memcpy(msg, recvpkt.payload, sizeof recvpkt.payload); // get massage
					printf("[rdt_recv] size of received...............siz=%d\n", nbytes);///
					lastSequence = recvSequence;
					lastACKnum = recvSequence;
					recvSequence = abs(recvSequence - 1);// 0 or 1

					return recvpkt.length; // be careful here!!!
				}else{
					Packet ackpkt;
					bzero(&ackpkt, sizeof ackpkt);
					makeACKPacket(&ackpkt, lastSequence);
					ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

					udt_send(fd, &ackpkt, sizeof ackpkt, 0);
					printf("[rdt_recv] wrong datapkt: resend ack......seq=%u\n", ackpkt.sequence);////
				}
			}else if(recvpkt.type == 2){
				// may get ack. Discard it.
				printf("[rdt_recv] get ackpkt: ignore it.\n");
			}
		}
	}
}

/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd){
//implement the Stop-and-Wait ARQ (rdt3.0) logic

	// Prepare for select(): fd_set.
	fd_set master, read_fds;
	struct timeval timer; // for timeout
	u16b_t checkvalue;
	int status, nbytes;

	FD_ZERO(&master); // clear the master and temp sets
	FD_ZERO(&read_fds);
	FD_SET(fd, &master);

	while(1){
		timer.tv_sec = 0;
		timer.tv_usec = TWAIT;
		read_fds = master;
		status = select(fd+1, &read_fds, NULL, NULL, &timer);

		if(status == 0){
			// Timeout: close
			printf("CLOSE!\n");
		    if(close(fd) < 0){
		    	perror("[rdt_close]close:");
		    }
		    return 0;
		}else if(status > 0){
			// packet arrival
			Packet recvpkt;
			bzero(&recvpkt, sizeof recvpkt);
			nbytes = recv(fd, &recvpkt, sizeof recvpkt, 0);
			checkvalue = checksum((u8b_t *)&recvpkt, nbytes);

			if((nbytes > 0)&&(checkvalue == 0)&&(recvpkt.type == 1)&&(recvpkt.sequence == lastACKnum)){
				// resend ACK
				Packet ackpkt;
				bzero(&ackpkt, sizeof ackpkt);
				makeACKPacket(&ackpkt, lastACKnum);
				ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

				printf("[rdt_close]resend ACK: type=%u, seq=%u\n", ackpkt.type, ackpkt.sequence);////
				udt_send(fd, &ackpkt, sizeof ackpkt, 0);
			}
		}
	}
}

#endif
