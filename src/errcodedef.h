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

/*************************************************************/
/*********** OUTPUT ERROR CODE DEFINITIONS *******************/
/*************************************************************/

#define	NO_ERR_CODE						0		// No error

#define	ERR_CODE_INVALID_TCP_PORT		-1		// Invalid TCP port on configuration file
#define	ERR_CODE_INVALID_IP_ADDESS		-2		// Invalid IP Addess on configuration file
#define ERR_CODE_NO_SERVERS				-3		// No server configured
#define ERR_CODE_TOOMANY_SERVERS		-4		// Too many server on configuration files
#define ERR_CODE_THR_MALLOC_FAIL		-5		// Thread allocation fail
#define ERR_CODE_MB_MALLOC_FAIL			-6		// Modbus memory mapping allocation fail
#define ERR_CODE_TCP_THRPAR_MALLOC_FAIL	-7		// TCP thread parameter allocation fail
#define ERR_CODE_RS1_THRPAR_MALLOC_FAIL	-8		// RS232 (type 1) thread parameter allocation fail
#define ERR_CODE_RS2_THRPAR_MALLOC_FAIL	-9		// RS232 (type 2) thread parameter allocation fail
#define ERR_CODE_MB_CON_ALLOC_FAIL		-10		// Libmodbus context allocation fail
#define ERR_CODE_SELECT_FAIL			-11		// Select() function fail
#define	ERR_CODE_SERIAL_PORT_OPEN_FAIL	-12
#define	ERR_CODE_
#define	ERR_CODE_

#define	ERR_SIGNAL_CAPTURE				-20		// End signal captured
