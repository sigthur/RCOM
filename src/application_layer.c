// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define START_CONTROL 0x02
#define END_CONTROL   0x03
#define DATA_PACKET   0x01


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
    if (llopen(connectionParameters) < 0) {
        perror("Connection error\n");
        exit(-1);
    }

    if (connectionParameters.role == LlTx) {
        FILE *file = fopen(filename, "rb");
        if (!file) {
            perror("[TX] Failed to open file");
            return;
        }

        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        unsigned char startPacket[5];
        startPacket[0] = START_CONTROL;
        startPacket[1] = 0x00;     // type (size)
        startPacket[2] = 0x02;     // length
        startPacket[3] = fileSize / 256;
        startPacket[4] = fileSize % 256;

        printf("Starter packe\n");
        if (llwrite(startPacket, sizeof(startPacket)) < 0) {
            printf("Failed to send starter packet\n");
            fclose(file);
            return;
        }

        unsigned char buffer[MAX_PAYLOAD_SIZE];
        size_t bytesRead;
        int seq = 0;

        printf("Sending file\n");
        while ((bytesRead = fread(buffer, 1, MAX_PAYLOAD_SIZE - 3, file)) > 0) {
            unsigned char dataPacket[MAX_PAYLOAD_SIZE];
            dataPacket[0] = DATA_PACKET;
            dataPacket[1] = (bytesRead / 256);
            dataPacket[2] = (bytesRead % 256);
            memcpy(&dataPacket[3], buffer, bytesRead);

            int totalSize = bytesRead + 3;
            if (llwrite(dataPacket, totalSize) < 0) {
                printf("Error sending data packet\n");
                fclose(file);
                return;
            }

            seq = (seq + 1) % 2;
        }

        fclose(file);

        unsigned char endPacket[5];
        endPacket[0] = END_CONTROL;
        endPacket[1] = 0x00;
        endPacket[2] = 0x02;
        endPacket[3] = fileSize / 256;
        endPacket[4] = fileSize % 256;

        printf("Sending END control packet...\n");
        llwrite(endPacket, sizeof(endPacket));

        llclose(connectionParameters.role);
        printf("File '%s' sent successfully!\n", filename);
    }

    else if (connectionParameters.role == LlRx) {
        FILE *output = fopen("penguin-received.gif", "wb+");
        if (!output) {
            perror("Error creating file");
            return;
        }

        unsigned char packet[MAX_PAYLOAD_SIZE];
        int packetSize;
        int totalBytes = 0;
        int fileSize = 0;
        int started = 0;
        printf("Waiting\n");

        int finished = 0;
        while (!finished) {
            packetSize = llread(packet);
            if (packetSize <= 0)
                continue;

            unsigned char type = packet[0];

            if (type == START_CONTROL) {
                fileSize = packet[3] * 256 + packet[4];
                printf("Start packet received. File size: %d bytes\n", fileSize);
                started = 1;
            }

            else if (type == DATA_PACKET && started) {
                int dataLen = packetSize - 3;
                fwrite(&packet[3], 1, dataLen, output);
                totalBytes += dataLen;
                printf("Received %d/%d bytes...\n", totalBytes, fileSize);
            }

            else if (type == END_CONTROL) {
                printf("End packet received.\n");
                finished = 1;
            }
        }

        fclose(output);
        llclose(connectionParameters.role);
        printf("File received\n");
    }
}
