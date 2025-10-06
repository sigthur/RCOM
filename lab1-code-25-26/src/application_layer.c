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
  connectionParameters.role = strcmp(role, "tx") ? LlRx : LlTx;
  connectionParameters.baudRate = baudRate;
  connectionParameters.nRetransmissions = nTries;
  connectionParameters.timeout = timeout;

  llopen(connectionParameters);
}
