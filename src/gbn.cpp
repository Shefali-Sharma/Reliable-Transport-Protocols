#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <strings.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

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

char messages[1000][20];
int isAccept = 1;
int send_base = 0, nextseqnum = 0;
int N, W = 1;
int expectedseq = 0, currentseq = 0;
int isACKSend = 0;
int i = 0;
int RTT;

int checksum(struct pkt packet)
{
    int sum = 0;
    char *s = (char *) malloc(30 * sizeof(char));
    
    //printf("Here ! \n");

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

void A_output(struct msg message)
{
	strcpy(messages[i], message.data);
    	i++;

	if(isACKSend == 0 && isAccept == 1){
		if((nextseqnum < (N + send_base))&&(nextseqnum<i)){
			pkt send_message;
			send_message.seqnum = nextseqnum;
			send_message.acknum = -1;
			strncpy(send_message.payload, messages[nextseqnum], 20);
        		send_message.checksum = checksum(send_message);
            
        		printf("Message being sent = %s \n", send_message.payload);
            
        		tolayer3(0, send_message);
        		printf("Message sent to B. \n");
            
        		if(send_base==nextseqnum){
       	     			starttimer(0, RTT);//((N * 5) + 5));
       	     			printf("Timer started for seq = %d \n", send_base);
       	 		}
			++nextseqnum;
		}
	}

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	printf("ACK received. \n");
	printf("Base sequence = %d    Packet.seqnum = %d \n ", send_base, packet.seqnum);
	int check_sum = checksum(packet);
	currentseq = send_base;

	if(packet.acknum == send_base){
		if(check_sum == packet.checksum){
			stoptimer(0);
			send_base++;

			isAccept = 0;
			isACKSend = 1;

			int i = (currentseq + N);
			while(i<nextseqnum && i<send_base+N){

				pkt send_message;
				send_message.seqnum = i;
				send_message.acknum = -1;
				strncpy(send_message.payload, messages[i], 20);
        			send_message.checksum = checksum(send_message);
            
        			printf("Message being sent from A_input= %s \n", send_message.payload);
				
				tolayer3(0, send_message);
        			printf("Message sent to B. \n");
				
				i++;
			}
			isACKSend = 0;
			isAccept = 1;
			starttimer(0, RTT);
		}
	}
	else if(packet.acknum > send_base){
		if(check_sum == packet.checksum){
			stoptimer(0);
			send_base = packet.acknum + 1;
			
			isAccept = 0;
			isACKSend = 1;

			int i = (currentseq + N);
			while(i<nextseqnum && i<send_base+N){

				pkt send_message;
				send_message.seqnum = i;
				send_message.acknum = -1;
				strncpy(send_message.payload, messages[i], 20);
        			send_message.checksum = checksum(send_message);
            
        			printf("Message being sent from A_input= %s \n", send_message.payload);
				
				tolayer3(0, send_message);
        			printf("Message sent to B. \n");
				
				i++;
			}

			isACKSend = 0;
			isAccept = 1;
			starttimer(0, RTT);
			
		}
	}
	else{
		printf("Invalid ACK received. \n");
	}

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("Time's up ! \n");
	isACKSend = 1;

	int count = send_base;
	
	//if((send_base + N) <= nextseqnum){

		while(count<(send_base + N) && count<nextseqnum){
			pkt send_message;
			send_message.seqnum = count;
			send_message.acknum = -1;
			strncpy(send_message.payload, messages[count], 20);
        		send_message.checksum = checksum(send_message);
            
        		printf("Message being sent from A_input= %s \n", send_message.payload);
				
			tolayer3(0, send_message);
        		printf("Message sent to B. \n");
				
			count++;
		}

		starttimer(0, RTT);
	//}
	isACKSend = 0;
	isAccept = 1;

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	N = getwinsize();
	RTT = 40;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	printf("Message Received at B ! \n");
	int check_sum;
	pkt ack_pkt;
	
	if(isAccept == 1){
		if(expectedseq == packet.seqnum){
			printf("Packet has correct sequence number : %d \n", packet.seqnum);
        		check_sum = checksum(packet);
        		if(check_sum == packet.checksum){
				printf("Packet has correct checksum ! \n");

               	 		tolayer5 (1, packet.payload);                
                		printf("Message sent to Layer 5 successful : %s \n", packet.payload);

                		ack_pkt.acknum = expectedseq;
				ack_pkt.seqnum = 0;
				strcpy(ack_pkt.payload, "ACK");
				ack_pkt.checksum = checksum(ack_pkt);

				printf("Sending ACK back from B. \n");
				tolayer3 (1, ack_pkt); //Sending ACK to A

                		expectedseq++;

				//currentseq = send_base;
				//send_base++;

				//printf("Base sequence = %d    Packet.seqnum = %d     Expected  = %d \n ", send_base, ack_pkt.seqnum, expectedseq);
                
                		
			}
			else{
				printf("Incorrect checksum \n");
				isAccept = 0;
				isACKSend = 1;
			}
		
		}
		else if(packet.seqnum < expectedseq){
			check_sum = checksum(packet);
        		if(check_sum == packet.checksum){
				printf("Message already sent to Layer5. Sending ACK now . . .\n");

				ack_pkt.acknum = packet.seqnum;
				ack_pkt.seqnum = 0;
				strcpy(ack_pkt.payload, "ACK");
				ack_pkt.checksum = checksum(ack_pkt);

				printf("Sending ACK back from B. \n");
				tolayer3 (1, ack_pkt); //Sending ACK to A

			}
/*			printf("Incorrect sequence \n");
			isAccept = 0;
			isACKSend = 1;
*/
		}
	}
	

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{

}