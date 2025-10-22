// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define _POSIX_C_SOURCE 200809L
#define BAUDRATE 38400
#define BUF_SIZE 5
#define ESC 0x7D
#define ESC_FLAG 0x5E
#define ESC_ESC 0x5D
#define IN0 0x00
#define IN1 0x40
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81

unsigned char flag = 0x7E;
unsigned char A = 0x01;
unsigned char C = 0x07;
unsigned char BCC1 = 0x06;
unsigned char add = 0x03;
unsigned char c = 0x03;
unsigned char bcc = 0x00;
volatile int STOP = FALSE;
static unsigned char sequenceNumber = 0;

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////


int alarmEnabled = FALSE;
int alarmCount = 0;

void alarmHandler(int signal)
{
// Can be used to change a flag that increases the number of alarms
alarmEnabled = FALSE;
alarmCount++;
printf("Alarm #%d received\n", alarmCount);

}


int llopen(LinkLayer connectionParameters) {
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);



        if (fd < 0) {
            perror("Serial port not opened");
            exit(1);
    }



    struct sigaction act = {0};
    act.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
    perror("sigaction");
    exit(1);
    }



    int nBytesBuf = 0;
    int currentState = 0;
    if (connectionParameters.role == LlTx) {
        unsigned char buf[BUF_SIZE] = {0};
        buf[0] = flag;
        buf[1] = add;
        buf[2] = c;
        buf[3] = bcc;
        buf[4] = flag;
        int bytes = writeBytesSerialPort(buf, BUF_SIZE);
        printf("%d bytes written to serial port\n", bytes);
        sleep(1);
        
   

        //int i = 0;
        int j = 0;
        while (STOP == FALSE) {
            unsigned char byte;
            int read_bytes = readByteSerialPort(&byte);
            nBytesBuf += read_bytes;
            
            /*int t = 3;
            if (alarm(t)) {
                writeBytesSerialPort(buf, BUF_SIZE);
                i++;
            }

            if (i == 2 ){
            STOP = TRUE;
            printf("limite 3x");
            }*/

            printf("var = 0x%02X\n", byte);
            j++;
            if(j == 5) {
                STOP = TRUE;
                //alarm(0);
            }

        }
        
    }

    
    // TODO: Implement this function
    
    if (connectionParameters.role == LlRx) {
        while (STOP == FALSE)
        {   
        // Read one byte from serial port.
        // NOTE: You must check how many bytes were actually read by reading the return value.
        // In this example, we assume that the byte is always read, which may not be true.
        unsigned char byte;
        int bytes = readByteSerialPort(&byte);
        nBytesBuf += bytes;


        printf("var = 0x%02X\n", byte);

        
    
        switch(currentState){
            case 0:
                if(byte == flag)
                currentState = 1;
                break;
            case 1:
                if(byte == 0X03)
                currentState = 2;
                else if(byte == flag)
                continue;
                else
                currentState = 0;
                break;
            case 2:
                if (byte == 0x03)
                currentState = 3;
                else if (byte == flag)
                currentState = 1;
                else
                currentState = 0;
                break;
            case 3: 
            if (byte == 0x00)
                currentState = 4;
                else if (byte == flag)
                currentState = 1;
                else
                currentState = 0;
                break;
            case 4: 
            if (byte == flag){
                STOP = TRUE;
                alarm(0);
            }

            else
            currentState = 0;
            break;
            

        }
        }
        printf("Total bytes received: %d\n", nBytesBuf);
        unsigned char buf[BUF_SIZE] = {0};
        buf[0] = flag;
        buf[1] = A;
        buf[2] = C;
        buf[3] = BCC1;
        buf[4] = flag;
        int bytes = writeBytesSerialPort(buf, 5);
        printf("%d bytes written to serial port\n", bytes);

        

    }
    return 0;
}

int Byte_stuff(const unsigned char *byte, int byteSize, unsigned char *output) {
    int outIndex = 0;
    for (int i = 0; i < byteSize; i++) {
        if (byte[i] == flag) {
            output[outIndex++] = ESC;
            output[outIndex++] = ESC_FLAG;
        } 
        else if (byte[i] == ESC) {
            output[outIndex++] = ESC;
            output[outIndex++] = ESC_ESC;
        } 
        else {
            output[outIndex++] = byte[i];
        }
    }
    return outIndex;
}


////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO: Implement this function
    unsigned char frame[MAX_PAYLOAD_SIZE * 2 + 6];
    unsigned char stuffed[MAX_PAYLOAD_SIZE * 2];
    unsigned char bcc2 = buf[0];  // calculate BCC2 over data
    for (int i = 1; i < bufSize; i++) {
          bcc2 ^= buf[i];
    }

    int stuffedSize = Byte_stuff(buf, bufSize, stuffed);
    unsigned char bcc2Stuffed[2];
    int bcc2Size = Byte_stuff(&bcc2, 1, bcc2Stuffed);
    int idx = 0;
    frame[idx++] = flag;                     // Start FLAG
    frame[idx++] = 0x01;                     // Address
    frame[idx++] = 0x00 | (sequenceNumber << 6); // Control (Ns)
    frame[idx] = 0x01 ^ frame[idx-1];      // BCC1 = A ^ C
    idx++;
    for (int i = 0; i < stuffedSize; i++)    // Data
        frame[idx++] = stuffed[i];
    for (int i = 0; i < bcc2Size; i++)       // BCC2
        frame[idx++] = bcc2Stuffed[i];
    frame[idx++] = flag;                     // End FLAG

    int written = writeBytesSerialPort(frame, idx);
    if (written != idx) {
        printf("[llwrite] Error writing frame\n");
        return -1;
    }

    sequenceNumber = (sequenceNumber + 1) % 2;
    return bufSize;
}


int Byte_Destuff(const unsigned char *byte, int byteSize, unsigned char *output) {
    int outIndex = 0;
    for (int i = 0; i < byteSize; i++) {
        if (byte[i] == ESC) {
            i++; // skip ESC
            if (i >= byteSize) break; // safety check
            if (byte[i] == ESC_FLAG)
                output[outIndex++] = flag;
            else if (byte[i] == ESC_ESC)
                output[outIndex++] = ESC;
        } 
        else {
            output[outIndex++] = byte[i];
        }
    }
    return outIndex;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO: Implement this function
    unsigned char byte;
    unsigned char frame[MAX_PAYLOAD_SIZE * 2 + 6];
    int state = 0;
    int idx = 0;
    
    // Simple state machine to read a full I-frame
    int frameComplete = 0;
    while (!frameComplete) {
        int res = readByteSerialPort(&byte);
        if (res <= 0) continue;

        switch (state) {
            case 0:
                if (byte == flag) {
                    state = 1;
                    idx = 0;
                    frame[idx++] = byte;
                }
                break;
            case 1:
                frame[idx++] = byte;
                if (byte == flag) {
                    frameComplete = 1;
                }
                break;
        }
    }

    int frameSize = idx;
    unsigned char destuffed[MAX_PAYLOAD_SIZE + 1];
    int dataSize = frameSize - 5;
    int destuffedSize = Byte_Destuff(&frame[4], dataSize, destuffed);

    // Verify BCC2
    unsigned char bcc2 = destuffed[destuffedSize - 1];
    unsigned char calcBCC2 = 0x00;
    for (int i = 0; i < destuffedSize - 1; i++)
        calcBCC2 ^= destuffed[i];

    if (bcc2 != calcBCC2) {
        printf("[llread] BCC2 error! (expected %02X, got %02X)\n", calcBCC2, bcc2);
        return -1;
    }

    memcpy(packet, destuffed, destuffedSize - 1);
    printf("[llread] Received %d bytes (BCC OK)\n", destuffedSize - 1);
    return destuffedSize - 1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{
    // TODO: Implement this function
    int fd = closeSerialPort();
    if (fd < 0) {
        perror("Error closing serial port");
        return -1;
    }

    printf("Closed.\n");
    return 0;

    return 0;
}
