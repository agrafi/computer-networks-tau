/*
 * SocketHandler.cpp
 *
 *  Created on: Nov 29, 2011
 *      Author: ittai zeidman
 */


#include "SocketHandler.h"
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
//Sockets
#include <netinet/in.h>
//Internet addresses
#include <arpa/inet.h>
//Working with Internet addresses
#include <netdb.h>
//Domain Name Service (DNS)

bool receiveAllData(int workingServerSocket, char *buffer,
		int demandedNumOfBytes) {
	int numBytesReceived = recv(workingServerSocket, buffer, demandedNumOfBytes,
			MSG_WAITALL);
	if (numBytesReceived != demandedNumOfBytes) {
		//problem not received all demanded bytes
		perror("Received less bytes than expected");
		return false;
	}
	return true;
}
bool sendAllDataFullDuplex(int workingServerSocket, char *buffer,unsigned int *demandedNumOfBytes,
		pfnReceiveCallback pCallback) {
	// Socket declarations
	FDPairSetsStruct set;

	// File declarations
	unsigned int total = 0;
	// how many bytes we've sent
	unsigned int bytesleft = *demandedNumOfBytes; // how many we have left to send
	int numBytesCurrentlySent;
	while (total < *demandedNumOfBytes) {
		set.init();
		FD_SET(workingServerSocket, &set.read);
		FD_SET(workingServerSocket, &set.write);

		if (select(workingServerSocket + 1, &set.read, &set.write, NULL, NULL)
					== -1) {
				perror("select");
				return -1;
			}

		// If server data is pending, pass it to the supplied callback function
		if (FD_ISSET(workingServerSocket, &set.read))
		{
			pCallback(workingServerSocket);
		}

		// If socket ready for write, fill it with requested data
		if (FD_ISSET(workingServerSocket, &set.write))
		{
			numBytesCurrentlySent = send(workingServerSocket, buffer + total, bytesleft, 0);
			if (numBytesCurrentlySent == -1) {
				perror("Error sending data");
				break;
			}
			total += numBytesCurrentlySent;
			bytesleft -= numBytesCurrentlySent;
		}
	}

	*demandedNumOfBytes = total; // return number actually sent here
	return (numBytesCurrentlySent != -1); // return -1 on failure, 0 on success

}
bool sendAllData(int workingServerSocket, char *buffer,unsigned int *demandedNumOfBytes) {
	unsigned int total = 0;
	// how many bytes we've sent
	unsigned int bytesleft = *demandedNumOfBytes; // how many we have left to send
	int numBytesCurrentlySent;
	while (total < *demandedNumOfBytes) {
		numBytesCurrentlySent = send(workingServerSocket, buffer + total, bytesleft, 0);
		if (numBytesCurrentlySent == -1) {
			perror("Error sending data");
			break;
		}
		total += numBytesCurrentlySent;
		bytesleft -= numBytesCurrentlySent;
	}
	*demandedNumOfBytes = total; // return number actually sent here
	return (numBytesCurrentlySent != -1); // return -1 on failure, 0 on success
}

