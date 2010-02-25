/*
    FreeRTOS V6.0.3 - Copyright (C) 2010 Real Time Engineers Ltd.

    ***************************************************************************
    *                                                                         *
    * If you are:                                                             *
    *                                                                         *
    *    + New to FreeRTOS,                                                   *
    *    + Wanting to learn FreeRTOS or multitasking in general quickly       *
    *    + Looking for basic training,                                        *
    *    + Wanting to improve your FreeRTOS skills and productivity           *
    *                                                                         *
    * then take a look at the FreeRTOS eBook                                  *
    *                                                                         *
    *        "Using the FreeRTOS Real Time Kernel - a Practical Guide"        *
    *                  http://www.FreeRTOS.org/Documentation                  *
    *                                                                         *
    * A pdf reference manual is also available.  Both are usually delivered   *
    * to your inbox within 20 minutes to two hours when purchased between 8am *
    * and 8pm GMT (although please allow up to 24 hours in case of            *
    * exceptional circumstances).  Thank you for your support!                *
    *                                                                         *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public 
    License and the FreeRTOS license exception along with FreeRTOS; if not it 
    can be viewed here: http://www.freertos.org/a00114.html and also obtained 
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

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
void vButtonHandler( void ) __attribute__ ((noinline));

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
	__asm volatile( "bl vButtonHandler" );

	/* Restore the context of whichever task is going to run once the interrupt
	completes. */
	portRESTORE_CONTEXT();
}



