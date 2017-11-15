#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include <algorithm>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */

/*
struct pkt {
int seqnum;
int acknum;
int checksum;
char payload[20];
};

struct msg {
char data[20];
};

*/

class MatchesID
{
    int _ID;

public:
    MatchesID(const int &ID) : _ID(ID) {}

    bool operator()(const pkt &packet) const
    {
        return packet.seqnum == _ID;
    }
};

struct MessageList{
	int time;	//Time when packet was sent to B
	int ack;
	pkt packet;
};

std::vector <struct MessageList> messagelist;
std::vector <struct pkt> messagelistrecv;
int nextseqnum = 0;
int send_base = 0;
//int recv_base = 0;
int expectedseq;
int N = getwinsize(); 
int RTT;
char messagebuffer[2000][20] = {0};

int checksum(struct pkt packet)
{
    int sum = 0;
    char *s = (char *) malloc(30 * sizeof(char));
    
    //printf("Here 1 \n");
    
    strncpy(s, packet.payload, 20);
    
    for(int i = 0; i<20 && *s != 0 ; i++)
    {
        sum += *s;
        s++;
    }
    sum += packet.seqnum;
    sum += packet.acknum;
    
    return sum;
}

void pushBacktoMessageslist(struct msg message, int counter){
	
	pkt newpkt;
	newpkt.seqnum = counter;
	newpkt.acknum = -1;
	strncpy(newpkt.payload, message.data, 20);
	newpkt.checksum = checksum(newpkt);
	
	struct MessageList *newMessage = (struct MessageList*) malloc (sizeof(struct MessageList));
	newMessage->packet = newpkt;
	newMessage->time = 0;
	newMessage->ack = -1;

	messagelist.push_back(*newMessage);
	printf("Packet added to list : %s \n", newMessage->packet.payload);
}

void A_output(struct msg message)
{
	pushBacktoMessageslist(message, nextseqnum);
	
	printf("N = %d, Base = %d, Next Seq = %d \n", N, send_base, nextseqnum);

	if((nextseqnum >= send_base) && (nextseqnum < (send_base + N))){
		messagelist[nextseqnum].time = get_sim_time();

		printf("Sending packet to B. Seqnum : %d  Message: %s   Starttime : %d \n", nextseqnum, messagelist[nextseqnum].packet.payload, messagelist.at(nextseqnum).time);
		tolayer3(0, messagelist.at(nextseqnum).packet);	
	}
	
	if(nextseqnum == send_base){
		printf("Starting timer for base : %d \n", send_base);
		starttimer(0, RTT);
	}
	nextseqnum++;

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	printf("ACK received !");
	int check_sum = checksum(packet);
	printf("Packer ACK : %d    Send_base : %d   Checksum : %d    Packet checksum : %d \n", packet.acknum, send_base, check_sum, packet.checksum);	
	
	printf("Here !\n");
	if((check_sum == packet.checksum) && (packet.acknum >= send_base) && (packet.acknum < (send_base + N))){
		//printf("ACK >= send_base\n  %d \n", (send_base + N));
		printf("Valid ACK received.\n");
		
		messagelist[packet.acknum].ack = packet.acknum;

		if(send_base == packet.acknum){
			stoptimer(0);
			send_base++;
			printf("send_base == acknum. \n");
			for(int i = send_base; (i<nextseqnum) && (i< (send_base + N)); i++ ){
				
				if(messagelist.size() <= (i+N) ){
					//printf("i = %d  messagelist[ %d ].ack = %d \n", i, i+N, messagelist[i + N].ack);
					if(messagelist[i].ack == -1){
						//if(nextseqnum>(i + N)){
						printf("Sending packet to B. send_base : %d Seqnum : %d  Message: %s \n",send_base ,i, messagelist[i].packet.payload);
						tolayer3(0, messagelist.at(i).packet);
						messagelist[i].time = get_sim_time();
					//}
					}
					if(messagelist[i].ack == -1){
						//++send_base; //Change done
						starttimer(0, (RTT - (get_sim_time() - (messagelist[i].time))));
						break;
					}
					++send_base;
				}
			}
		}
	}

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("Time up ! \n");
	int min = send_base;

	printf("Send_base : %d  \n", send_base);

	for(int i = send_base; ((i<(send_base + N)) && (i<nextseqnum)); i++){

		if((messagelist.at(i).ack != -1) && (i==send_base)){ ++send_base;}
		printf("ACK at %d = %d \n", i, messagelist.at(i).ack);
		if(messagelist.at(i).ack == -1){			
			printf("Sending message to B from timerInterrupt : %s \n", messagelist.at(i).packet.payload);
			tolayer3(0, messagelist.at(i).packet);
			messagelist.at(i).time = get_sim_time();
			//printf("Here !\n");
		}
		if(messagelist.at(min).ack != -1){
			min++;
			printf("Increasing min \n");
			//break;
			
		}
		
	}
	
	
	/*printf("Finding min \n");
	for(int i = send_base; i<(send_base + N); i++){
		if(messagelist.at(i).ack != -1){
			min++;
			printf("Increasing min \n");
			break;
			
		}
	}*/

	if(min == send_base){
		starttimer(0, RTT);
	}
	else{
		printf("Min : %d  RTT delay : %d \n", min, (RTT - (get_sim_time() - messagelist.at(min).time)));
		starttimer(0, RTT - (get_sim_time() - messagelist.at(min).time));
	}

	


}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	N = getwinsize();
	RTT = 15;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	pkt ack_pkt;
	printf("Packet received at B \n");
	int check_sum = checksum(packet);
	//strcpy(messagebuffer[packet.seqnum], "");

	if((check_sum == packet.checksum)&&((packet.seqnum >= expectedseq) && (packet.seqnum < (expectedseq + N)))){
		printf("Checksum matched. Packet not corrupt. \n");
		printf("Message text : %s \n", packet.payload);	

		//if (std::find_if(messagelistrecv.begin(), messagelistrecv.end(), MatchesID((packet.seqnum))) == messagelistrecv.end()){
		if(strlen(messagebuffer[packet.seqnum])==0){
			printf("New packet received. Message being added to receive list. \n");
			
			ack_pkt.acknum = packet.seqnum;
			strcpy(ack_pkt.payload, "ACK");
			ack_pkt.seqnum = 0;
			ack_pkt.checksum = checksum(ack_pkt);

			tolayer3(1, ack_pkt);
			
			//messagelistrecv.push_back(packet);
			strncpy(messagebuffer[packet.seqnum], packet.payload, 20);
			
			//See again - not necessary to delete
			int temp = expectedseq + N -1;
			
			//printf("Message = %s \n", messagebuffer[expectedseq]);
			while(expectedseq < temp){
				printf("Expected seq : %d   temp= %d \n", expectedseq, temp);
				//if((messagelistrecv.at(it).acknum == expectedseq) && (messagelistrecv.at(it).acknum != -1)){
				if(strlen(messagebuffer[expectedseq]) != 0){
					//char msgtosend[20];
					//strncpy(msgtosend, messagebuffer[expectedseq], 20);
					tolayer5(1, messagebuffer[expectedseq]);
					printf("Message sent to Layer5. Message : %s \n", messagebuffer[expectedseq]);
					expectedseq++;
				}
				else{
					printf("Breaking out of loop.\n");
					break;
				}

			}
			
						
		}
		else{
			printf("Packet already received ! \n");
			
			ack_pkt.acknum = packet.seqnum;
			strcpy(ack_pkt.payload, "ACK");
			ack_pkt.seqnum = 0;
			ack_pkt.checksum = checksum(ack_pkt);
			
			tolayer3(1, ack_pkt);
		}
	}
	else{
			
			printf("Packet already received ! \n");
			if(check_sum == packet.checksum){
				ack_pkt.acknum = packet.seqnum;
				strcpy(ack_pkt.payload, "ACK");
				ack_pkt.seqnum = 0;
				ack_pkt.checksum = checksum(ack_pkt);

				tolayer3(1, ack_pkt);
			}
		}

	
	
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expectedseq = 0;
	messagebuffer[2000][20];

}