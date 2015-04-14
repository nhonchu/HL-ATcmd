/*
 * hl-atcmd.c
 *
 * Simple Utility for sending AT commands to serial device 
 *
 *  Created on: feb 21st 2015
 *      Author: nhonchu (nchu@sierrawireless.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <ctype.h>

// Global var
volatile int g_bIsVerbose = 0;

int open_port(char* szDevice)
{
	int fd;

	fd = open(szDevice, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		if (g_bIsVerbose)
		{
			printf("open_port: Unable to open %s\n", szDevice);
		}
	}
	else
	{
		fcntl(fd, F_SETFL, 0);
	}
	return (fd);
}


int main(int argc, char** argv)
{
	int	fd = 0;
	int 	nRet = 0;
	struct	termios options;
	struct	termios bk_options;
	char	szATcmd[64];

	char	szDevicePort[128];
	char	szResponseFilter[128];
	int 	lBaudrate = 0;

	strcpy(szDevicePort, "/dev/ttyUSB0");
	memset(szResponseFilter, 0, sizeof(szResponseFilter));
	memset(szATcmd, 0, sizeof(szATcmd));

	while (optind < argc)
	{
		int c;

		if ((c = getopt (argc, argv, "vd:b:f:")) != -1)
		{
			switch (c)
			{
				case 'v':
					g_bIsVerbose = 1;
					break;
				case 'd':
					strcpy(szDevicePort, optarg);
					break;
				case 'b':
					lBaudrate = atol(optarg);
					break;
				case 'f':
					strcpy(szResponseFilter, optarg);
					break;
				default:
					break;
			}
		}
		else
		{
			strcpy(szATcmd, argv[optind++]);
		}
	}

	speed_t speed = B115200;

	switch (lBaudrate)
	{
		case 9600:
			speed = B9600;
			break;
		case 19200:
			speed = B19200;
			break;
		case 38400:
			speed = B38400;
			break;
		default:
			lBaudrate = 115200;
			speed = B115200;
			break;
	}

	if (g_bIsVerbose)
	{
		printf("DevicePort: %s\n", szDevicePort);
		printf("BaudRate: %d\n", lBaudrate);
		printf("AT-Cmd: %s\n", szATcmd);
		printf("Filter: %s\n", szResponseFilter);
	}

	if (strlen(szATcmd) == 0)
	{
		printf("Usage: hl-atcmd -d device -b baudrate -f filter at-cmd\n");
		printf("   Example 1: ./hl-atcmd -d /dev/ttyUSB0 -b 115200 AT+CPIN?\n");
		printf("   Example 2: ./hl-atcmd -d /dev/ttyUSB0 -b 115200 'AT+CPIN=\"1234\"'\n");
		printf("   Example 3: ./hl-atcmd -d /dev/ttyUSB0 -b 115200 -f AT+CGSN AT+CGSN\n");
		printf("   Example 4: ./hl-atcmd -d /dev/ttyUSB0 -b 115200 AT+CGMR\n");
		printf("   Ex5 Verbose : ./hl-atcmd -v AT+CGSN\n");
		return 1;
	}

	if (g_bIsVerbose)
	{
		printf("Opening %s", szDevicePort);
	}

	fd = open_port(szDevicePort);
	if (fd == -1)
	{

		if (g_bIsVerbose)
		{
			printf("... failed\n");
		}
		return 1;	//failed to open port
	}
	if (g_bIsVerbose)
	{
		printf("... done\n");
		printf("configuring com port...");
	}

	tcgetattr(fd, &options);
	tcgetattr(fd, &bk_options);	//save current settings

	cfsetispeed(&options, speed);	//default baudrate
	cfsetospeed(&options, speed);

	options.c_cflag |= (CLOCAL | CREAD);	// Enable the receiver and set local mode...
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;	// no parity 
	options.c_cflag &= ~CSIZE;	// Mask the character size bits
	options.c_cflag |= CS8;    // Select 8 data bits
	options.c_cflag |= CRTSCTS;    // Also called CRTSCTS

	options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag     &= ~OPOST;	//raw output
	options.c_cc[VMIN]  = 0;
	options.c_cc[VTIME] = 20;

	tcflush( fd, TCIFLUSH );

	tcsetattr(fd, TCSANOW, &options);
	if (g_bIsVerbose)
	{
		printf("done\n");
		printf("sending %s to serial port...", szATcmd);
	}
	
	strcat(szATcmd, "\r");	//add line feed to the cmd
	if (write(fd, szATcmd, strlen(szATcmd)) < strlen(szATcmd))
	{
		if (g_bIsVerbose)
		{
			printf(" failed!\n");
		}
		nRet = 2;	//failed to write to port
	}
	else
	{
		if (g_bIsVerbose)
		{
			printf("OK\n");
			printf("reading serial port...");
		}

		char buffer[255];
		int	index = 0;
		char buf;      
		int  nbytes=0;

		memset(buffer, 0, sizeof(buffer));

		do
		{
			nbytes = read(fd, &buf, 1);
			buffer[index++] = buf;

		} while (nbytes > 0);


		if (index > 0)
		{
			if (g_bIsVerbose)
			{
				printf("succeeded (%d)\n", index);
			}

			if (strlen(szResponseFilter) > 0)
			{
				//Filtering Response
				char * pszSubstr = NULL;

				pszSubstr = strstr(buffer, szResponseFilter);
				if (pszSubstr != NULL)
				{
					pszSubstr += strlen(szResponseFilter);
					while (isspace( *pszSubstr))
					{
					pszSubstr++;
				}
				char * pTemp = pszSubstr;
				while (pTemp < (buffer+strlen(buffer)))
				{
					if (*pTemp == '\n')	//remove cariage return
					{
						*pTemp = 0;	//zero terminate string
						break;
					}
					pTemp++;
				}
					printf("%s\n", pszSubstr);
				}
			}
			else
			{
				//Not filtering response
				printf("%s", buffer);
				fflush(stdout);
			}
		}
		else
		{
			if (g_bIsVerbose)
			{
				printf("failed\n");
			}
			printf("%s", buffer);
			nRet = 3;	//failed to read buffer
		}
	}

	tcsetattr(fd, TCSANOW, &bk_options);	//restore previous settings
	close(fd);

	return nRet;
}


