#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <stdio.h>
#include <open62541/plugin/log_stdout.h>
#include <string.h>
#include <open62541/client_subscriptions.h>


typedef struct {
    UA_String streamName;
    UA_String clientData;  // Ez lehet IP cím vagy port szám
} ClientInfo;

ClientInfo clients[10];
int clientCount = 0;
int maxClients = 10;

UA_NodeId foundMatchNodeId;
UA_Server *globalServer;

static void checkAndSendData() {
    for (int i = 0; i < clientCount; i++) {
        for (int j = i + 1; j < clientCount; j++) {
            if (UA_String_equal(&clients[i].streamName, &clients[j].streamName)) {
                // Adatok kicserélése
                UA_Variant value;
                UA_Variant_setScalar(&value, &clients[i].clientData, &UA_TYPES[UA_TYPES_STRING]);
                UA_Server_writeValue(globalServer, UA_NODEID_NUMERIC(1, 6002), value);  // Port küldése

                UA_Variant_setScalar(&value, &clients[j].clientData, &UA_TYPES[UA_TYPES_STRING]);
                UA_Server_writeValue(globalServer, UA_NODEID_NUMERIC(1, 6003), value);  // IP cím küldése

                UA_Variant booleanvalue;
                UA_Boolean foundMatch = true;
                UA_Variant_setScalar(&booleanvalue, &foundMatch, &UA_TYPES[UA_TYPES_BOOLEAN]);
                UA_Server_writeValue(globalServer, foundMatchNodeId, booleanvalue);

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"Data exchanged between %.*s and %.*s.\n", clients[i].streamName.length, clients[i].streamName.data,
                                                                  clients[j].streamName.length, clients[j].streamName.data);
            }
        }
    }
}

static void addClientInfo(UA_String streamName, UA_String clientData) {
    if (clientCount < maxClients) {
        clients[clientCount].streamName =  UA_String_fromChars(streamName.data);
        clients[clientCount].clientData =  UA_String_fromChars(clientData.data);
        clientCount++;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Client added: %.*s\n", (int)streamName.length, streamName.data);
        checkAndSendData();  // Új klienst hozzáadtunk, ellenőrizzük, hogy van-e match
    }
}

// Függvény a kliensek adatainak fogadására
static UA_StatusCode handleClientData(UA_Server* server,
    const UA_NodeId* sessionId, void* sessionContext,
    const UA_NodeId* methodId, void* methodContext,
    const UA_NodeId* objectId, void* objectContext,
    size_t inputSize, const UA_Variant* input,
    size_t outputSize, UA_Variant* output) {

    if (inputSize < 2) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Incorrect number of inputs.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_String streamName = *(UA_String*)input[0].data;
    UA_String clientData = *(UA_String*)input[1].data;  // IP cím vagy port

    // TODO: logolás

    addClientInfo(streamName, clientData); // kliens hozzáadása

    return UA_STATUSCODE_GOOD;
}

int main(void) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    globalServer = server;

    // foundMatch Node hozzáadása
    foundMatchNodeId = UA_NODEID_NUMERIC(1, 5001);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Found Match");
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Boolean foundMatch = false;
    UA_Variant_setScalar(&attr.value, &foundMatch, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_addVariableNode(server, foundMatchNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, "FoundMatch"),
                              UA_NODEID_NULL, attr, NULL, NULL);

    // Kliens adatait kezelő metódus hozzáadása

    UA_NodeId methodId = UA_NODEID_STRING(1, "handleClientData");
    UA_Argument inputArguments[2];

    // streamName
    UA_Argument_init(&inputArguments[0]);
    inputArguments[0].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[0].description = UA_LOCALIZEDTEXT("en-US", "streamName");
    inputArguments[0].name = UA_STRING("streamName");
    inputArguments[0].valueRank = -1;

    // clientData
    UA_Argument_init(&inputArguments[1]);
    inputArguments[1].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[1].description = UA_LOCALIZEDTEXT("en-US", "clientData");
    inputArguments[1].name = UA_STRING("clientData");
    inputArguments[1].valueRank = -1;

    UA_MethodAttributes methodAttr = UA_MethodAttributes_default;
    methodAttr.description = UA_LOCALIZEDTEXT("en-US", "Handle Client Data");
    methodAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Handle Client Data");
    methodAttr.executable = true;
    methodAttr.userExecutable = true;

    UA_Server_addMethodNode(server, methodId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "handleClientData"),
        methodAttr, &handleClientData,
        2, &inputArguments, 0, NULL, NULL, NULL);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Starting OPC-UA server...\n");
    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}