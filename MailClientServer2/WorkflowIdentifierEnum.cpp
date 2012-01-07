/*
 * WorkflowIdentifierEnum.cpp
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */

#include "WorkflowIdentifierEnum.h"
const string CONNECT_TO_SERVER_STR 	= "CONNECT_TO_SERVER";
const string LOGIN_STR 				= "LOGIN            ";
const string SHOW_INBOX_STR 		= "SHOW_INBOX       ";
const string GET_MAIL_STR 			= "GET_MAIL         ";
const string GET_ATTACHMENT_STR 	= "GET_ATTACHMENT   ";
const string DELETE_MAIL_STR 		= "DELETE_MAIL      ";
const string QUIT_STR		 		= "QUIT             ";
const string COMPOSE_MAIL_STR 		= "COMPOSE_MAIL     ";
const string SHOW_ONLINE_USERS_SRT  = "SHOW_ONLINE_USERS";
const string SEND_CHAT_MSG_SRT		= "MSG              ";
const string RECEIVE_CHAT_MSG_SRT	= "RECEIVE_CHAT_MSG ";
bool workflowStrBeginsWithInput(const string &workflowStr,const string&input){
	return !(workflowStr.compare(0, input.length(), input));
}
WorkflowIdentifier getWorkflowIdentifierEnum(const string workflowIdentifierStr){
	if (workflowStrBeginsWithInput(CONNECT_TO_SERVER_STR,workflowIdentifierStr)){
		return CONNECT_TO_SERVER;
	}
	if (workflowStrBeginsWithInput(LOGIN_STR,workflowIdentifierStr)){
		return LOGIN;
	}
	if (workflowStrBeginsWithInput(SHOW_INBOX_STR,workflowIdentifierStr)){
		return SHOW_INBOX;
	}
	if (workflowStrBeginsWithInput(GET_MAIL_STR,workflowIdentifierStr)){
		return GET_MAIL;
	}
	if (workflowStrBeginsWithInput(GET_ATTACHMENT_STR,workflowIdentifierStr)){
		return GET_ATTACHMENT;
	}
	if (workflowStrBeginsWithInput(DELETE_MAIL_STR,workflowIdentifierStr)){
		return DELETE_MAIL;
	}
	if (workflowStrBeginsWithInput(QUIT_STR,workflowIdentifierStr)){
		return QUIT;
	}
	if (workflowStrBeginsWithInput(COMPOSE_MAIL_STR,workflowIdentifierStr)){
		return COMPOSE_MAIL;
	}
	if (workflowStrBeginsWithInput(QUIT_STR,workflowIdentifierStr)){
		return QUIT;
	}
	if (workflowStrBeginsWithInput(SHOW_ONLINE_USERS_SRT,workflowIdentifierStr)){
		return SHOW_ONLINE_USERS;
	}
	if (workflowStrBeginsWithInput(SEND_CHAT_MSG_SRT,workflowIdentifierStr)){
		return SEND_CHAT_MSG;
	}
	if (workflowStrBeginsWithInput(RECEIVE_CHAT_MSG_SRT,workflowIdentifierStr)){
		return RECEIVE_CHAT_MSG;
	}

	return UNKNOWN;
}


string getWorkflowIdentifierString(const WorkflowIdentifier workflowIdentifier)
{

	switch (workflowIdentifier)
	{

	case CONNECT_TO_SERVER:
		return CONNECT_TO_SERVER_STR;

	case LOGIN:
		return LOGIN_STR;

	case SHOW_INBOX:
		return SHOW_INBOX_STR;

	case GET_MAIL:
		return GET_MAIL_STR;

	case GET_ATTACHMENT:
		return GET_ATTACHMENT_STR;

	case DELETE_MAIL:
		return DELETE_MAIL_STR;

	case QUIT:
		return QUIT_STR;

	case COMPOSE_MAIL:
		return COMPOSE_MAIL_STR;

	case SHOW_ONLINE_USERS:
		return SHOW_ONLINE_USERS_SRT;

	case SEND_CHAT_MSG:
		return SEND_CHAT_MSG_SRT;

	case RECEIVE_CHAT_MSG:
		return RECEIVE_CHAT_MSG_SRT;

	default:
		return NULL;
	}
}
