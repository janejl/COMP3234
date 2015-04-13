/**************************************************************
rdt-part2.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
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
	// Header
	int type; // 2:ACK, 1:Data
	int sequence;
	int checksum;
	// Payload
	char * payload; // ACK packet has 0 payload
} Packet;
int currentSequence = 0; // 0 or 1
int recvSequence = 0; // 0 or 1, expected receiving sequence
int lastSequence = 1;

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

int packetChecksum(Packet packet, int length);

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

/* An assisting funtion to check the checksum for the whole packet
 * including both types of packets.
 * length -- total length (byte) of packet
 */
int packetChecksum(Packet packet, int length){
	char *packBuff, *typeBuff, *seqBuff, *checkBuff;
	packBuff = (char *)malloc((PAYLOAD+12) * sizeof(char));
	typeBuff = (char *)malloc(4 * sizeof(char));
	seqBuff = (char *)malloc(4 * sizeof(char));
	checkBuff = (char *)malloc(4 * sizeof(char));

	sprintf(typeBuff, "%d", packet.type);
	sprintf(seqBuff, "%d", packet.sequence);
	sprintf(checkBuff, "%d", packet.checksum);

	if(packet.type == 1){
		// for data packet
		sprintf(packBuff, "%s%s%s%s", typeBuff, seqBuff, checkBuff, packet.payload);
	}else{
		// for ACK packet
		sprintf(packBuff, "%s%s%s", typeBuff, seqBuff, checkBuff);
	}

	int result = (int)checksum((u8b_t *)packBuff, length);

	return result;
}

/* Application process calls this function to transmit a message to
   target (rdt_target) remote process through RDT socket.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
   return	-> size of data sent on success, -1 on error
*/
int rdt_send(int fd, char * msg, int length){
//implement the Stop-and-Wait ARQ (rdt3.0) logic
	printf("rdt_send start...");
	Packet sendpkt, recvpkt;
	recvpkt.payload = (char*) malloc(PAYLOAD * sizeof(char));
	int pktlength = length + 12; //type + seq + checks = 12 bytes

	sendpkt.type = 1;
	sendpkt.sequence = currentSequence;
	sendpkt.payload = msg;
	sendpkt.checksum = 0;
	sendpkt.checksum = packetChecksum(sendpkt, pktlength);

	//int udt_send(int fd, void * pkt, int pktLen, unsigned int flags)
	printf("[rdt_send]:udt_send start... msn packet.");///
	udt_send(fd, &sendpkt, pktlength, 0);
	printf("[rdt_send]:udt_send end... msn packet.");///

	// Prepare for select(): fd_set.
	fd_set master, read_fds;
	struct timeval timer; // for timeout
	int fd_max, status, nbytes, checkvalue;

	FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(fd, &master);
    fd_max = fd; // keep track of the biggest fd

	while(1){
		read_fds = master;
		timer.tv_sec = 0;
		timer.tv_usec = TIMEOUT;
		status = select(fd_max+1, &read_fds, NULL, NULL, &timer);

		if(status == -1){
			perror("[rdt_send()]:select:");
			exit(4);
		}else if(status == 0){
			// Timeout --> retransmit
			//printf("[rdt_send]:while-loop: timeout, retransmit...");///
			udt_send(fd, &sendpkt, length, 0);
		}else if(status > 0){
			// packet arrival
			//get packet from readfd and check the corrupted
			nbytes = recv(fd, &recvpkt, sizeof(recvpkt), 0);

			if(nbytes <= 0){
				// got error or connection closed by client
				//0 when the peer has performed an orderly shutdown
				if(nbytes == 0){
					// connection closed
					printf("nbytes=0");
				}else{
					perror("[rdt_send()]:recv:");
				}
				//close(fd);
				//FD_CLR(fd, &master);
			}else{
				// got data from peer
				checkvalue = packetChecksum(recvpkt, nbytes);

				if(checkvalue != recvpkt.checksum){
					// do nothing, keep waiting
				}else if(recvpkt.type == 2){
					// check whether the ACK is expected or not
					// if not, do nothing, keep waiting
					if(recvpkt.sequence == currentSequence){
						break;
					}
				}else if(recvpkt.type == 1){
					// may get data due to previous corrupted connection
					// Discard it and do nothing right now.
				}
			}
		}
	}
	printf("[rdt_send]:expected ACK comes... msn packet.");///
	// Only when the expected ACK comes will this be executed.
	currentSequence = abs(currentSequence - 1); // 0 or 1
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
	//implement the Stop-and-Wait ARQ (rdt3.0) logic
	printf("rdt_recv() start...");
	int nbytes;
	Packet recvpkt;
	recvpkt.payload = (char*) malloc(PAYLOAD * sizeof(char));
	int checkvalue;

	Packet ackpkt;
	ackpkt.type = 2;
	ackpkt.sequence = recvSequence;
	ackpkt.checksum = 0;
	ackpkt.payload = NULL;
	ackpkt.checksum = packetChecksum(ackpkt, 12);

	// for debug
	send(fd, &ackpkt, sizeof ackpkt, 0);

	//printf("[rdt_recv]:while-loop start...");///
	while(1){
		nbytes = recv(fd, recvpkt.payload, sizeof recvpkt.payload, 0);

		if(nbytes <= 0){
			// got error or connection closed by client
			//0 when the peer has performed an orderly shutdown
			if(nbytes == 0){
				// peer's connection closed
				printf("nbytes=0");
			}else{
				perror("recv");
			}
		}else{
			// got data from peer
			checkvalue = packetChecksum(recvpkt, length+12);

			if(checkvalue != recvpkt.checksum){
				// corrupted, send duplicate ACK
				ackpkt.sequence = lastSequence;
				udt_send(fd, &ackpkt, sizeof ackpkt, 0);
			}else if(recvpkt.type == 1){
				// check whether the DATA packet is expected
				if(recvpkt.sequence == recvSequence){
					ackpkt.sequence = recvSequence;
					udt_send(fd, &ackpkt, sizeof ackpkt, 0);
					break;
				}
			}else if(recvpkt.type == 1){
				// may get data due to previous corrupted connection
				// Discard it and do nothing right now.
			}
		}
	}
	//printf("[rdt_recv]:while-loop end...");///
	msg = recvpkt.payload; // get massage
	lastSequence = recvSequence;
	recvSequence = abs(recvSequence - 1);// 0 or 1

	return sizeof(recvpkt.payload);
}

/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd){
//implement the Stop-and-Wait ARQ (rdt3.0) logic
// use select() on a UDP socket, the system could easily check the arrival of
// packet and timeout at the same time.
	printf("rdt_close() start...");
	Packet ackpkt, recvpkt;
	recvpkt.payload = (char*) malloc(PAYLOAD * sizeof(char));

	ackpkt.type = 1;
	ackpkt.sequence = lastSequence;
	ackpkt.checksum = 0;
	ackpkt.payload = NULL;

	// Prepare for select(): fd_set.
	fd_set master, read_fds;
	struct timeval timer; // for timeout
	int fd_max; // max num of fd
	u16b_t checkvalue;
	int status;
	int nbytes;

	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
	FD_SET(fd, &master);
	fd_max = fd; // keep track of the biggest fd

	//printf("[rdt_close]:while-loop start...");///

	while(1){
		read_fds = master;
		timer.tv_sec = 0;
		timer.tv_usec = TWAIT;
		status = select(fd_max+1, &read_fds, NULL, NULL, &timer);

		if(status == 0){
			// Timeout --> close
			break;
		}else if(status > 0){
			// packet arrival
			// get packet from readfd and check the corrupted
			nbytes = recv(fd, &recvpkt, sizeof recvpkt, 0);
			checkvalue = packetChecksum(recvpkt, nbytes);

			if((nbytes > 0)&&(checkvalue == recvpkt.checksum)&&(recvpkt.type == 1)){
				// resend ACK
				ackpkt.sequence = lastSequence;
				udt_send(fd, &ackpkt, sizeof ackpkt, 0);
			}
		}
	}
	//printf("[rdt_close]:while-loop end...");///

	// Only when TWAIT is done will this be executed.
    if(close(fd) < 0){
    	perror("close");
    }
    return 0;
}

#endif
