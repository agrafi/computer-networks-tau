/*
 * ProtocolParserIn.h
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */

#ifndef PROTOCOLPARSERIN_H_
#define PROTOCOLPARSERIN_H_
#include "ProtocolParserCommon.h"
#include <vector>
using namespace std;

void parseLoginMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled);
void parseGetMailMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled);
void parseDeleteMailMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled);
void parseComposeMailMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled);
void parseGetAttachmentTextualMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled);
void parseSendChatMsgTextualMessageFromBuffer(const char *inBuffer, int expectedBodyLength,Body& bodyToBeFilled);


#endif /* PROTOCOLPARSERIN_H_ */
