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


char message_to_send[20];
int isACKReceived = 1; //1 for 'Yes' and '0' for 'No'
int updateSeq = 0, expectedSeq = 0;
int retransmit = 0;
int messageReceived5 = 0;
std::vector<struct msg>messages_list;

struct Messages{
    char data[20];
    struct Messages *next;
};

struct Messages *my_A_output = NULL;

void addMessage(char data[20]){
    
    struct Messages* head = (struct Messages*)malloc(sizeof(struct Messages));
    strcpy(head->data, data);
    head->next = NULL;

    struct Messages *temp = my_A_output;
    
    if(my_A_output == NULL){
        my_A_output = head;
    }
    else{
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = head;
        
        
    }
    
    printf("Message added to list successfully !\n");
    
    return;
}

char* getMessage(){
    
    struct Messages* head = my_A_output;
    char *message = (char *) malloc(sizeof(char) * 20);
    if(my_A_output!=NULL){
        strcpy(message, my_A_output->data);
        my_A_output = my_A_output->next;
        head->next = NULL;
        //free(head);
    }

    printf("Message Read : %s \n", message);
    return message;
}

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
    
    //delete(s);
    return sum;
}

void sendMsgToB(){

	pkt send_pkt;
	isACKReceived = 0;
	//if((my_A_output == NULL) && (retransmit != 1)){
	if((messages_list.empty() == 1) && (retransmit != 1)){
		isACKReceived = 1;

		printf("I'm here \n");
		//return;
	}
	else{
		if(retransmit == 0 ){
			//strncpy(message_to_send, getMessage(), 20);
			strncpy(message_to_send, messages_list.at(0).data, 20);
			messages_list.erase(messages_list.begin());
		}
		retransmit = 0;
		//messageReceived5 = 0;
		strncpy(send_pkt.payload, message_to_send, 20);

		printf("Message to send : %s  \n", send_pkt.payload);
		send_pkt.acknum = -1;
		send_pkt.seqnum = updateSeq;
		printf("Seq number updated = %d \n", send_pkt.seqnum);

		send_pkt.checksum = checksum(send_pkt);
	
		printf("Packet Created !\nSending the packet now . . . \nStarting timer . . . \n");
    
		starttimer (0, 20.0);
    
		printf("Sending to layer 3. \n");
    		tolayer3 (0, send_pkt);
	}

}


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{	

	printf("New Message received : %s \n", message.data);
	addMessage(message.data);
	messages_list.push_back(message);

	if(isACKReceived == 1){
		
			isACKReceived = 0;
			sendMsgToB();
		
	}
	
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	printf("ACK received. \n");
	
	stoptimer(0);

	printf("Seq number updated = %d \n", updateSeq);
	printf("Seq number expected = %d \n", expectedSeq);

	if(updateSeq == expectedSeq){
		return;
	}
	if(expectedSeq != updateSeq){
		printf("ACK for correct Seq received. Message = %s \n", message_to_send);
		isACKReceived = 1;

		//Updating the UpdateSeq for the next message   ---   // put updated seq == expected seq
		updateSeq = !updateSeq;

		messageReceived5 = 0;
		printf("Seq number updated = %d \n", updateSeq);
		printf("Seq number expected = %d \n", expectedSeq);
		
	}

	if(messages_list.empty() != 1){
		//sendMsgToB();
		pkt send_pkt;
		isACKReceived = 0;
		retransmit = 0;
		//strncpy(message_to_send, getMessage(), 20);

		strncpy(message_to_send, messages_list.at(0).data, 20);
		messages_list.erase(messages_list.begin());

		strncpy(send_pkt.payload, message_to_send, 20);

		printf("Message to send : %s  \n", send_pkt.payload);

		send_pkt.seqnum = updateSeq;
		printf("Seq number updated = %d \n", send_pkt.seqnum);

		send_pkt.checksum = checksum(send_pkt);
	
		printf("Packet Created !\nSending the packet now . . . \nStarting timer . . . \n");
    
		starttimer (0, 20.0);
    
		printf("Sending to layer 3. \n");
   	 	tolayer3 (0, send_pkt);
		
	}
	
}

/* called when A's timer goes off */
void A_timerinterrupt()
{

	printf("Time's up ! \n");
	printf("Message received at B ? : %d \n", messageReceived5);

	if(messageReceived5 == 1){
		isACKReceived = 1;
		updateSeq = expectedSeq;
		messageReceived5 = 0;


	}
	else{
		printf("Retransmitting message . . . \n");
		retransmit = 1;
	}
	
	pkt send_pkt;
	isACKReceived = 0;
	//if((my_A_output == NULL) && (retransmit != 1)){
	if((messages_list.empty() == 1) && (retransmit != 1)){
		isACKReceived = 1;

		printf("I'm here \n");
		//return;
	}
	else{

		if(retransmit == 0 ){
			//strncpy(message_to_send, getMessage(), 20);
			strncpy(message_to_send, messages_list.at(0).data, 20);
			messages_list.erase(messages_list.begin());
		}
		retransmit = 0;
	//messageReceived5 = 0;
		strncpy(send_pkt.payload, message_to_send, 20);

		printf("Message to send : %s  \n", send_pkt.payload);

		send_pkt.seqnum = updateSeq;
		printf("Seq number updated = %d \n", send_pkt.seqnum);

		send_pkt.checksum = checksum(send_pkt);
		printf("Checksum = %d \n", send_pkt.checksum);	


		printf("Packet Created !\nSending the packet now . . . \nStarting timer . . . \n");
    
		starttimer (0, 20.0);
    
		printf("Sending to layer 3. \n");
    		tolayer3 (0, send_pkt);

	}
	//sendMsgToB();
	
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	int check_sum;
	pkt ack_pkt;

	printf("\nPacket received at B !\n");
	
	if(packet.seqnum == expectedSeq){
		printf("Seq num is correct at B. \n");
		check_sum = checksum(packet);

		//printf("Checksum = %d \n", check_sum);
		//printf("Packet Checksum = %d \n", packet.checksum);
		if(check_sum == packet.checksum){
			printf("Checksum is correct at B.\n");
			tolayer5 (1, packet.payload);

			printf("Message sent to Layer 5 successful : %s \n", packet.payload);
			
			ack_pkt.seqnum = packet.seqnum;
			ack_pkt.acknum = packet.seqnum;
			strcpy(ack_pkt.payload, "ACK");
			ack_pkt.checksum =  checksum(ack_pkt);
			
			if(expectedSeq == 1){
				expectedSeq = 0;
			}
			else if(expectedSeq == 0){
				expectedSeq = 1;
			}
			
			messageReceived5 = 1;
			//printf("Seq number updated = %d \n", updateSeq);
			//printf("Seq number expected = %d \n", expectedSeq);

			printf("Sending ACK back from B. \n");
			tolayer3 (1, ack_pkt);
			
		}

	}
	else{
		
		if(packet.checksum == checksum(packet)){
			ack_pkt.seqnum = packet.seqnum;
			ack_pkt.acknum = packet.seqnum;
			strcpy(ack_pkt.payload, "ACK");
			ack_pkt.checksum =  checksum(ack_pkt);
			//printf("Sending ACK back from B. \n");

			//printf("Seq number updated = %d \n", updateSeq);
			//printf("Seq number expected = %d \n", expectedSeq);

			if(packet.seqnum == updateSeq){
				printf("ACK sent again. \n");
				tolayer3 (1, ack_pkt);
			}
		}
	}


}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{

}