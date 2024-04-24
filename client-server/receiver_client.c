#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <stdio.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_highlevel.h>
#include <signal.h>
#include <unistd.h>

static volatile UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Ctrl-C or SIGINT. Exiting...");
    running = false;
}

static void setTSNStreamParameters(UA_Client *client) {
    UA_NodeId streamNameRNodeId = UA_NODEID_NUMERIC(1, 5002);
    UA_String streamNameR = UA_STRING("Stream2");
    UA_Variant streamNameRValue;
    UA_Variant_setScalar(&streamNameRValue, &streamNameR, &UA_TYPES[UA_TYPES_STRING]);
    UA_Client_writeValueAttribute(client, streamNameRNodeId, &streamNameRValue);

    UA_NodeId ipNodeId = UA_NODEID_NUMERIC(1, 5002);
    UA_String ip = UA_STRING("192.068.0.1");
    UA_Variant ipValue;
    UA_Variant_setScalar(&ipValue, &ip, &UA_TYPES[UA_TYPES_STRING]);
    UA_Client_writeValueAttribute(client, ipNodeId, &ipValue);
    // További paramétereket hasonlóképp
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "TSN Stream Parameters set successfully.");
}

int main(void) {
    signal(SIGINT, stopHandler);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_Client_connect(client, "opc.tcp://localhost:4840");
    setTSNStreamParameters(client); //paraméterek beállítása a szerveren

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Press Ctrl-C to exit...");
    while(running) {
        usleep(100000);
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Exiting...");

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}

