/**************************************************************
rdt-part3.h
Student name: JIANG Ling
Student No. : 3035030066
Date and version: 17/04/2015
Development platform: Mac OS
Development language: C
Compilation:
	Can be compiled with
*****************************************************************/

#ifndef RDT3_H
#define RDT3_H

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
#define W 5					//For Extended S&W - define pipeline window size


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
	u32b_t sequence; // should be larger than window size W
	u16b_t checksum;
	int length;
	char payload[PAYLOAD]; // ackpkt has 0 payload
}Packet;
u32b_t nextseqnum = 1; // 0 is kept for empty cell in seqlist
u32b_t lastwindowbase = 1;
u32b_t currentwindowbase = 1;

u32b_t expectedseqnum = 1;


void makeDataPacket(Packet *pkt, u32b_t seq, char *payload, int length);
void makeACKPacket(Packet *pkt, u32b_t seq);
int isNotCorrupt(Packet *pkt, int length);
int isACK(Packet *pkt);
int isACKbetween(Packet *pkt, u32b_t left, u32b_t right);

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

/* Application process calls this function to transmit a message to
   target (rdt_target) remote process through RDT socket; this call will
   not return until the whole message has been successfully transmitted
   or when encountered errors.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
   return	-> size of data sent on success, -1 on error
*/
int rdt_send(int fd, char * msg, int length){
//implement the Extended Stop-and-Wait ARQ logic

	// set number of packets
	int numpkt = (length+PAYLOAD-1)/PAYLOAD;
	if(numpkt > W) {
		printf("~~~~~~~msg overflow the window!!!\n");
		return -1;
	}// msg length is supposed to be <= PAYLOAD*W

	printf("****************   Curent Window: [%u, %u]   ****************\n", currentwindowbase, currentwindowbase+numpkt-1);
	printf("[rdt_send] length=%d, Number of packets=%d\n", length, numpkt);/////

	int remainlength = length;
	int copylength = 0;
	char *smallmsg = (char *)malloc(PAYLOAD*sizeof(char));
	int ackList[W] = {0}; // ACK situation of current window
	Packet *pktList = (Packet *)malloc(numpkt*sizeof(Packet)); // store pkts this round for ACK checking

	// Send all pkts within current window
	for(int i=0; i<numpkt; ++i){
		if(remainlength > PAYLOAD) copylength = PAYLOAD;
		else copylength = remainlength;

		bzero(smallmsg, PAYLOAD);
		memcpy(smallmsg, msg, copylength);
		Packet sendpkt;
		bzero(&sendpkt, sizeof sendpkt);
		makeDataPacket(&sendpkt, nextseqnum, smallmsg, copylength);
		sendpkt.checksum = checksum((u8b_t *)&sendpkt, sizeof sendpkt);

		++nextseqnum;
		remainlength -= copylength;
		msg += copylength;

		pktList[i] = sendpkt;
		udt_send(fd, &sendpkt, sizeof sendpkt, 0);
		printf("[rdt_send] send datapkt... seq=%u, length=%d, nextseq=%u\n", sendpkt.sequence, sendpkt.length, nextseqnum);///
	}

	// All packet at this round have been sent, start timer
	fd_set master, read_fds;
	struct timeval timer; // for timeout
	int status;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(fd, &master);

	bzero(&ackList, W);
	// Wait for ACK of current window
	while(1){
		timer.tv_sec = 0;
		timer.tv_usec = TIMEOUT;
		read_fds = master;
		status = select(fd+1, &read_fds, NULL, NULL, &timer);

		if(status == -1){
			perror("[rdt_send] error_select:");
			return -1;
		}else if(status == 0){
			// Timeout: retransmit all unacked pkts within current window
			for(int i=0; i<numpkt; ++i){
				if(ackList[i]==0){ //unack = 0; acked = 1
					// retransmit this packet
					printf("[rdt_send] Tout resnd pkt, seq=%u, length=%d\n", pktList[i].sequence,  pktList[i].length);
					udt_send(fd, &(pktList[i]), sizeof pktList[i], 0);
				}
			}

		}else if(status > 0){
			// packet arrive
			Packet recvpkt;
			bzero(&recvpkt, sizeof recvpkt);
			int nbytes = recv(fd, &recvpkt, sizeof recvpkt, 0);

			if(nbytes <= 0){
				printf("[rdt_send] got error or connection closed by client\n");
				return -1;
			}else if(isNotCorrupt(&recvpkt, sizeof recvpkt)){
				if(isACK(&recvpkt)){
					printf("[rdt_send] got ackpkt......seq=%u\n", recvpkt.sequence);
					if(isACKbetween(&recvpkt, currentwindowbase, currentwindowbase+numpkt-1)){
						if(isACKbetween(&recvpkt, currentwindowbase,  currentwindowbase+numpkt-2)){
							// set all pkt inbetween be acked (accumulate ACK)
							for(int j=0; j<=(recvpkt.sequence-currentwindowbase); ++j){
								ackList[j] = 1;
							}
						}else{
							// recvpkt.seq == (currentwindowbase+numpkt-1)
							// Stop timer; all pkt in window this round are acked, slide window
							lastwindowbase = currentwindowbase;
							currentwindowbase = nextseqnum;
							printf("[rdt_send] all pkt in window this round are acked, slide window......\n");
							printf("****************   Origin Window: [%u, %u]   ****************\n", lastwindowbase, currentwindowbase-1);
							return length;
						}
					}else{
						printf("[rdt_send] pkt received is not within range\n");
						continue;
					}
				}else{
					// sender receive a datapkt
					if(recvpkt.sequence < expectedseqnum){
						// receive a datapkt: ack it if it is retransmitted to avoid deadlock
						Packet ackpkt;
						bzero(&ackpkt, sizeof ackpkt);
						makeACKPacket(&ackpkt, recvpkt.sequence);
						ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

						udt_send(fd, &ackpkt, sizeof ackpkt, 0);
						printf("[rdt_send] sender receive a resent datapkt: ack it\n");
					}else if((recvpkt.sequence==1) && (strcmp(recvpkt.payload, "OKAY")==0)){
						// this is server's response during the handshaking
						// regard this as a ackpkt for previous sendpkt (current window).
						// Don't send back any ackpkt to make the peer hold on rdt_send() and wait for my side's rdt_recv()
						printf("OKAY................\n");

						// Stop timer; all pkt in window this round are acked, slide window
						lastwindowbase = currentwindowbase;
						currentwindowbase = nextseqnum;
						printf("[rdt_send] all pkt in window this round are acked, slide window......\n");
						printf("****************   Origin: [%u, %u]   ****************\n", lastwindowbase, currentwindowbase-1);
						return length;
					}else{
						printf("[rdt_send] sender receive a unexpc datapkt: drop it\n");
					}
				}
			}else{
				printf("[rdt_send] pkt received is corrupt\n");
			}
		}
	}
}

/* Application process calls this function to wait for a message of any
   length from the remote process; the caller will be blocked waiting for
   the arrival of the message. 
   msg		-> pointer to the receiving buffer
   length	-> length of receiving buffer
   return	-> size of data received on success, -1 on error
*/
int rdt_recv(int fd, char * msg, int length){
//implement the Extended Stop-and-Wait ARQ logic
	while(1){
		Packet recvpkt;
		bzero(&recvpkt, sizeof recvpkt);
		int nbytes = recv(fd, &recvpkt, sizeof recvpkt, 0);

		if(nbytes <= 0){
			printf("[rdt_recv] got error or connection closed by client\n");
			return -1;
		}

		if(isNotCorrupt(&recvpkt, sizeof recvpkt)){
			printf("[rdt_recv] ....receive a pkt...... type=%u, seq=%u, exp=%u, length=%d\n", recvpkt.type, recvpkt.sequence, expectedseqnum, recvpkt.length);

			if(!isACK(&recvpkt)){
				// receive a datapkt: check whether the DATA packet is expected
				if(recvpkt.sequence == expectedseqnum){
					Packet ackpkt;
					bzero(&ackpkt, sizeof ackpkt);
					makeACKPacket(&ackpkt, expectedseqnum);
					ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

					udt_send(fd, &ackpkt, sizeof ackpkt, 0);
					printf("[rdt_recv] expected datapkt! send ack......seq=%u\n", ackpkt.sequence);////

					memcpy(msg, recvpkt.payload, sizeof recvpkt.payload); // get massage
					expectedseqnum++;
					return recvpkt.length;
				}else{
					Packet ackpkt;
					bzero(&ackpkt, sizeof ackpkt);
					makeACKPacket(&ackpkt, expectedseqnum-1);
					ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

					udt_send(fd, &ackpkt, sizeof ackpkt, 0);
					printf("[rdt_recv] retraned datapkt! resend ack....seq=%u\n", ackpkt.sequence);////
				}
			}else{
				printf("[rdt_recv] receiver get ackpkt: ignore it.\n");
			}
		}else{
			// corrupted, send duplicate ACK
			Packet ackpkt;
			bzero(&ackpkt, sizeof ackpkt);
			makeACKPacket(&ackpkt, expectedseqnum-1);
			ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

			udt_send(fd, &ackpkt, sizeof ackpkt, 0);
			printf("[rdt_recv] pkt is corrupted: resend ack....seq=%u\n", ackpkt.sequence);////
		}
	}
}

/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd){
//implement the Extended Stop-and-Wait ARQ logic

	fd_set master, read_fds;
	struct timeval timer;
	u16b_t checkvalue;
	int status, nbytes;

	FD_ZERO(&master);
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
			// packet arrive
			Packet recvpkt;
			bzero(&recvpkt, sizeof recvpkt);
			nbytes = recv(fd, &recvpkt, sizeof recvpkt, 0);
			checkvalue = checksum((u8b_t *)&recvpkt, nbytes);

			if((nbytes > 0)&&(checkvalue == 0)&&(recvpkt.type == 1)){
				if((recvpkt.sequence >= expectedseqnum-W) && (recvpkt.sequence < expectedseqnum)){
					// receieve datapkt within previous window: resend ACK
					Packet ackpkt;
					bzero(&ackpkt, sizeof ackpkt);
					makeACKPacket(&ackpkt, expectedseqnum - 1);
					ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

					udt_send(fd, &ackpkt, sizeof ackpkt, 0);
					printf("[rdt_close] resend ACK: seq=%u\n", ackpkt.sequence);////
				}
			}
		}
	}
}


void makeDataPacket(Packet *pkt, u32b_t seq, char *payload, int length){
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

int isNotCorrupt(Packet *pkt, int length){
	if(checksum((u8b_t *)pkt, length)) return 0; //false: pkt is corrupt
	return 1;
}

int isACK(Packet *pkt){
	if(pkt->type == 2) return 1;
	return 0;
}

int isACKbetween(Packet *pkt, u32b_t left, u32b_t right){
	if(pkt->sequence >= left && pkt->sequence <= right ) return 1;
	return 0;
}

#endif
