/*
 * mbserver : Modbus interface for temperature sensors CRYCON18, SI-9300 
 * 			  and SI Twickenham (or any similar) using Stéphane Raimbault's 
 * 			  <stephane.raimbault@gmail.com> Modbus C library libmodbus
 * 
 * Copyright © 2013 Jesus Vasquez <jesusvasquez333@gmail.com>
 * 
 * This file is part of mbserver.
 *
 * mbserver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mbserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mbserver.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Version 1.0
 * This version reads the temperature values from any of the configured 
 * instrument at a fixed, defined rate and then writes these value onto 
 * local memory that can be access from a remote Modbus/TCP client.
 * 
 * Author: 		Jesus Vasquez
 * Created on: 	Jun 04, 2013
 * Contact: 	jesusvasquez333@gmail.com
 * 
 */
 
#include <termios.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <modbus.h>
#include <signal.h>

#include "mbserver.h"
#include "errcodedef.h"

/*************************************************************/
/*********** MAIN ********************************************/
/*************************************************************/
int main(int argc, char **argv) 
{

    int socket_mb;
    int err;
	uint8_t num_servers, num_servers_tcp, num_servers_232_t1, num_servers_232_t2, server_count;
	int i;
	pthread_t *threads;
	FILE *fr = NULL;
	struct file_struct_tcp input_args_tcp[MAX_NUM_SERVERS];
	struct file_struct_232 input_args_232_t1[MAX_NUM_SERVERS];
	struct file_struct_232 input_args_232_t2[MAX_NUM_SERVERS];
    fd_set refset;
    fd_set rdset;
    int fdmax;
    int master_socket;
    int rc;
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
	
	setlinebuf(stdout);
	
    // Capture termination signals
    signal(SIGINT, intHandler);		// CRTL+C
    signal(SIGTERM, intHandler); 	// KILL
	
	// Read TCP server file 
	printf("Opening file servers TCP...\n");
	fr = fopen(SERVER_FILE_TCP, "rt");

	if (fr == NULL) 
	{
		fprintf(stderr, "Error opening %s: %s\n", SERVER_FILE_TCP, strerror(errno));
		num_servers_tcp = 0;
	}
	else
	{
		printf("Reading file servers TCP...\n");
		i=0;
		
		while(fscanf(fr, "%s %d", input_args_tcp[i].ip_addr, &input_args_tcp[i].tcp_port) != EOF) 
		{
			if ((input_args_tcp[i].tcp_port < 1) || (input_args_tcp[i].tcp_port > 65535)) 
			{
				printf("ERROR: Bad TCP port number on file.\n");
				exit(ERR_CODE_INVALID_TCP_PORT);
			}

			if (!isValidIpAddress(input_args_tcp[i].ip_addr)) 
			{
				printf("ERROR: Bad IP address on file.\n");
				exit(ERR_CODE_INVALID_IP_ADDESS);
			}


			if (++i >= MAX_NUM_SERVERS)
				break;
		}
		
		printf("Closing file servers TCP...\n");
		fclose(fr);
	
		num_servers_tcp = i;
	}
	
	printf("\n");
	
	// Read RS232 (type 1) server file 
	printf("Opening file servers RS232 type 1...\n");
	fr = fopen(SERVER_FILE_232_T1, "rt");

	if (fr == NULL) 
	{
		fprintf(stderr, "Error opening %s: %s\n", SERVER_FILE_232_T1, strerror(errno));
		num_servers_232_t1 = 0;
	}
	else
	{
		printf("Reading file servers RS232 type 1...\n");
		i=0;
		
		while(fscanf(fr, "%s", input_args_232_t1[i].serial_port) != EOF)
			if (++i >= MAX_NUM_SERVERS)
				break;
		
		printf("Closing file servers RS232 type 1...\n");
		fclose(fr);
				
		num_servers_232_t1 = i;
	}
	
	printf("\n");

	// Read RS232 (type 2) server file 
	printf("Opening file servers RS232 type 2...\n");
	fr = fopen(SERVER_FILE_232_T2, "rt");

	if (fr == NULL) 
	{
		fprintf(stderr, "Error opening %s: %s\n", SERVER_FILE_232_T2, strerror(errno));
		num_servers_232_t2 = 0;
	}
	else
	{
		printf("Reading file servers RS232 type 2...\n");
		i=0;
		
		while(fscanf(fr, "%s", input_args_232_t2[i].serial_port) != EOF)
			if (++i >= MAX_NUM_SERVERS)
				break;

		printf("Closing file servers RS232 type 2...\n");
		fclose(fr);
				
		num_servers_232_t2 = i;
	}
	
	printf("\n");
	
	// Total number of server to read from
	num_servers = num_servers_tcp + num_servers_232_t1 + num_servers_232_t2;
	
	if (!num_servers)
	{
		printf("Error: no server infomration found!\n");		
		exit(ERR_CODE_NO_SERVERS);
	}

	if (num_servers > MAX_NUM_SERVERS)
	{
		printf("Error: more that %d server defined!\n", MAX_NUM_SERVERS);
		exit(ERR_CODE_TOOMANY_SERVERS);
	}

	printf("Data read from files:\n");
	printf("Total number of data servers = %d\n\n", num_servers);

	if (num_servers_tcp)
	{
		printf("Number of TCP servers: %d\n", num_servers_tcp);
		for (i = 0; i < num_servers_tcp; i++)
			printf("\tip address = %s,\ttcp port = %d\n", input_args_tcp[i].ip_addr, input_args_tcp[i].tcp_port);
	}

	if (num_servers_232_t1)		
	{
		printf("Number of RS232 (type 1) servers: %d\n", num_servers_232_t1);
		for (i = 0; i < num_servers_232_t1; i++)
			printf("\tserial port = %s\n", input_args_232_t1[i].serial_port);
	}
	
	if (num_servers_232_t2)		
	{
		printf("Number of RS232 (type 2) servers: %d\n", num_servers_232_t2);
		for (i = 0; i < num_servers_232_t2; i++)
			printf("\tserial port = %s\n", input_args_232_t2[i].serial_port);
	}	


    if ((threads = malloc(num_servers * sizeof(*threads))) == NULL)
   	{
		perror("Fail to allocate the threads");
		exit(ERR_CODE_THR_MALLOC_FAIL);
	}

	// Allocate memory for the Modbus registers
    if ((mb_mapping = modbus_mapping_new(MB_BITS_ADR + MB_INPUT_BITS_NB, MB_INPUT_BITS_ADD + MB_INPUT_BITS_NB, MB_REGISTERS_ADD + MB_REGISTERS_NB, MB_INPUT_REGISTERS_ADD + MB_INPUT_REGISTERS_NB)) == NULL)
    {
		fprintf(stderr, "Failed to allocate the modbus mapping: %s\n",	modbus_strerror(errno));
        modbus_free(ctx);
        exit(ERR_CODE_MB_MALLOC_FAIL);
    }

	server_count = 0;
	// Launch threads for TCP servers 
	for(i=0; i<num_servers_tcp; i++) 
	{
		struct arg_struct_tcp *args_tcp;

		if((args_tcp = malloc(sizeof(*args_tcp))) == NULL)
		{
			perror("Fail to allocate the parameters of the TCP thread");
			exit(ERR_CODE_TCP_THRPAR_MALLOC_FAIL);
		}

		args_tcp->ip_addr = input_args_tcp[i].ip_addr;
		args_tcp->tcp_port = input_args_tcp[i].tcp_port;
		args_tcp->mb_offset = MB_BASE_ADDR + 16*i;
		args_tcp->mb_error_reg = MB_BASE_ADDR_ERROR;
		args_tcp->mb_allarm_reg = MB_BASE_ADDR_ALLARM + server_count;
		args_tcp->error_bit_pos = server_count + 8;
		
		SET_BIT(mb_mapping->tab_registers[MB_BASE_ADDR_ERROR], server_count);
		
		if ((err = pthread_create(threads, NULL, &scanTcp, (void *)args_tcp)) != 0)
			perror("\nCan't create thread");
		else
			printf("\n Thread created successfully\n");

		server_count++;
		sleep(5);
	}

	// Launch threads for RS232 (type 1) servers 
	for(i=0; i<num_servers_232_t1; i++) 
	{
		struct arg_struct_232 *args_232_t1;

		if((args_232_t1 = malloc(sizeof(*args_232_t1))) == NULL)
		{
			perror("Fail to allocate the parameters of the RS232 (type 1) thread");
			exit(ERR_CODE_RS1_THRPAR_MALLOC_FAIL);
		}

		args_232_t1->serial_port = input_args_232_t1[i].serial_port;
		args_232_t1->mb_offset = MB_BASE_ADDR + 16*(i + num_servers_tcp);
		args_232_t1->mb_error_reg = MB_BASE_ADDR_ERROR;
		args_232_t1->mb_allarm_reg = MB_BASE_ADDR_ALLARM + server_count;
		args_232_t1->error_bit_pos = server_count + 8;
		
		SET_BIT(mb_mapping->tab_registers[MB_BASE_ADDR_ERROR], server_count);
		
		if ((err = pthread_create(threads, NULL, &scanRS232_t1, (void *)args_232_t1)) != 0)
			perror("\nCan't create thread");
		else
			printf("\n Thread created successfully\n");

		server_count++;
		sleep(5);
	}
	
	// Launch threads for RS232 (type 2) servers 
	for(i=0; i<num_servers_232_t2; i++) 
	{
		struct arg_struct_232 *args_232_t2;

		if((args_232_t2 = malloc(sizeof(*args_232_t2))) == NULL)
		{
			perror("Fail to allocate the parameters of the RS232 (type 2) thread");
			exit(ERR_CODE_RS2_THRPAR_MALLOC_FAIL);
		}

		args_232_t2->serial_port = input_args_232_t2[i].serial_port;
		args_232_t2->mb_offset = MB_BASE_ADDR + 16*(i + num_servers_tcp + num_servers_232_t1);
		args_232_t2->mb_error_reg = MB_BASE_ADDR_ERROR;
		args_232_t2->mb_allarm_reg = MB_BASE_ADDR_ALLARM + server_count;
		args_232_t2->error_bit_pos = server_count + 8;
		
		SET_BIT(mb_mapping->tab_registers[MB_BASE_ADDR_ERROR], server_count);
		
		if ((err = pthread_create(threads, NULL, &scanRS232_t2, (void *)args_232_t2)) != 0)
			perror("\nCan't create thread");
		else
			printf("\n Thread created successfully\n");

		server_count++;
		sleep(5);
	}

	// Create a libmodbus context for TCP/IPv4
    if ((ctx = modbus_new_tcp("127.0.0.1", MB_TCP_PORT)) == NULL) 
	{
		fprintf(stderr, "Unable to allocate libmodbus context: %s\n", modbus_strerror(errno));
		exit(ERR_CODE_MB_CON_ALLOC_FAIL);
	}
	
	//  Create a TCP socket for the modbus server
	socket_mb = modbus_tcp_listen(ctx, NB_CONNECTION);

    // Clear the reference set of socket
    FD_ZERO(&refset);
    
    // Add the server socket
    FD_SET(socket_mb, &refset);

    // Keep track of the max file descriptor 
    fdmax = socket_mb;

	printf("Entering main loop...\n");
    while(1)
    {
		// Monitor all socket
        rdset = refset;
        if (select(fdmax+1, &rdset, NULL, NULL, NULL) == -1) 
        {
            perror("Server select() failure");
            exit(ERR_CODE_SELECT_FAIL);
        }

        // Run through the existing connections looking for data to be read
        for (master_socket = 0; master_socket <= fdmax; master_socket++) 
        {

            if (!FD_ISSET(master_socket, &rdset)) 
                continue;

            if (master_socket == socket_mb) 
            {
                // A client is asking a new connection 
                socklen_t addrlen;
                struct sockaddr_in clientaddr;
                int newfd;

                // Handle new connections 
                addrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, sizeof(clientaddr));
                newfd = accept(socket_mb, (struct sockaddr *)&clientaddr, &addrlen);
                if (newfd == -1)
                    perror("Server accept() error");
                else 
                {
                    FD_SET(newfd, &refset);

                    if (newfd > fdmax) // Keep track of the maximum 
                        fdmax = newfd;
                    
                    printf("New connection from %s:%d on socket %d\n",
                           inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, newfd);
                }
            } 
            else 
            {
                modbus_set_socket(ctx, master_socket);
                rc = modbus_receive(ctx, query);
                if (rc > 0)
                    modbus_reply(ctx, query, rc, mb_mapping);
                else if (rc == -1) 
                {
					// Close connection
                    printf("Connection closed on socket %d\n", master_socket);
                    close(master_socket);

                    // Remove from reference set
                    FD_CLR(master_socket, &refset);

                    if (master_socket == fdmax)
                        fdmax--;
                }
            }
        }
	}
	
    exit(NO_ERR_CODE);
}

/*************************************************************/
/*********** FUNCTIONS ***************************************/
/*************************************************************/
void intHandler(int n) 
{
    printf("Exiting [code = %d]...\n\n", n);
    modbus_free(ctx);
    modbus_mapping_free(mb_mapping);
    exit(ERR_SIGNAL_CAPTURE);
}

// Check if IP address string is corretc 
int isValidIpAddress(char *ipAddress) 
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

// TCP scan function (one thread for each instrument)
void* scanTcp(void *arguments) 
{
	
	int sockfd;
	int n;
	struct sockaddr_in servaddr;
	char sendline[BUFSIZE];
	char recvline[BUFSIZE];
	char vars[8][10];
	int var_count;
	uint16_t a[2];
	float f;
	struct arg_struct_tcp *args = arguments;
	int mb_offset, mb_error_reg, mb_allarm_reg;
	uint8_t error_bit_pos;
	struct timeval tv;

	tv.tv_sec = 5;  // 5 Secs Timeout 
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors
	
	strcpy(vars[0], "INP? A");
	strcpy(vars[1], "INP? B");
	strcpy(vars[2], "INP? C");
	strcpy(vars[3], "INP? D");
	strcpy(vars[4], "INP? E");
	strcpy(vars[5], "INP? F");
	strcpy(vars[6], "INP? G");
	strcpy(vars[7], "INP? H");

	printf("ip= %s, port= %d\n",args->ip_addr, args->tcp_port );
	mb_offset = args->mb_offset;
	mb_error_reg = args->mb_error_reg;
	mb_allarm_reg = args->mb_allarm_reg;
	error_bit_pos = args->error_bit_pos;
	
	SET_BIT(mb_mapping->tab_registers[mb_error_reg], error_bit_pos);	// Start with the communication error bit set (it will be clear once the connection is stablished)
	
	while(1) 
	{
		
		sockfd=socket(AF_INET,SOCK_STREAM,0);
		bzero(&servaddr,sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr=inet_addr(args->ip_addr);
		servaddr.sin_port=htons(args->tcp_port);

		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
		
		printf("Connecting...\n");
		while(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
			sleep(5);

		printf("Connection Succesfully.\n");
		var_count = 0;

		while(1) 
		{
			strcpy(sendline, vars[var_count]);
			n = send(sockfd, sendline, strlen(sendline), 0);

			if (n <= 0) 	// Communication error
			{
				printf("Error writting to socket [%d].\n", n);
				break;
			}
			
			bzero(recvline, BUFSIZE);
			//n = read(sockfd, recvline, BUFSIZE);
			n = recv(sockfd, recvline, sizeof(recvline), 0);
			
			if (n <= 0) 		// Communication error 
			{
				printf("Error reading from socket [%d].\n", n);
				break;
			}
			
			// Normal communication
			CLEAR_BIT(mb_mapping->tab_registers[mb_error_reg], error_bit_pos);				// Clear the communication error bit
			
			if (!strncmp(recvline, ".......", 7))			// No sensor present
			{
				mb_mapping->tab_registers[mb_offset + 2*var_count] = 0;
				mb_mapping->tab_registers[mb_offset + 2*var_count + 1] = 0;
				SET_BIT(mb_mapping->tab_registers[mb_allarm_reg], var_count);
				CLEAR_BIT(mb_mapping->tab_registers[mb_allarm_reg], (var_count + 8));
			}
			else if (!strncmp(recvline, "-------", 7))		// Sensor out of range
			{
				mb_mapping->tab_registers[mb_offset + 2*var_count] = 0;
				mb_mapping->tab_registers[mb_offset + 2*var_count + 1] = 0;
				SET_BIT(mb_mapping->tab_registers[mb_allarm_reg], (var_count + 8));
				CLEAR_BIT(mb_mapping->tab_registers[mb_allarm_reg], var_count);
			}
			else 		// Normal temperature reading
			{
				f = (float)atof(recvline);
				memcpy(a, &f, sizeof(f));
				mb_mapping->tab_registers[mb_offset + 2*var_count] = a[0];
				mb_mapping->tab_registers[mb_offset + 2*var_count + 1] = a[1];
				CLEAR_BIT(mb_mapping->tab_registers[mb_allarm_reg], var_count);
				CLEAR_BIT(mb_mapping->tab_registers[mb_allarm_reg], (var_count + 8));
			}
			if (++var_count >= 8)
				var_count = 0;
			usleep(500000);
		}
		// You get here after a communication error
		SET_BIT(mb_mapping->tab_registers[mb_error_reg], error_bit_pos);			// Set the communication error bit
		mb_mapping->tab_registers[mb_allarm_reg] = 0;								// Clear the allarm register
		memset(&mb_mapping->tab_registers[mb_offset], 0, 16 * sizeof(uint16_t));	// Clear the temperature registers
		
		printf("Closing socket...\n");
		close(sockfd);
		sleep(5);
	}
}

// RS232 scan function for instruments type 1 (S.I. 9300; 8 Chs) (one thread for each instrument)
void* scanRS232_t1(void *arguments) 
{
	int fd, res;
	struct termios oldtio,newtio;          // definition of signal action
	char buf[BUFSIZE];
	char devicename[80];
	char *token;
	char *aux_ptr;
	int i;
	struct arg_struct_232 *args = arguments;
	uint16_t a[2];
	float f[8];
	char isonline;
	int mb_offset, mb_error_reg;
	uint8_t error_bit_pos;

	strcpy(devicename, args->serial_port);
	mb_offset = args->mb_offset;
	mb_error_reg = args->mb_error_reg;
	error_bit_pos = args->error_bit_pos;

	printf("serial port= %s\n", args->serial_port);

	// open the device to be non-blocking (read will return immediatly)
	fd = open(devicename, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd <0) 
	{
		perror(devicename); 
		exit(ERR_CODE_SERIAL_PORT_OPEN_FAIL); 
	}

	tcgetattr(fd,&oldtio); // save current serial port settings 
	bzero(&newtio, sizeof(newtio)); // clear struct for new port settings 

	/* 
	  BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
	  CRTSCTS : output hardware flow control (only used if the cable has
				all necessary lines. See sect. 7 of Serial-HOWTO)
	  CS8     : 8n1 (8bit,no parity,1 stopbit)
	  CLOCAL  : local connection, no modem contol
	  CREAD   : enable receiving characters
	*/
	 newtio.c_cflag = B1200 | CS8 | CLOCAL | CREAD;
	 
	/*
	  IGNPAR  : ignore bytes with parity errors
	  ICRNL   : map CR to NL (otherwise a CR input on the other computer
				will not terminate input)
	  otherwise make device raw (no other input processing)
	*/
	newtio.c_iflag = IGNPAR;
	 
	/*
	 Raw output.
	*/
	 newtio.c_oflag = 0;
	 
	/*
	  ICANON  : enable canonical input
	  disable all echo functionality, and don't send signals to calling program
	*/
	 newtio.c_lflag = ICANON;
	 
	/* 
	  initialize all control characters 
	  default values can be found in /usr/include/termios.h, and are given
	  in the comments, but we don't need them here
	*/
	newtio.c_cc[VINTR]    = 0;		/* Ctrl-c */
	newtio.c_cc[VQUIT]    = 0;		/* Ctrl-\ */
	newtio.c_cc[VERASE]   = 0;		/* del */ 
	newtio.c_cc[VKILL]    = 0;		/* @ */ 
	newtio.c_cc[VEOF]     = 4;		/* Ctrl-d */ 
	newtio.c_cc[VTIME]    = 0;		/* inter-character timer unused */ 
	newtio.c_cc[VMIN]     = 1;		/* blocking read until 1 character arrives */ 
	newtio.c_cc[VSWTC]    = 0;		/* '\0' */ 
	newtio.c_cc[VSTART]   = 0;		/* Ctrl-q */  
	newtio.c_cc[VSTOP]    = 0;		/* Ctrl-s */ 
	newtio.c_cc[VSUSP]    = 0;		/* Ctrl-z */ 
	newtio.c_cc[VEOL]     = 0;		/* '\0' */ 
	newtio.c_cc[VREPRINT] = 0;		/* Ctrl-r */ 
	newtio.c_cc[VDISCARD] = 0;		/* Ctrl-u */ 
	newtio.c_cc[VWERASE]  = 0;		/* Ctrl-w */ 
	newtio.c_cc[VLNEXT]   = 0;		/* Ctrl-v */ 
	newtio.c_cc[VEOL2]    = 0;		/* '\0' */ 

	// Now clean the modem line and activate the settings for the port
	 tcflush(fd, TCIFLUSH);
	 tcsetattr(fd,TCSANOW,&newtio);

	 while (1) 
	{     	
		write(fd,"T",1);
		
		sleep(1);
		
		res = read(fd,buf,BUFSIZE);
		
		if (res>48)			// Normal communication
		{
			isonline = TRUE;
			CLEAR_BIT(mb_mapping->tab_registers[mb_error_reg], error_bit_pos);	// Clear the communication error bit
			aux_ptr = buf;
			for (i=0; i<8; i++)
			{
				token = strsep(&aux_ptr, ",");
				if (!token)
					break;
					
				f[i] = (float)atof(token);
				f[i] = (float)f[i]/100.0;
			}
			
			if (i==8)
			{
				for (i=0; i<8; i++)		// Copy the temperatures readings
				{
					memcpy(a, &f[i], sizeof(f[i]));
					mb_mapping->tab_registers[mb_offset + 2*i] = a[0];
					mb_mapping->tab_registers[mb_offset + 2*i + 1] = a[1];
				}
			}
		}
		else 			// Communication error
		{
			if (isonline)
			{
				isonline = FALSE;
				memset(&mb_mapping->tab_registers[mb_offset], 0, 16 * sizeof(uint16_t));	// Clear the temperature registers
			}
			SET_BIT(mb_mapping->tab_registers[mb_error_reg], error_bit_pos);				// Set the communication error bit
		}

	 }
	 
	 // restore the old port settings 
	 tcsetattr(fd,TCSANOW,&oldtio);
}

// RS232 scan function for instruments type 2 (S.I. ?, CRYOGENIC; 6 Chs) (one thread for each instrument)
void* scanRS232_t2(void *arguments) 
{
	int fd, res;
	struct termios oldtio,newtio;          // definition of signal action
	char buf[BUFSIZE];
	char devicename[80];
	char *token;
	char *aux_ptr;
	int i;
	struct arg_struct_232 *args = arguments;
	uint16_t a[2];
	float f[6];
	char isonline;
	int mb_offset, mb_error_reg, mb_allarm_reg;
	uint8_t error_bit_pos;
	uint8_t sensors_outrange, sensors_nopresent = NULL;

	strcpy(devicename, args->serial_port);
	mb_offset = args->mb_offset;
	mb_error_reg = args->mb_error_reg;
	mb_allarm_reg = args->mb_allarm_reg;
	error_bit_pos = args->error_bit_pos;

	printf("serial port= %s\n", args->serial_port);

	// open the device to be non-blocking (read will return immediatly)
	fd = open(devicename, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd <0) 
	{
		perror(devicename); 
		exit(ERR_CODE_SERIAL_PORT_OPEN_FAIL); 
	}

	tcgetattr(fd,&oldtio); 				// save current serial port settings
	bzero(&newtio, sizeof(newtio));		// clear struct for new port settings
	
	/* 
	  BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
	  CRTSCTS : output hardware flow control (only used if the cable has
				all necessary lines. See sect. 7 of Serial-HOWTO)
	  CS8     : 8n1 (8bit,no parity,1 stopbit)
	  CLOCAL  : local connection, no modem contol
	  CREAD   : enable receiving characters
	  CSTOPV  : 2 stop bits
	*/
	newtio.c_cflag = B9600 | CS8 | CSTOPB | CLOCAL | CREAD;
	 
	/*
	  IXON	  : Enable software flow control (outgoing)
	  IXOFF	  : Enable software flow control (incoming)
	  IGNPAR  : ignore bytes with parity errors
	  ICRNL   : map CR to NL (otherwise a CR input on the other computer
				will not terminate input)
	  otherwise make device raw (no other input processing)
	*/
	newtio.c_iflag = IGNPAR;
	 
	/*
	 Raw output.
	*/
	newtio.c_oflag = 0;
	 
	/*
	  ICANON  : enable canonical input
	  disable all echo functionality, and don't send signals to calling program
	*/
	 newtio.c_lflag = ICANON;
	 
	/* 
	  initialize all control characters 
	  default values can be found in /usr/include/termios.h, and are given
	  in the comments, but we don't need them here
	*/
	newtio.c_cc[VINTR]    = 0;		/* Ctrl-c */  
	newtio.c_cc[VQUIT]    = 0;		/* Ctrl-\ */ 
	newtio.c_cc[VERASE]   = 0;		/* del */ 
	newtio.c_cc[VKILL]    = 0;		/* @ */ 
	newtio.c_cc[VEOF]     = 4;		/* Ctrl-d */ 
	newtio.c_cc[VTIME]    = 0;		/* inter-character timer unused */ 
	newtio.c_cc[VMIN]     = 1;		/* blocking read until 1 character arrives */ 
	newtio.c_cc[VSWTC]    = 0;		/* '\0' */ 
	newtio.c_cc[VSTART]   = 0;		/* Ctrl-q */  
	newtio.c_cc[VSTOP]    = 0;		/* Ctrl-s */ 
	newtio.c_cc[VSUSP]    = 0;		/* Ctrl-z */ 
	newtio.c_cc[VEOL]     = 0;		/* '\0' */ 
	newtio.c_cc[VREPRINT] = 0;		/* Ctrl-r */ 
	newtio.c_cc[VDISCARD] = 0;		/* Ctrl-u */ 
	newtio.c_cc[VWERASE]  = 0;		/* Ctrl-w */ 
	newtio.c_cc[VLNEXT]   = 0;		/* Ctrl-v */ 
	newtio.c_cc[VEOL2]    = 0;		/* '\0' */ 
	 
	//  now clean the modem line and activate the settings for the port
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);

	// This instrument has not last 2 sensors, so set the last two "no sensor present" bits in the allarm word
	SET_BIT(sensors_nopresent, 6);
	SET_BIT(sensors_nopresent, 7);
					
	 while (1) 
	{
		
		write(fd,"R\r\n",3);

		usleep(500000);

		res = read(fd,buf,BUFSIZE);

		if ((res>19) & ((buf[0] >= '0') & (buf[0] <= '9')))		// Normal communication
		{
			isonline = TRUE;
			CLEAR_BIT(mb_mapping->tab_registers[mb_error_reg], error_bit_pos);	// Clear the communicaction error bit
			aux_ptr = &buf[18];

			for (i=0; i<6; i++)
			{
				token = strsep(&aux_ptr, ",");
				if (!token)
					break;

				if (!strncmp(token, " OPEN", 5))		// No sensor present
				{
					SET_BIT(sensors_nopresent, i);
					CLEAR_BIT(sensors_outrange, i);
					f[i] = 0;
				}
				else if ((!strncmp(token, " ERR_", 5)) | (!strncmp(token, " OVRNG", 6)))	// Sensor out of range
				{
					SET_BIT(sensors_outrange, i);
					CLEAR_BIT(sensors_nopresent, i);
					f[i] = 0;
				}
				else 	// Normal temperature reading
				{
					CLEAR_BIT(sensors_outrange, i);
					CLEAR_BIT(sensors_nopresent, i);
					f[i] = (float)atof(token);
				}
			}
			
			if (i==6)
			{
				for (i=0; i<6; i++)		// Copy the temperatures readings
				{
					memcpy(a, &f[i], sizeof(f[i]));
					mb_mapping->tab_registers[mb_offset + 2*i] = a[0];
					mb_mapping->tab_registers[mb_offset + 2*i + 1] = a[1];
				}
				mb_mapping->tab_registers[mb_allarm_reg] = ((((uint16_t)sensors_outrange) << 8) | (sensors_nopresent));		// Copy the allarm word
			}
		}
		else 	// Communication error
		{
			if (isonline)
			{
				isonline = FALSE;
				mb_mapping->tab_registers[mb_allarm_reg] = 0;								// Clear the allarm register
				memset(&mb_mapping->tab_registers[mb_offset], 0, 16 * sizeof(uint16_t));	// Clear the temperatures registers
			}
			SET_BIT(mb_mapping->tab_registers[mb_error_reg], error_bit_pos);				// Set the communicaction error bit
		}

	 }
	 
	 // restore the old port settings
	 tcsetattr(fd,TCSANOW,&oldtio);
}
