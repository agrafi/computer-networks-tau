CXX=g++
CXXFLAGS= -Wall -pedantic-errors -g -lg++

CLIENT_SOURCES=SocketHandler.cpp StringUtils.cpp WorkflowIdentifierEnum.cpp ProtocolParserCommon.cpp ClientApp.cpp
SERVER_SOURCES=SocketHandler.cpp StringUtils.cpp WorkflowIdentifierEnum.cpp ProtocolParserCommon.cpp ProtocolParserServerIn.cpp ProtocolParserServerOut.cpp ProtocolParserServer.cpp ServerApp.cpp
TEST_SERVER_SOURCES=SocketHandler.cpp StringUtils.cpp WorkflowIdentifierEnum.cpp ProtocolParserCommon.cpp TestServer.cpp

CLIENT_OBJECTS=$(CLIENT_SOURCES:.cpp=.o)
SERVER_OBJECTS=$(SERVER_SOURCES:.cpp=.o)
TEST_SERVER_OBJECTS =$(TEST_SERVER_SOURCES:.cpp=.o)
SERVER=mail_server
CLIENT=mail_client
TEST_SERVER=test_mailServer

all: $(CLIENT_SOURCES) $(SERVER_SOURCES) $(TEST_SERVER_SOURCES) $(TEST_SERVER) $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_OBJECTS)
	$(CXX) -o $@ $(SERVER_OBJECTS)

$(TEST_SERVER): $(TEST_SERVER_OBJECTS)
	$(CXX) -o $@ $(TEST_SERVER_OBJECTS)

$(CLIENT): $(CLIENT_OBJECTS)
	$(CXX) -o $@ $(CLIENT_OBJECTS)

clean:
	rm -f $(CLIENT_OBJECTS) $(SERVER_OBJECTS) $(TEST_SERVER_OBJECTS) $(TEST_SERVER) $(SERVER) $(CLIENT)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<
