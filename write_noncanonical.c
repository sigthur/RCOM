// Example of how to write to the serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BAUDRATE 38400
#define BUF_SIZE 5

int fd = -1;           // File descriptor for open serial port
struct termios oldtio; // Serial port settings to restore on closing
volatile int STOP = FALSE;

int openSerialPort(const char *serialPort, int baudRate);
int closeSerialPort();
int readByteSerialPort(unsigned char *byte);
int writeBytesSerialPort(const unsigned char *bytes, int nBytes);
unsigned char flag = 0x7E;
unsigned char add = 0x03;
unsigned char c = 0x03;
unsigned char bcc = 0x00;
int alarmEnabled = FALSE;
int alarmCount = 0;


void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d received\n", alarmCount);
}

// ---------------------------------------------------
// MAIN
// ---------------------------------------------------
int main(int argc, char *argv[])
{

    struct sigaction act = {0};
    act.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS0\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    //
    // NOTE: See the implementation of the serial port library in "serial_port/".
    const char *serialPort = argv[1];

    if (openSerialPort(serialPort, BAUDRATE) < 0)
    {
        perror("openSerialPort");
        exit(-1);
    }

    printf("Serial port %s opened\n", serialPort);

    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};
    buf[0] = flag;
    buf[1] = add;
    buf[2] = c;
    buf[3] = bcc;
    buf[4] = flag;
    

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    buf[5] = '\n';

    int bytes = writeBytesSerialPort(buf, BUF_SIZE);
    printf("%d bytes written to serial port\n", bytes);

    // Wait until all bytes have been written to the serial port
    sleep(1);

    int nBytesBuf = 0;
    while (STOP == FALSE)
    {
        // Read one byte from serial port.
        // NOTE: You must check how many bytes were actually read by reading the return value.
        // In this example, we assume that the byte is always read, which may not be true.

        alarm(3);
        if(alarmCount == 3) {
            printf("reenviar");
            writeBytesSerialPort(buf, BUF_SIZE);
        }
        unsigned char byte;
        int bytes = readByteSerialPort(&byte);
        nBytesBuf += bytes;

        printf("var = 0x%02X\n", byte);

    }

    // Close serial port
    if (closeSerialPort() < 0)
    {
        perror("closeSerialPort");
        exit(-1);
    }

    printf("Serial port %s closed\n", serialPort);

    return 0;
}

// ---------------------------------------------------
// SERIAL PORT LIBRARY IMPLEMENTATION
// ---------------------------------------------------

// Open and configure the serial port.
// Returns -1 on error.
int openSerialPort(const char *serialPort, int baudRate)
{
    // Open with O_NONBLOCK to avoid hanging when CLOCAL
    // is not yet set on the serial port (changed later)
    int oflags = O_RDWR | O_NOCTTY | O_NONBLOCK;
    fd = open(serialPort, oflags);
    if (fd < 0)
    {
        perror(serialPort);
        return -1;
    }

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        return -1;
    }

    // Convert baud rate to appropriate flag

    // Baudrate settings are defined in <asm/termbits.h>, which is included by <termios.h>
#define CASE_BAUDRATE(baudrate) \
    case baudrate:              \
        br = B##baudrate;       \
        break;

    tcflag_t br;
    switch (baudRate)
    {
        CASE_BAUDRATE(1200);
        CASE_BAUDRATE(1800);
        CASE_BAUDRATE(2400);
        CASE_BAUDRATE(4800);
        CASE_BAUDRATE(9600);
        CASE_BAUDRATE(19200);
        CASE_BAUDRATE(38400);
        CASE_BAUDRATE(57600);
        CASE_BAUDRATE(115200);
    default:
        fprintf(stderr, "Unsupported baud rate (must be one of 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200)\n");
        return -1;
    }
#undef CASE_BAUDRATE

    // New port settings
    struct termios newtio;
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = br | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Block reading
    newtio.c_cc[VMIN] = 1;  // Byte by byte

    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        close(fd);
        return -1;
    }

    // Clear O_NONBLOCK flag to ensure blocking reads
    oflags ^= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, oflags) == -1)
    {
        perror("fcntl");
        close(fd);
        return -1;
    }

    return fd;
}

// Restore original port settings and close the serial port.
// Returns 0 on success and -1 on error.
int closeSerialPort()
{
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    return close(fd);
}

// Wait up to 0.1 second (VTIME) for a byte received from the serial port.
// Must check whether a byte was actually received from the return value.
// Save the received byte in the "byte" pointer.
// Returns -1 on error, 0 if no byte was received, 1 if a byte was received.
int readByteSerialPort(unsigned char *byte)
{
    return read(fd, byte, 1);
}

// Write up to numBytes from the "bytes" array to the serial port.
// Must check how many were actually written in the return value.
// Returns -1 on error, otherwise the number of bytes written.
int writeBytesSerialPort(const unsigned char *bytes, int nBytes)
{
    return write(fd, bytes, nBytes);
}

