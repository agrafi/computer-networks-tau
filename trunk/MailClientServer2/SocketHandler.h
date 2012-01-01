/*
 * SocketHandler.h
 *
 *  Created on: Nov 29, 2011
 *      Author: ittai zeidman
 */

#ifndef SOCKETHANDLER_H_
#define SOCKETHANDLER_H_
#include <string>
using namespace std;
const int defaultPort = 6423;
const string defaultPortStr = "6423";
bool receiveAllData(int workingServerSocket, char *buffer, int demandedNumOfBytes);
bool sendAllData(int workingServerSocket, char *buffer, unsigned int *demandedNumOfBytes);


#endif /* SOCKETHANDLER_H_ */
