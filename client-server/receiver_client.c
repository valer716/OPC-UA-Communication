#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <stdio.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_highlevel.h>

static void getAndAcknowledgeTSNStreamParameters(UA_Client *client) {
    // Példa: Interval változó lekérése
    UA_NodeId intervalNodeId = UA_NODEID_NUMERIC(1, 5002);
    UA_Variant value;
    UA_Variant_init(&value);

    UA_StatusCode status = UA_Client_readValueAttribute(client, intervalNodeId, &value);
    if (status == UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
        UA_Int32 interval = *(UA_Int32 *)value.data;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,"Received Interval: %d\n", interval);

        // További feldolgozás, például recv program beállítása
        // Itt lehetne a recv programot elindítani a kapott paraméterekkel

        // Az OK üzenet küldése, ha a paraméterek sikeresen lekérdezve és feldolgozva
        UA_NodeId okNodeId = UA_NODEID_NUMERIC(1, 5003);
        UA_Boolean ok = true;  // OK üzenet beállítása true-ra
        UA_Variant_setScalar(&value, &ok, &UA_TYPES[UA_TYPES_BOOLEAN]);

        status = UA_Client_writeValueAttribute(client, okNodeId, &value);
        if (status == UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,"OK Message sent successfully.\n");
        } else {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,"Failed to send OK Message: %s\n", UA_StatusCode_name(status));
        }
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,"Failed to read Interval or incorrect data type: %s\n", UA_StatusCode_name(status));
    }
}

int main(void) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode status = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if (status == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,"Connected to the CUC server.\n");
        getAndAcknowledgeTSNStreamParameters(client); // Paraméterek lekérése és OK üzenet küldése
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,"Failed to connect to the CUC server: %s\n", UA_StatusCode_name(status));
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}