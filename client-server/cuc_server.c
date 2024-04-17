#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <stdio.h>
#include <open62541/plugin/log_stdout.h>

/*
   A szerveren létrehozunk egy objektumot, amely különböző változókat tartalmaz.
   Ezek a változók reprezentálják a TSN stream paramétereit, mint például hogy mikor induljon a stream, stb.
   Ezeket a változókat a szerver tárolja, és elérhetővé teszi az OPC-UA hálózaton keresztül.
*/
static void addTSNStreamObject(UA_Server *server) {
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

    // Példa változó hozzáadása: Interval
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Interval");
    UA_Int32 interval = 0;  // Kezdeti érték
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId intervalNodeId = UA_NODEID_NUMERIC(1, 5002);
    status = UA_Server_addVariableNode(server, intervalNodeId, objectNodeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "Interval"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       attr, &interval, NULL);
    if (status == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Interval variable created successfully.\n");
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to create Interval variable.\n");
    }
    // Hasonlóan adhatók hozzá további változók...

    // OK Message változó létrehozása
    UA_VariableAttributes okMsgAttr = UA_VariableAttributes_default;
    okMsgAttr.displayName = UA_LOCALIZEDTEXT("en-US", "OK Message");
    UA_String okMessage = UA_STRING("Not received");  // Kezdeti érték
    okMsgAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE; // Fontos!
    UA_NodeId okMessageNodeId = UA_NODEID_NUMERIC(1, 5003);
    UA_Server_addVariableNode(server, okMessageNodeId, objectNodeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "OKMessage"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              okMsgAttr, &okMessage, NULL);

}

int main(void) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // TSN stream paraméterek objektum hozzáadása
    addTSNStreamObject(server);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Starting OPC-UA server...\n");
    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}