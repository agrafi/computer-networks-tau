/*
 * ProtocolParserServerOut.cpp
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */
#include "ProtocolParserServerOut.h"
#include "StringUtils.h"
const char QUOTES = '"';
string parseOutputStringFromShowInboxMessage(const Body &body){
	string outputString;
		for (unsigned int i=0;i<body.messages.size();i++){
			//For each non deleted messages
			if (body.messages[i] != NULL){
				//add mail id
				outputString+=parseIntToString(body.messages[i]->mailId);
				//Add field separation characters
				outputString+=PIPE_FIELD_SEPARATOR;
				//add sender
				outputString+=body.messages[i]->sender;
				//Add field separation characters
				outputString+=PIPE_FIELD_SEPARATOR;
				//subject
				outputString+=QUOTES+body.messages[i]->subject+QUOTES;
				//Add field separation characters
				outputString+=PIPE_FIELD_SEPARATOR;
				//#attachments
				outputString+=parseIntToString(body.messages[i]->numberOfAttachments);
				//Add line separation characters
				outputString+=LINE_FEED_CHARACTER;
			}
		}
	return outputString;
}
string parseOutputStringFromGetMailMessage(const Body &body){
	string outputString;
	MailMessage *messageToOutput = body.messages[0];
	//add sender
	outputString+=messageToOutput->sender;
	//Add field separation characters
	outputString+=PIPE_FIELD_SEPARATOR;
	for (unsigned int i=0;i<messageToOutput->recipients.size();i++){
		//add recipient
		outputString+=messageToOutput->recipients[i];
		if (i!=messageToOutput->recipients.size()-1){//If it's not the last member
			//Add list separation characters
			outputString+=COMMA_LIST_SEPARATOR;
		}
	}
	//Add field separation characters
	outputString+=PIPE_FIELD_SEPARATOR;
	//add subject
	outputString+=messageToOutput->subject;
	//Add field separation characters
	outputString+=PIPE_FIELD_SEPARATOR;
	for (unsigned int i=0;i<messageToOutput->attachments.size();i++){
		//add attachment name
		outputString+=messageToOutput->attachments[i]->name;
		if (i!=messageToOutput->attachments.size()-1){//If it's not the last member
			//Add list separation characters
			outputString+=COMMA_LIST_SEPARATOR;
		}
	}
	//Add field separation characters
	outputString+=PIPE_FIELD_SEPARATOR;

	//add message text
	outputString+=messageToOutput->messageText;
	return outputString;
}
string parseOutputStringFromConnectToServerMessage(const Body &body){
	return body.infoMessage;
}
string parseOutputStringFromLoginMessage(const Body &body){
	return body.infoMessage;
}
string parseOutputStringFromDeleteMailMessage(const Body &body){
	string emptyString;
	return emptyString;
}
string parseOutputStringFromQuitMessage(const Body &body){
	string emptyString;
	return emptyString;
}
string parseOutputStringFromComposeMailMessage(const Body &body){
	return body.infoMessage;
}
