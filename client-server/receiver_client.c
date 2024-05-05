#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <stdio.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_highlevel.h>
#include <signal.h>
#include <unistd.h>
#include <open62541/client_subscriptions.h>

static void onFoundMatchNotification(UA_Client *client, UA_UInt32 subId, void *subContext,
                                     UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
        UA_Boolean dataChanged = *(UA_Boolean *)value->value.data;
        if(dataChanged) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Data change detected, retrieving configuration data.\n");
            UA_Variant v;
            UA_Variant_init(&v);
            UA_StatusCode retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(1, 6002), &v);
            if(retval == UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_STRING])) {
                UA_String receivedData = *(UA_String *)v.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received data: %.*s\n", (int)receivedData.length, receivedData.data);
            } else { UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to exchange data.\n"); }
            UA_Variant_clear(&v);
        }
    }
}

int main(void) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    // Adatok elküldése a szerveren definiált metóduson keresztül
    UA_String streamName = UA_STRING("Stream1");
    UA_String ipAddress = UA_STRING("192.168.1.100");
    UA_Variant input[2];
    UA_Variant_init(&input[0]);
    UA_Variant_init(&input[1]);
    UA_Variant_setScalarCopy(&input[0], &streamName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalarCopy(&input[1], &ipAddress, &UA_TYPES[UA_TYPES_STRING]);

    size_t outputSize;
    UA_Variant *output;

    retval = UA_Client_call(client,
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                          UA_NODEID_STRING(1, "handleClientData"),
                                          2, input,
                                          &outputSize, &output);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Method call was successful.\n");
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Method call failed with %s.\n", UA_StatusCode_name(retval));
    }


    // Várakozás és port szám lekérdezése
    // Create a subscription
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                             NULL, NULL, NULL);

    // Create a MonitoredItem
    UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(1, 5001));  // DataChanged Node
    monRequest.requestedParameters.samplingInterval = 1000.0;  // 1 second
    UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                              UA_TIMESTAMPSTORETURN_BOTH, monRequest,
                                              NULL, onFoundMatchNotification, NULL);

    // Running the client
    while(!UA_Client_run_iterate(client, 1000));  // Run client every 1 second

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}

