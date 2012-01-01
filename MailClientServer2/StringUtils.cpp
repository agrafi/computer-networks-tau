/*
 * StringUtils.cpp
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */
#include "StringUtils.h"
#include <sstream>
#include <string>
#include <istream>
#include <streambuf>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
using namespace std;

int parseStringToInt(const string inputString){
	istringstream inputStreamStringStream(inputString);
	int returnIntValue;
	inputStreamStringStream >> returnIntValue;
	return returnIntValue;
}
string parseIntToStringWithMinimalSize(const int inputInt,const unsigned int minimalReturnSize){
    ostringstream ss;
    ss << setw(minimalReturnSize) << setfill('0') << inputInt;
    string result = ss.str();
    if (result.length() > minimalReturnSize){
        result.erase(0, result.length() - minimalReturnSize);
    }
    return result;
}
string parseIntToString(const int inputInt){
	stringstream out;
	out << inputInt;
	return out.str();
}
vector<string> splitString(const string inputString,const char delimiter){
	vector<string> outputVector;
	stringstream ss(inputString);
	string s;

	while (getline(ss, s, delimiter)) {
		outputVector.push_back(s);
	}
	return outputVector;
}
int parseHexaStringToInt(const string inputString){
	unsigned int x;
	stringstream ss;
	ss << hex << inputString;
	ss >> x;
	return x;
}
string parseIntToHexaString(const int inputInt,const unsigned int minimalReturnSize){
	stringstream ss;
	ss << setw(minimalReturnSize) << setfill('0') << hex << inputInt;
    string result = ss.str();
    if (result.length() > minimalReturnSize){
        result.erase(0, result.length() - minimalReturnSize);
    }
	return result;
}
void trimTrailingSpaces(string &input){
	// trim trailing spaces
	size_t endpos = input.find_last_not_of(" \t");
	if( string::npos != endpos )
	{
		input = input.substr( 0, endpos+1 );
	}
}
void trimLeadingspaces(string &input){
	// trim leading spaces
	size_t startpos = input.find_first_not_of(" \t");
	if( string::npos != startpos )
	{
		input = input.substr( startpos );
	}
}
void trimString( string &input){
	trimTrailingSpaces(input);
	trimLeadingspaces(input);
}
void getTrimmedRestOfStringAfterPosition(string &input,const int position){
	input = input.substr(position);
	trimString(input);
}


string getTextualField(const char separator,const char *inBuffer){
	int fieldSize=0;
	string strField;
	while (inBuffer[fieldSize] != separator){
		strField+=inBuffer[fieldSize];
		fieldSize++;
	}
	return strField;
}
vector<string> splitFieldsBySpaceFromString(const string &inString){
	vector<string> tokens;
	istringstream iss(inString);
	copy(istream_iterator<string>(iss),
	         istream_iterator<string>(),
	         back_inserter<vector<string> >(tokens));
	return tokens;
}

int outputStringToBuffer(const string stringToOutput,char *outBuffer){
	for (unsigned int i=0;i<stringToOutput.length();i++){
		*outBuffer++ = stringToOutput.at(i);
	}
	return stringToOutput.length();
}
int outputCharacterToBuffer(const char charToOutput,char *outBuffer){
	*outBuffer = charToOutput;
	return sizeof(char);
}
void splitFieldsFromBuffer(char *inBuffer,const int headerSize, const char delimeter, vector<string> &responseTokens){
	if (inBuffer == NULL){
		return;
	}
	int i = 0;
	int totalRead = 0;
	while(totalRead<headerSize){
		if (inBuffer[i] == delimeter || (totalRead+1==headerSize)){
			string curString (inBuffer,i);
			inBuffer +=(i+1);//Need to advance him past the delimiter
			i=0;
			responseTokens.push_back(curString);
		} else {
			i++;
		}
		totalRead++;
	}
}
