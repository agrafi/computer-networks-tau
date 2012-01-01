/*
 * WorkflowIdentifierEnum.h
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */

#ifndef WORKFLOWIDENTIFIERENUM_H_
#define WORKFLOWIDENTIFIERENUM_H_
#include <string>
using namespace std;
const int FIXED_WORKFLOW_IDENTIFIER_SIZE = 17;

enum WorkflowIdentifier {
	CONNECT_TO_SERVER,LOGIN,SHOW_INBOX, GET_MAIL,GET_ATTACHMENT,DELETE_MAIL,QUIT,COMPOSE_MAIL,UNKNOWN
};
WorkflowIdentifier getWorkflowIdentifierEnum(const string workflowIdentifierStr);
string getWorkflowIdentifierString(const WorkflowIdentifier workflowIdentifier);



#endif /* WORKFLOWIDENTIFIERENUM_H_ */
