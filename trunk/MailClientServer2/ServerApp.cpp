/*
 * ServerApp.cpp
 *
 *  Created on: Nov 22, 2011
 *      Author: ittai zeidman
 */

#include "ProtocolParserServer.h"
#include "StringUtils.h"
#include "SocketHandler.h"
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <sys/socket.h>
//Sockets
#include <netinet/in.h>
//Internet addresses
#include <arpa/inet.h>
//Working with Internet addresses
#include <netdb.h>
//Domain Name Service (DNS)
#include <errno.h>
//working with erros
using namespace std;
typedef struct MailboxStruct {
	int lastInsertedMailMessageId;
	string password;
	vector<MailMessage*> messages;
} Mailbox;
typedef pair<string, Mailbox> MailboxUserPair;
typedef map<string, Mailbox> MailStore;
const char USER_RECORD_SEPARATOR = '\t';
const int USER_RECORD_SEPARATOR_LENGTH = 1;
const int EOF_LENGTH = 2;
#define getUserRecordInFileLength() MAX_USERNAME_LENGTH+MAX_PASSWORD_LENGTH+USER_RECORD_SEPARATOR_LENGTH+EOF_LENGTH
void initUsers(MailStore &userMailboxes,const string usersFilePath) {
	//Initialize server data structures
	//Init user map
	//add entry for each user in user-file inclusive of password and empty mailbox
	//mailbox starts as an empty list of MailMessages and a lastInsertedMailMessageId int member initialized to 0
	char buffer[getUserRecordInFileLength()];
	ifstream myfile(usersFilePath.c_str());

	while (!myfile.eof()) {
		myfile.getline(buffer, getUserRecordInFileLength());
		string username = getTextualField(USER_RECORD_SEPARATOR, buffer);
		Mailbox m;
		m.lastInsertedMailMessageId = 0;
		//we're looking for then null character since getLine removes the \n characters and appends the null character instead
		m.password = getTextualField(NULL,
				(buffer + username.length() + USER_RECORD_SEPARATOR_LENGTH));
		userMailboxes.insert(MailboxUserPair(username, m));
	}
}
bool authenticationPassed(Body &body, MailStore &userMailboxes) {
	bool authenticationPassed = false;
	MailStore::iterator iter = userMailboxes.find(
			body.userLogin.username);
	if (iter != userMailboxes.end()) {
		authenticationPassed =
				(iter->second.password == body.userLogin.password);
	}
	return authenticationPassed;
}

bool isExpectedHeader(int workingServerSocket, Header &header,
		WorkflowIdentifier expectedWorkflowIdentifier) {
	char headerBuffer[FIXED_HEADER_SIZE];
	bool shouldContinue = receiveAllData(workingServerSocket, headerBuffer,
			FIXED_HEADER_SIZE);
	if (!shouldContinue) {
		return shouldContinue;
	}
	parseHeaderFromBuffer(headerBuffer, header);
	return (header.workflowIdentifier == expectedWorkflowIdentifier);
}
bool handleConnectRequest(int workingServerSocket) {
	Header header;
	return isExpectedHeader(workingServerSocket, header, CONNECT_TO_SERVER);
}
bool handleTextualMessageResponse(const int workingServerSocket,const TextualProtocolMessage &textualProtocolMessage){
	string outString = parseBufferFromTextualMessage(textualProtocolMessage);
	unsigned int messageSize = outString.length();
	char *outBuffer = new char[messageSize];
	outputStringToBuffer(outString,outBuffer);
	bool shouldContinue = sendAllData(workingServerSocket,outBuffer,&messageSize);
	delete[] outBuffer;
	return shouldContinue;
}
bool handleInfoMessageResponse(int workingServerSocket,WorkflowIdentifier workflowIdentifier,string infoMessage){
	TextualProtocolMessage textualProtocolMessage;
	textualProtocolMessage.header.workflowIdentifier = workflowIdentifier;
	textualProtocolMessage.body.infoMessage = infoMessage;
	textualProtocolMessage.header.bodyLength =
			textualProtocolMessage.body.infoMessage.length();
	return handleTextualMessageResponse(workingServerSocket,textualProtocolMessage);
}
bool handleConnectResponse(int workingServerSocket) {
	return handleInfoMessageResponse(workingServerSocket,CONNECT_TO_SERVER,"Welcome!");
}
bool handleConnectState(int workingServerSocket) {
	bool shouldContinue = handleConnectRequest(workingServerSocket);
	if (shouldContinue) {
		shouldContinue = handleConnectResponse(workingServerSocket);
	}
	return shouldContinue;
}
bool handleLoginResponse(int workingServerSocket,bool loginSucceeded) {
	string infoMessage;
	if (loginSucceeded){
		infoMessage = "Connected to server";
	} else {
		infoMessage = "Username password pair not correct";
	}
	return handleInfoMessageResponse(workingServerSocket,LOGIN,infoMessage);
}
bool handleLoginRequest(int workingServerSocket,
		MailStore &userMailboxes,string &usernameOfLoggedInUserToBeFilled,bool *loginSucceeded) {
	Header header;
	if (!isExpectedHeader(workingServerSocket, header, LOGIN)) {
		return false;
	}
	char *loginBodyBuffer = new char[header.bodyLength];
	Body loginTextualBody;
	bool shouldContinue = receiveAllData(workingServerSocket, loginBodyBuffer,
			header.bodyLength);
	if (shouldContinue) {
		parseTextualBasedBodyFromBuffer(loginBodyBuffer, header,
				loginTextualBody);
		*loginSucceeded = authenticationPassed(loginTextualBody, userMailboxes);
		if (*loginSucceeded){
			usernameOfLoggedInUserToBeFilled = loginTextualBody.userLogin.username;
		}
	}
	delete[] loginBodyBuffer;
	return shouldContinue;
}
bool handleLoginState(int workingServerSocket,MailStore &userMailboxes,string &usernameOfLoggedInUserToBeFilled,bool &loginSucceeded) {
	bool shouldContinue = handleLoginRequest(workingServerSocket,userMailboxes,usernameOfLoggedInUserToBeFilled,&loginSucceeded);
	if (shouldContinue) {
		shouldContinue = handleLoginResponse(workingServerSocket,loginSucceeded);
	}
	return shouldContinue;
}
int getMailIndexFromMailId(int mailId){
	return mailId - 1;
}
bool handleDeleteMailInternal(const int mailIndex,Mailbox &mailbox){
	//delete the attachment
	MailMessage *&mailMessage = mailbox.messages.at(mailIndex);
	if (mailMessage == NULL){
		return false;
	}
	//free attachments
	for (unsigned int i=0;i<mailMessage->attachments.size();i++){
		delete[] mailMessage->attachments.at(i)->file.data;//delete attachment content
		delete mailMessage->attachments.at(i);
		mailMessage->attachments.at(i) = NULL;
	}
	//free message
	delete mailMessage;
	mailbox.messages.at(mailIndex) = NULL;
	return true;
}
bool handleDeleteMailBusiness(Body &textualBodyInput,Mailbox &mailbox){
	int mailIndex = getMailIndexFromMailId(textualBodyInput.messages.at(0)->mailId);
	bool shouldContinue = handleDeleteMailInternal(mailIndex,mailbox);
	//delete input mailMessage;
	//here we know only the message pointer itself was initialized
	delete textualBodyInput.messages.at(0);
	return shouldContinue;
}
bool handleDeleteMailResponse(int workingServerSocket){
	return handleInfoMessageResponse(workingServerSocket,DELETE_MAIL,"");
}
bool handleDeleteMailState(int workingServerSocket, Body &textualBodyInput,Mailbox &mailbox){
	bool shouldContinue = handleDeleteMailBusiness(textualBodyInput,mailbox);
	if (shouldContinue){
		shouldContinue = handleDeleteMailResponse(workingServerSocket);
	}
	return shouldContinue;
}
bool handleShowInboxBusiness(TextualProtocolMessage &textualProtocolMessage,Mailbox &mailbox){
	textualProtocolMessage.header.workflowIdentifier = SHOW_INBOX;
	textualProtocolMessage.body.messages = mailbox.messages;
	return true;
}
bool handleShowInboxState(int workingServerSocket,Mailbox &mailbox){
	TextualProtocolMessage textualProtocolMessage;
	bool shouldContinue = handleShowInboxBusiness(textualProtocolMessage,mailbox);
	if (shouldContinue){
		shouldContinue = handleTextualMessageResponse(workingServerSocket,textualProtocolMessage);
	}
	return shouldContinue;
}
bool handleGetMailBusiness(TextualProtocolMessage &textualProtocolMessage, Body &textualBodyInput,Mailbox &mailbox){
	textualProtocolMessage.header.workflowIdentifier = GET_MAIL;
	int mailIndex = getMailIndexFromMailId(textualBodyInput.messages.at(0)->mailId);
	textualProtocolMessage.body.messages.push_back(mailbox.messages.at(mailIndex));
	//here we know only the message pointer itself was initialized
	delete textualBodyInput.messages.at(0);
	return true;
}
bool handleGetMailState(int workingServerSocket, Body &textualBodyInput,Mailbox &mailbox){
	TextualProtocolMessage textualProtocolMessage;
	bool shouldContinue = handleGetMailBusiness(textualProtocolMessage,textualBodyInput,mailbox);
	if (shouldContinue){
		shouldContinue = handleTextualMessageResponse(workingServerSocket,textualProtocolMessage);
	}
	return shouldContinue;
}
bool handleGetAttachmentState(int workingServerSocket, Body &textualBodyInput,Mailbox &mailbox){
	int mailIndex = getMailIndexFromMailId(textualBodyInput.messages.at(0)->mailId);
	MailMessage *&mailMessage = mailbox.messages.at(mailIndex);
	if (mailMessage == NULL){
		return false;
	}
	unsigned int attachmentIndex = textualBodyInput.messages.at(0)->numberOfAttachments-1;
	if (attachmentIndex>=mailMessage->attachments.size()){
		return false;
	}
	Attachment *&attachment = mailMessage->attachments.at(attachmentIndex);
	unsigned int attachmentSize = attachment->file.size;
	string headerOutputString = parseOutputStringFromHeader(GET_ATTACHMENT,attachmentSize);
	unsigned int bytesToSendForHeader = FIXED_HEADER_SIZE;
	char *headerBuffer = new char[bytesToSendForHeader];
	outputStringToBuffer(headerOutputString,headerBuffer);
	bool shouldContinue = sendAllData(workingServerSocket,headerBuffer,&bytesToSendForHeader);
	if (shouldContinue){
		shouldContinue = sendAllData(workingServerSocket,attachment->file.data,&attachmentSize);
	}
	delete[] headerBuffer;
	//delete input mailMessage;
	//here we know only the message pointer itself was initialized
	delete textualBodyInput.messages.at(0);
	return shouldContinue;
}
//helper method to print the ids of all mails for all user
void printMailBoxes(vector<string> &users,MailStore &userMailboxes){
	for (unsigned int i=0;i<users.size();i++){
		Mailbox &recipientMailbox = userMailboxes.at(users.at(i));
		cout <<"user: " << users.at(i);
		cout<<endl;
		vector<MailMessage*>::iterator it;
		for (it = recipientMailbox.messages.begin();it<recipientMailbox.messages.end();it++){
			MailMessage *&m = *it;
			if (m != NULL){
				unsigned int j = m->mailId;
				cout << "mailId:";
				cout << j;
				cout << endl;
			}
		}
	}
}
void deleteMailBoxes(MailStore &userMailboxes){
	map<string, Mailbox>::iterator itr;
	for(itr = userMailboxes.begin();itr!=userMailboxes.end();itr++){
		Mailbox &recipientMailbox = itr->second;
		for (unsigned int mailIndex=0; mailIndex < recipientMailbox.messages.size(); mailIndex++){
			handleDeleteMailInternal(mailIndex,recipientMailbox);
		}
	}
}
bool handleComposeMailState(int workingServerSocket, Body &textualBodyInput,MailStore &userMailboxes,string username){
	MailMessage *&inputMailMessage = textualBodyInput.messages.at(0);
	//iterate over the recipients and duplicate the message for every recipient
	for (unsigned int i=0;i<inputMailMessage->recipients.size();i++){
		//for each recipient we will clone the message
		Mailbox &recipientMailbox = userMailboxes.at(inputMailMessage->recipients.at(i));
		MailMessage *mailMessage = new MailMessage;
		mailMessage->mailId = ++recipientMailbox.lastInsertedMailMessageId;
		mailMessage->sender = username;
		mailMessage->subject = inputMailMessage->subject;
		mailMessage->messageText = inputMailMessage->messageText;
		mailMessage->recipients = inputMailMessage->recipients;
		mailMessage->numberOfAttachments = inputMailMessage->numberOfAttachments;
		for (unsigned int i=0;i<inputMailMessage->attachments.size();i++){
			Attachment *&inputAttachment = inputMailMessage->attachments.at(i);
			Attachment *attachment = new Attachment;
			attachment->name = inputAttachment->name;
			attachment->serialNumber = inputAttachment->serialNumber;
			attachment->file.size = inputAttachment->file.size;
			attachment->file.data = new char[attachment->file.size];
			memcpy(attachment->file.data,inputAttachment->file.data,attachment->file.size);
			mailMessage->attachments.push_back(attachment);
		}
		recipientMailbox.messages.push_back(mailMessage);
	}
	//clean up of the input
	for (unsigned int i=0;i<inputMailMessage->attachments.size();i++){
		Attachment *&attachment = inputMailMessage->attachments.at(i);
		delete[] attachment->file.data;
		delete attachment;
	}
	delete inputMailMessage;
	return handleInfoMessageResponse(workingServerSocket,COMPOSE_MAIL,"Mail Sent");
}

void handleUserSession(int workingServerSocket,
		MailStore &userMailboxes) {
	char headerBuffer[FIXED_HEADER_SIZE];
	bool shouldContinue = handleConnectState(workingServerSocket);
	if (!shouldContinue) {
		return;
	}
	string usernameOfLoggedInUser;
	bool loginSucceeded = false;
	shouldContinue = handleLoginState(workingServerSocket, userMailboxes,usernameOfLoggedInUser,loginSucceeded);
	if (!shouldContinue || !loginSucceeded) {
		return;
	}

	bool endUserSession = false;
	Mailbox &loggedInUserMailbox = userMailboxes.at(usernameOfLoggedInUser);
	while (!endUserSession) {
		Header header;
		shouldContinue = receiveAllData(workingServerSocket, headerBuffer,
				FIXED_HEADER_SIZE);
		if (!shouldContinue) {
			break;
		}
		parseHeaderFromBuffer(headerBuffer, header);
		char *bodyBuffer = new char[header.bodyLength];
		if (header.bodyLength>0){
			shouldContinue = receiveAllData(workingServerSocket, bodyBuffer, header.bodyLength);
		}
		if (!shouldContinue) {
			break;
		}
		Body textualBodyInput;
		parseTextualBasedBodyFromBuffer(bodyBuffer, header, textualBodyInput);
		switch (header.workflowIdentifier){
		case SHOW_INBOX:handleShowInboxState(workingServerSocket,loggedInUserMailbox);break;
		case GET_MAIL:handleGetMailState(workingServerSocket,textualBodyInput,loggedInUserMailbox);break;
		case DELETE_MAIL:handleDeleteMailState(workingServerSocket,textualBodyInput,loggedInUserMailbox);break;
		case COMPOSE_MAIL:handleComposeMailState(workingServerSocket,textualBodyInput,userMailboxes,usernameOfLoggedInUser);break;
		case GET_ATTACHMENT:handleGetAttachmentState(workingServerSocket,textualBodyInput,loggedInUserMailbox);break;
		default:endUserSession=true;break;//If a different kind of message arrives it is either quit or a bug in the client which causes
		//the session to be closed
		}
		delete[] bodyBuffer;
	}
}
typedef struct ArgumentsServerStruct {
	string usersFilePath;
	int parameterPort;
} ArgumentsServer;
bool parseArgumentsServer(int argc, char* argv[],ArgumentsServer &arguments){
	if (argc < 2){
		return false;
	}
	string usersFilePathArgument(argv[1]);
	arguments.usersFilePath = usersFilePathArgument;
	if (argc==2){
		//default port
		arguments.parameterPort = defaultPort;
	} else {
		string parameterPortArgument(argv[2]);
		arguments.parameterPort = parseStringToInt(parameterPortArgument);
	}
	return true;
}
int main(int argc, char* argv[]){
	ArgumentsServer arguments;
	bool successfullyParsedArgs = parseArgumentsServer(argc,argv,arguments);
	if (!successfullyParsedArgs){
		return 0;
	}
	MailStore userMailboxes;
	initUsers(userMailboxes, arguments.usersFilePath);

	int listeningServerSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (listeningServerSocket < 0) {
		perror("socket creation");
		return -1;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(arguments.parameterPort);
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	int retValue = bind(listeningServerSocket,
			(struct sockaddr *) &serverAddress, sizeof(serverAddress));
	if (retValue < 0) {
		perror("port in use");
		TEMP_FAILURE_RETRY(close (listeningServerSocket));//We do this as to overcome temproray issues with closing the socket
		return -1;
	}

	retValue = listen(listeningServerSocket, 5);
	if (retValue < 0) {
		perror("listen");
		TEMP_FAILURE_RETRY(close (listeningServerSocket));//We do this as to overcome temproray issues with closing the socket
		return -1;
	}
	while (true) {
		sockaddr_in client_info;
		unsigned int client_info_size = sizeof(client_info);
		int workingServerSocket = accept(listeningServerSocket,
				(struct sockaddr*) &client_info, &client_info_size);
		if (workingServerSocket < 0) {
			if (errno == EINTR) {
				// EINTR == This call did not succeed because it was interrupted.
				//			However, if you try again, it will probably work.
				//	So we will try again.
				continue;
			}
			//Other errors are probably fatal and we'll be shutting down
		    perror("accept");
			TEMP_FAILURE_RETRY(close (listeningServerSocket));//We do this as to overcome temproray issues with closing the socket
			return -1;
		}
		try {
			handleUserSession(workingServerSocket, userMailboxes);
		} catch (...){
			//we don't do anything here since we explicitly don't care if a user sent in faulty input
			//their session is automatically ended and the server continues on with its business
		}
		TEMP_FAILURE_RETRY(close (workingServerSocket));//We do this as to overcome temproray issues with closing the socket
	}
	TEMP_FAILURE_RETRY(close (listeningServerSocket));//We do this as to overcome temproray issues with closing the socket
	deleteMailBoxes(userMailboxes);
	return 0;
}

