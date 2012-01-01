/*
 * ProtocolParserServerIn.cpp
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */
#include "ProtocolParserServerIn.h"
#include "StringUtils.h"
#include <cstring>
void parseLoginMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled){
	string username = getTextualField(PIPE_FIELD_SEPARATOR,inBuffer);
	bodyToBeFilled.userLogin.username = username;
	inBuffer += username.length() + 1;
	int expectedPasswordLength = expectedBodyLength- (username.length() + 1);
	string password (inBuffer,expectedPasswordLength);
	bodyToBeFilled.userLogin.password = password;
}
void parseGetMailMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled){
	string mailIdStr (inBuffer,expectedBodyLength);//this creates a string from the first k bytes of the buffer (k="expectedBodyLength")
	MailMessage *mailMessage = new MailMessage;
	mailMessage->mailId = parseStringToInt(mailIdStr);
	bodyToBeFilled.messages.push_back(mailMessage);
}
void parseDeleteMailMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled){
	string mailIdStr (inBuffer,expectedBodyLength);//this creates a string from the first k bytes of the buffer (k="expectedBodyLength")
	MailMessage *mailMessage = new MailMessage;
	mailMessage->mailId = parseStringToInt(mailIdStr);
	bodyToBeFilled.messages.push_back(mailMessage);
}
void parseComposeMailMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled){
	MailMessage *mailMessage = new MailMessage;
	int bodyRead = 0;
	//parse recipients
	string recipients = getTextualField(PIPE_FIELD_SEPARATOR,inBuffer);
	bodyRead += recipients.length() +1;
	inBuffer += recipients.length() + 1;
	mailMessage->recipients = splitString(recipients,COMMA_LIST_SEPARATOR);
	//parse subject
	mailMessage->subject = getTextualField(PIPE_FIELD_SEPARATOR,inBuffer);
	bodyRead += mailMessage->subject.length() +1;
	inBuffer += mailMessage->subject.length() + 1;
	//parse messageText
	mailMessage->messageText = getTextualField(PIPE_FIELD_SEPARATOR,inBuffer);
	bodyRead += mailMessage->messageText.length() +1;
	inBuffer += mailMessage->messageText.length() + 1;
	//parse attachments names
	string attachments = getTextualField(PIPE_FIELD_SEPARATOR,inBuffer);
	bodyRead += attachments.length() +1;
	inBuffer += attachments.length() + 1;
	if (!attachments.empty()){
		vector<string> attachmentNames = splitString(attachments,COMMA_LIST_SEPARATOR);
		for (unsigned int i=0;i<attachmentNames.size();i++){
			Attachment *attachment = new Attachment;
			attachment->name = attachmentNames[i];
			attachment->serialNumber = i;
			mailMessage->attachments.push_back(attachment);
		}
		//parse attachments data
		unsigned int attachmentIndex = 0;
		while (bodyRead<expectedBodyLength && attachmentIndex<mailMessage->attachments.size()){
			string attachmentSizeHexa (inBuffer,BINARY_SIZE_FIELD_LENGTH);
			bodyRead += BINARY_SIZE_FIELD_LENGTH;
			inBuffer +=BINARY_SIZE_FIELD_LENGTH;
			int attachmentSize = parseHexaStringToInt(attachmentSizeHexa);
			Attachment *&attachment = mailMessage->attachments[attachmentIndex];
			attachment->file.data = new char[attachmentSize];
			attachment->file.size = attachmentSize;
			memcpy(attachment->file.data,inBuffer,attachment->file.size);
			bodyRead += attachmentSize;
			inBuffer += attachmentSize;
			attachmentIndex++;
		}
	}
	mailMessage->numberOfAttachments = mailMessage->attachments.size();
	bodyToBeFilled.messages.push_back(mailMessage);
}
void parseGetAttachmentTextualMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled){
	MailMessage *mailMessage = new MailMessage;
	string mailIdStr = getTextualField(PIPE_FIELD_SEPARATOR,inBuffer);
	mailMessage->mailId = parseStringToInt(mailIdStr);

	inBuffer += mailIdStr.length() + 1;
	int expectedAttachmentNumLength = expectedBodyLength - (mailIdStr.length() + 1);
	string attachmentNumStr (inBuffer,expectedAttachmentNumLength);
	mailMessage->numberOfAttachments = parseStringToInt(attachmentNumStr);

	bodyToBeFilled.messages.push_back(mailMessage);
}
