/**************************************************************
rdt-part3.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
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
	char payload[PAYLOAD]; // ackpkt has 0 payload
}Packet;
u32b_t nextseqnum = 1; // 0 is kept for empty cell in seqlist
u32b_t currentwindowbase = 1;
u32b_t expectedseqnum = 1;

Packet makeDataPacket(u32b_t seq, char *payload, int length);
Packet makeACKPacket(u32b_t seq);
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

	// set number of packets every round
	int numpkt = length/PAYLOAD;
	int rounds = (numpkt + W - 1)/W;
	int *N = (int *)malloc(rounds*sizeof(int));
	if(rounds > 1){
		memset(N, W, rounds*sizeof(int)); // set all entries except last one of N into W
		N[rounds-1] = N - rounds*W;
	}else N[0] = numpkt;

	int remain = length;
	int copylength = 0;
	u32b_t *seqList = (u32b_t *)malloc(W*sizeof(u32b_t)); // keep track of seqs of current window
	int *ackList = (int *)malloc(W*sizeof(int)); // ACK situation of seqList's corresponding entry
	char *smallmsg = (char *)malloc(PAYLOAD*sizeof(char));

	for(int r=0; r<rounds; ++r){
		// Every rounds, at most W packets will be sent
		bzero(seqList, W);
		bzero(ackList, W);
		Packet *pktList = (Packet *)malloc(N[r]*sizeof(Packet)); // store pkts this round for ACK checking

		// Send all pkt within current window
		for(int i=0; i<N[r]; ++i){
			copylength = remainlength>PAYLOAD? PAYLOAD:remainlength;
			bzero(smallmsg, PAYLOAD);
			memcpy(smallmsg, msg, copylength);

			Packet sendpkt = makeDataPacket(nextseqnum, smallmsg, copylength);
			sendpkt.checksum = checksum((u8b_t *)&sendpkt, sizeof sendpkt);

			seqList[i] = nextseqnum;
			nextseqnum++;
			remainlength -= copylength;

			pktList[i] = sendpkt;
			udt_send(fd, sendpkt, copylength, 0);
			printf("[rdt_send] send datapkt... seq=%d\n", sendpkt.sequence);///
		}
		// All packet at this round have been sent, start timer
		fd_set master, read_fds;
		struct timeval timer; // for timeout
		int status, nbytes;
		u16b_t checkvalue;

		FD_ZERO(&master);
		FD_ZERO(&read_fds);
	    FD_SET(fd, &master);

	    // Wait for ACK for current window
		while(1){
			timer.tv_sec = 0;
			timer.tv_usec = TIMEOUT;
			read_fds = master;
			status = select(fd+1, &read_fds, NULL, NULL, &timer);

			if(status == -1){
				perror("[rdt_send] error_select:");
				return -1;
			}else if(status == 0){
				// Timeout: retransmit
				//printf("[rdt_send]timeout, retransmit... seq=%d\n", sendpkt.sequence);
				int t = 0;
				while(seqList[t]){ //empty cell will be 0
					if(!ackList[t]){
						// retransmit this packet
						printf("[rdt_send] retransmit pkt, seq=%d\n", pktList[t].sequence);
						udt_send(fd, &(pktList[t]), sizeof (pktList[t]), 0);
					}
					++i;
				}
			}else if(status > 0){
				// packet arrive
				Packet recvpkt;
				nbytes = recv(fd, &recvpkt, sizeof recvpkt, 0);

				if(nbytes <= 0){
					// got error or connection closed by client
					if(nbytes == 0) printf("[rdt_send()]nbytes=0");
					else perror("[rdt_send()]recv_error:");
					return -1;
				}else if(isNotCorrupt(&recvpkt, sizeof recvpkt)){
					if(isACK(&recvpkt)){
						if(isACKbetween(&recvpkt, seqList[0], seqList[N[r]-1])){
							if(isACKbetween(&recvpkt, seqList[0], seqList[N[r]-2])){
								// set all pkt inbetween be acked (accumulate ACK)
								for(int j=0; j<(recvpkt.sequence-seqList[0]); ++j){
									ackList[j] = 1;
								}
							}else{
								// recvpkt.seq == seqList[N[r]-1]: stop timer
								break;
							}
						}
						printf("[rdt_send] pkt received is not within range\n");
						continue;
					}else{
						// receive a datapkt: ack it to avoid deadlock
						Packet ackpkt = makeACKPacket(recvpkt.sequence);
						ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

						udt_send(fd, &ackpkt, sizeof ackpkt, 0);
						printf("[rdt_send] sender receive a datapkt: ack it\n");
					}
				}
				printf("[rdt_send] pkt received is corrupt\n");
			}
		}

		// timer is stopped; all pkt in window this round are acked, slide window
		currentwindowbase = nextseqnum;
		printf("[rdt_send] all pkt in window this round are acked, slide window......\n");
	}

	return length;
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
	int nbytes;

	while(1){
		Packet recvpkt;
		nbytes = recv(fd, &recvpkt, sizeof recvpkt, 0);

		if(nbytes <= 0){
			// got error or connection closed by client
			if(nbytes == 0) printf("[rdt_recv()] nbytes=0");
			else perror("[rdt_recv()] error_recv:");

			return -1;
		}else if(isNotCorrupt(&recvpkt, sizeof recvpkt)){
			if(!isACK(&recvpkt)){
				// receive a datapkt: check whether the DATA packet is expected
				printf("[rdt_recv] receive a datapkt...... expect seq: %d\n", expectedseqnum);///

				if(recvpkt.sequence == expectedseqnum){
					Packet ackpkt = makeACKPacket(expectedseqnum);
					ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

					udt_send(fd, &ackpkt, sizeof ackpkt, 0);
					printf("[rdt_recv] expected datapkt!!! send ack...... type=%u, seq=%u\n", ackpkt.type, ackpkt.sequence);////

					break;
				}else{
					Packet ackpkt = makeACKPacket(expectedseqnum-1);
					ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

					udt_send(fd, &ackpkt, sizeof ackpkt, 0);
					printf("[rdt_recv] wrong datapkt!!! resend ack...... type=%u, seq=%u\n", ackpkt.type, ackpkt.sequence);////
				}
			}
			printf("[rdt_recv] get ackpkt: ignore it.\n");
		}else{
			// corrupted, send duplicate ACK
			ackpkt = makeACKPacket(expectedseqnum-1);
			ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

			printf("[rdt_recv] pkt is corrupted: resend ACK type=%u, seq=%u\n", ackpkt.type, ackpkt.sequence);////
			udt_send(fd, &ackpkt, sizeof ackpkt, 0);
		}
	}

	memcpy(msg, &(recvpkt.payload), sizeof recvpkt.payload); // retrieve massage
	expectedseqnum++;
	// record peer's currentwindowbase
	if(expectedseqnum == (currentwindowbase+W)) currentwindowbase = expectedseqnum;

	return strlen(recvpkt.payload);
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
			break;
		}else if(status > 0){
			// packet arrive
			Packet recvpkt;
			nbytes = recv(fd, &recvpkt, sizeof recvpkt, 0);
			checkvalue = checksum((u8b_t *)&recvpkt, nbytes);

			if((nbytes > 0)&&(checkvalue == 0)&&(recvpkt.type == 1)){
				if((recvpkt.sequence >= (currentwindowbase-W)) && (recvpkt.sequence < currentwindowbase)){
					// receieve datapkt within previous window: resend ACK
					Packet ackpkt = makeACKPacket(currentwindowbase - 1);
					ackpkt.checksum = checksum((u8b_t *)&ackpkt, sizeof ackpkt);

					udt_send(fd, &ackpkt, sizeof ackpkt, 0);
					printf("[rdt_close] resend ACK: currentbase=%u, type=%u, seq=%u\n", currentwindowbase, ackpkt.type, ackpkt.sequence);////
				}
			}
		}
	}

	// Only when TWAIT is done will this be executed.
	printf("CLOSE!\n");
    if(close(fd) < 0){
    	perror("[rdt_close]close:");
    }
    return 0;
}

Packet makeDataPacket(u32b_t seq, char *payload, int length){
	Packet pkt;
	pkt.type = 1;
	pkt.sequence = seq;
	pkt.checksum = 0; // checksum should be assigned later on.
	memcpy(&pkt.payload, payload, length);
	return pkt;
}

Packet makeACKPacket(u32b_t seq){
	Packet pkt;
	pkt.type = 2;
	pkt.sequence = seq;
	pkt.checksum = 0;
	return pkt;
}

int isNotCorrupt(Packet *pkt, int length){
	if(checksum((u8b_t *)&pkt, length)) return 1;
	return 0; //false: pkt is corrupt
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
