/*
 * ProtocolParserCommon.h
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */
#ifndef PROTOCOLPARSERCOMMON_H_
#define PROTOCOLPARSERCOMMON_H_
#include <string>
#include <vector>
#include "WorkflowIdentifierEnum.h"

using namespace std;
const char PIPE_FIELD_SEPARATOR = '|';
const char LINE_FEED_CHARACTER = '\n';
const char COMMA_LIST_SEPARATOR = ',';
const char END_OF_HEADER_START_MARKER = LINE_FEED_CHARACTER;
const string EOF = "\n";
const string END_OF_HEADER_MARKER=EOF+EOF;
const int MAX_USERNAME_LENGTH = 30;
const int MAX_PASSWORD_LENGTH = 30;
const int BODY_LENGTH_FIELD_SIZE = 10;
const int HEADER_END_SIZE = 2;
const int FIXED_HEADER_SIZE = (FIXED_WORKFLOW_IDENTIFIER_SIZE + sizeof(PIPE_FIELD_SEPARATOR) + BODY_LENGTH_FIELD_SIZE + HEADER_END_SIZE);
const int BINARY_SIZE_FIELD_LENGTH = 8;
typedef struct UserLoginStruct {
	string username;
	string password;
} UserLogin;
typedef struct FileStruct {
	char *data;
	unsigned int size;
} File;
typedef struct AttachmentStruct {
	File file;
	string name;
	unsigned int serialNumber;
} Attachment;
typedef struct MailMessageStruct {
	unsigned int mailId;
	string sender;
	string subject;
	vector<string> recipients;
	vector<Attachment*> attachments;
	unsigned int numberOfAttachments;
	string messageText;
} MailMessage;
typedef struct HeaderStruct {
	WorkflowIdentifier workflowIdentifier;
	unsigned int bodyLength;
} Header;
typedef struct BodyStruct {
	UserLogin userLogin;
	string infoMessage;
	vector<MailMessage*> messages;
} Body;
typedef struct TextualProtocolMessageStruct {
	Header header;
	Body body;
} TextualProtocolMessage;
typedef struct BinaryProtocolMessageStruct {
	Header header;
	vector<File*> files;
} BinaryProtocolMessage;

int parseHeaderFromBuffer(char inBuffer[FIXED_HEADER_SIZE],Header &headerToBeFilled);
string parseOutputStringFromHeader(WorkflowIdentifier workflowIdentifier,int bodyLength);
#endif /* PROTOCOLPARSERCOMMON_H_ */
