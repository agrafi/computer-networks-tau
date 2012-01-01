/*
 * StringUtils.h
 *
 *  Created on: Nov 12, 2011
 *      Author: ittai zeidman
 */

#ifndef STRINGUTILS_H_
#define STRINGUTILS_H_
#include <string>
#include <vector>
using namespace std;

int parseStringToInt(const string inputString);
int parseHexaStringToInt(const string inputString);
string parseIntToHexaString(const int inputInt,const unsigned int minimalReturnSize);
string parseIntToString(const int inputInt);
string parseIntToStringWithMinimalSize(const int inputInt,const unsigned int minimalReturnSize);
vector<string> splitString(const string inputString,const char delimiter);
void trimString( string &input);
void getTrimmedRestOfStringAfterPosition(string &input,const int position);
string getTextualField(const char separator,const char *inBuffer);
int outputStringToBuffer(const string stringToOutput,char *outBuffer);
int outputCharacterToBuffer(const char charToOutput,char *outBuffer);
vector<string> splitFieldsBySpaceFromString(const string &inString);
void splitFieldsFromBuffer(char *inBuffer,const int bufferSize, const char delimeter, vector<string> &responseTokens);
#endif /* STRINGUTILS_H_ */
