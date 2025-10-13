// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define _POSIX_C_SOURCE 200809L
#define BAUDRATE 38400
#define BUF_SIZE 5


unsigned char flag = 0x7E;
unsigned char A = 0x01;
unsigned char C = 0x07;
unsigned char BCC1 = 0x06;
unsigned char add = 0x03;
unsigned char c = 0x03;
unsigned char bcc = 0x00;
volatile int STOP = FALSE;
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
        
   

        int i = 0;
        int j = 0;
        while (STOP == FALSE) {
            unsigned char byte;
            int read_bytes = readByteSerialPort(&byte);
            nBytesBuf += read_bytes;
            
            int t = 3;
            if (alarm(t)) {
                writeBytesSerialPort(buf, BUF_SIZE);
                i++;
            }

            if (i == 2 ){
            STOP = TRUE;
            printf("limite 3x");
            }

            printf("var = 0x%02X\n", byte);
            j++;
            if(j == 5) {
                STOP = TRUE;
                alarm(0);
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

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO: Implement this function

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO: Implement this function

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{
    // TODO: Implement this function

    return 0;
}
