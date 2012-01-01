/*
 * ProtocolParserServer.h
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */
#ifndef PROTOCOLPARSERSERVER_H_
#define PROTOCOLPARSERSERVER_H_
#include <string>
#include <vector>
#include "ProtocolParserCommon.h"
#include "WorkflowIdentifierEnum.h"

using namespace std;
void parseTextualBasedBodyFromBuffer(char *inBuffer,const Header &header,Body& bodyToBeFilled);
string parseBufferFromTextualMessage(const TextualProtocolMessage& textualProtocolMessage);
#endif /* PROTOCOLPARSERSERVER_H_ */
