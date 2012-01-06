CXX=g++
CXXFLAGS= -Wall -pedantic-errors -g -lg++

CLIENT_SOURCES=SocketHandler.cpp StringUtils.cpp WorkflowIdentifierEnum.cpp ProtocolParserCommon.cpp ClientApp.cpp
SERVER_SOURCES=SocketHandler.cpp StringUtils.cpp WorkflowIdentifierEnum.cpp ProtocolParserCommon.cpp ProtocolParserServerIn.cpp ProtocolParserServerOut.cpp ProtocolParserServer.cpp ServerApp.cpp
DUMMY_CLIENT_SOURCES=SocketHandler.cpp StringUtils.cpp WorkflowIdentifierEnum.cpp ProtocolParserCommon.cpp DummyClient.cpp

CLIENT_OBJECTS=$(CLIENT_SOURCES:.cpp=.o)
SERVER_OBJECTS=$(SERVER_SOURCES:.cpp=.o)
DUMMY_CLIENT_OBJECTS =$(DUMMY_CLIENT_SOURCES:.cpp=.o)
SERVER=mail_server
CLIENT=mail_client
DUMMY_CLIENT=dummy_mailClient

all: $(CLIENT_SOURCES) $(SERVER_SOURCES) $(DUMMY_CLIENT_SOURCES) $(DUMMY_CLIENT) $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_OBJECTS)
	$(CXX) -o $@ $(SERVER_OBJECTS)

$(DUMMY_CLIENT): $(DUMMY_CLIENT_OBJECTS)
	$(CXX) -o $@ $(DUMMY_CLIENT_OBJECTS)

$(CLIENT): $(CLIENT_OBJECTS)
	$(CXX) -o $@ $(CLIENT_OBJECTS)

clean:
	rm -f $(CLIENT_OBJECTS) $(SERVER_OBJECTS) $(DUMMY_CLIENT_OBJECTS) $(DUMMY_CLIENT) $(SERVER) $(CLIENT)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<
