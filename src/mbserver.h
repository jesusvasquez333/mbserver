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

/*************************************************************/
/*********** GENERAL CONFIGURATION ***************************/
/*************************************************************/
#define SERVER_FILE_TCP		"/home/pi/servers_tcp.txt"	// File where the instruments directions are
#define SERVER_FILE_232_T1	"/home/pi/servers_232_t1.txt"	// File where the instruments directions are
#define SERVER_FILE_232_T2	"/home/pi/servers_232_t2.txt"	// File where the instruments directions are
#define MAX_NUM_SERVERS 	6				// Max number of data servers (istruments) 

/*************************************************************/
/*********** GENERAL DEFINITIONS *****************************/
/*************************************************************/
#define FALSE 		0
#define TRUE 		1
#define BUFSIZE 	100

/*************************************************************/
/*********** SERIAL PORT CONFIGURATION ***********************/
/*************************************************************/
#define _POSIX_SOURCE 		1 		// POSIX compliant source

/*************************************************************/
/*********** MODBUS SERVER CONFIGURATION *********************/
/*************************************************************/
#define NB_CONNECTION			5	// Max number of MODBUS client connections
#define MB_BASE_ADDR			0	// Strating ModBus address for the temperature readings
#define MB_BASE_ADDR_ERROR		96	// Strating ModBus address for the communication error register
#define MB_BASE_ADDR_ALLARM		98	// Strating ModBus address for the sensor error register
#define MB_TCP_PORT			502	// Standard TCP port for MODBUS server
#define MB_BITS_NB			0	// Number of "Coil" modbus register to allocate
#define MB_BITS_ADR			0	// Start address for "Coil" modbus register to allocate
#define MB_INPUT_BITS_NB		0	// Number of "Discrete Inputs" modbus register to allocate
#define MB_INPUT_BITS_AD		0	// Start address for "Discrete Inputs" modbus to allocate
#define MB_REGISTERS_NB			150	// Number of "Holding Registers" modbus register to allocate
#define MB_REGISTERS_ADD		0	// Start address for "Holding Registers" modbus register to allocate
#define MB_INPUT_REGISTERS_NB		0	// Number of "Input Register" modbus register to allocate
#define MB_INPUT_REGISTERS_ADD		0	// Start address for "Input Register" modbus register to allocate
/*************************************************************/
/*********** GLOBAL VARIABLES ********************************/
/*************************************************************/
modbus_t *ctx = NULL;
//int server_socket;
modbus_mapping_t *mb_mapping;

/*************************************************************/
/*********** DATA STRUCTURES *********************************/
/*************************************************************/
struct file_struct_tcp 
{
	char	ip_addr[20];
	int		tcp_port;
};

struct arg_struct_tcp 
{
	char*   ip_addr;
    int    	tcp_port;
    int    	mb_offset;
    int		mb_allarm_reg;
    int		mb_error_reg;
    uint8_t	error_bit_pos;
};

struct file_struct_232 
{
	char	serial_port[20];
};

struct arg_struct_232 
{
	char*	serial_port;
	int		mb_offset;
    int		mb_allarm_reg;
    int		mb_error_reg;
    uint8_t	error_bit_pos;
};


/*************************************************************/
/******************* MACROS **********************************/
/*************************************************************/
#define	SET_BIT(word,bit_pos)		word |= (1 << bit_pos);
#define	CLEAR_BIT(word,bit_pos)		word &= (~(1 << bit_pos));

/*************************************************************/
/*********** FUNCTIONS ***************************************/
/*************************************************************/
void intHandler(int n);
int isValidIpAddress(char *ipAddress);
void* scanTcp(void *arguments);
void* scanRS232_t1(void *arguments);
void* scanRS232_t2(void *arguments);
