/*
 *  ClientApp.cpp
 *
 *  Created on: Nov 22, 2011
 *      Author: Razi Mukatren
 */

#include "ProtocolParserCommon.h"
#include "StringUtils.h"
#include "SocketHandler.h"
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <iterator>
#include <istream>
#include <sstream>
#include <stdio.h>
#include <cstring>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <wordexp.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>
#include <sys/stat.h>

#define MAX_BUFFER (1024*1024)
using namespace std;

void disconnect_check(int sock, struct addrinfo *servinfo) {
	close(sock);
	if (servinfo != NULL)
		freeaddrinfo(servinfo);
}

typedef struct ArgumentsClientStruct {
	string hostname;
	string parameterPort;
} ArgumentsClient;
void parseArgumentsClient(int argc, char* argv[],ArgumentsClient &arguments){
	if (argc == 1){
		arguments.hostname = "localhost";
		arguments.parameterPort = defaultPortStr;
		return;
	}
	string hostname(argv[1]);
	arguments.hostname = hostname;
	if (argc==2){
		//default port
		arguments.parameterPort = defaultPortStr;
	} else {
		string parameterPortArgument(argv[2]);
		arguments.parameterPort = parameterPortArgument;
	}
	return;
}
bool getHeader(int workingServerSocket, Header &header) {
	char *inBuffer = new char[FIXED_HEADER_SIZE];
	bool receivedSuccess = receiveAllData(workingServerSocket, inBuffer,
			FIXED_HEADER_SIZE);
	if (receivedSuccess){
		parseHeaderFromBuffer(inBuffer, header);
	}
	delete[] inBuffer;
	return receivedSuccess;
}

void getTrimmedInputLine(string &inputContainer,const int positionToRemove){
	getline(cin, inputContainer); // Reads line into recipients
	getTrimmedRestOfStringAfterPosition(inputContainer,positionToRemove);//remove the textual header;
}

bool sendMessageToServer(int c_sock, const string &message) {
	unsigned int messageSize = message.length();
	char *outBuffer = new char[messageSize];
	outputStringToBuffer(message, outBuffer);
	bool shouldContinue = sendAllData(c_sock, outBuffer, &messageSize);
	delete[] outBuffer;
	return shouldContinue;
}
bool receieveChatMessage(int c_sock,Header &header){
	char *bodyInBuffer = new char[header.bodyLength+1];
	bool shouldContinue = receiveAllData(c_sock, bodyInBuffer,
			header.bodyLength);
	bodyInBuffer[header.bodyLength] = '\0';
	if (shouldContinue) {
		cout << bodyInBuffer << endl;
	}
	delete[] bodyInBuffer;
	return shouldContinue;
}
bool infoMessageResponse(int c_sock) {
	Header header;
	bool shouldContinue = getHeader(c_sock, header);
	while (shouldContinue && header.workflowIdentifier == RECEIVE_CHAT_MSG){
		receieveChatMessage(c_sock,header);
		shouldContinue = getHeader(c_sock, header);
	}
	if (!shouldContinue){
		return shouldContinue;
	}
	if (header.bodyLength>0){
		char *bodyInBuffer = new char[header.bodyLength+1];
		shouldContinue = receiveAllData(c_sock, bodyInBuffer,
				header.bodyLength);
		bodyInBuffer[header.bodyLength] = '\0';
		if (shouldContinue) {
			cout << bodyInBuffer << endl;
		}
		delete[] bodyInBuffer;
	}
	return shouldContinue;
}
bool delimitedTextMessageResponse(int c_sock,vector<string> &responseTokens) {
	Header header;
	bool shouldContinue = getHeader(c_sock, header);
	while (shouldContinue && header.workflowIdentifier == RECEIVE_CHAT_MSG){
		receieveChatMessage(c_sock,header);
		shouldContinue = getHeader(c_sock, header);
	}
	if (shouldContinue && header.bodyLength > 0) {
		char *bodyInBuffer = new char[header.bodyLength+1];
		bool shouldContinue = receiveAllData(c_sock, bodyInBuffer,
				header.bodyLength);
		if (!shouldContinue) {
			return shouldContinue;
		}
		bodyInBuffer[header.bodyLength] = '\0';
		splitFieldsFromBuffer(bodyInBuffer,header.bodyLength+1,PIPE_FIELD_SEPARATOR,responseTokens);
		delete[] bodyInBuffer;
	}
	return shouldContinue;
}
bool sendRequestToServer(int c_sock, vector<string> &inputTokens,
		unsigned int numParams, WorkflowIdentifier workflowIdentifier) {
	numParams++; //The +1 is for the workflowIdentifier which we already parsed
	if (inputTokens.size() < numParams) {
		return false;
	}
	int bodyLength = 0;
	string body = "";
	for (unsigned int i = 1; i < numParams; i++) {
		trimString(inputTokens[i]);
		bodyLength += inputTokens[i].length();
		body += inputTokens[i];
		if (i + 1 != numParams) { //The last iteration
			body += PIPE_FIELD_SEPARATOR;
			bodyLength++;
		}
	}
	string outputHeaderString = parseOutputStringFromHeader(workflowIdentifier,
			bodyLength);
	bool shouldContinue = sendMessageToServer(c_sock,
			outputHeaderString + body);
	return shouldContinue;
}
bool tryLoginRequest(int c_sock){
	string username;
	string password;
	getTrimmedInputLine(username,5);// Reads line into username
	getTrimmedInputLine(password,9);// Reads line into username
	string body = username + PIPE_FIELD_SEPARATOR + password;
	//GET HEADER
	unsigned int bodyLength = body.length();
	string outputHeaderString = parseOutputStringFromHeader(LOGIN,bodyLength);
	unsigned int messageLength = body.length()+outputHeaderString.length();
	char *outBuffer = new char[messageLength];
	outputStringToBuffer(outputHeaderString+body,outBuffer);
	bool shouldContinue = sendAllData(c_sock,outBuffer,&messageLength);
	delete[] outBuffer;
	return shouldContinue;
}
bool tryLoginResponse(int c_sock,bool &loginSucceeded) {
	Header header;
	bool shouldContinue = getHeader(c_sock, header);
	if (!shouldContinue){
		return shouldContinue;
	}
	char *bodyInBuffer = new char[header.bodyLength+1];
	shouldContinue = receiveAllData(c_sock, bodyInBuffer,
			header.bodyLength);
	bodyInBuffer[header.bodyLength] = '\0';
	loginSucceeded = (bodyInBuffer[0] == 'C');//We know that if and only if the logic succeeded the message will start with C (Connected...)
	if (shouldContinue) {
		cout << bodyInBuffer << endl;
	}
	delete[] bodyInBuffer;
	return shouldContinue;
}
bool tryLogin(int c_sock,bool &loginSucceeded){
	bool shouldContinue = tryLoginRequest(c_sock);
	if (shouldContinue){
		shouldContinue = tryLoginResponse(c_sock,loginSucceeded);
	}
	return shouldContinue;
}
bool showInboxRequest(int c_sock, vector<string> &inputTokens) {
	bool shouldContinue = sendRequestToServer(c_sock, inputTokens, 0,
			SHOW_INBOX);
	return shouldContinue;
}
bool showInboxResponse(int c_sock) {
	vector<string> responseTokens;
	bool shouldContinue = delimitedTextMessageResponse(c_sock,responseTokens);
	if (shouldContinue){
		for (unsigned int i=0;i<responseTokens.size();i++){
			cout << responseTokens[i];
			if (i+1 != responseTokens.size()){
				cout << ' ';
			}
		}
	}
	return shouldContinue;
}
bool showInbox(int c_sock, vector<string> &inputTokens) {
	bool shouldContinue = showInboxRequest(c_sock, inputTokens);
	if (shouldContinue){
		shouldContinue = showInboxResponse(c_sock);
	}
	return shouldContinue;
}
bool getMailRequest(int c_sock, vector<string> &inputTokens) {
	bool shouldContinue = sendRequestToServer(c_sock, inputTokens, 1, GET_MAIL);
	return shouldContinue;
}
bool getMailResponse(int c_sock) {
	vector<string> responseTokens;
	bool shouldContinue = delimitedTextMessageResponse(c_sock,responseTokens);
	if (!shouldContinue || responseTokens.size() < 5){
		return false;
	}
	cout << "From: " << responseTokens[0] << endl;
	cout << "To: " << responseTokens[1] << endl;
	cout << "Subject: " << responseTokens[2] << endl;
	cout << "Attachments: " << responseTokens[3] << endl;
	cout << "Text: " << responseTokens[4] << endl;
	return true;
}
bool getMail(int c_sock, vector<string> &inputTokens) {
	bool shouldContinue = getMailRequest(c_sock, inputTokens);
	if (!shouldContinue) {
		return shouldContinue;
	}
	shouldContinue = getMailResponse(c_sock);
	return shouldContinue;
}
string getChatMessageText(string &inputString){
	int indexOfColon = inputString.find(':');
	return inputString.substr(indexOfColon+1);
}
bool sendChatMessageRequest(int c_sock, string &inputString, vector<string> &inputTokens){
	if (inputTokens.size() < 3) {
		return false;
	}

	string recipients = inputTokens[1];
	//this is to replace the ":" symbol which is inputed from the user
	recipients.replace(recipients.size()-1,1,PIPE_FIELD_SEPARATOR_STRING);
	string chatMessageText = getChatMessageText(inputString);
	string body = recipients + chatMessageText;

	string outputHeaderString = parseOutputStringFromHeader(SEND_CHAT_MSG,
			body.size());
	bool shouldContinue = sendMessageToServer(c_sock,
			outputHeaderString + body);
	return shouldContinue;

}
bool sendChatMessageResponse(int c_sock){
	bool shouldContinue = infoMessageResponse(c_sock);
	return shouldContinue;
}
bool sendChatMessage(int c_sock, string inputString, vector<string> &inputTokens) {
	bool shouldContinue = sendChatMessageRequest(c_sock, inputString,inputTokens);
	if (!shouldContinue) {
		return shouldContinue;
	}
	shouldContinue = sendChatMessageResponse(c_sock);
	return shouldContinue;
}

bool getAttachmentRequest(int c_sock, vector<string> &inputTokens) {
	bool shouldContinue = sendRequestToServer(c_sock, inputTokens, 2,
			GET_ATTACHMENT);
	return shouldContinue;
}
void cleanInputPathForFullPathNoQuotes(string &attachmentPath){
	//only if the path begins with ~/ then we want to expand. if it's ~as then it's just a file name
	//we increment the indexes because of the opening quote
	if (attachmentPath.at(1) =='~' && attachmentPath.at(2) == '/'){
		wordexp_t exp_result;
		const string tilde = "~";
		wordexp(tilde.c_str(), &exp_result, 0);
		attachmentPath = exp_result.we_wordv[0] + attachmentPath.substr(2,attachmentPath.size()-3);//we drop the tilde and the quotes
		wordfree(&exp_result);
	} else {
		attachmentPath = attachmentPath.substr(1,attachmentPath.size()-2);//only remove the quotes
	}
}
bool saveAttachmentToDisk(string &attachmentPath, char * bodyInBuffer,
		int bodyLength) {
	cleanInputPathForFullPathNoQuotes(attachmentPath);
	ofstream outfile(attachmentPath.c_str(), ofstream::binary);
	outfile.write(bodyInBuffer, bodyLength);
	outfile.close();
	return true;
}
long getAttachmentSizeFromDisk(string &attachmentPath)
{
	struct stat filestatus;

	if (-1 == stat( attachmentPath.c_str(), &filestatus ))
	{
		return 0;
	}

	return filestatus.st_size;
}
string readAttachmentFromDisk(string &attachmentPath){
	cleanInputPathForFullPathNoQuotes(attachmentPath);

	ifstream in(attachmentPath.c_str(),ifstream::binary);

	string contents((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
	return contents;
}
bool getAttachmentResponse(int c_sock,string &attachmentPath) {
	Header header;
	bool shouldContinue = getHeader(c_sock, header);
	while (shouldContinue && header.workflowIdentifier == RECEIVE_CHAT_MSG){
		receieveChatMessage(c_sock,header);
		shouldContinue = getHeader(c_sock, header);
	}
	char *bodyInBuffer = new char[header.bodyLength];
	shouldContinue = receiveAllData(c_sock, bodyInBuffer,
			header.bodyLength);
	if (!shouldContinue) {
		return shouldContinue;
	}
	shouldContinue = saveAttachmentToDisk(attachmentPath, bodyInBuffer, header.bodyLength);
	cout << "Attachment saved" << endl;
	delete[] bodyInBuffer;
	return shouldContinue;
}
bool getAttachment(int c_sock, vector<string> &inputTokens) {
	if (inputTokens.size() < 4){
		return false;
	}
	bool shouldContinue = getAttachmentRequest(c_sock, inputTokens);
	if (!shouldContinue) {
		return false;
	}
	//We're amending the possibly "broken" path because of spaces
	string attachmentPath = inputTokens[3];
	for (unsigned int i=4;i<inputTokens.size();i++){
		attachmentPath+=' '+inputTokens[i];
	}
	shouldContinue = getAttachmentResponse(c_sock, attachmentPath);
	return shouldContinue;
}
bool deleteMailRequest(int c_sock, vector<string> &inputTokens) {
	bool shouldContinue = sendRequestToServer(c_sock, inputTokens, 1,
			DELETE_MAIL);
	return shouldContinue;
}
bool deleteMailResponse(int c_sock) {
	bool shouldContinue = infoMessageResponse(c_sock);
	return shouldContinue;
}
bool deleteMail(int c_sock, vector<string> &inputTokens) {
	bool shouldContinue = deleteMailRequest(c_sock, inputTokens);
	if (!shouldContinue) {
		return shouldContinue;
	}
	shouldContinue = deleteMailResponse(c_sock);
	return shouldContinue;
}
bool quit(int c_sock, vector<string> &inputTokens) {
	bool shouldContinue = sendRequestToServer(c_sock, inputTokens, 0, QUIT);
	return shouldContinue;
}
bool composeMailResponse(int c_sock) {
	bool shouldContinue = infoMessageResponse(c_sock);
	return shouldContinue;
}
bool recieveChatMessageFromServer(int c_sock){
	Header header;
	bool shouldContinue = getHeader(c_sock, header);
	if (!shouldContinue){
		return shouldContinue;
	}
	shouldContinue = receieveChatMessage(c_sock,header);
	return shouldContinue;
}
bool composeMailRequest(int c_sock) {
	string recipients,subject,attachments,messageText;
	getTrimmedInputLine(recipients,3);// Reads line into recipients
	getTrimmedInputLine(subject,8);// Reads line into subject
	getTrimmedInputLine(attachments,12);// Reads line into attachments
	getTrimmedInputLine(messageText,5);// Reads line into messageText
	string body = recipients + PIPE_FIELD_SEPARATOR +
							subject + PIPE_FIELD_SEPARATOR +
							messageText + PIPE_FIELD_SEPARATOR +
							attachments + PIPE_FIELD_SEPARATOR;

	//READ FILES
	//TODO change file reading so that it happens in chunks
	//We should iterate over the files and see how big they are
	//Since in addition to the file contents we also send the file size in hexa then the actual length of the body of the message is
	// for (file f in files)
	//		file.size + parseIntToHexaString(file.size,BINARY_SIZE_FIELD_LENGTH);
	//Then we should do the get header and only send it
	//lastly we should iterate over the files and read chuncks of the attachments and
	//perform select on c_sock (reading chat message) and c_sock (sending files)
	//if c_sock is ready for reading we should call recieveChatMessageFromServer [I prepared it for you]
	//if c_sock is ready for writing we should send the relevant chunk of the attachment we're currently uploading
	vector<string> attachmentsVec = splitString(attachments,COMMA_LIST_SEPARATOR);
	unsigned int bodyLength = 0;
	for (unsigned int i=0;i<attachmentsVec.size();i++){
		string hexaCurFileSize = parseIntToHexaString(getAttachmentSizeFromDisk(attachmentsVec[i]),BINARY_SIZE_FIELD_LENGTH);
		bodyLength += hexaCurFileSize.size() + hexaCurFileSize.size();

	}
	//GET HEADER
	//bodyLength = body.length();
	string outputHeaderString = parseOutputStringFromHeader(COMPOSE_MAIL,bodyLength);
	unsigned int messageLength = outputHeaderString.length();
	char *outBuffer = new char[messageLength];
	outputStringToBuffer(outputHeaderString,outBuffer);
	bool shouldContinue = sendAllData(c_sock,outBuffer,&messageLength);
	delete[] outBuffer;

	// Return on error
	if (!shouldContinue)
		return shouldContinue;

	body.clear();
	for (unsigned int i=0;i<attachmentsVec.size();i++){
		char buffer[MAX_BUFFER] = {0};
		unsigned int nleft = getAttachmentSizeFromDisk(attachmentsVec[i]);
		unsigned int nread = 0;

		// string curFile = readAttachmentFromDisk(attachmentsVec[i]);
		// Read File from disk in chunk by chunk and send it over socket
		cleanInputPathForFullPathNoQuotes(attachmentsVec[i]);

		// Open file for binary read
		ifstream myfile(attachmentsVec[i].c_str(),ifstream::binary);

		// Send file size
		string hexaCurFileSize = parseIntToHexaString(getAttachmentSizeFromDisk(attachmentsVec[i]),BINARY_SIZE_FIELD_LENGTH);
		unsigned int hexaCurFileSizeLength = hexaCurFileSize.size();
		shouldContinue = sendAllData(c_sock,(char*)hexaCurFileSize.c_str(),&hexaCurFileSizeLength);

		// Iterate over file while it's not EOF
		while(myfile.good())
		{
			myfile.read(buffer, MAX_BUFFER);
			nread = myfile.gcount();
			shouldContinue = sendAllDataFullDuplex(c_sock,buffer,&nread, recieveChatMessageFromServer);

			nleft -= nread;
		}
	}

	return shouldContinue;
}
bool composeMail(int c_sock){
	bool shouldContinue = composeMailRequest(c_sock);
	if (shouldContinue){
		shouldContinue = composeMailResponse(c_sock);
	}
	return shouldContinue;
}
bool showOnlineUsersRequest(int c_sock, vector<string> &inputTokens){
	bool shouldContinue = sendRequestToServer(c_sock, inputTokens, 0,
			SHOW_ONLINE_USERS);
	return shouldContinue;
}
bool showOnlineUsersResponse(int c_sock){
	bool shouldContinue = infoMessageResponse(c_sock);
	return shouldContinue;
}

bool showOnlineUsers(int c_sock, vector<string> &inputTokens){
	bool shouldContinue = showOnlineUsersRequest(c_sock,inputTokens);
	if (shouldContinue){
		shouldContinue = showOnlineUsersResponse(c_sock);
	}
	return shouldContinue;
}
bool handleUserInput(int c_sock){
	string inputString; // Where to store each line.
	getline(cin, inputString); // Reads line into inputString
	vector<string> inputTokens = splitFieldsBySpaceFromString(inputString);
	if (inputTokens.empty()) {
		return true;
	}
	trimString(inputTokens[0]);
	WorkflowIdentifier workflowIdentifier = getWorkflowIdentifierEnum(
			inputTokens[0]);
	switch (workflowIdentifier) {
	case SHOW_INBOX:
		if (!showInbox(c_sock, inputTokens) != 0) {
			cerr << "Error: show_inbox failed" << endl;
			return false;
		}
		break;
	case GET_MAIL:
		if (!getMail(c_sock, inputTokens) != 0) {
			cerr << "Error: get_mail failed" << endl;
			return false;
		}
		break;
	case GET_ATTACHMENT:
		if (!getAttachment(c_sock, inputTokens) != 0) {
			cerr << "Error: get_attachment failed" << endl;
			return false;
		}
		break;
	case DELETE_MAIL:
		if (!deleteMail(c_sock, inputTokens) != 0) {
			cerr << "Error: delete_mail failed" << endl;
			return false;
		}
		break;
	case COMPOSE_MAIL:
		if (!composeMail(c_sock)) {
			cerr << "Error: compose_mail failed" << endl;
			return false;
		}
		break;
	case SHOW_ONLINE_USERS:
		if (!showOnlineUsers(c_sock,inputTokens)){
			cerr << "Error: show_online_users failed" << endl;
			return false;
		}
		break;
	case SEND_CHAT_MSG:
		//The chat message needs the unformatted input string as it preserves the spaces between the tokens
		if (!sendChatMessage(c_sock, inputString, inputTokens) != 0) {
			cerr << "Error: msg failed" << endl;
			return false;
		}
		break;
	case QUIT:
		return quit(c_sock, inputTokens);
	default: cerr << "Error: Usage 'SHOW_INBOX' or " << endl
					<< "'GET_MAIL <mail_id>' or'" << endl
					<< "'GET_ATTACHMENT <mail_id> <attachment_num> \"path\"' or" << endl
					<< "'DELETE_MAIL <mail_id>' or" << endl
					<< "'SHOW_ONLINE_USERS' or" << endl
					<< "'MSG <username>: <message>' or" << endl
					<< "'QUIT' or" << endl
					<< "'COMPOSE\n'" << endl;break;
	}
	return true;
}
int mailLoop(int c_sock) {
	fd_set readFromServerAndStdIn;
	FD_ZERO(&readFromServerAndStdIn);
	FD_SET(0,&readFromServerAndStdIn);
	FD_SET(c_sock,&readFromServerAndStdIn);
	while (true) {
		if (select(c_sock + 1, &readFromServerAndStdIn, NULL, NULL, NULL)
				== -1) {
			perror("select");
			return -1;
		}
		if (FD_ISSET(c_sock,&readFromServerAndStdIn)){
			if (!recieveChatMessageFromServer(c_sock)) {
				cerr << "Error: receive chat message from server failed" << endl;
				return -1;
			}

		}
		if (FD_ISSET(0,&readFromServerAndStdIn)){
			handleUserInput(c_sock);
		}
		//TODO change from blocking on input from user to selecting on either stdin (read) or server (read)
		//If server is ready then
		/*
		 * 	if (!recieveChatMessageFromServer(c_sock)) {
				cerr << "Error: receive chat message from server failed" << endl;
				return -1;
			}
		 *
		 * If stdin is ready (both can be ready) then we need to buffer the input line into inputString until \n (should be replaced by \0)
		 */
	}
	return -1;
}
int main(int argc, char* argv[]) {
	ArgumentsClient argumentsClient;
	string user;
	string password;
	int c_sock = 0;

	struct addrinfo hints, *servinfo = NULL; //configuration
	struct sockaddr *serv_socket = NULL;

	parseArgumentsClient(argc, argv, argumentsClient);

	//create the server socket
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(argumentsClient.hostname.c_str(), argumentsClient.parameterPort.c_str(), &hints, &servinfo) == -1) {
		cerr << "getaddrinfo failed" << endl <<"Error: " << strerror(errno) << endl;
		freeaddrinfo(servinfo);
		return -1;
	}

	serv_socket = servinfo->ai_addr;

	//create the client socket
	c_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (c_sock == -1) {
		cerr << "failed to get socket" << endl <<"Error: " << strerror(errno) << endl;
		freeaddrinfo(servinfo);
		return -1;
	}

	//connect to the server
	while ((servinfo != NULL)
			&& (connect(c_sock, serv_socket, sizeof(struct sockaddr)) == -1)) {

		if (servinfo->ai_next == NULL)
			break;

		servinfo = servinfo->ai_next;
		serv_socket = servinfo->ai_addr;
	}
	//check reason for exit from loop
	if (servinfo == NULL) {
		cerr << "Connection to " << argumentsClient.hostname <<" on port " << argumentsClient.parameterPort << " failed" << endl;
		cerr << "Error: " << strerror(errno) << endl;
		disconnect_check(c_sock, servinfo);
		return -1;
	}
	//get welcome message from server
	vector<string> inputTokens;
	inputTokens.push_back(getWorkflowIdentifierString(CONNECT_TO_SERVER));
	bool shouldContinue = sendRequestToServer(c_sock, inputTokens, 0, CONNECT_TO_SERVER);
	if (!shouldContinue){
		cerr << "welcome failed, bye" << endl;
		disconnect_check(c_sock,servinfo);
		return -1;
	}

	Header header;
	shouldContinue = getHeader(c_sock, header);
	if (!shouldContinue){
		cerr << "problem in getting welcome header response, bye" << endl;
	}
	char *bodyInBuffer = new char[header.bodyLength+1];
	shouldContinue = receiveAllData(c_sock,bodyInBuffer,header.bodyLength);
	if (!shouldContinue){
		delete[] bodyInBuffer;
		disconnect_check(c_sock, servinfo);
		return -1;
	}
	bodyInBuffer[header.bodyLength] = '\0';

	cout << bodyInBuffer << endl;
	delete[] bodyInBuffer;
	bool loginSucceeded;
	shouldContinue = tryLogin(c_sock,loginSucceeded);
	if (!shouldContinue){
		cerr << "problem in login, bye" << endl;
	}
	if (!shouldContinue || !loginSucceeded){
		disconnect_check(c_sock,servinfo);
		return -1;
	}
	shouldContinue = mailLoop(c_sock);
	if (!shouldContinue){
		cerr << "Error in mail_loop" << endl;
		disconnect_check(c_sock, servinfo);
		return -1;
	}
	disconnect_check(c_sock, servinfo);
	return 0;
}

