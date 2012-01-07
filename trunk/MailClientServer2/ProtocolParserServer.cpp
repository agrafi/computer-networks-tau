/*
 * ProtocolParserImplementation.cpp
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */
#include "ProtocolParserServer.h"
#include "ProtocolParserServerIn.h"
#include "ProtocolParserServerOut.h"
#include "WorkflowIdentifierEnum.h"
#include "StringUtils.h"
#include <string>
#include <vector>
using namespace std;

void parseTextualBasedBodyFromBuffer(char *inBuffer,const Header &header,Body& bodyToBeFilled){
	switch (header.workflowIdentifier){
	case LOGIN: parseLoginMessageFromBuffer(inBuffer,header.bodyLength, bodyToBeFilled);break;
	case GET_MAIL: parseGetMailMessageFromBuffer(inBuffer,header.bodyLength, bodyToBeFilled); break;
	case DELETE_MAIL: parseDeleteMailMessageFromBuffer(inBuffer,header.bodyLength, bodyToBeFilled);break;
	case COMPOSE_MAIL: parseComposeMailMessageFromBuffer(inBuffer,header.bodyLength, bodyToBeFilled);break;
	case GET_ATTACHMENT: parseGetAttachmentTextualMessageFromBuffer(inBuffer,header.bodyLength, bodyToBeFilled);break;
	case SEND_CHAT_MSG: parseSendChatMsgTextualMessageFromBuffer(inBuffer,header.bodyLength, bodyToBeFilled);break;
	default: break;//We ignore since other types are not supported or have no body when incomming to server
	}
}
string parseBufferFromTextualMessage(const TextualProtocolMessage& textualProtocolMessage){
	string outputBodyString;
	const Header &header = textualProtocolMessage.header;
	const Body &body = textualProtocolMessage.body;
	switch(header.workflowIdentifier){
	case CONNECT_TO_SERVER: outputBodyString = parseOutputStringFromConnectToServerMessage(body);break;
	case LOGIN: outputBodyString = parseOutputStringFromLoginMessage(body);break;
	case SHOW_INBOX: outputBodyString = parseOutputStringFromShowInboxMessage(body); break;
	case GET_MAIL: outputBodyString = parseOutputStringFromGetMailMessage(body); break;
	case DELETE_MAIL: outputBodyString = parseOutputStringFromDeleteMailMessage(body);break;
	case QUIT: outputBodyString = parseOutputStringFromQuitMessage(body);break;
	case COMPOSE_MAIL: outputBodyString = parseOutputStringFromComposeMailMessage(body);break;
	case SEND_CHAT_MSG: outputBodyString = parseOutputStringFromSendChatMsgMessage(body);break;
	case RECEIVE_CHAT_MSG: outputBodyString = parseOutputStringFromReceiveChatMsgMessage(body);break;
	case SHOW_ONLINE_USERS: outputBodyString = parseOutputStringFromShowOnlineUsersMessage(body);break;
	default: break;//We ignore since other types are not supported
	}
	string outputHeaderString = parseOutputStringFromHeader(header.workflowIdentifier,outputBodyString.length());
	return outputHeaderString+outputBodyString;
}
