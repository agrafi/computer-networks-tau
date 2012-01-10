/*
 * SocketHandler.h
 *
 *  Created on: Nov 29, 2011
 *      Author: ittai zeidman
 */

#ifndef SOCKETHANDLER_H_
#define SOCKETHANDLER_H_
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

typedef struct FDPairSetsStruct {
	fd_set read;
	fd_set write;
	void init(){
		FD_ZERO(&read);
		FD_ZERO(&write);
	}
	void copy(FDPairSetsStruct &other){
		read = other.read;
		write = other.write;
	}
} FDPairSets;

const int defaultPort = 6423;
const string defaultPortStr = "6423";
bool receiveAllData(int workingServerSocket, char *buffer, int demandedNumOfBytes);
bool sendAllData(int workingServerSocket, char *buffer, unsigned int *demandedNumOfBytes);
typedef bool (*pfnReceiveCallback)(int);
bool sendAllDataFullDuplex(int workingServerSocket, char *buffer,unsigned int *demandedNumOfBytes, pfnReceiveCallback pCallback);


#endif /* SOCKETHANDLER_H_ */
