/*
 * TestServer.cpp
 *
 *  Created on: Nov 30, 2011
 *      Author: ittai zeidman
 */
#include "StringUtils.h"
#include "SocketHandler.h"
#include "ProtocolParserCommon.h"
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
int connectToServer(char *hostname, char *port){
	int    sock = 0;
	unsigned long  err_check = 0;
	struct addrinfo hints, *servinfo, *p;


	// Resolve hostname
	memset(&hints, 0, sizeof hints);
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((err_check = getaddrinfo(hostname, port, &hints, &servinfo)) != 0)
	{
		cout << "getaddrinfo: " << gai_strerror(err_check) << endl;
		return -1;
	}

	// Try connect to server
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			cout << "socket error" << endl;
			continue;
		}

		if (connect(sock, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sock);
			cout << "connect error" << endl;
			continue;
		}

		break;
	}

	if (p == NULL)
	{
	// looped off the end of the list with no connection
		cout << "failed to connect" << endl;
		freeaddrinfo(servinfo);
		return -1;
	}

	freeaddrinfo(servinfo);

	return sock;
}
void getHeaderTest(int workingServerSocket,Header &header){
	char *inBuffer = new char[FIXED_HEADER_SIZE];
	bool receivedSuccess = receiveAllData(workingServerSocket,inBuffer,FIXED_HEADER_SIZE);
	assert(receivedSuccess);
	parseHeaderFromBuffer(inBuffer,header);
	delete[] inBuffer;
}
void sendMessage(int workingServerSocket,WorkflowIdentifier workflowIdentifier,string &body){
	int bodySize = body.length();
	string headerOutputString = parseOutputStringFromHeader(workflowIdentifier,bodySize);
	unsigned int messageSize = bodySize+FIXED_HEADER_SIZE;
	string message = headerOutputString+body;
	char *outBuffer = new char[messageSize];
	outputStringToBuffer(message,outBuffer);
	bool sentSuccess = sendAllData(workingServerSocket,outBuffer,&messageSize);
	assert(sentSuccess);
	delete[] outBuffer;
}
void testResponse(int workingServerSocket,WorkflowIdentifier workflowIdentifier,string &expectedBody){
	Header header;
	getHeaderTest(workingServerSocket,header);
	assert(header.workflowIdentifier == workflowIdentifier);
	assert(header.bodyLength == expectedBody.length());
	if (expectedBody.length()!=0){
		char *bodyInBuffer = new char[header.bodyLength];
		bool receivedSuccess = receiveAllData(workingServerSocket,bodyInBuffer,header.bodyLength);
		assert(receivedSuccess);
		assert(expectedBody.compare(0,header.bodyLength,bodyInBuffer,header.bodyLength)==0);
		delete[] bodyInBuffer;
	}
}
void testMethod(int workingServerSocket,WorkflowIdentifier workflowIdentifier,bool expectedResponse,string body,string expectedBody){
	cout << "test" << getWorkflowIdentifierString(workflowIdentifier) << endl;
	sendMessage(workingServerSocket,workflowIdentifier,body);
	if (expectedResponse){
		testResponse(workingServerSocket,workflowIdentifier,expectedBody);
	}
}
void testConnectToServer(int workingServerSocket){
	testMethod(workingServerSocket,CONNECT_TO_SERVER,true,"","Welcome!");
}
void testLogin(int workingServerSocket,string username,string password,string expectedBody){
	testMethod(workingServerSocket,LOGIN,true,username+"|"+password,expectedBody);
}
void testLoginSuccess(int workingServerSocket){
	testLogin(workingServerSocket,"ittai","ittaiPassword","Connected to server");
}
void testQuit(int workingServerSocket){
	testMethod(workingServerSocket,QUIT,false,"","");
}
void testLoginFailPassword(int workingServerSocket){
	testLogin(workingServerSocket,"ittai","ittai2Password","Username password pair not correct");
}
void testLoginFailUsername(int workingServerSocket){
	testLogin(workingServerSocket,"ittai2","ittaiPassword","Username password pair not correct");
}
void testLoginFailUsernameAndPassword(int workingServerSocket){
	testLogin(workingServerSocket,"ittai","ittaiPassword2","Username password pair not correct");
}
void testShowInboxEmpty(int workingServerSocket){
	testMethod(workingServerSocket,SHOW_INBOX,true,"","");
}
void testComposeMessageNoAttachments(int workingServerSocket){
	testMethod(workingServerSocket,COMPOSE_MAIL,true,"razi,joe|how you doin no attach?|this is a very long and unrestricted message text for a message||","Mail Sent");
}
void testComposeMessageTwoAttachments(int workingServerSocket){
	testMethod(workingServerSocket,COMPOSE_MAIL,true,"razi,joe|how you doin?|this is very short message text|~/docs/funny.jpg,~/docs/funny2.jpg|0000000AABCDEFGHIJ0000000512345","Mail Sent");
}
void testShowInboxOneMail(int workingServerSocket,int mailId){
	testMethod(workingServerSocket,SHOW_INBOX,true,"",parseIntToString(mailId)+"|ittai|\"how you doin?\"|2\n");
}
void testShowInboxTwoMails(int workingServerSocket,int mailId){
	testMethod(workingServerSocket,SHOW_INBOX,true,"",parseIntToString(mailId)+"|ittai|\"how you doin?\"|2\n"+parseIntToString(mailId+1)+"|ittai|\"how you doin no attach?\"|0\n");
}
void testGetMailWithAttachments(int workingServerSocket,int mailId){
	testMethod(workingServerSocket,GET_MAIL,true,parseIntToString(mailId),"ittai|razi,joe|how you doin?|~/docs/funny.jpg,~/docs/funny2.jpg|this is very short message text");
}
void testGetMailNoAttachments(int workingServerSocket,int mailId){
	testMethod(workingServerSocket,GET_MAIL,true,parseIntToString(mailId),"ittai|razi,joe|how you doin no attach?||this is a very long and unrestricted message text for a message");
}
void testGetAttachment(int workingServerSocket,int mailId){
	testMethod(workingServerSocket,GET_ATTACHMENT,true,parseIntToString(mailId)+"|2","12345");
}
void testDeleteMail(int workingServerSocket,int mailId){
	testMethod(workingServerSocket,DELETE_MAIL,true,parseIntToString(mailId),"");
}
void assertParseWorkflowIdentifierEnum(){
	WorkflowIdentifier w = getWorkflowIdentifierEnum("CONNECT_TO_SERVER");
	assert(w==CONNECT_TO_SERVER);
	w = getWorkflowIdentifierEnum("LOGIN");
	assert(w==LOGIN);

}
string host = "127.0.0.1";
string port = "8088";
int main(int argc, char* argv[]){
	if (argc == 3){
		host = argv[1];
		port = argv[2];
	}
	assertParseWorkflowIdentifierEnum();
	int i=1;
	for (;i<12;i+=2){
		int workingServerSocket=connectToServer((char *)host.c_str(),(char *)port.c_str());
		if (workingServerSocket == -1){
			return 0;
		}
		testConnectToServer(workingServerSocket);
		testLoginSuccess(workingServerSocket);
		testQuit(workingServerSocket);
		close(workingServerSocket);

		workingServerSocket=connectToServer((char *)host.c_str(),(char *)port.c_str());
		testConnectToServer(workingServerSocket);
		testLoginFailPassword(workingServerSocket);
		close(workingServerSocket);

		workingServerSocket=connectToServer((char *)host.c_str(),(char *)port.c_str());
		testConnectToServer(workingServerSocket);
		testLoginFailUsername(workingServerSocket);
		close(workingServerSocket);

		workingServerSocket=connectToServer((char *)host.c_str(),(char *)port.c_str());
		testConnectToServer(workingServerSocket);
		testLoginFailUsernameAndPassword(workingServerSocket);
		close(workingServerSocket);

		workingServerSocket=connectToServer((char *)host.c_str(),(char *)port.c_str());
		testConnectToServer(workingServerSocket);
		testLoginSuccess(workingServerSocket);
		testShowInboxEmpty(workingServerSocket);
		testComposeMessageTwoAttachments(workingServerSocket);
		testComposeMessageNoAttachments(workingServerSocket);
		testQuit(workingServerSocket);
		close(workingServerSocket);

		workingServerSocket=connectToServer((char *)host.c_str(),(char *)port.c_str());
		testConnectToServer(workingServerSocket);
		testLogin(workingServerSocket,"razi","raziPassword","Connected to server");
		testShowInboxTwoMails(workingServerSocket,i);
		testGetMailWithAttachments(workingServerSocket,i);
		testGetMailNoAttachments(workingServerSocket,i+1);
		testGetAttachment(workingServerSocket,i);
		testDeleteMail(workingServerSocket,i);
		testDeleteMail(workingServerSocket,i+1);
		testQuit(workingServerSocket);
		close(workingServerSocket);

		workingServerSocket=connectToServer((char *)host.c_str(),(char *)port.c_str());
		testConnectToServer(workingServerSocket);
		testLogin(workingServerSocket,"joe","joePassword","Connected to server");
		testShowInboxTwoMails(workingServerSocket,i);
		testGetMailWithAttachments(workingServerSocket,i);
		testGetMailNoAttachments(workingServerSocket,i+1);
		testDeleteMail(workingServerSocket,i);
		testDeleteMail(workingServerSocket,i+1);
		testQuit(workingServerSocket);
		close(workingServerSocket);
	}
	return 0;
}


