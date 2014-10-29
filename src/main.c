#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "uart_api.h"

#define HANBACK_START_BIT_FRONT 0x76
#define HANBACK_START_BIT_REAR  0x00

#define MAX_BUFF_SIZE	64

// SPO2_Pulsation(IRPPG = 0x49, HEARTBEAT = 0x48, BLOODSUGAR = 0x53)
#define HANBACK_SPO2_PLSTN_FRONT	0x49	// 0x49 == I(char)
#define HANBACK_SPO2_PLSTN_MIDDLE	0x48	// 0x48 == H(char)
#define HANBACK_SPO2_PLSTN_REAR		0x53	// 0x53 == S(char)
#define LAXTHA_UBPULSE_FRONT		0xFF	// 0xFF == 255(int)
#define LAXTHA_UBPULSE_REAR			0xFE	// 0xFE == 254(int)

void *thread_hanback(void* data);
void *thread_ubpulse(void* data);
void strncat_s(unsigned char *target, unsigned char *buff, int target_size, int buff_size);
void getElapsedTime(struct timeval Tstart, struct timeval Tend);

int main(int argc, char *argv[])
{
	int status[2];
	pthread_t p_thread[2];

	printf("<Input: %d, %s, %s>\n", argc, argv[0], argv[1]);

	if(!strcmp(argv[1], "1")) {
//		if(pthread_create(&p_thread[0], NULL, thread_hanback, NULL) < 0) {
//			perror("hanback pthread_create() Error");
//			exit(0);
//		}
//		pthread_join(p_thread[0], (void **)&status[0]);

		char	*serial	= "/dev/ttyUSB0";
		int		bourbit	= 57600;

		int i = 0;
		int flag = 0;		// I발견시 1로 바뀌고 뒤 숫자 4개 모두 출력시 0으로 바뀜
		int flag_cnt = 0;	// 수신 카운터 플래그를 올릴지 말지 결정
		int readSize = 0;
		int readTotalSize = 0;
		unsigned char readBuff[MAX_BUFF_SIZE] = { -1 };

		struct timeval Time;

		int fd = user_uart_open(serial);
		user_uart_config(fd, bourbit, 8, 0, 1);

		if(access(serial, R_OK & W_OK) == 0)
		{
			//printf("Access OK!\n");
			fd = user_uart_open(serial);
			user_uart_config(fd, bourbit, 8, 0, 1);

			//start bit가 일치 할 때 까지 계속 data를 받음
			while(1)
			{
				if((readSize = user_uart_read(fd, readBuff, 19)) == -1) {
					readSize = 0;
					continue;
				}

				gettimeofday(&Time, NULL);

				for(i = 0; i < readSize; i++) {
					if(readBuff[i] == HANBACK_SPO2_PLSTN_FRONT)			flag = 1;
					else if(readBuff[i] == HANBACK_SPO2_PLSTN_MIDDLE)	flag = 2;
					else if(readBuff[i] == HANBACK_SPO2_PLSTN_MIDDLE)	flag = 3;

					if(flag == 1) {
						flag_cnt++;
						printf("%c", readBuff[i]);
					}
					if(flag_cnt > 4) {
						flag = 0;
						flag_cnt = 0;
						printf("\t%d\t%d\n", Time.tv_sec, Time.tv_usec);
					}

	//				printf("%d\t%d\n", flag, flag_cnt);
				}

				usleep(10 * 1000);
				memset(readBuff, -1, sizeof(readBuff));
			}

			user_uart_close(fd);
		}
	}
	else if(!strcmp(argv[1], "2")) {
//		if(pthread_create(&p_thread[1], NULL, thread_ubpulse, NULL) < 0) {
//			perror("ubpulse pthread_create() Error");
//			exit(0);
//		}
//		pthread_join(p_thread[1], (void **)&status[1]);

		char	*serial	= "/dev/ttyACM0";
		int		bourbit	= 115200;

		int i = 0;
		int flag = 0;		// I발견시 1로 바뀌고 뒤 숫자 4개 모두 출력시 0으로 바뀜
		int flag_cnt = 0;	// 수신 카운터 플래그를 올릴지 말지 결정
	    int readSize = 0;
	    int readTotalSize = 0;
	    unsigned char readBuff[MAX_BUFF_SIZE] = { -1 };
	    unsigned char readTemp[MAX_BUFF_SIZE] = { -1 };

	    int H_bit = 0;
	    int L_bit = 0;
	    int Pulsation_Data = 0;

	    struct timeval Time;

		int fd = user_uart_open(serial);
		user_uart_config(fd, bourbit, 8, 0, 1);

		if(access(serial, R_OK & W_OK) == 0)
		{
			//printf("Access OK!\n");
			fd = user_uart_open(serial);
			user_uart_config(fd, bourbit, 8, 0, 1);

			//start bit가 일치 할 때 까지 계속 data를 받음
			while(1)
			{
		        for(readTotalSize = 0; readTotalSize < 19;) {
		            if((readSize = user_uart_read(fd, readTemp, 19)) == -1) {
		                readSize = 0;
		                continue;
		            }

		            strncat_s(readBuff, readTemp, readTotalSize, readSize);

					if( readBuff[0] == LAXTHA_UBPULSE_FRONT &&
						readBuff[1] == LAXTHA_UBPULSE_REAR) {
		                readTotalSize += readSize;
		            }
		            else {
		                readSize = 0;
		                readTotalSize = 0;
		            }
		        }

				gettimeofday(&Time, NULL);

				H_bit = readBuff[7];
				L_bit = readBuff[8];
				Pulsation_Data = H_bit * 256 + L_bit;
				printf("%d\t%d\t%d\n", Pulsation_Data, Time.tv_sec, Time.tv_usec);

				usleep(10 * 1000);
				memset(readBuff, -1, sizeof(readBuff));
			}

			user_uart_close(fd);
		}
	}

	return 1;
}

void *thread_hanback(void* data)
{
	char	*serial	= "/dev/ttyUSB0";
	int		bourbit	= 57600;

	int i = 0;
	int flag = 0;		// I발견시 1로 바뀌고 뒤 숫자 4개 모두 출력시 0으로 바뀜
	int flag_cnt = 0;	// 수신 카운터 플래그를 올릴지 말지 결정
	int readSize = 0;
	int readTotalSize = 0;
	unsigned char readBuff[MAX_BUFF_SIZE] = { -1 };

	struct timeval Time;

	int fd = user_uart_open(serial);
	user_uart_config(fd, bourbit, 8, 0, 1);

	if(access(serial, R_OK & W_OK) == 0)
	{
		//printf("Access OK!\n");
		fd = user_uart_open(serial);
		user_uart_config(fd, bourbit, 8, 0, 1);

		//start bit가 일치 할 때 까지 계속 data를 받음
		while(1)
		{
			if((readSize = user_uart_read(fd, readBuff, 19)) == -1) {
				readSize = 0;
				continue;
			}

			gettimeofday(&Time, NULL);

			for(i = 0; i < readSize; i++) {
				if(readBuff[i] == HANBACK_SPO2_PLSTN_FRONT)			flag = 1;
				else if(readBuff[i] == HANBACK_SPO2_PLSTN_MIDDLE)	flag = 2;
				else if(readBuff[i] == HANBACK_SPO2_PLSTN_MIDDLE)	flag = 3;

				if(flag == 1) {
					flag_cnt++;
					printf("%c", readBuff[i+1]);
				}
				if(flag_cnt > 3) {
					flag = 0;
					flag_cnt = 0;
					printf("\t%d\t%d\n", Time.tv_sec, Time.tv_usec);
				}

//				printf("%d\t%d\n", flag, flag_cnt);
			}

			usleep(10 * 1000);
			memset(readBuff, -1, sizeof(readBuff));
		}

		user_uart_close(fd);
	}
}

void *thread_ubpulse(void* data)
{
	char	*serial	= "/dev/ttyACM0";
	int		bourbit	= 115200;

	int i = 0;
	int flag = 0;		// I발견시 1로 바뀌고 뒤 숫자 4개 모두 출력시 0으로 바뀜
	int flag_cnt = 0;	// 수신 카운터 플래그를 올릴지 말지 결정
    int readSize = 0;
    int readTotalSize = 0;
    unsigned char readBuff[MAX_BUFF_SIZE] = { -1 };
    unsigned char readTemp[MAX_BUFF_SIZE] = { -1 };

    int H_bit = 0;
    int L_bit = 0;
    int Pulsation_Data = 0;

    struct timeval Time;

	int fd = user_uart_open(serial);
	user_uart_config(fd, bourbit, 8, 0, 1);

	if(access(serial, R_OK & W_OK) == 0)
	{
		//printf("Access OK!\n");
		fd = user_uart_open(serial);
		user_uart_config(fd, bourbit, 8, 0, 1);

		//start bit가 일치 할 때 까지 계속 data를 받음
		while(1)
		{
	        for(readTotalSize = 0; readTotalSize < 19;) {
	            if((readSize = user_uart_read(fd, readTemp, 19)) == -1) {
	                readSize = 0;
	                continue;
	            }

	            strncat_s(readBuff, readTemp, readTotalSize, readSize);

				if( readBuff[0] == LAXTHA_UBPULSE_FRONT &&
					readBuff[1] == LAXTHA_UBPULSE_REAR) {
	                readTotalSize += readSize;
	            }
	            else {
	                readSize = 0;
	                readTotalSize = 0;
	            }
	        }

			gettimeofday(&Time, NULL);

			H_bit = readBuff[7];
			L_bit = readBuff[8];
			Pulsation_Data = H_bit * 256 + L_bit;
			printf("%d\t%d\t%d\n", Pulsation_Data, Time.tv_sec, Time.tv_usec);

			usleep(10 * 1000);
			memset(readBuff, -1, sizeof(readBuff));
		}

		user_uart_close(fd);
	}
}

void strncat_s(unsigned char *target, unsigned char *buff, int target_size, int buff_size)
{
	int i, j;

	for(i = 0, j = target_size; j < MAX_BUFF_SIZE && i < buff_size; i++, j++) {
		target[j] = buff[i];
	}
}

void getElapsedTime(struct timeval Tstart, struct timeval Tend)
{
	Tend.tv_usec = Tend.tv_usec - Tstart.tv_usec;
	Tend.tv_sec  = Tend.tv_sec - Tstart.tv_sec;
	Tend.tv_usec += (Tend.tv_sec*1000000);

	printf("Elapsed Time: %lf sec\n", Tend.tv_usec / 1000000.0);
}

