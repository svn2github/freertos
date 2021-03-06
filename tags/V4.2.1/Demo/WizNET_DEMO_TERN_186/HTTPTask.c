/*
	FreeRTOS.org V4.2.1 - Copyright (C) 2003-2007 Richard Barry.

	This file is part of the FreeRTOS.org distribution.

	FreeRTOS.org is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS.org is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS.org; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS.org, without being obliged to provide
	the source code for any proprietary components.  See the licensing section 
	of http://www.FreeRTOS.org for full details of how and when the exception
	can be applied.

	***************************************************************************
	See http://www.FreeRTOS.org for documentation, latest information, license 
	and contact details.  Please ensure to read the configuration and relevant 
	port sections of the online documentation.

	Also see http://www.SafeRTOS.com for an IEC 61508 compliant version along
	with commercial development and support options.
	***************************************************************************
*/

/* 
 * Very simple task that responds with a single WEB page to http requests.
 *
 * The WEB page displays task and system status.  A semaphore is used to 
 * wake the task when there is processing to perform as determined by the 
 * interrupts generated by the Ethernet interface.
 */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Tern includes. */
#include "utils\system_common.h"
#include "i2chip_hw.h"
#include "socket.h"

/* FreeRTOS.org includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* The standard http port on which we are going to listen. */
#define httpPORT 80

#define httpTX_WAIT 2

/* Network address configuration. */
const unsigned portCHAR ucMacAddress[] =			{ 12, 128, 12, 34, 56, 78 };
const unsigned portCHAR ucGatewayAddress[] =		{ 192, 168, 2, 1 };
const unsigned portCHAR ucIPAddress[] =				{ 172, 25, 218, 210 };
const unsigned portCHAR ucSubnetMask[] =			{ 255, 255, 255, 0 };

/* The number of sockets this task is going to handle. */
#define httpSOCKET_NUM                       3
unsigned portCHAR ucConnection[ httpSOCKET_NUM ];

/* The maximum data buffer size we can handle. */
#define httpSOCKET_BUFFER_SIZE	2048

/* Standard HTTP response. */
#define httpOUTPUT_OK	"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"

/* Hard coded HTML components.  Other data is generated dynamically. */
#define HTML_OUTPUT_BEGIN "\
<HTML><head><meta http-equiv=\"Refresh\" content=\"1\;url=index.htm\"></head>\
<BODY bgcolor=\"#CCCCFF\"><font face=\"arial\"><H2>FreeRTOS.org<sup>tm</sup> + Tern E-Engine<sup>tm</sup></H2>\
<a href=\"http:\/\/www.FreeRTOS.org\">FreeRTOS.org Homepage</a><P>\
<HR>Task status table:\r\n\
<p><font face=\"courier\"><pre>Task          State  Priority  Stack	#<br>\
************************************************<br>"

#define HTML_OUTPUT_END   "\
</font></BODY></HTML>"

/*-----------------------------------------------------------*/

/*
 * Initialise the data structures used to hold the socket status.
 */
static void prvHTTPInit( void );

/*
 * Setup the Ethernet interface with the network addressing information.
 */
static void prvNetifInit( void );

/*
 * Generate the dynamic components of the served WEB page and transmit the 
 * entire page through the socket.
 */
static void prvTransmitHTTP( unsigned portCHAR socket );
/*-----------------------------------------------------------*/

/* This variable is simply incremented by the idle task hook so the number of
iterations the idle task has performed can be displayed as part of the served
page. */
unsigned portLONG ulIdleLoops = 0UL;

/* Data buffer shared by sockets. */
unsigned portCHAR ucSocketBuffer[ httpSOCKET_BUFFER_SIZE ];

/* The semaphore used by the Ethernet ISR to signal that the task should wake
and process whatever caused the interrupt. */
xSemaphoreHandle xTCPSemaphore = NULL;

/*-----------------------------------------------------------*/
void vHTTPTask( void * pvParameters )
{
portSHORT i, sLen;
unsigned portCHAR ucState;

	( void ) pvParameters;

    /* Create the semaphore used to communicate between this task and the
    WIZnet ISR. */
    vSemaphoreCreateBinary( xTCPSemaphore );

	/* Make sure everything is setup before we start. */
	prvNetifInit();
	prvHTTPInit();

	for( ;; )
	{
		/* Wait until the ISR tells us there is something to do. */
    	xSemaphoreTake( xTCPSemaphore, portMAX_DELAY );

		/* Check each socket. */
		for( i = 0; i < httpSOCKET_NUM; i++ )
		{
			ucState = select( i, SEL_CONTROL );

			switch (ucState)
			{
				case SOCK_ESTABLISHED :  /* new connection established. */

					if( ( sLen = select( i, SEL_RECV ) ) > 0 )
					{
						if( sLen > httpSOCKET_BUFFER_SIZE ) 
						{
							sLen = httpSOCKET_BUFFER_SIZE;
						}

						disable();
						
						sLen = recv( i, ucSocketBuffer, sLen );    

						if( ucConnection[ i ] == 1 )
						{	
							/* This is our first time processing a HTTP
							 request on this connection. */
							prvTransmitHTTP( i );
							ucConnection[i] = 0;
						}
						enable();
					}
					break;

				case SOCK_CLOSE_WAIT :

					close(i);
					break;

				case SOCK_CLOSED :

					ucConnection[i] = 1;
					socket( i, SOCK_STREAM, 80, 0x00 );
					NBlisten( i ); /* reinitialize socket. */
					break;
			}
		}
	}
}
/*-----------------------------------------------------------*/

static void prvHTTPInit( void )
{
unsigned portCHAR ucIndex;

	/* There are 4 total sockets available; we will claim 3 for HTTP. */
	for(ucIndex = 0; ucIndex < httpSOCKET_NUM; ucIndex++)
	{
		socket( ucIndex, SOCK_STREAM, httpPORT, 0x00 );
		NBlisten( ucIndex );
		ucConnection[ ucIndex ] = 1;
	}
}
/*-----------------------------------------------------------*/

static void prvNetifInit( void )
{
	i2chip_init();
	initW3100A();

	setMACAddr( ( unsigned portCHAR * ) ucMacAddress );
	setgateway( ( unsigned portCHAR * ) ucGatewayAddress );
	setsubmask( ( unsigned portCHAR * ) ucSubnetMask );
	setIP( ( unsigned portCHAR * ) ucIPAddress );

	/* See definition of 'sysinit' in socket.c
	 - 8 KB transmit buffer, and 8 KB receive buffer available.  These buffers
	   are shared by all 4 channels.
	 - (0x55, 0x55) configures the send and receive buffers at 
		httpSOCKET_BUFFER_SIZE bytes for each of the 4 channels. */
	sysinit( 0x55, 0x55 );
}
/*-----------------------------------------------------------*/

static void prvTransmitHTTP(unsigned portCHAR socket)
{
extern portSHORT usCheckStatus;

	/* Send the http and html headers. */
	send( socket, ( unsigned portCHAR * ) httpOUTPUT_OK, strlen( httpOUTPUT_OK ) );
	send( socket, ( unsigned portCHAR * ) HTML_OUTPUT_BEGIN, strlen( HTML_OUTPUT_BEGIN ) );

	/* Generate then send the table showing the status of each task. */
	vTaskList( ucSocketBuffer );
 	send( socket, ( unsigned portCHAR * ) ucSocketBuffer, strlen( ucSocketBuffer ) );

	/* Send the number of times the idle task has looped. */
    sprintf( ucSocketBuffer, "</pre></font><p><br>The idle task has looped 0x%08lx times<br>", ulIdleLoops );
	send( socket, ( unsigned portCHAR * ) ucSocketBuffer, strlen( ucSocketBuffer ) );

	/* Send the tick count. */
    sprintf( ucSocketBuffer, "The tick count is 0x%08lx<br>", xTaskGetTickCount() );
	send( socket, ( unsigned portCHAR * ) ucSocketBuffer, strlen( ucSocketBuffer ) );

	/* Show a message indicating whether or not the check task has discovered 
	an error in any of the standard demo tasks. */
    if( usCheckStatus == 0 )
    {
	    sprintf( ucSocketBuffer, "No errors detected." );
		send( socket, ( unsigned portCHAR * ) ucSocketBuffer, strlen( ucSocketBuffer ) );
    }
    else
    {
	    sprintf( ucSocketBuffer, "<font color=\"red\">An error has been detected in at least one task %x.</font><p>", usCheckStatus );
		send( socket, ( unsigned portCHAR * ) ucSocketBuffer, strlen( ucSocketBuffer ) );
    }

	/* Finish the page off. */
	send( socket, (unsigned portCHAR*)HTML_OUTPUT_END, strlen(HTML_OUTPUT_END));

	/* Must make sure the data is gone before closing the socket. */
	while( !tx_empty( socket ) )
    {
    	vTaskDelay( httpTX_WAIT );
    }
	close(socket);
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	ulIdleLoops++;
}




















