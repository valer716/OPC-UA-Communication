#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <open62541/client_subscriptions.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>

static void onConnect(UA_Client *client, UA_SecureChannelState channelState, UA_SessionState sessionState, UA_StatusCode connectStatus) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Async connect returned with status code %s\n", UA_StatusCode_name(connectStatus));
}

static void fileBrowsed(UA_Client *client, void *userdata, UA_UInt32 requestId, UA_BrowseResponse *response) {
    printf("%-50s%u\n", "Received BrowseResponse for request ", requestId);
    UA_String us = *(UA_String *) userdata;
    printf("---%.*s passed safely \n", (int) us.length, us.data);
}

static void readValueAttributeCallback(UA_Client *client, void *userdata, UA_UInt32 requestId, UA_StatusCode status, UA_DataValue *var) {
    printf("%-50s%u\n", "Read value attribute for request", requestId);
    if(UA_Variant_hasScalarType(&var->value, &UA_TYPES[UA_TYPES_INT32])) {
        UA_Int32 int_val = *(UA_Int32*) var->value.data;
        printf("---%-40s%-8i\n", "Reading the value of node (1, \"the.answer\"):", int_val);
    }
}

static void attrWritten(UA_Client *client, void *userdata, UA_UInt32 requestId, UA_WriteResponse *response) {
    printf("%-50s%u\n", "Wrote value attribute for request ", requestId);
    UA_WriteResponse_clear(response);
}

static void methodCalled(UA_Client *client, void *userdata, UA_UInt32 requestId, UA_CallResponse *response) {
    printf("%-50s%u\n", "Called method for request ", requestId);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode retval = response->responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(response->resultsSize == 1)
            retval = response->results[0].statusCode;
        else
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_CallResponse_clear(response);
        printf("---Method call was unsuccessful, returned %x values.\n", retval);
    } else {
        /* Move the output arguments */
        output = response->results[0].outputArguments;
        outputSize = response->results[0].outputArgumentsSize;
        response->results[0].outputArguments = NULL;
        response->results[0].outputArgumentsSize = 0;
        printf("---Method call was successful, returned %lu values.\n", (unsigned long)outputSize);
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    }
    UA_CallResponse_clear(response);
}

static void onFoundMatchNotification(UA_Client *client, UA_UInt32 subId, void *subContext,
                                     UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
        UA_Boolean dataChanged = *(UA_Boolean *)value->value.data;
        if(dataChanged) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Data change detected, retrieving configuration data.\n");
            UA_Variant v;
            UA_Variant_init(&v);
            UA_StatusCode retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(1, 6003), &v);
            if(retval == UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_STRING])) {
                UA_String receivedData = *(UA_String *)v.data;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received data: %.*s\n", (int)receivedData.length, receivedData.data);
            } else { UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to exchange data.\n"); }
            UA_Variant_clear(&v);
        }
    }
}

int main(void) {
    /*UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_UInt32 reqId = 0;

    UA_StatusCode retval = UA_Client_connectAsync(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }*/
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
    UA_UInt32 reqId = 0;

    UA_String userdata = UA_STRING("userdata");

    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;


    config->stateCallback = onConnect;
    UA_Client_connectAsync(client, "opc.tcp://localhost:4840");

    //sleep(100);

    UA_Client_sendAsyncBrowseRequest(client, &bReq, fileBrowsed, &userdata, &reqId);
    UA_DateTime startTime = UA_DateTime_nowMonotonic();
    do {
        UA_SessionState ss;
        UA_Client_getState(client, NULL, &ss, NULL);
        if(ss == UA_SESSIONSTATE_ACTIVATED) {
            UA_Client_sendAsyncBrowseRequest(client, &bReq, fileBrowsed, &userdata, &reqId);
        }
        UA_BrowseRequest_clear(&bReq);
        UA_Client_run_iterate(client, 0);
        //sleep(100);

        if(UA_DateTime_nowMonotonic() - startTime > 2000 * UA_DATETIME_MSEC)
            break;
    } while(reqId < 10);

    // Adatok elküldése a szerveren definiált metóduson keresztül
    UA_String streamName = UA_STRING("Stream1");
    UA_String port = UA_STRING("12345");  // Példa port érték
    UA_Variant sNVariant;
    UA_Variant pVariant;
    UA_Variant_init(&sNVariant);
    UA_Variant_init(&pVariant);

    UA_Variant input[2];
    UA_Variant_init(&input[0]);
    UA_Variant_init(&input[1]);

    /*UA_Variant_setScalarCopy(&input[0], &sNVariant, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalarCopy(&input[1], &pVariant, &UA_TYPES[UA_TYPES_STRING]);
    retval = UA_Client_call_async(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_STRING(1, "handleClientData"), 1, input, methodCalled, NULL, &reqId);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Method call was successful.\n");
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Method call failed with %s.\n", UA_StatusCode_name(retval));
    }*/

    //UA_Client_run_iterate(client, 0);
    //UA_Client_run_iterate(client, 0);

    UA_SessionState ss;
    UA_Client_getState(client, NULL, &ss, NULL);
    if(ss == UA_SESSIONSTATE_ACTIVATED) {
            UA_Variant_setScalarCopy(&input[0], &sNVariant, &UA_TYPES[UA_TYPES_STRING]);
            UA_Variant_setScalarCopy(&input[1], &pVariant, &UA_TYPES[UA_TYPES_STRING]);

            UA_Client_writeValueAttribute_async(client, UA_NODEID_STRING(1, "the.answer"), &sNVariant, attrWritten, NULL, &reqId);
            UA_Variant_clear(&sNVariant);
            UA_Client_writeValueAttribute_async(client, UA_NODEID_STRING(1, "the.answer"), &pVariant, attrWritten, NULL, &reqId);
            UA_Variant_clear(&pVariant);

            UA_Client_readValueAttribute_async(client, UA_NODEID_STRING(1, "the.answer"), readValueAttributeCallback, NULL, &reqId);
            UA_String stringValue = UA_String_fromChars("World");
            UA_Variant_setScalar(&input, &stringValue, &UA_TYPES[UA_TYPES_STRING]);
            UA_Client_call_async(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_STRING(1, "handleClientData"), 2, input, methodCalled, NULL, &reqId);
            UA_String_clear(&stringValue);
            UA_Client_run_iterate(client, 0);
            UA_Client_run_iterate(client, 0);
    }
    // Create a subscription
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                             NULL, NULL, NULL);
    // Create a MonitoredItem
    UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(1, 5001));  // DataChanged Node
    monRequest.requestedParameters.samplingInterval = 1000.0;  // 1 second
    UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId, UA_TIMESTAMPSTORETURN_BOTH, monRequest, NULL, onFoundMatchNotification, NULL);

    // Running the client
    while(!UA_Client_run_iterate(client, 0));  // Run client every 1 second

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}