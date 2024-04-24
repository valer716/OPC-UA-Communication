#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <signal.h>
#include <unistd.h>

static volatile UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Ctrl-C or SIGINT. Exiting...");
    running = false;
}

static void setTSNStreamParameters(UA_Client *client) {
    UA_NodeId streamNameSNodeId = UA_NODEID_NUMERIC(1, 5002);
    UA_String streamNameS = UA_STRING("Stream1");
    UA_Variant streamNameSValue;
    UA_Variant_setScalar(&streamNameSValue, &streamNameS, &UA_TYPES[UA_TYPES_STRING]);
    UA_Client_writeValueAttribute(client, streamNameSNodeId, &streamNameSValue);

    UA_NodeId portNodeId = UA_NODEID_NUMERIC(1, 5002);
    UA_Int32 port = 100;
    UA_Variant portValue;
    UA_Variant_setScalar(&portValue, &port, &UA_TYPES[UA_TYPES_INT32]);
    UA_Client_writeValueAttribute(client, portNodeId, &portValue);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "TSN Stream Parameters set successfully.");
}

int main(void) {
    signal(SIGINT, stopHandler);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_Client_connect(client, "opc.tcp://localhost:4840");
    setTSNStreamParameters(client); //paraméterek beállítása a szerveren

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Press Ctrl-C to Exit...");

    while(running){
        usleep(100000);
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Exiting...");

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}