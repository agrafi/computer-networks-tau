/*
 * MessageParser.h
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */

#ifndef MESSAGEPARSER_H_
#define MESSAGEPARSER_H_

#include "ProtocolParserCommon.h"
#include <vector>
using namespace std;

string parseOutputStringFromConnectToServerMessage(const Body &body);
string parseOutputStringFromLoginMessage(const Body &body);
string parseOutputStringFromShowInboxMessage(const Body &body);
string parseOutputStringFromGetMailMessage(const Body &body);
string parseOutputStringFromDeleteMailMessage(const Body &body);
string parseOutputStringFromQuitMessage(const Body &body);
string parseOutputStringFromComposeMailMessage(const Body &body);
string parseOutputStringFromSendChatMsgMessage(const Body &body);
string parseOutputStringFromReceiveChatMsgMessage(const Body &body);
string parseOutputStringFromShowOnlineUsersMessage(const Body &body);

#endif /* MESSAGEPARSER_H_ */
