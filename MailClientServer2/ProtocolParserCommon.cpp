/*
 * ProtocolParserImplementation.cpp
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */
#include "ProtocolParserCommon.h"
#include "WorkflowIdentifierEnum.h"
#include "StringUtils.h"
#include <string>
#include <vector>
using namespace std;
string parseWorkflowIdentifierStr(char *inBuffer){
	string workflowIdentifierStr = getTextualField(PIPE_FIELD_SEPARATOR,inBuffer);
	return workflowIdentifierStr;
}
string parseBodyLength(const char *inBuffer){
	string bodyLengthStr = getTextualField(END_OF_HEADER_START_MARKER,inBuffer);
	return bodyLengthStr;
}
string parseOutputStringFromHeader(WorkflowIdentifier workflowIdentifier,int bodyLength){
	string outputStringHeader = getWorkflowIdentifierString(workflowIdentifier);
	outputStringHeader += PIPE_FIELD_SEPARATOR;
	outputStringHeader += parseIntToStringWithMinimalSize(bodyLength,BODY_LENGTH_FIELD_SIZE);
	outputStringHeader += END_OF_HEADER_MARKER;
	return outputStringHeader;
}

int parseHeaderFromBuffer(char *inBuffer,Header &headerToBeFilled){
	int numCharsProcessedFromBuffer;
	string workflowIdentifierStr = parseWorkflowIdentifierStr(inBuffer);
	headerToBeFilled.workflowIdentifier = getWorkflowIdentifierEnum(workflowIdentifierStr);
	numCharsProcessedFromBuffer = (workflowIdentifierStr.size()+sizeof(PIPE_FIELD_SEPARATOR));
	//This is ok since we know we are in the textual part of the buffer
	inBuffer = inBuffer + numCharsProcessedFromBuffer;

	string bodyLengthStr = parseBodyLength(inBuffer);
	numCharsProcessedFromBuffer += (bodyLengthStr.size()+END_OF_HEADER_MARKER.size());
	headerToBeFilled.bodyLength =parseStringToInt(bodyLengthStr);
	return numCharsProcessedFromBuffer;
}
