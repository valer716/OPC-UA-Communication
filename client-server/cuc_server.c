#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <stdio.h>
#include <open62541/plugin/log_stdout.h>
#include <string.h>

typedef struct {
    UA_String streamName;
    UA_String clientData;  // Ez lehet IP cím vagy port szám
    UA_Boolean dataSent;   // Az adat elküldését jelző flag
} ClientInfo;

ClientInfo clients[10];
int clientCount = 0;
int maxClients = 4;

static void checkAndSendData() {
    for (int i = 0; i < clientCount; i++) {
        for (int j = i + 1; j < clientCount; j++) {
            if (UA_String_equal(&clients[i].streamName, &clients[j].streamName) && !clients[i].dataSent && !clients[j].dataSent) {
                // TODO: Adatok elküldése

                clients[i].dataSent = true;
                clients[j].dataSent = true;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Data has been sent to the clients.\n");
            }
        }
    }
}

static void addClientInfo(UA_String streamName, UA_String clientData) {
    if (clientCount < maxClients) {
        clients[clientCount].streamName = streamName;
        clients[clientCount].clientData = clientData;
        clients[clientCount].dataSent = false;
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

/*
   A szerveren létrehozunk egy objektumot, amely különböző változókat tartalmaz.
   Ezek a változók reprezentálják a TSN stream paramétereit, mint például hogy mikor induljon a stream, stb.
   Ezeket a változókat a szerver tárolja, és elérhetővé teszi az OPC-UA hálózaton keresztül.
*/
/*static void addTSNStreamObject(UA_Server *server) {
    // Alap objektum attribútumok beállítása
    UA_ObjectAttributes objAttr = UA_ObjectAttributes_default;
    objAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TSN Stream Parameters");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId objectNodeId = UA_NODEID_NUMERIC(1, 5001);

    // Objektum node hozzáadása
    UA_StatusCode status = UA_Server_addObjectNode(server, objectNodeId, parentNodeId,
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                   UA_QUALIFIEDNAME(1, "TSNStreamParameters"),
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                                   objAttr, NULL, NULL);
    if (status == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "TSN Stream Parameters object created successfully.\n");
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to create TSN Stream Parameters object.\n");
    }

    // Változó hozzáadása: StreamNameS
    UA_VariableAttributes streamNameSAttr = UA_VariableAttributes_default;
    streamNameSAttr.displayName = UA_LOCALIZEDTEXT("en-US", "StreamNameS");
    UA_String streamNameS = UA_STRING("name");  // Kezdeti érték
    streamNameSAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId streamNameSNodeId = UA_NODEID_NUMERIC(1, 5002);
    status = UA_Server_addVariableNode(server, streamNameSNodeId, objectNodeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "StreamNameS"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       streamNameSAttr, &streamNameS, NULL);
    if (status == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "StreamNameS variable created successfully.\n");
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to create StreamNameS variable.\n");
    }
    // Hasonlóan adhatók hozzá további változók...
    // Változó hozzáadása: StreamNameR
    UA_VariableAttributes streamNameRAttr = UA_VariableAttributes_default;
    streamNameRAttr.displayName = UA_LOCALIZEDTEXT("en-US", "StreamNameR");
    UA_String streamNameR = UA_STRING("name");  // Kezdeti érték
    streamNameRAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId streamNameRNodeId = UA_NODEID_NUMERIC(1, 5003);
    status = UA_Server_addVariableNode(server, streamNameRNodeId, objectNodeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "StreamNameR"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       streamNameRAttr, &streamNameR, NULL);
    if (status == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "StreamNameR variable created successfully.\n");
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to create StreamNameR variable.\n");
    }

    // Változó hozzáadása: Port
    UA_VariableAttributes portAttr = UA_VariableAttributes_default;
    portAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Port");
    UA_Int32 port = 0;  // Kezdeti érték
    portAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId portNodeId = UA_NODEID_NUMERIC(1, 5004);
    status = UA_Server_addVariableNode(server, portNodeId, objectNodeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "Port"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       portAttr, &port, NULL);
    if (status == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Port variable created successfully.\n");
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to create Port variable.\n");
    }

    // Változó hozzáadása: IP
    UA_VariableAttributes ipAttr = UA_VariableAttributes_default;
    ipAttr.displayName = UA_LOCALIZEDTEXT("en-US", "IP");
    UA_String ip = UA_STRING("name");  // Kezdeti érték
    ipAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId ipNodeId = UA_NODEID_NUMERIC(1, 5005);
    status = UA_Server_addVariableNode(server, ipNodeId, objectNodeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "IP"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       ipAttr, &ip, NULL);
    if (status == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "IP variable created successfully.\n");
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to create IP variable.\n");
    }
}*/

    // Függvény az ellenőrzésre
/*void checkStreamValues(UA_Server *server, UA_NodeId streamNameSNodeId, UA_NodeId streamNameRNodeId) {
    // Változók inicializálása a beolvasott értékek tárolására
    UA_Variant streamNameSValue;
   UA_Variant_init(&streamNameSValue);

    UA_Variant streamNameRValue;
    UA_Variant_init(&streamNameRValue);

    // Az értékek lekérése a szerverről
    UA_Server_readValue(server, streamNameSNodeId, &streamNameSValue);
    UA_String streamNameS = *(UA_String*)streamNameSValue.data;

    UA_Server_readValue(server, streamNameRNodeId, &streamNameRValue);
    UA_String streamNameR = *(UA_String*)streamNameRValue.data;

   // Felszabadítás
    UA_Variant_clear(&streamNameSValue);
    UA_Variant_clear(&streamNameRValue);

    // Ellenőrzés
    if(strcmp((char*)streamNameS.data, "Stream1") == 0 && strcmp((char*)streamNameR.data, "Stream2") == 0) {
        printf("Ready\n");
    }
}*/

int main(void) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // TSN stream paraméterek objektum hozzáadása
    addTSNStreamObject(server);

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
    UA_Argument_init(&inputArguments[0]);
    inputArguments[0].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[0].description = UA_LOCALIZEDTEXT("en-US", "clientData");
    inputArguments[0].name = UA_STRING("clientData");
    inputArguments[0].valueRank = -1;

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