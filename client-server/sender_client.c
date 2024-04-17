#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>

static void setTSNStreamParameters(UA_Client *client) {
    UA_NodeId intervalNodeId = UA_NODEID_NUMERIC(1, 5002);
    UA_Int32 interval = 100;  // Példaérték
    UA_Variant value;
    UA_Variant_setScalar(&value, &interval, &UA_TYPES[UA_TYPES_INT32]);
    UA_Client_writeValueAttribute(client, intervalNodeId, &value);
    // További paramétereket hasonlóképp
}

int main(void) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_Client_connect(client, "opc.tcp://localhost:4840");
    setTSNStreamParameters(client); //paraméterek beállítása a szerveren
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}