// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if (connectionParameters.role == LlTx) {
         unsigned char buf[BUF_SIZE] = {0};
        buf[0] = flag;
        buf[1] = add;
        buf[2] = c;
        buf[3] = bcc;
        buf[4] = flag;
        int bytes = writeBytesSerialPort(buf, BUF_SIZE);
        printf("%d bytes written to serial port\n", bytes);
    }
    // TODO: Implement this function
    int nBytesBuf = 0;
    int currentState = 0;
    if (connectionParameters.role = LlRx) {
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
            if (byte == flag)
                STOP = TRUE;

            else
            currentState = 0;
            break;
            

        }
        }
        unsigned char buf[BUF_SIZE] = {0};
        buf[0] = flag;
        buf[1] = A;
        buf[2] = C;
        buf[3] = BCC1;
        buf[4] = flag;
        int bytes = writeBytesSerialPort(buf, 5);

        printf("Total bytes received: %d\n", nBytesBuf);

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
