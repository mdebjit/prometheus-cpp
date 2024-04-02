#pragma once
#include "prometheus/http_server.h"
#include "prometheus/handler.h"
#include <synchapi.h>

#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)(x)
#endif

namespace prometheus {

//ULONG
//RequestHandler::handleGet(HANDLE hReqQueue, PHTTP_REQUEST pRequest)
//{
//	UNUSED_PARAMETER(hReqQueue);
//	UNUSED_PARAMETER(pRequest);
//	return 0;
//}
void
HttpServer::HttpCleanup()
{
    HANDLE reqQueue = hReqQueue();
    std::vector<std::wstring> urllist = urls();
    //
    // Call HttpRemoveUrl for all added URLs.
    //
    for(int i=0; i<UrlAdded; i++) {
        HttpRemoveUrl(
              reqQueue,     // Req Queue
              urllist[i].c_str()        // Fully qualified URL
              );
    }

    //
    // Close the Request Queue handle.
    //
    if(NULL != reqQueue) {
        CloseHandle(reqQueue);
        reqQueue = NULL;
    }

    // 
    // Call HttpTerminate.
    //
    HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
    //WaitForSingleObject(hThread_, INFINITE);
    //wprintf(L"Server thread signalled\n");

    if (hThread_ != NULL) {
        CloseHandle(hThread_);
    }
    if(threadData_ != NULL) {
        HeapFree(GetProcessHeap(), 0, threadData_);
    }
	return;
}

int HttpServer::SetupServer(std::vector<std::wstring> urls)
{
    ULONG           retCode;
    HANDLE          hReqQueue      = NULL;
    HTTPAPI_VERSION HttpApiVersion = HTTPAPI_VERSION_1;
    
    //if (argc < 2)
    //{
    //    wprintf(L"%ws: <Url1> [Url2] ... \n", argv[0]);
    //    return -1;
    //}

    //
    // Initialize HTTP Server APIs
    //
    retCode = HttpInitialize( 
                HttpApiVersion,
                HTTP_INITIALIZE_SERVER,    // Flags
                NULL                       // Reserved
                );

    if (retCode != NO_ERROR)
    {
        wprintf(L"HttpInitialize failed with %lu \n", retCode);
        return retCode;
    }

    //
    // Create a Request Queue Handle
    //
    retCode = HttpCreateHttpHandle(
                &hReqQueue,        // Req Queue
                0                  // Reserved
                );

    if (retCode != NO_ERROR)
    {    
        wprintf(L"HttpCreateHttpHandle failed with %lu \n", retCode);
        HttpCleanup();
        return retCode;
    }

    //
    // The command line arguments represent URIs that to 
    // listen on. Call HttpAddUrl for each URI.
    //
    // The URI is a fully qualified URI and must include the
    // terminating (/) character.
    //
    for (int i = 0; i < urls.size(); i++)
    {
        wprintf(L"listening for requests on the following url: %s\n", urls[i].c_str());

        retCode = HttpAddUrl(
                    hReqQueue,    // Req Queue
                    urls[i].c_str(),      // Fully qualified URL
                    NULL          // Reserved
                    );

        if (retCode != NO_ERROR) {
            wprintf(L"HttpAddUrl failed with %lu \n", retCode);
            HttpCleanup();
            return retCode;
        } else {
            //
            // Track the currently added URLs.
            //
            UrlAdded ++;
        }
    }

    // Spawn a thread for the server loop
    threadData_ = (pServerContext)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(serverContext));
    if(threadData_ != NULL) {
        threadData_->hReqQueue = hReqQueue;
        //threadData_->server = reinterpret_cast<HttpServer*>(this);
        threadData_->server = reinterpret_cast<HttpServer*>(this);

        wprintf(L"Spawning server thread\n");
        hThread_ = CreateThread(
            NULL,
            0,
            DoReceiveRequests,
            threadData_,
            0,
            NULL
        );

        wprintf(L"Spawned server thread\n");

    }
    //DoReceiveRequests(hReqQueue, this);

    return retCode;
}

/*******************************************************************++

Routine Description:
    The function to receive a request. This function calls the  
    corresponding function to handle the response.

Arguments:
    hReqQueue - Handle to the request queue

Return Value:
    Success/Failure.

--*******************************************************************/
DWORD WINAPI DoReceiveRequests(
    __in LPVOID serverContext
    //__in HANDLE hReqQueue,
    //__in HttpServer* server
    )
{
    ULONG              result;
    HTTP_REQUEST_ID    requestId;
    DWORD              bytesRead;
    PHTTP_REQUEST      pRequest;
    PCHAR              pRequestBuffer;
    ULONG              RequestBufferLength;

    pServerContext context = (pServerContext)serverContext;
    HANDLE hReqQueue = context->hReqQueue;
    HttpServer* server = reinterpret_cast<HttpServer*>(context->server);
    //
    // Allocate a 2 KB buffer. This size should work for most 
    // requests. The buffer size can be increased if required. Space
    // is also required for an HTTP_REQUEST structure.
    //
    RequestBufferLength = sizeof(HTTP_REQUEST) + 2048;
    pRequestBuffer      = (PCHAR) ALLOC_MEM( RequestBufferLength );

    if (pRequestBuffer == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pRequest = (PHTTP_REQUEST)pRequestBuffer;

    //
    // Wait for a new request. This is indicated by a NULL 
    // request ID.
    //

    HTTP_SET_NULL_ID( &requestId );

    //LPOVERLAPPED myOverLapped = (LPOVERLAPPED) ALLOC_MEM( sizeof(OVERLAPPED) );
    //memset(myOverLapped, 0, sizeof(OVERLAPPED));
    //myOverLapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    for(;;)
    {
        wprintf(L"Waiting to Receive \n");
        RtlZeroMemory(pRequest, RequestBufferLength);

        result = HttpReceiveHttpRequest(
                    hReqQueue,          // Req Queue
                    requestId,          // Req ID
                    0,                  // Flags
                    pRequest,           // HTTP request buffer
                    RequestBufferLength,// req buffer length
                    &bytesRead,         // bytes received
                    NULL                // LPOVERLAPPED
                    );
                    //myOverLapped
                    //);
        
        if(NO_ERROR == result)
        {
            //
            // Worked! 
            // 
            wprintf(L"Received a request for %ws \n", 
                    pRequest->CookedUrl.pFullUrl);
            switch(pRequest->Verb)
            {
                case HttpVerbGET:
                    {
                        wprintf(L"Got a GET request for %ws \n", 
                                pRequest->CookedUrl.pFullUrl);

                        //SetLastRequestContext(hReqQueue, pRequest);
//                        request_handler handler = server->GetReqHandler();
                        //auto getHandle = server->metricsHandler->(*(server->getHandler));
                        if (true) {
                            //wprintf(L"Got handler %p\n", (server->metricsHandler->*(server->getHandler())));
                            //result = (server->metricsHandler->*(server->getHandler()))(hReqQueue, pRequest);
                            wprintf(L"Got handler 1 %p\n", ((server->getHandler)));
                            wprintf(L"Got handler 2 %p\n", (server->metricsHandler->getHandler));
                            wprintf(L"Got Handler 3 %lu\n", (server->metricsHandler->*(server->getHandler))(hReqQueue, pRequest));
                            wprintf(L"Get handler Result %lu\n", result);
                            //result = (server->metricsHandler->getHandler)(hReqQueue, pRequest));
                            //result = (server->getHandler)(hReqQueue, pRequest));
                        } else {
                            result = prometheus::SendHttpResponse(
                                     hReqQueue, 
                                     pRequest, 
                                     200,
                                     PSTR("OK"),
                                     PSTR("Hey! You hit the OLD server \r\n")
                                     );
                        }
                    }
                    break;

                case HttpVerbPOST:

                    wprintf(L"Got a POST request for %ws \n", 
                            pRequest->CookedUrl.pFullUrl);

                    //result= SendHttpPostResponse(hReqQueue, pRequest);
                    //break;

                default:
                 {
                    wprintf(L"Got an unknown request for %ws \n", 
                        pRequest->CookedUrl.pFullUrl);

                    result = prometheus::SendHttpResponse(
                        hReqQueue, 
                        pRequest,
                        503,
                        PSTR("Not Implemented"),
                        NULL
                        );
                 }
                    break;
            }

            if(result != NO_ERROR)
            {
                break;
            }

            //
            // Reset the Request ID to handle the next request.
            //
            HTTP_SET_NULL_ID( &requestId );
        }
        else if(result == ERROR_MORE_DATA)
        {
            wprintf(L"HttpReceiveHttpRequest returned ERROR_MORE_DATA \n");
            //
            // The input buffer was too small to hold the request
            // headers. Increase the buffer size and call the 
            // API again. 
            //
            // When calling the API again, handle the request
            // that failed by passing a RequestID.
            //
            // This RequestID is read from the old buffer.
            //
            requestId = pRequest->RequestId;

            //
            // Free the old buffer and allocate a new buffer.
            //
            RequestBufferLength = bytesRead;
            FREE_MEM( pRequestBuffer );
            pRequestBuffer = (PCHAR) ALLOC_MEM( RequestBufferLength );

            if (pRequestBuffer == NULL)
            {
                result = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            pRequest = (PHTTP_REQUEST)pRequestBuffer;

        }
        else if(ERROR_CONNECTION_INVALID == result && 
                !HTTP_IS_NULL_ID(&requestId))
        {
            wprintf(L"Connection was closed by client while processing a request.\n");
            // The TCP connection was corrupted by the peer when
            // attempting to handle a request with more buffer. 
            // Continue to the next request.
            
            HTTP_SET_NULL_ID( &requestId );
        }
        else
        {
            break;
        }

    }

    if(pRequestBuffer)
    {
        wprintf(L"Freeing Request Buffer\n");
        FREE_MEM( pRequestBuffer );
    }

    return result;
}

/*******************************************************************++

Routine Description:
    The routine sends a HTTP response

Arguments:
    hReqQueue     - Handle to the request queue
    pRequest      - The parsed HTTP request
    StatusCode    - Response Status Code
    pReason       - Response reason phrase
    pEntityString - Response entity body

Return Value:
    Success/Failure.
--*******************************************************************/

DWORD SendHttpResponse(
    IN HANDLE        hReqQueue,
    IN PHTTP_REQUEST pRequest,
    IN USHORT        StatusCode,
    IN PSTR          pReason,
    IN PSTR          pEntityString
    )
{
    HTTP_RESPONSE   response;
    HTTP_DATA_CHUNK dataChunk;
    DWORD           result;
    DWORD           bytesSent;

    //
    // Initialize the HTTP response structure.
    //
    INITIALIZE_HTTP_RESPONSE(&response, StatusCode, pReason);

    //
    // Add a known header.
    //
    ADD_KNOWN_HEADER(response, HttpHeaderContentType, "text/html");
   
    if(pEntityString)
    {
        // 
        // Add an entity chunk.
        //
        dataChunk.DataChunkType           = HttpDataChunkFromMemory;
        dataChunk.FromMemory.pBuffer      = pEntityString;
        dataChunk.FromMemory.BufferLength = 
                                       (ULONG) strlen(pEntityString);

        response.EntityChunkCount         = 1;
        response.pEntityChunks            = &dataChunk;
    }

    // 
    // Because the entity body is sent in one call, it is not
    // required to specify the Content-Length.
    //
    
    result = HttpSendHttpResponse(
                    hReqQueue,           // ReqQueueHandle
                    pRequest->RequestId, // Request ID
                    0,                   // Flags
                    &response,           // HTTP response
                    NULL,                // pReserved1
                    &bytesSent,          // bytes sent  (OPTIONAL)
                    NULL,                // pReserved2  (must be NULL)
                    0,                   // Reserved3   (must be 0)
                    NULL,                // LPOVERLAPPED(OPTIONAL)
                    NULL                 // pReserved4  (must be NULL)
                    ); 

    if(result != NO_ERROR)
    {
        wprintf(L"HttpSendHttpResponse failed with %lu \n", result);
    }

    return result;
}

#define MAX_ULONG_STR ((ULONG) sizeof("4294967295"))

/*******************************************************************++

Routine Description:
    The routine sends a HTTP response after reading the entity body.

Arguments:
    hReqQueue     - Handle to the request queue.
    pRequest      - The parsed HTTP request.

Return Value:
    Success/Failure.
--*******************************************************************/
/*
int SendHttpPostResponse(
    IN HANDLE        hReqQueue,
    IN PHTTP_REQUEST pRequest
    )
{
    HTTP_RESPONSE   response;
    int             result;
    int             bytesSent;
    PUCHAR          pEntityBuffer;
    ULONG           EntityBufferLength;
    ULONG           BytesRead;
    ULONG           TempFileBytesWritten;
    HANDLE          hTempFile;
    TCHAR           szTempName[MAX_PATH + 1];
    CHAR            szContentLength[MAX_ULONG_STR];
    HTTP_DATA_CHUNK dataChunk;
    ULONG           TotalBytesRead = 0;

    BytesRead  = 0;
    hTempFile  = INVALID_HANDLE_VALUE;

    //
    // Allocate space for an entity buffer. Buffer can be increased 
    // on demand.
    //
    EntityBufferLength = 2048;
    pEntityBuffer      = (PUCHAR) ALLOC_MEM( EntityBufferLength );

    if (pEntityBuffer == NULL)
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
        wprintf(L"Insufficient resources \n");
        goto Done;
    }

    //
    // Initialize the HTTP response structure.
    //
    INITIALIZE_HTTP_RESPONSE(&response, 200, "OK");

    //
    // For POST, echo back the entity from the
    // client
    //
    // NOTE: If the HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY flag had been
    //       passed with HttpReceiveHttpRequest(), the entity would 
    //       have been a part of HTTP_REQUEST (using the pEntityChunks
    //       field). Because that flag was not passed, there are no
    //       o entity bodies in HTTP_REQUEST.
    //
   
    if(pRequest->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS)
    {
        // The entity body is sent over multiple calls. Collect 
        // these in a file and send back. Create a temporary 
        // file.
        //

        if(GetTempFileName(
                L".", 
                L"New", 
                0, 
                szTempName
                ) == 0)
        {
            result = GetLastError();
            wprintf(L"GetTempFileName failed with %lu \n", result);
            goto Done;
        }

        hTempFile = CreateFile(
                        szTempName,
                        GENERIC_READ | GENERIC_WRITE, 
                        0,                  // Do not share.
                        NULL,               // No security descriptor.
                        CREATE_ALWAYS,      // Overrwrite existing.
                        FILE_ATTRIBUTE_NORMAL,    // Normal file.
                        NULL
                        );

        if(hTempFile == INVALID_HANDLE_VALUE)
        {
            result = GetLastError();
            wprintf(L"Cannot create temporary file. Error %lu \n",
                     result);
            goto Done;
        }

        do
        {
            //
            // Read the entity chunk from the request.
            //
            BytesRead = 0; 
            result = HttpReceiveRequestEntityBody(
                        hReqQueue,
                        pRequest->RequestId,
                        0,
                        pEntityBuffer,
                        EntityBufferLength,
                        &BytesRead,
                        NULL 
                        );

            switch(result)
            {
                case NO_ERROR:

                    if(BytesRead != 0)
                    {
                        TotalBytesRead += BytesRead;
                        WriteFile(
                                hTempFile, 
                                pEntityBuffer, 
                                BytesRead,
                                &TempFileBytesWritten,
                                NULL
                                );
                    }
                    break;

                case ERROR_HANDLE_EOF:

                    //
                    // The last request entity body has been read.
                    // Send back a response. 
                    //
                    // To illustrate entity sends via 
                    // HttpSendResponseEntityBody, the response will 
                    // be sent over multiple calls. To do this,
                    // pass the HTTP_SEND_RESPONSE_FLAG_MORE_DATA
                    // flag.
                    
                    if(BytesRead != 0)
                    {
                        TotalBytesRead += BytesRead;
                        WriteFile(
                                hTempFile, 
                                pEntityBuffer, 
                                BytesRead,
                                &TempFileBytesWritten,
                                NULL
                                );
                    }

                    //
                    // Because the response is sent over multiple
                    // API calls, add a content-length.
                    //
                    // Alternatively, the response could have been
                    // sent using chunked transfer encoding, by  
                    // passimg "Transfer-Encoding: Chunked".
                    //

                    // NOTE: Because the TotalBytesread in a ULONG
                    //       are accumulated, this will not work
                    //       for entity bodies larger than 4 GB. 
                    //       For support of large entity bodies,
                    //       use a ULONGLONG.
                    // 

                  
                    sprintf_s(szContentLength, MAX_ULONG_STR, "%lu", TotalBytesRead);

                    ADD_KNOWN_HEADER(
                            response, 
                            HttpHeaderContentLength, 
                            szContentLength
                            );

                    result = 
                        HttpSendHttpResponse(
                               hReqQueue,           // ReqQueueHandle
                               pRequest->RequestId, // Request ID
                               HTTP_SEND_RESPONSE_FLAG_MORE_DATA,
                               &response,       // HTTP response
                               NULL,            // pReserved1
                               &bytesSent,      // bytes sent-optional
                               NULL,            // pReserved2
                               0,               // Reserved3
                               NULL,            // LPOVERLAPPED
                               NULL             // pReserved4
                               );

                    if(result != NO_ERROR)
                    {
                        wprintf(
                           L"HttpSendHttpResponse failed with %lu \n", 
                           result
                           );
                        goto Done;
                    }

                    //
                    // Send entity body from a file handle.
                    //
                    dataChunk.DataChunkType = 
                        HttpDataChunkFromFileHandle;

                    dataChunk.FromFileHandle.
                        ByteRange.StartingOffset.QuadPart = 0;

                    dataChunk.FromFileHandle.
                        ByteRange.Length.QuadPart = 
                                          HTTP_BYTE_RANGE_TO_EOF;

                    dataChunk.FromFileHandle.FileHandle = hTempFile;

                    result = HttpSendResponseEntityBody(
                                hReqQueue,
                                pRequest->RequestId,
                                0,           // This is the last send.
                                1,           // Entity Chunk Count.
                                &dataChunk,
                                NULL,
                                NULL,
                                0,
                                NULL,
                                NULL
                                );

                    if(result != NO_ERROR)
                    {
                       wprintf(
                          L"HttpSendResponseEntityBody failed %lu\n", 
                          result
                          );
                    }

                    goto Done;

                    break;
                       

                default:
                  wprintf( 
                   L"HttpReceiveRequestEntityBody failed with %lu \n", 
                   result);
                  goto Done;
            }

        } while(TRUE);
    }
    else
    {
        // This request does not have an entity body.
        //
        
        result = HttpSendHttpResponse(
                   hReqQueue,           // ReqQueueHandle
                   pRequest->RequestId, // Request ID
                   0,
                   &response,           // HTTP response
                   NULL,                // pReserved1
                   &bytesSent,          // bytes sent (optional)
                   NULL,                // pReserved2
                   0,                   // Reserved3
                   NULL,                // LPOVERLAPPED
                   NULL                 // pReserved4
                   );
        if(result != NO_ERROR)
        {
            wprintf(L"HttpSendHttpResponse failed with %lu \n",
                    result);
        }
    }

Done:

    if(pEntityBuffer)
    {
        FREE_MEM(pEntityBuffer);
    }

    if(INVALID_HANDLE_VALUE != hTempFile)
    {
        CloseHandle(hTempFile);
        DeleteFile(szTempName);
    }

    return result;
}
*/
} // namespace prometheus