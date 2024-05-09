#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <stdio.h>
#include <open62541/plugin/log_stdout.h>
#include <signal.h>
#include <unistd.h>
#include <open62541/client_subscriptions.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>

static void readValueAttributeCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                                       UA_StatusCode status, UA_DataValue *var) {
    if (status == UA_STATUSCODE_GOOD) {
        if (var->hasValue && UA_Variant_hasScalarType(&var->value, &UA_TYPES[UA_TYPES_STRING])) {
            UA_String receivedData = *(UA_String *)var->value.data;
            printf("Received data asynchronously: %.*s\n", (int)receivedData.length, receivedData.data);
        } else {
            printf("Asynchronous read returned good status, but no valid value found.\n");
        }
    } else {
        printf("Asynchronous read failed with status: %s\n", UA_StatusCode_name(status));
    }
}

static void onFoundMatchNotification(UA_Client *client, UA_UInt32 subId, void *subContext,
                                     UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
        UA_Boolean foundMatch = *(UA_Boolean *)value->value.data;
        if (foundMatch) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Match found, data change detected.\n");
            UA_UInt32 reqId = 0;
            UA_StatusCode retval = UA_Client_readValueAttribute_async(client, UA_NODEID_NUMERIC(1, 6002), readValueAttributeCallback, NULL, &reqId);
            if (retval != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Asynchronous read initiation failed with status: %s\n", UA_StatusCode_name(retval));
            }
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


    // Subscription létrehozása
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);

    // Monitored item létrehozása
    UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(1, 5001));
    monRequest.requestedParameters.samplingInterval = 1000.0;
    UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                              UA_TIMESTAMPSTORETURN_BOTH, monRequest,
                                              NULL, onFoundMatchNotification, NULL);
    // Eseményciklus
   while (true) {
        retval = UA_Client_run_iterate(client, 1000);
        if (retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Connection lost, attempting to reconnect...");

            // Kapcsolat bontása és újracsatlakozás
            UA_Client_disconnect(client);
            retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
            if (retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Reconnect failed with %s", UA_StatusCode_name(retval));
                break;  // Ha nem sikerült újracsatlakozni, kilépünk a ciklusból
            }

            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Reconnected successfully.");
        }
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}

