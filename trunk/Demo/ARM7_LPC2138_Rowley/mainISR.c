/*
	FreeRTOS.org V5.4.0 - Copyright (C) 2003-2009 Richard Barry.

	This file is part of the FreeRTOS.org distribution.

	FreeRTOS.org is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License (version 2) as published
	by the Free Software Foundation and modified by the FreeRTOS exception.
	**NOTE** The exception to the GPL is included to allow you to distribute a
	combined work that includes FreeRTOS.org without being obliged to provide
	the source code for any proprietary components.  Alternative commercial
	license and support terms are also available upon request.  See the 
	licensing section of http://www.FreeRTOS.org for full details.

	FreeRTOS.org is distributed in the hope that it will be useful,	but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
	FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
	more details.

	You should have received a copy of the GNU General Public License along
	with FreeRTOS.org; if not, write to the Free Software Foundation, Inc., 59
	Temple Place, Suite 330, Boston, MA  02111-1307  USA.


	***************************************************************************
	*                                                                         *
	* Get the FreeRTOS eBook!  See http://www.FreeRTOS.org/Documentation      *
	*                                                                         *
	* This is a concise, step by step, 'hands on' guide that describes both   *
	* general multitasking concepts and FreeRTOS specifics. It presents and   *
	* explains numerous examples that are written using the FreeRTOS API.     *
	* Full source code for all the examples is provided in an accompanying    *
	* .zip file.                                                              *
	*                                                                         *
	***************************************************************************

	1 tab == 4 spaces!

	Please ensure to read the configuration and relevant port sections of the
	online documentation.

	http://www.FreeRTOS.org - Documentation, latest information, license and
	contact details.

	http://www.SafeRTOS.com - A version that is certified for use in safety
	critical systems.

	http://www.OpenRTOS.com - Commercial support, development, porting,
	licensing and training services.
*/
#include "FreeRTOS.h"
#include "semphr.h"

#define isrCLEAR_EINT_1 2

/*
 * Interrupt routine that simply wakes vButtonHandlerTask on each interrupt 
 * generated by a push of the built in button.  The wrapper takes care of
 * the ISR entry.  This then calls the actual handler function to perform
 * the work.  This work should not be done in the wrapper itself unless
 * you are absolutely sure that no stack space is used.
 */
void vButtonISRWrapper( void ) __attribute__ ((naked));
void vButtonHandler( void );

void vButtonHandler( void )
{
extern xSemaphoreHandle xButtonSemaphore;
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	xSemaphoreGiveFromISR( xButtonSemaphore, &xHigherPriorityTaskWoken );

	if( xHigherPriorityTaskWoken )
	{
		/* We have woken a task.  Calling "yield from ISR" here will ensure
		the interrupt returns to the woken task if it has a priority higher
		than the interrupted task. */
		portYIELD_FROM_ISR();
	}

    EXTINT = isrCLEAR_EINT_1;
    VICVectAddr = 0;
}
/*-----------------------------------------------------------*/

void vButtonISRWrapper( void )
{
	/* Save the context of the interrupted task. */
	portSAVE_CONTEXT();

	/* Call the handler to do the work.  This must be a separate function to
	the wrapper to ensure the correct stack frame is set up. */
	vButtonHandler();

	/* Restore the context of whichever task is going to run once the interrupt
	completes. */
	portRESTORE_CONTEXT();
}



