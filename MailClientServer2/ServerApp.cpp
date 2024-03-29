/*
 * ServerApp.cpp
 *
 *  Created on: Nov 22, 2011
 *      Author: ittai zeidman
 */

#include "ProtocolParserServer.h"
#include "StringUtils.h"
#include "SocketHandler.h"
#include <string.h>
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
const int MAX_PERMITTED_USERS = 1023;
const string NO_LOGIN_STR = "NO_LOGIN";
typedef pair<string, bool> LoggedInUserPair;
typedef vector<LoggedInUserPair> LoggedInUsersVector;
LoggedInUsersVector loggedInUsersVector;
typedef struct MailboxStruct {
	int lastInsertedMailMessageId;
	string password;
	vector<MailMessage*> messages;
	vector<string> chatMessages;
	unsigned int indexInLoggedInUsersList;
	int activeSocket;
} Mailbox;
typedef struct BufferedHeaderStruct {
	unsigned int numRemainingBytes;
	char buffer[FIXED_HEADER_SIZE];
	Header header;
} BufferedHeader;
enum ConversationState {
	READ_HEADER, READ_BODY, PREPARE_RESPONSE, SEND_RESPONSE, FINISHED
};
enum ConnectionState {
	CONNECTION, LOGIN_S, WORK
};
typedef struct BufferedTextualProtocolMessageStruct {
	unsigned int numRemainingBytes;
	char *startLocationBuffer;//Points to the begining of the message
	char *workingLocationBuffer;//Points to the current working location of the message
	TextualProtocolMessage textualProtocolMessage;
	void clearBuffer() {
		if (startLocationBuffer != NULL) {
			delete[] startLocationBuffer;
			startLocationBuffer = NULL;
			workingLocationBuffer = NULL;
		}
	}
	void initBuffer(int numBytes) {
		numRemainingBytes = numBytes;
		startLocationBuffer = new char[numBytes];
		workingLocationBuffer = startLocationBuffer;
	}
	bool bufferUninitialized() {
		return startLocationBuffer == NULL;
	}
	void reset() {
		numRemainingBytes = 0;
		clearBuffer();
		TextualProtocolMessage dummyNullMessage;
		textualProtocolMessage = dummyNullMessage;//This will override the current message with a null message
	}
} BufferedTextualProtocolMessage;
typedef struct ServerSingleConversationStruct {
	BufferedTextualProtocolMessage inMessage;
	BufferedTextualProtocolMessage outMessage;
	ConversationState state;
	void advanceConversation() {
		switch (state) {
		case READ_HEADER:
			state = READ_BODY;
			break;
		case READ_BODY:
			state = PREPARE_RESPONSE;
			break;
		case PREPARE_RESPONSE:
			state = SEND_RESPONSE;
			break;
		case SEND_RESPONSE:
			state = FINISHED;
			break;
		case FINISHED:
			break;
		}
	}
	bool isInState(ConversationState inputState) {
		return state == inputState;
	}
	bool isInReadHeaderState() {
		return isInState(READ_HEADER);
	}
	bool isInReadBodyState() {
		return isInState(READ_BODY);
	}
	bool isInPrepareResponseState() {
		return isInState(PREPARE_RESPONSE);
	}
	bool isInSendResponseState() {
		return isInState(SEND_RESPONSE);
	}
	//TODO see if this method is needed
	bool isInFinishedState() {
		return isInState(FINISHED);
	}
	void reset() {
		state = READ_HEADER;
		inMessage.reset();
		outMessage.reset();
	}
} ServerSingleConversation;
typedef struct ServerConnectionStruct {
	int workingSocket;
	ServerSingleConversation clientInitiatedConversation;
	ServerSingleConversation serverInitiatedConversation;
	Mailbox *userMailBox;
	string loggedInUsername;
	ConnectionState connectionState;
	void init(int workingServerSocket){
		connectionState = CONNECTION;
		workingSocket = workingServerSocket;
		loggedInUsername = NO_LOGIN_STR;
		userMailBox = NULL;
		BufferedTextualProtocolMessage dummyMessage;
		dummyMessage.numRemainingBytes = 0;
		dummyMessage.startLocationBuffer = NULL;
		dummyMessage.workingLocationBuffer = NULL;
		clientInitiatedConversation.state = READ_HEADER;
		serverInitiatedConversation.state = READ_HEADER;
		clientInitiatedConversation.inMessage = dummyMessage;
		serverInitiatedConversation.inMessage = dummyMessage;
		clientInitiatedConversation.outMessage = dummyMessage;
		serverInitiatedConversation.outMessage = dummyMessage;
	}
	bool containsChatMessagesToBeSent(){
		return (((userMailBox != NULL) && (!userMailBox->chatMessages.empty())) ||
				serverInitiatedConversation.isInSendResponseState());
	}
} ServerConnection;
typedef struct ServerFDSetsStruct {
	FDPairSets fDPairSetsCurSelectResponse;
	FDPairSets fDPairSetsNextSelectResponse;
	FDPairSets fDPairSetsPrevSelectRequest;
	int curfdmax;
	int nextfdmax;
	void init(){
		fDPairSetsCurSelectResponse.init();
		fDPairSetsNextSelectResponse.init();
		fDPairSetsPrevSelectRequest.init();
		nextfdmax = 0;
		curfdmax = 0;
	}
	void beginIteration(){
		//we want to actually schedule for the select operation the releavnt fd_sets
		fDPairSetsCurSelectResponse.copy(fDPairSetsNextSelectResponse);
		//we want to backup the working requests
		fDPairSetsPrevSelectRequest.copy(fDPairSetsNextSelectResponse);
		//We want to re initialize the sets for the iteration itself
		fDPairSetsNextSelectResponse.init();
		curfdmax = nextfdmax;
		nextfdmax = 0;
	}
} ServerFDSets;
typedef pair<string, Mailbox> MailboxUserPair;
typedef map<string, Mailbox> MailStore;
typedef pair<int, ServerConnection> ConnectionStorePair;
typedef map<int, ServerConnection> ConnectionStore;
const char USER_RECORD_SEPARATOR = '\t';
const int USER_RECORD_SEPARATOR_LENGTH = 1;
const int EOF_LENGTH = 2;
#define getUserRecordInFileLength() MAX_USERNAME_LENGTH+MAX_PASSWORD_LENGTH+USER_RECORD_SEPARATOR_LENGTH+EOF_LENGTH
void initUsers(MailStore &userMailboxes, const string usersFilePath) {
	//Initialize server data structures
	//Init user map
	//add entry for each user in user-file inclusive of password and empty mailbox
	//mailbox starts as an empty list of MailMessages and a lastInsertedMailMessageId int member initialized to 0
	char buffer[getUserRecordInFileLength()];
	ifstream myfile(usersFilePath.c_str());
	unsigned int indexInLoggedInUsersList = 0;
	while (!myfile.eof()) {
		myfile.getline(buffer, getUserRecordInFileLength());
		string username = getTextualField(USER_RECORD_SEPARATOR, buffer);
		Mailbox m;
		m.lastInsertedMailMessageId = 0;
		//we're looking for then null character since getLine removes the \n characters and appends the null character instead
		m.password = getTextualField(NULL,
				(buffer + username.length() + USER_RECORD_SEPARATOR_LENGTH));
		m.indexInLoggedInUsersList = indexInLoggedInUsersList++;
		userMailboxes.insert(MailboxUserPair(username, m));
		loggedInUsersVector.push_back(LoggedInUserPair(username, false));
	}
}
bool authenticationPassed(Body &body, MailStore &userMailboxes) {
	bool authenticationPassed = false;
	MailStore::iterator iter = userMailboxes.find(body.userLogin.username);
	if (iter != userMailboxes.end()) {
		authenticationPassed =
				(iter->second.password == body.userLogin.password);
	}
	return authenticationPassed;
}

ActionResult receiveHeader(ServerConnection &serverConnection) {
	ServerSingleConversation &clientInitiatedConversation =
			serverConnection.clientInitiatedConversation;
	BufferedTextualProtocolMessage &inMessage =
			clientInitiatedConversation.inMessage;
	if (inMessage.bufferUninitialized()) {
		//we need to initialize
		inMessage.initBuffer(FIXED_HEADER_SIZE);
	}
	ActionResult result = receiveData(serverConnection.workingSocket,
			inMessage.workingLocationBuffer, inMessage.numRemainingBytes);
	if (result.errorOccurred) {
		inMessage.clearBuffer();
	}
	if (result.shouldContinue()) {
		parseHeaderFromBuffer(inMessage.startLocationBuffer,
				inMessage.textualProtocolMessage.header);
		inMessage.clearBuffer();
	}
	return result;
}
ActionResult isExpectedHeader(ServerConnection &serverConnection,
		WorkflowIdentifier expectedWorkflowIdentifier) {
	ActionResult result = receiveHeader(serverConnection);
	if (result.shouldContinue()) {
		ServerSingleConversation &clientInitiatedConversation =
				serverConnection.clientInitiatedConversation;
		BufferedTextualProtocolMessage &inMessage =
				clientInitiatedConversation.inMessage;
		result.errorOccurred =
				(inMessage.textualProtocolMessage.header.workflowIdentifier
						!= expectedWorkflowIdentifier);
	}
	return result;
}

void prepareTextualMessageResponse(BufferedTextualProtocolMessage &outMessage) {
	TextualProtocolMessage &textualProtocolMessage =
			outMessage.textualProtocolMessage;
	string outString = parseBufferFromTextualMessage(textualProtocolMessage);
	unsigned int messageSize = outString.length();
	outMessage.initBuffer(messageSize);
	outputStringToBuffer(outString, outMessage.startLocationBuffer);
}
void prepareInfoMessageResponse(
		ServerSingleConversation &serverSingleConversation,
		WorkflowIdentifier workflowIdentifier, string infoMessage) {
	TextualProtocolMessage &textualProtocolMessage =
			serverSingleConversation.outMessage.textualProtocolMessage;
	textualProtocolMessage.header.workflowIdentifier = workflowIdentifier;
	textualProtocolMessage.body.infoMessage = infoMessage;
	textualProtocolMessage.header.bodyLength =
			textualProtocolMessage.body.infoMessage.length();
	prepareTextualMessageResponse(serverSingleConversation.outMessage);
}
void prepareConnectResponse(
		ServerSingleConversation &serverSingleConversation) {
	prepareInfoMessageResponse(serverSingleConversation, CONNECT_TO_SERVER,
			"Welcome!");
}
ActionResult handleConnectRequest(ServerConnection &serverConnection) {
	ActionResult userActionResult = isExpectedHeader(serverConnection,
			CONNECT_TO_SERVER);
	return userActionResult;
}
bool isSocketMemberOfSet(int workingSocket,fd_set *fdSet) {
	return FD_ISSET(workingSocket,fdSet);
}
bool isSocketReadyForReading(int workingSocket,ServerFDSets &serverFDSets) {
	return isSocketMemberOfSet(workingSocket,&serverFDSets.fDPairSetsCurSelectResponse.read);
}
bool isSocketReadyForWriting(int workingSocket,ServerFDSets &serverFDSets) {
	return isSocketMemberOfSet(workingSocket,&serverFDSets.fDPairSetsCurSelectResponse.write);
}
bool isSocketScheduledForReading(int workingSocket,ServerFDSets &serverFDSets) {
	return isSocketMemberOfSet(workingSocket,&serverFDSets.fDPairSetsNextSelectResponse.read);
}
bool isSocketScheduledForWriting(int workingSocket,ServerFDSets &serverFDSets) {
	return isSocketMemberOfSet(workingSocket,&serverFDSets.fDPairSetsNextSelectResponse.write);
}
bool socketWasPreviouselyScheduledForRead(int workingSocket,ServerFDSets &serverFDSets){
	return isSocketMemberOfSet(workingSocket,&serverFDSets.fDPairSetsPrevSelectRequest.read);
}
bool socketWasPreviouselyScheduledForWrite(int workingSocket,ServerFDSets &serverFDSets){
	return isSocketMemberOfSet(workingSocket,&serverFDSets.fDPairSetsPrevSelectRequest.write);
}

void scheduleSocket(int workingSocket,fd_set *fdSet,int &nextFdMax){
	FD_SET(workingSocket, fdSet);
	if (workingSocket > nextFdMax) { // keep track of the max
		nextFdMax = workingSocket;
	}
}
void scheduleSocketForReading(int workingSocket,ServerFDSets &serverFDSets) {
	scheduleSocket(workingSocket, &serverFDSets.fDPairSetsNextSelectResponse.read,serverFDSets.nextfdmax);
}
void scheduleSocketForWriting(int workingSocket,ServerFDSets &serverFDSets) {
	scheduleSocket(workingSocket, &serverFDSets.fDPairSetsNextSelectResponse.write,serverFDSets.nextfdmax);
}
ActionResult handleConnectState(ServerConnection &serverConnection,ServerFDSets &serverFDSets) {
	ActionResult userActionResult = PROBLEM_RESULT;
	ServerSingleConversation &clientInitiatedConversation =
			serverConnection.clientInitiatedConversation;
	if (isSocketReadyForReading(serverConnection.workingSocket,serverFDSets)
			&& clientInitiatedConversation.isInReadHeaderState()) {
		userActionResult = handleConnectRequest(serverConnection);
		if (userActionResult.shouldContinue()) {
			clientInitiatedConversation.advanceConversation();
			//We're skipping the read body state because we know that the connect request is body-less
			clientInitiatedConversation.advanceConversation();
		}
	}
	if (clientInitiatedConversation.isInPrepareResponseState()) {
		prepareConnectResponse(clientInitiatedConversation);
		clientInitiatedConversation.advanceConversation();
	}
	if (clientInitiatedConversation.isInSendResponseState()
			&& isSocketReadyForWriting(serverConnection.workingSocket,serverFDSets)) {
		//We know we can send data without fear of a "serverInitiatedConversation" because it is blocked at this phase
		userActionResult = sendData(serverConnection.workingSocket,
				clientInitiatedConversation.outMessage.workingLocationBuffer,
				clientInitiatedConversation.outMessage.numRemainingBytes);
		if (userActionResult.shouldContinue()) {
			serverConnection.connectionState = LOGIN_S;
			serverConnection.clientInitiatedConversation.reset();
		}
	}
	return userActionResult;
}
void prepareLoginResponse(ServerSingleConversation &serverSingleConversation,
		bool loginSucceeded) {
	string infoMessage;
	if (loginSucceeded) {
		infoMessage = "Connected to server";
	} else {
		infoMessage = "Username password pair not correct";
	}
	prepareInfoMessageResponse(serverSingleConversation, LOGIN, infoMessage);
}
ActionResult handleLoginReadHeaderState(
		ServerConnection &serverConnection) {
	ActionResult userActionResult = isExpectedHeader(serverConnection,
			LOGIN);
	return userActionResult;
}
void updateLoggedInUsersVector(unsigned int indexInLoggedInUsersVector, bool loggedIn) {
	if (indexInLoggedInUsersVector<loggedInUsersVector.size()){
		LoggedInUserPair &loggedInUserPair = loggedInUsersVector.at(indexInLoggedInUsersVector);
		loggedInUserPair.second = loggedIn;
	}
}

ActionResult handleLoginReadBodyState(ServerConnection &serverConnection,
		MailStore &userMailboxes) {
	ServerSingleConversation &clientInitiatedConversation =
			serverConnection.clientInitiatedConversation;
	BufferedTextualProtocolMessage &inMessage =
			clientInitiatedConversation.inMessage;
	if (inMessage.bufferUninitialized()) {
		Header &header = inMessage.textualProtocolMessage.header;
		inMessage.initBuffer(header.bodyLength);
	}
	ActionResult userActionResult = receiveData(
			serverConnection.workingSocket, inMessage.workingLocationBuffer,
			inMessage.numRemainingBytes);
	if (userActionResult.errorOccurred) {
		inMessage.clearBuffer();
	}
	if (userActionResult.shouldContinue()) {
		bool loginSucceeded = false;
		TextualProtocolMessage &message = inMessage.textualProtocolMessage;
		parseTextualBasedBodyFromBuffer(inMessage.startLocationBuffer, message.header,
				message.body);
		loginSucceeded = authenticationPassed(message.body, userMailboxes);
		if (loginSucceeded) {
			serverConnection.loggedInUsername = message.body.userLogin.username;
			MailStore::iterator iter = userMailboxes.find(serverConnection.loggedInUsername);
			//No need to check if the iter is different than end since we have been authenticated => the user is in the store
			serverConnection.userMailBox = &iter->second;
			serverConnection.userMailBox->activeSocket = serverConnection.workingSocket;
			updateLoggedInUsersVector(serverConnection.userMailBox->indexInLoggedInUsersList, true);
		} else {
			serverConnection.loggedInUsername = NO_LOGIN_STR;
		}
		inMessage.clearBuffer();
		clientInitiatedConversation.advanceConversation();
		prepareLoginResponse(clientInitiatedConversation, loginSucceeded);
	}
	return userActionResult;
}
ActionResult handleLoginRequest(ServerConnection &serverConnection,
		MailStore &userMailboxes) {
	ActionResult userActionResult = PROBLEM_RESULT;
	ServerSingleConversation &clientInitiatedConversation =
			serverConnection.clientInitiatedConversation;
	if (clientInitiatedConversation.isInReadHeaderState()) {
		userActionResult = handleLoginReadHeaderState(serverConnection);
		if (userActionResult.shouldContinue()) {
			clientInitiatedConversation.advanceConversation();
		}
	}
	if (clientInitiatedConversation.isInReadBodyState()) {
		userActionResult = handleLoginReadBodyState(serverConnection,
				userMailboxes);
		if (userActionResult.shouldContinue()) {
			clientInitiatedConversation.advanceConversation();
		}
	}
	return userActionResult;
}
ActionResult handleLoginState(ServerConnection &serverConnection,
		MailStore &userMailboxes,ServerFDSets &serverFDSets) {
	ActionResult userActionResult = PROBLEM_RESULT;
	ServerSingleConversation &clientInitiatedConversation =
			serverConnection.clientInitiatedConversation;
	if (isSocketReadyForReading(serverConnection.workingSocket,serverFDSets)
			&& !clientInitiatedConversation.isInSendResponseState()) {
		userActionResult = handleLoginRequest(serverConnection, userMailboxes);
	}
	if (isSocketReadyForWriting(serverConnection.workingSocket,serverFDSets)
			&& clientInitiatedConversation.isInSendResponseState()) {
		//We know we can send data without fear of a "serverInitiatedConversation" because it is blocked at this phase
		userActionResult = sendData(serverConnection.workingSocket,
				clientInitiatedConversation.outMessage.workingLocationBuffer,
				clientInitiatedConversation.outMessage.numRemainingBytes);
		if (userActionResult.shouldContinue()) {
			if (serverConnection.loggedInUsername != NO_LOGIN_STR) { //Login succeeded
				serverConnection.connectionState = WORK;
				clientInitiatedConversation.reset();
			} else {
				userActionResult.errorOccurred = true; //Singal to throw the user out
			}
		}
	}
	return userActionResult;
}
int getMailIndexFromMailId(int mailId) {
	return mailId - 1;
}
bool handleDeleteMailInternal(const int mailIndex, Mailbox &mailbox) {
	//delete the attachment
	MailMessage *&mailMessage = mailbox.messages.at(mailIndex);
	if (mailMessage == NULL) {
		return false;
	}
	//free attachments
	for (unsigned int i = 0; i < mailMessage->attachments.size(); i++) {
		delete[] mailMessage->attachments.at(i)->file.data; //delete attachment content
		delete mailMessage->attachments.at(i);
		mailMessage->attachments.at(i) = NULL;
	}
	//free message
	delete mailMessage;
	mailbox.messages.at(mailIndex) = NULL;
	return true;
}
bool handleDeleteMailBusiness(Body &textualBodyInput, Mailbox &mailbox) {
	int mailIndex = getMailIndexFromMailId(
			textualBodyInput.messages.at(0)->mailId);
	bool shouldContinue = handleDeleteMailInternal(mailIndex, mailbox);
	//delete input mailMessage;
	//here we know only the message pointer itself was initialized
	delete textualBodyInput.messages.at(0);
	return shouldContinue;
}
void handleDeleteMailResponse(
		ServerSingleConversation &serverSingleConversation) {
	prepareInfoMessageResponse(serverSingleConversation, DELETE_MAIL, "");
}
bool handleDeleteMailState(ServerSingleConversation &serverSingleConversation,
		Mailbox &mailbox) {
	bool shouldContinue = handleDeleteMailBusiness(
			serverSingleConversation.inMessage.textualProtocolMessage.body,
			mailbox);
	if (shouldContinue) {
		handleDeleteMailResponse(serverSingleConversation);
	}
	return shouldContinue;
}
void handleShowInboxBusiness(TextualProtocolMessage &textualProtocolMessage,
		Mailbox &mailbox) {
	textualProtocolMessage.header.workflowIdentifier = SHOW_INBOX;
	textualProtocolMessage.body.messages = mailbox.messages;
}
void handleShowInboxState(ServerSingleConversation &serverSingleConversation,
		Mailbox &mailbox) {
	handleShowInboxBusiness(
			serverSingleConversation.outMessage.textualProtocolMessage,
			mailbox);
	prepareTextualMessageResponse(serverSingleConversation.outMessage);
}
void handleGetMailBusiness(TextualProtocolMessage &textualProtocolMessage,
		Body &textualBodyInput, Mailbox &mailbox) {
	textualProtocolMessage.header.workflowIdentifier = GET_MAIL;
	int mailIndex = getMailIndexFromMailId(
			textualBodyInput.messages.at(0)->mailId);
	textualProtocolMessage.body.messages.push_back(
			mailbox.messages.at(mailIndex));
	//here we know only the message pointer itself was initialized
	delete textualBodyInput.messages.at(0);
}
void handleGetMailState(ServerSingleConversation &serverSingleConversation,
		Mailbox &mailbox) {
	handleGetMailBusiness(
			serverSingleConversation.outMessage.textualProtocolMessage,
			serverSingleConversation.inMessage.textualProtocolMessage.body,
			mailbox);
	prepareTextualMessageResponse(serverSingleConversation.outMessage);
}
void outputAttachmentToBuffer(Attachment *&attachment,
		char *attachmentBeginingPointer) {
	memcpy(attachmentBeginingPointer, attachment->file.data, attachment->file.size);
}
bool handleGetAttachmentState(
		ServerSingleConversation &serverSingleConversation, Mailbox &mailbox) {
	Body &textualBodyInput =
			serverSingleConversation.inMessage.textualProtocolMessage.body;
	int mailIndex = getMailIndexFromMailId(
			textualBodyInput.messages.at(0)->mailId);
	MailMessage *&mailMessage = mailbox.messages.at(mailIndex);
	if (mailMessage == NULL) {
		return false;
	}
	unsigned int attachmentIndex =
			textualBodyInput.messages.at(0)->numberOfAttachments - 1;
	if (attachmentIndex >= mailMessage->attachments.size()) {
		return false;
	}
	Attachment *&attachment = mailMessage->attachments.at(attachmentIndex);
	unsigned int attachmentSize = attachment->file.size;
	string headerOutputString = parseOutputStringFromHeader(GET_ATTACHMENT,
			attachmentSize);
	BufferedTextualProtocolMessage &outMessage =
			serverSingleConversation.outMessage;
	outMessage.initBuffer(FIXED_HEADER_SIZE + attachmentSize);
	outputStringToBuffer(headerOutputString, outMessage.startLocationBuffer);
	char *attachmentBeginingPointer = outMessage.startLocationBuffer + FIXED_HEADER_SIZE;
	outputAttachmentToBuffer(attachment, attachmentBeginingPointer);
	//delete input mailMessage;
	//here we know only the message pointer itself was initialized
	delete textualBodyInput.messages.at(0);
	return true;
}
//helper method to print the ids of all mails for all user
void printMailBoxes(vector<string> &users, MailStore &userMailboxes) {
	for (unsigned int i = 0; i < users.size(); i++) {
		Mailbox &recipientMailbox = userMailboxes.at(users.at(i));
		cout << "user: " << users.at(i);
		cout << endl;
		vector<MailMessage*>::iterator it;
		for (it = recipientMailbox.messages.begin();
				it < recipientMailbox.messages.end(); it++) {
			MailMessage *&m = *it;
			if (m != NULL) {
				unsigned int j = m->mailId;
				cout << "mailId:";
				cout << j;
				cout << endl;
			}
		}
	}
}
void deleteMailBoxes(MailStore &userMailboxes) {
	map<string, Mailbox>::iterator itr;
	for (itr = userMailboxes.begin(); itr != userMailboxes.end(); itr++) {
		Mailbox &recipientMailbox = itr->second;
		for (unsigned int mailIndex = 0;
				mailIndex < recipientMailbox.messages.size(); mailIndex++) {
			handleDeleteMailInternal(mailIndex, recipientMailbox);
		}
	}
}
void saveMail(Mailbox &recipientMailbox,string &sender,MailMessage *&inputMailMessage){
	MailMessage *mailMessage = new MailMessage;
	mailMessage->mailId = ++recipientMailbox.lastInsertedMailMessageId;
	mailMessage->sender = sender;
	mailMessage->subject = inputMailMessage->subject;
	mailMessage->messageText = inputMailMessage->messageText;
	mailMessage->recipients = inputMailMessage->recipients;
	mailMessage->numberOfAttachments =
			inputMailMessage->numberOfAttachments;
	for (unsigned int i = 0; i < inputMailMessage->attachments.size();
			i++) {
		Attachment *&inputAttachment = inputMailMessage->attachments.at(i);
		Attachment *attachment = new Attachment;
		attachment->name = inputAttachment->name;
		attachment->serialNumber = inputAttachment->serialNumber;
		attachment->file.size = inputAttachment->file.size;
		attachment->file.data = new char[attachment->file.size];
		memcpy(attachment->file.data, inputAttachment->file.data,
				attachment->file.size);
		mailMessage->attachments.push_back(attachment);
	}
	recipientMailbox.messages.push_back(mailMessage);
}
void handleComposeMailState(ServerSingleConversation &serverSingleConversation,
		MailStore &userMailboxes, string username) {
	MailMessage *&inputMailMessage =
			serverSingleConversation.inMessage.textualProtocolMessage.body.messages.at(
					0);
	//iterate over the recipients and duplicate the message for every recipient
	for (unsigned int i = 0; i < inputMailMessage->recipients.size(); i++) {
		//for each recipient we will clone the message
		string &recipient = inputMailMessage->recipients.at(i);
		Mailbox &recipientMailbox = userMailboxes.at(recipient);
		saveMail(recipientMailbox,username,inputMailMessage);
	}
	//clean up of the input
	for (unsigned int i = 0; i < inputMailMessage->attachments.size(); i++) {
		Attachment *&attachment = inputMailMessage->attachments.at(i);
		delete[] attachment->file.data;
		delete attachment;
	}
	delete inputMailMessage;
	prepareInfoMessageResponse(serverSingleConversation, COMPOSE_MAIL,
			"Mail Sent");
}
bool shouldSendChatMessage(ServerConnection &serverConnection,ServerFDSets &serverFDSets) {
	//handle chat message sending only if the socket is ready for writing
	//we aren't already sending something to the client on they're own request
	//and we actually have something to send them (i.e. a chat message)
	return isSocketReadyForWriting(serverConnection.workingSocket,serverFDSets)
			&& !serverConnection.clientInitiatedConversation.isInSendResponseState()
			&& serverConnection.containsChatMessagesToBeSent();
}
bool shouldSendNonChatMessageResponse(ServerConnection &serverConnection,ServerFDSets &serverFDSets) {
	return isSocketReadyForWriting(serverConnection.workingSocket,serverFDSets)
			&& serverConnection.clientInitiatedConversation.isInSendResponseState()
			&& !serverConnection.serverInitiatedConversation.isInSendResponseState();
}
void prepareChatMessageForClient(vector<string> &chatMessages,ServerSingleConversation &serverInitiatedConversation){
	string messagesStr;
	for (vector<string>::iterator itr = chatMessages.begin(); itr != chatMessages.end(); itr++) {
		messagesStr.append(*itr);
	}
	messagesStr.erase(messagesStr.size()-1);//Remove the last line feed character
	prepareInfoMessageResponse(serverInitiatedConversation, RECEIVE_CHAT_MSG,
			messagesStr);
	chatMessages.clear();
}
ActionResult handleChatMessageServerSending(ServerConnection &serverConnection) {
	ServerSingleConversation &serverInitiatedConversation =
			serverConnection.serverInitiatedConversation;
	BufferedTextualProtocolMessage &outMessage =
			serverInitiatedConversation.outMessage;
	if (!serverInitiatedConversation.isInSendResponseState()) { //If we haven't started sending yet we need to prepare the message
		prepareChatMessageForClient(serverConnection.userMailBox->chatMessages,serverInitiatedConversation);
		serverInitiatedConversation.state = SEND_RESPONSE;
	}
	ActionResult userActionResult = sendData(serverConnection.workingSocket,
			outMessage.workingLocationBuffer, outMessage.numRemainingBytes);
	return userActionResult;
}
bool messageContainsNoBody(BufferedTextualProtocolMessage &message){
	return (message.textualProtocolMessage.header.bodyLength == 0);
}
string getListOfLoggedInUsers(){
	string listOfLoggedInUsers = "Online users: ";
	for (vector<LoggedInUserPair>::iterator it = loggedInUsersVector.begin();
			it < loggedInUsersVector.end(); it++) {
		LoggedInUserPair &loggedInUserPair = *it;
		if (loggedInUserPair.second == true){
			listOfLoggedInUsers.append(loggedInUserPair.first);
			listOfLoggedInUsers.append(COMMA_LIST_SEPARATOR_STRING);
		}
	}
	listOfLoggedInUsers.erase(listOfLoggedInUsers.size()-1);//we want to remove the last comma
	return listOfLoggedInUsers;
}
void handleShowOnlineUsersState(ServerSingleConversation &serverSingleConversation){
	string loggedInUsers = getListOfLoggedInUsers();
	prepareInfoMessageResponse(serverSingleConversation, SHOW_ONLINE_USERS,
			loggedInUsers);
}
bool isUserOnline(Mailbox &userMailbox){
	bool isOnline = false;
	unsigned int indexInLoggedInUsersVector = userMailbox.indexInLoggedInUsersList;
	if (indexInLoggedInUsersVector<loggedInUsersVector.size()){
		LoggedInUserPair &loggedInUserPair = loggedInUsersVector.at(indexInLoggedInUsersVector);
		isOnline = loggedInUserPair.second;
	}

	return isOnline;
}
void scheduleRecipientForWriting(Mailbox &recipientMailbox,ServerFDSets &serverFDSets){
	scheduleSocketForWriting(recipientMailbox.activeSocket,serverFDSets);
}
void handleSendChatMessageState(ServerSingleConversation &serverSingleConversation, MailStore &userMailboxes, string &sender,ServerFDSets &serverFDSets) {
	MailMessage *&inputMailMessage =
			serverSingleConversation.inMessage.textualProtocolMessage.body.messages.at(
					0);
	string responseMessage;
	for (unsigned int i = 0; i < inputMailMessage->recipients.size(); i++) {
		Mailbox &recipientMailbox = userMailboxes.at(
				inputMailMessage->recipients.at(i));
		if (isUserOnline(recipientMailbox)) {
			string chatMessage = "New message from " + sender + ":"
					+ inputMailMessage->messageText + LINE_FEED_CHARACTER;
			recipientMailbox.chatMessages.push_back(chatMessage);
			scheduleRecipientForWriting(recipientMailbox,serverFDSets);
			responseMessage = "";//If the user is online then no message is required back
		} else {
			//for each recipient we will clone the message
			saveMail(recipientMailbox,sender,inputMailMessage);
			responseMessage = "User is offline, message sent as mail";
		}
	}
	delete inputMailMessage;
	prepareInfoMessageResponse(serverSingleConversation, SEND_CHAT_MSG,
			responseMessage);
}

ActionResult handleUserInteraction(ServerConnection &serverConnection,
		MailStore &userMailboxes,ServerFDSets &serverFDSets) {
	ActionResult userActionResult = PROBLEM_RESULT;
	if (shouldSendChatMessage(serverConnection,serverFDSets)) {
		userActionResult = handleChatMessageServerSending(serverConnection);
		if (userActionResult.errorOccurred) {
			return userActionResult;
		}
		if (userActionResult.shouldContinue()) {
			serverConnection.serverInitiatedConversation.reset();
		}
	}
	Mailbox &loggedInUserMailbox = userMailboxes.at(
			serverConnection.loggedInUsername);
	ServerSingleConversation &clientInitiatedConversation =
			serverConnection.clientInitiatedConversation;
	if (isSocketReadyForReading(serverConnection.workingSocket,serverFDSets)
			&& clientInitiatedConversation.isInReadHeaderState()) {
		userActionResult = receiveHeader(serverConnection);
		if (userActionResult.shouldContinue()) {
			clientInitiatedConversation.advanceConversation();
		}
		if (messageContainsNoBody(clientInitiatedConversation.inMessage)){
			//If there is no body we can skip the read body state
			clientInitiatedConversation.advanceConversation();
		}
	}
	if (isSocketReadyForReading(serverConnection.workingSocket,serverFDSets)
			&& clientInitiatedConversation.isInReadBodyState()) {
		BufferedTextualProtocolMessage &inMessage =
				clientInitiatedConversation.inMessage;
		Header &header = inMessage.textualProtocolMessage.header;
		if (inMessage.bufferUninitialized()) {
			inMessage.initBuffer(header.bodyLength);
		}
		userActionResult = receiveData(serverConnection.workingSocket,
				inMessage.workingLocationBuffer, inMessage.numRemainingBytes);
		if (userActionResult.errorOccurred) {
			inMessage.clearBuffer();
		}
		if (userActionResult.shouldContinue()) {
			parseTextualBasedBodyFromBuffer(inMessage.startLocationBuffer, header,
					inMessage.textualProtocolMessage.body);
			clientInitiatedConversation.advanceConversation();
			inMessage.clearBuffer();
		}
	}
	if (clientInitiatedConversation.isInPrepareResponseState()) {
		switch (clientInitiatedConversation.inMessage.textualProtocolMessage.header.workflowIdentifier) {
		case SHOW_INBOX:
			handleShowInboxState(clientInitiatedConversation,
					loggedInUserMailbox);
			break;
		case GET_MAIL:
			handleGetMailState(clientInitiatedConversation,
					loggedInUserMailbox);
			break;
		case DELETE_MAIL:
			handleDeleteMailState(clientInitiatedConversation,
					loggedInUserMailbox);
			break;
		case COMPOSE_MAIL:
			handleComposeMailState(clientInitiatedConversation, userMailboxes,
					serverConnection.loggedInUsername);
			break;
		case GET_ATTACHMENT:
			handleGetAttachmentState(clientInitiatedConversation,
					loggedInUserMailbox);
			break;
		case SHOW_ONLINE_USERS:
			handleShowOnlineUsersState(clientInitiatedConversation);
			break;
		case SEND_CHAT_MSG:
			handleSendChatMessageState(clientInitiatedConversation, userMailboxes, serverConnection.loggedInUsername,serverFDSets);
			break;
		default:
			userActionResult.errorOccurred = true;
			break; //If a different kind of message arrives it is either quit or a bug in the client which causes
			//the session to be closed
		}
		if (!userActionResult.errorOccurred){
			clientInitiatedConversation.advanceConversation();
		}
	}
	if (shouldSendNonChatMessageResponse(serverConnection,serverFDSets)) {
		BufferedTextualProtocolMessage &outMessage =
				serverConnection.clientInitiatedConversation.outMessage;
		userActionResult = sendData(serverConnection.workingSocket,
				outMessage.workingLocationBuffer, outMessage.numRemainingBytes);
		if (userActionResult.shouldContinue()) {
			clientInitiatedConversation.reset();
		}
	}
	return userActionResult;
}
ActionResult handleUserSession(ServerConnection &serverConnection,
		MailStore &userMailboxes,ServerFDSets &serverFDSets) {
	ActionResult userActionResult = PROBLEM_RESULT;
	switch (serverConnection.connectionState) {
	case CONNECTION:
		userActionResult = handleConnectState(serverConnection,serverFDSets);
		break;
	case LOGIN_S:
		userActionResult = handleLoginState(serverConnection, userMailboxes,serverFDSets);
		break;
	case WORK:
		userActionResult = handleUserInteraction(serverConnection,
				userMailboxes,serverFDSets);
		break;
	}
	return userActionResult;
}
typedef struct ArgumentsServerStruct {
	string usersFilePath;
	int parameterPort;
} ArgumentsServer;
bool parseArgumentsServer(int argc, char* argv[], ArgumentsServer &arguments) {
	if (argc < 2) {
		return false;
	}
	string usersFilePathArgument(argv[1]);
	arguments.usersFilePath = usersFilePathArgument;
	if (argc == 2) {
		//default port
		arguments.parameterPort = defaultPort;
	} else {
		string parameterPortArgument(argv[2]);
		arguments.parameterPort = parseStringToInt(parameterPortArgument);
	}
	return true;
}
void printScheduledSockets(ServerFDSets &serverFDSets,ConnectionStore &connectionStore,int listeningServerSocket){
	for (int i=0;i<=serverFDSets.curfdmax;i++){
		if (i == listeningServerSocket){
			continue;
		}
		cout << i << ": ";
		if (isSocketMemberOfSet(i,&serverFDSets.fDPairSetsNextSelectResponse.read)){
			cout << "reading";
		}
		if (isSocketMemberOfSet(i,&serverFDSets.fDPairSetsNextSelectResponse.write)){
			cout << "writing ";
		}
		ConnectionStore::iterator iter = connectionStore.find(i);
		if (iter != connectionStore.end()){
			ServerConnection &serverConnection = iter->second;
			cout << serverConnection.connectionState;
			if (serverConnection.loggedInUsername != NO_LOGIN_STR){
				cout << " loggedinuser: " << serverConnection.loggedInUsername;
			}
		}
		cout << endl;
	}
}
bool hasMaxNumberOfConnectionsBeenReached(int numConnectedConnections){
	return (numConnectedConnections == MAX_PERMITTED_USERS);
}
void deleteUserConnection(ServerConnection &serverConnection){
	//Kill the conversation
	TEMP_FAILURE_RETRY(close (serverConnection.workingSocket));
	//We do this as to overcome temproray issues with closing the socket
	serverConnection.clientInitiatedConversation.reset();
	serverConnection.serverInitiatedConversation.reset();
	if (serverConnection.loggedInUsername != NO_LOGIN_STR) {
		updateLoggedInUsersVector(
				serverConnection.userMailBox->indexInLoggedInUsersList, false);
		serverConnection.userMailBox->activeSocket = 0;
		serverConnection.userMailBox = NULL;
	}
}
void deleteConnectionStore(ConnectionStore &connectionStore){
	for (ConnectionStore::iterator iter = connectionStore.begin() ; iter != connectionStore.end() ; iter++){
		ServerConnection &serverConnection = iter->second;
		int socket = serverConnection.workingSocket;
		deleteUserConnection(serverConnection);
		connectionStore.erase(socket);
	}

}
int main(int argc, char* argv[]) {
	ArgumentsServer arguments;
	bool successfullyParsedArgs = parseArgumentsServer(argc, argv, arguments);
	if (!successfullyParsedArgs) {
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
		TEMP_FAILURE_RETRY(close (listeningServerSocket));
		//We do this as to overcome temproray issues with closing the socket
		return -1;
	}

	retValue = listen(listeningServerSocket, 50);
	if (retValue < 0) {
		perror("listen");
		TEMP_FAILURE_RETRY(close (listeningServerSocket));
		//We do this as to overcome temproray issues with closing the socket
		return -1;
	}
	ServerFDSets serverFDSets;
	serverFDSets.init();
	serverFDSets.nextfdmax = listeningServerSocket;
	ConnectionStore connectionStore;
	int numConnectedConnections=0;
	while (true) {
		printScheduledSockets(serverFDSets,connectionStore,listeningServerSocket);
		scheduleSocketForReading(0,serverFDSets);
		scheduleSocketForReading(listeningServerSocket,serverFDSets);
		serverFDSets.beginIteration();
		if (select(serverFDSets.curfdmax + 1, &serverFDSets.fDPairSetsCurSelectResponse.read, &serverFDSets.fDPairSetsCurSelectResponse.write, NULL, NULL)
				== -1) {
			perror("select");
			return -1;
		}
		// run through the existing connections looking for data to read or write
		for (int i = 0; i <= serverFDSets.curfdmax; i++) {
			if (i==0 && isSocketReadyForReading(i,serverFDSets)){
				goto exitSection;
			}
			if (i == listeningServerSocket) {
				if (!hasMaxNumberOfConnectionsBeenReached(numConnectedConnections) && isSocketReadyForReading(i, serverFDSets)) { // we got a connection
					// handle new connections
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
						TEMP_FAILURE_RETRY(close (listeningServerSocket));
						//We do this as to overcome temproray issues with closing the socket
						return -1;
					}
					scheduleSocketForReading(workingServerSocket,serverFDSets);
					ServerConnection serverConnection;
					serverConnection.init(workingServerSocket);
					connectionStore.insert(
							ConnectionStorePair(workingServerSocket,
									serverConnection));
					numConnectedConnections++;
				}
				continue;
			}
			if (isSocketReadyForReading(i,serverFDSets) || isSocketReadyForWriting(i,serverFDSets)) {
				// handle data from a client
				try {
					ConnectionStore::iterator iter = connectionStore.find(i) ;
					ServerConnection &serverConnection = iter->second;
					ActionResult userActionResult = handleUserSession(
							serverConnection, userMailboxes,serverFDSets);
					if (userActionResult.errorOccurred) {
						deleteUserConnection(serverConnection);
						connectionStore.erase(i);
						numConnectedConnections--;
					} else {
						scheduleSocketForReading(serverConnection.workingSocket,serverFDSets);
						//If we need to send something we schedule it for sending
						if (serverConnection.clientInitiatedConversation.isInSendResponseState() ||
								serverConnection.serverInitiatedConversation.isInSendResponseState() ){
							scheduleSocketForWriting(serverConnection.workingSocket,serverFDSets);
						}
					}
				} catch (...) {
					//we don't send the user anything here since we explicitly don't care if a user sent in faulty input
					//their session is automatically ended and the server continues on with its business
					try {
						ConnectionStore::iterator iter = connectionStore.find(i) ;
						ServerConnection &serverConnection = iter->second;
						deleteUserConnection(serverConnection);
						connectionStore.erase(i);
						numConnectedConnections--;
					} catch (...){
						//If this fails then there's nothing to do
					}
				}
			} else {
				//If the socket was scheduled for either reading or writing but hasn't popped up yet we need to reschedule it
				if (socketWasPreviouselyScheduledForRead(i,serverFDSets)){
					scheduleSocketForReading(i,serverFDSets);
				}
				if (socketWasPreviouselyScheduledForWrite(i,serverFDSets)){
					scheduleSocketForWriting(i,serverFDSets);
				}
			}
		}
	}
	exitSection:
	TEMP_FAILURE_RETRY(close (listeningServerSocket));
	//We do this as to overcome temproray issues with closing the socket
	deleteConnectionStore(connectionStore);
	deleteMailBoxes(userMailboxes);
	return 0;
}

