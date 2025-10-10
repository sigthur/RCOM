// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // TODO: Implement this function
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort,serialPort);
    connectionParameters.role = strcmp(role, "tx") ? LlRx : LlTx;
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;
    printf("Opening link layer as %s on %s\n", 
           connectionParameters.role == LlTx ? "Transmitter" : "Receiver",
           connectionParameters.serialPort);
    llopen(connectionParameters);
}
