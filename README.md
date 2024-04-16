This README contains all the edited files of this project (main.c isr_eoc_getValue.c and isr_averageData.c)
I added the account kimmosa as a collaborator of this project so you should be able to edit it.
```
/******************************************************************************
* File Name: main.c
*
* Version: 1.11
*
* Description: This is the source code for the ADC and UART code example.
*
* Related Document: CE195277_ADC_and_UART.pdf
*
* Hardware Dependency: See CE195277_ADC_and_UART.pdf
*
* Note:
* EDITED BY: Andreas BOTH
* DATE: 2024-04-16
* COURSE ID:tx00db04
* DESCRIPTION:  changed the output, such that the ADC temperature value and corresponding
*               temperature in Celsius coming from a LM35DZ temperature
*               sensor is transmitted via UART as JSON data script.
*
*******************************************************************************
* Copyright (2018-2020), Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* (“Software”), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries (“Cypress”) and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software (“EULA”).
*
* If no EULA applies, Cypress hereby grants you a personal, nonexclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress’s integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, 
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress 
* reserves the right to make changes to the Software without notice. Cypress 
* does not assume any liability arising out of the application or use of the 
* Software or any product or circuit described in the Software. Cypress does 
* not authorize its products for use in any products where a malfunction or 
* failure of the Cypress product may reasonably be expected to result in 
* significant property damage, injury or death (“High Risk Product”). By 
* including Cypress’s product in a High Risk Product, the manufacturer of such 
* system or application assumes all risk of such use and in doing so agrees to 
* indemnify Cypress against all liability.
*******************************************************************************/

#include <project.h>
#include "stdio.h"

/* Project Defines */
#define FALSE  0
#define TRUE   1
#define TRANSMIT_BUFFER_SIZE  50


/*Global Variables*/
/*used to store the sum of the input over the duration of one second
64 Bit value that way you can add 16 Bit into it about 2^48 times*/
volatile uint64 gInputSum = 0;

volatile uint32 gAverageData = 0;
volatile uint8 gSendAverage = 0; //Flag set every second

/*******************************************************************************
* Function Name: main
********************************************************************************
*
*
*******************************************************************************/
int main()
{
    /* Variable to store ADC result */
    uint16 temperatureIntPart;      //stores the integer value of the temperature
    uint16 temperatureDecimalPart;  //stores the decimal part of the temperature
   
    
    /* Transmit Buffer */
    char TransmitBuffer[TRANSMIT_BUFFER_SIZE];
    
    /* Start the components */
    ADC_DelSig_1_Start();
    UART_1_Start();
    
    isr_eoc_getValue_ClearPending();
    isr_eoc_getValue_Start();
    
    isr_averageData_ClearPending();
    isr_averageData_Start();
    
    CyGlobalIntEnable
    /* Initialize Variables */
    gSendAverage = FALSE;
    
    /* Start the ADC conversion */
    ADC_DelSig_1_StartConvert();
    
    /* Send message to verify COM port is connected properly */
    UART_1_PutString("COM Port Open\r\n");
    
    
    for(;;)
    {
            if(gSendAverage)
            {
             /* Format ADC result for transmition */
             CyGlobalIntDisable
             temperatureIntPart     = gAverageData / 10;    //drops the decimal part of the temperature
             temperatureDecimalPart = gAverageData % 10;    //drops the integer part of the temperature
             sprintf(TransmitBuffer, "{\n\"Voltage [mV]\" : %lu,\r\n \"Temperature [C]\" : %hu.%hu \r\n}\r\n", gAverageData, temperatureIntPart,temperatureDecimalPart);
             CyGlobalIntEnable
             /* Send out the data */
             UART_1_PutString(TransmitBuffer);
             /* Reset the sendAverage flag */
           
             gSendAverage = FALSE;
        }        
    }
}


/* [] END OF FILE */


/*******************************************************************************
* File Name: isr_eoc_getValue.c  
* Version 1.70
*
*  Description:
*   API for controlling the state of an interrupt.
*
*  Note:
* EDITED BY: Andreas BOTH
* DATE: 2024-04-16
* COURSE ID:tx00db04
* DESCRIPTION:  ISR is triggered by the eoc output of the ADC. This ISR sums up the single Values 
*               from the ADC (Samplerate: 10kHz)
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/


#include <cydevice_trm.h>
#include <CyLib.h>
#include <isr_eoc_getValue.h>
#include "cyapicallbacks.h"

#if !defined(isr_eoc_getValue__REMOVED) /* Check for removal by optimization */

/*******************************************************************************
*  Place your includes, defines and code here 
********************************************************************************/
/* `#START isr_eoc_getValue_intc` */
    #include "project.h"
    
    
    extern volatile uint64 gInputSum;
    
/* `#END` */

#ifndef CYINT_IRQ_BASE
#define CYINT_IRQ_BASE      16
#endif /* CYINT_IRQ_BASE */
#ifndef CYINT_VECT_TABLE
#define CYINT_VECT_TABLE    ((cyisraddress **) CYREG_NVIC_VECT_OFFSET)
#endif /* CYINT_VECT_TABLE */

/* Declared in startup, used to set unused interrupts to. */
CY_ISR_PROTO(IntDefaultHandler);


/*******************************************************************************
* Function Name: isr_eoc_getValue_Start
********************************************************************************
*
* Summary:
*  Set up the interrupt and enable it. This function disables the interrupt, 
*  sets the default interrupt vector, sets the priority from the value in the
*  Design Wide Resources Interrupt Editor, then enables the interrupt to the 
*  interrupt controller.
*
* Parameters:  
*   None
*
* Return:
*   None
*
*******************************************************************************/
void isr_eoc_getValue_Start(void)
{
    /* For all we know the interrupt is active. */
    isr_eoc_getValue_Disable();

    /* Set the ISR to point to the isr_eoc_getValue Interrupt. */
    isr_eoc_getValue_SetVector(&isr_eoc_getValue_Interrupt);

    /* Set the priority. */
    isr_eoc_getValue_SetPriority((uint8)isr_eoc_getValue_INTC_PRIOR_NUMBER);

    /* Enable it. */
    isr_eoc_getValue_Enable();
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_StartEx
********************************************************************************
*
* Summary:
*  Sets up the interrupt and enables it. This function disables the interrupt,
*  sets the interrupt vector based on the address passed in, sets the priority 
*  from the value in the Design Wide Resources Interrupt Editor, then enables 
*  the interrupt to the interrupt controller.
*  
*  When defining ISR functions, the CY_ISR and CY_ISR_PROTO macros should be 
*  used to provide consistent definition across compilers:
*  
*  Function definition example:
*   CY_ISR(MyISR)
*   {
*   }
*   Function prototype example:
*   CY_ISR_PROTO(MyISR);
*
* Parameters:  
*   address: Address of the ISR to set in the interrupt vector table.
*
* Return:
*   None
*
*******************************************************************************/
void isr_eoc_getValue_StartEx(cyisraddress address)
{
    /* For all we know the interrupt is active. */
    isr_eoc_getValue_Disable();

    /* Set the ISR to point to the isr_eoc_getValue Interrupt. */
    isr_eoc_getValue_SetVector(address);

    /* Set the priority. */
    isr_eoc_getValue_SetPriority((uint8)isr_eoc_getValue_INTC_PRIOR_NUMBER);

    /* Enable it. */
    isr_eoc_getValue_Enable();
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_Stop
********************************************************************************
*
* Summary:
*   Disables and removes the interrupt.
*
* Parameters:  
*   None
*
* Return:
*   None
*
*******************************************************************************/
void isr_eoc_getValue_Stop(void)
{
    /* Disable this interrupt. */
    isr_eoc_getValue_Disable();

    /* Set the ISR to point to the passive one. */
    isr_eoc_getValue_SetVector(&IntDefaultHandler);
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_Interrupt
********************************************************************************
*
* Summary:
*   The default Interrupt Service Routine for isr_eoc_getValue.
*
*   Add custom code between the coments to keep the next version of this file
*   from over writting your code.
*
* Parameters:  
*
* Return:
*   None
*
*******************************************************************************/
CY_ISR(isr_eoc_getValue_Interrupt)
{
    #ifdef isr_eoc_getValue_INTERRUPT_INTERRUPT_CALLBACK
        isr_eoc_getValue_Interrupt_InterruptCallback();
    #endif /* isr_eoc_getValue_INTERRUPT_INTERRUPT_CALLBACK */ 

    /*  Place your Interrupt code here. */
    /* `#START isr_eoc_getValue_Interrupt` */
    
        CyGlobalIntDisable
        gInputSum += ADC_DelSig_1_CountsTo_mVolts(ADC_DelSig_1_GetResult16()); // sum up each input
        CyGlobalIntEnable
    /* `#END` */
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_SetVector
********************************************************************************
*
* Summary:
*   Change the ISR vector for the Interrupt. Note calling isr_eoc_getValue_Start
*   will override any effect this method would have had. To set the vector 
*   before the component has been started use isr_eoc_getValue_StartEx instead.
* 
*   When defining ISR functions, the CY_ISR and CY_ISR_PROTO macros should be 
*   used to provide consistent definition across compilers:
*
*   Function definition example:
*   CY_ISR(MyISR)
*   {
*   }
*
*   Function prototype example:
*     CY_ISR_PROTO(MyISR);
*
* Parameters:
*   address: Address of the ISR to set in the interrupt vector table.
*
* Return:
*   None
*
*******************************************************************************/
void isr_eoc_getValue_SetVector(cyisraddress address)
{
    cyisraddress * ramVectorTable;

    ramVectorTable = (cyisraddress *) *CYINT_VECT_TABLE;

    ramVectorTable[CYINT_IRQ_BASE + (uint32)isr_eoc_getValue__INTC_NUMBER] = address;
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_GetVector
********************************************************************************
*
* Summary:
*   Gets the "address" of the current ISR vector for the Interrupt.
*
* Parameters:
*   None
*
* Return:
*   Address of the ISR in the interrupt vector table.
*
*******************************************************************************/
cyisraddress isr_eoc_getValue_GetVector(void)
{
    cyisraddress * ramVectorTable;

    ramVectorTable = (cyisraddress *) *CYINT_VECT_TABLE;

    return ramVectorTable[CYINT_IRQ_BASE + (uint32)isr_eoc_getValue__INTC_NUMBER];
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_SetPriority
********************************************************************************
*
* Summary:
*   Sets the Priority of the Interrupt. 
*
*   Note calling isr_eoc_getValue_Start or isr_eoc_getValue_StartEx will 
*   override any effect this API would have had. This API should only be called
*   after isr_eoc_getValue_Start or isr_eoc_getValue_StartEx has been called. 
*   To set the initial priority for the component, use the Design-Wide Resources
*   Interrupt Editor.
*
*   Note This API has no effect on Non-maskable interrupt NMI).
*
* Parameters:
*   priority: Priority of the interrupt, 0 being the highest priority
*             PSoC 3 and PSoC 5LP: Priority is from 0 to 7.
*             PSoC 4: Priority is from 0 to 3.
*
* Return:
*   None
*
*******************************************************************************/
void isr_eoc_getValue_SetPriority(uint8 priority)
{
    *isr_eoc_getValue_INTC_PRIOR = priority << 5;
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_GetPriority
********************************************************************************
*
* Summary:
*   Gets the Priority of the Interrupt.
*
* Parameters:
*   None
*
* Return:
*   Priority of the interrupt, 0 being the highest priority
*    PSoC 3 and PSoC 5LP: Priority is from 0 to 7.
*    PSoC 4: Priority is from 0 to 3.
*
*******************************************************************************/
uint8 isr_eoc_getValue_GetPriority(void)
{
    uint8 priority;


    priority = *isr_eoc_getValue_INTC_PRIOR >> 5;

    return priority;
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_Enable
********************************************************************************
*
* Summary:
*   Enables the interrupt to the interrupt controller. Do not call this function
*   unless ISR_Start() has been called or the functionality of the ISR_Start() 
*   function, which sets the vector and the priority, has been called.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void isr_eoc_getValue_Enable(void)
{
    /* Enable the general interrupt. */
    *isr_eoc_getValue_INTC_SET_EN = isr_eoc_getValue__INTC_MASK;
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_GetState
********************************************************************************
*
* Summary:
*   Gets the state (enabled, disabled) of the Interrupt.
*
* Parameters:
*   None
*
* Return:
*   1 if enabled, 0 if disabled.
*
*******************************************************************************/
uint8 isr_eoc_getValue_GetState(void)
{
    /* Get the state of the general interrupt. */
    return ((*isr_eoc_getValue_INTC_SET_EN & (uint32)isr_eoc_getValue__INTC_MASK) != 0u) ? 1u:0u;
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_Disable
********************************************************************************
*
* Summary:
*   Disables the Interrupt in the interrupt controller.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void isr_eoc_getValue_Disable(void)
{
    /* Disable the general interrupt. */
    *isr_eoc_getValue_INTC_CLR_EN = isr_eoc_getValue__INTC_MASK;
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_SetPending
********************************************************************************
*
* Summary:
*   Causes the Interrupt to enter the pending state, a software method of
*   generating the interrupt.
*
* Parameters:
*   None
*
* Return:
*   None
*
* Side Effects:
*   If interrupts are enabled and the interrupt is set up properly, the ISR is
*   entered (depending on the priority of this interrupt and other pending 
*   interrupts).
*
*******************************************************************************/
void isr_eoc_getValue_SetPending(void)
{
    *isr_eoc_getValue_INTC_SET_PD = isr_eoc_getValue__INTC_MASK;
}


/*******************************************************************************
* Function Name: isr_eoc_getValue_ClearPending
********************************************************************************
*
* Summary:
*   Clears a pending interrupt in the interrupt controller.
*
*   Note Some interrupt sources are clear-on-read and require the block 
*   interrupt/status register to be read/cleared with the appropriate block API 
*   (GPIO, UART, and so on). Otherwise the ISR will continue to remain in 
*   pending state even though the interrupt itself is cleared using this API.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void isr_eoc_getValue_ClearPending(void)
{
    *isr_eoc_getValue_INTC_CLR_PD = isr_eoc_getValue__INTC_MASK;
}

#endif /* End check for removal by optimization */


/* [] END OF FILE */


/*******************************************************************************
* File Name: isr_averageData.c  
* Version 1.70
*
*  Description:
*   API for controlling the state of an interrupt.
*
*
*  Note:
* EDITED BY: Andreas BOTH
* DATE: 2024-04-16
* COURSE ID:tx00db04
* DESCRIPTION:  ISR is triggered by Clock_1 (1Hz)
*               Calcuates the Average Value over the last second by divdiging
*               the sum by the sample rate (10_000)
*               Resets the sum and sets the flag for transmitting the data
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/


#include <cydevice_trm.h>
#include <CyLib.h>
#include <isr_averageData.h>
#include "cyapicallbacks.h"

#if !defined(isr_averageData__REMOVED) /* Check for removal by optimization */

/*******************************************************************************
*  Place your includes, defines and code here 
********************************************************************************/
/* `#START isr_averageData_intc` */
    #include "project.h"
    
    #define TRUE  1
    #define FALSE 0
    #define SAMPLE_RATE 10000
    
    extern volatile uint64 gInputSum;
    extern volatile uint32 gAverageData;
    extern volatile uint8 gSendAverage;
/* `#END` */

#ifndef CYINT_IRQ_BASE
#define CYINT_IRQ_BASE      16
#endif /* CYINT_IRQ_BASE */
#ifndef CYINT_VECT_TABLE
#define CYINT_VECT_TABLE    ((cyisraddress **) CYREG_NVIC_VECT_OFFSET)
#endif /* CYINT_VECT_TABLE */

/* Declared in startup, used to set unused interrupts to. */
CY_ISR_PROTO(IntDefaultHandler);


/*******************************************************************************
* Function Name: isr_averageData_Start
********************************************************************************
*
* Summary:
*  Set up the interrupt and enable it. This function disables the interrupt, 
*  sets the default interrupt vector, sets the priority from the value in the
*  Design Wide Resources Interrupt Editor, then enables the interrupt to the 
*  interrupt controller.
*
* Parameters:  
*   None
*
* Return:
*   None
*
*******************************************************************************/
void isr_averageData_Start(void)
{
    /* For all we know the interrupt is active. */
    isr_averageData_Disable();

    /* Set the ISR to point to the isr_averageData Interrupt. */
    isr_averageData_SetVector(&isr_averageData_Interrupt);

    /* Set the priority. */
    isr_averageData_SetPriority((uint8)isr_averageData_INTC_PRIOR_NUMBER);

    /* Enable it. */
    isr_averageData_Enable();
}


/*******************************************************************************
* Function Name: isr_averageData_StartEx
********************************************************************************
*
* Summary:
*  Sets up the interrupt and enables it. This function disables the interrupt,
*  sets the interrupt vector based on the address passed in, sets the priority 
*  from the value in the Design Wide Resources Interrupt Editor, then enables 
*  the interrupt to the interrupt controller.
*  
*  When defining ISR functions, the CY_ISR and CY_ISR_PROTO macros should be 
*  used to provide consistent definition across compilers:
*  
*  Function definition example:
*   CY_ISR(MyISR)
*   {
*   }
*   Function prototype example:
*   CY_ISR_PROTO(MyISR);
*
* Parameters:  
*   address: Address of the ISR to set in the interrupt vector table.
*
* Return:
*   None
*
*******************************************************************************/
void isr_averageData_StartEx(cyisraddress address)
{
    /* For all we know the interrupt is active. */
    isr_averageData_Disable();

    /* Set the ISR to point to the isr_averageData Interrupt. */
    isr_averageData_SetVector(address);

    /* Set the priority. */
    isr_averageData_SetPriority((uint8)isr_averageData_INTC_PRIOR_NUMBER);

    /* Enable it. */
    isr_averageData_Enable();
}


/*******************************************************************************
* Function Name: isr_averageData_Stop
********************************************************************************
*
* Summary:
*   Disables and removes the interrupt.
*
* Parameters:  
*   None
*
* Return:
*   None
*
*******************************************************************************/
void isr_averageData_Stop(void)
{
    /* Disable this interrupt. */
    isr_averageData_Disable();

    /* Set the ISR to point to the passive one. */
    isr_averageData_SetVector(&IntDefaultHandler);
}


/*******************************************************************************
* Function Name: isr_averageData_Interrupt
********************************************************************************
*
* Summary:
*   The default Interrupt Service Routine for isr_averageData.
*
*   Add custom code between the coments to keep the next version of this file
*   from over writting your code.
*
* Parameters:  
*
* Return:
*   None
*
*******************************************************************************/
CY_ISR(isr_averageData_Interrupt)
{
    #ifdef isr_averageData_INTERRUPT_INTERRUPT_CALLBACK
        isr_averageData_Interrupt_InterruptCallback();
    #endif /* isr_averageData_INTERRUPT_INTERRUPT_CALLBACK */ 

    /*  Place your Interrupt code here. */
    /* `#START isr_averageData_Interrupt` */
        CyGlobalIntDisable
        gAverageData    = gInputSum / SAMPLE_RATE;
        gInputSum       = 0;
        gSendAverage    = TRUE;
        CyGlobalIntEnable
    /* `#END` */
}


/*******************************************************************************
* Function Name: isr_averageData_SetVector
********************************************************************************
*
* Summary:
*   Change the ISR vector for the Interrupt. Note calling isr_averageData_Start
*   will override any effect this method would have had. To set the vector 
*   before the component has been started use isr_averageData_StartEx instead.
* 
*   When defining ISR functions, the CY_ISR and CY_ISR_PROTO macros should be 
*   used to provide consistent definition across compilers:
*
*   Function definition example:
*   CY_ISR(MyISR)
*   {
*   }
*
*   Function prototype example:
*     CY_ISR_PROTO(MyISR);
*
* Parameters:
*   address: Address of the ISR to set in the interrupt vector table.
*
* Return:
*   None
*
*******************************************************************************/
void isr_averageData_SetVector(cyisraddress address)
{
    cyisraddress * ramVectorTable;

    ramVectorTable = (cyisraddress *) *CYINT_VECT_TABLE;

    ramVectorTable[CYINT_IRQ_BASE + (uint32)isr_averageData__INTC_NUMBER] = address;
}


/*******************************************************************************
* Function Name: isr_averageData_GetVector
********************************************************************************
*
* Summary:
*   Gets the "address" of the current ISR vector for the Interrupt.
*
* Parameters:
*   None
*
* Return:
*   Address of the ISR in the interrupt vector table.
*
*******************************************************************************/
cyisraddress isr_averageData_GetVector(void)
{
    cyisraddress * ramVectorTable;

    ramVectorTable = (cyisraddress *) *CYINT_VECT_TABLE;

    return ramVectorTable[CYINT_IRQ_BASE + (uint32)isr_averageData__INTC_NUMBER];
}


/*******************************************************************************
* Function Name: isr_averageData_SetPriority
********************************************************************************
*
* Summary:
*   Sets the Priority of the Interrupt. 
*
*   Note calling isr_averageData_Start or isr_averageData_StartEx will 
*   override any effect this API would have had. This API should only be called
*   after isr_averageData_Start or isr_averageData_StartEx has been called. 
*   To set the initial priority for the component, use the Design-Wide Resources
*   Interrupt Editor.
*
*   Note This API has no effect on Non-maskable interrupt NMI).
*
* Parameters:
*   priority: Priority of the interrupt, 0 being the highest priority
*             PSoC 3 and PSoC 5LP: Priority is from 0 to 7.
*             PSoC 4: Priority is from 0 to 3.
*
* Return:
*   None
*
*******************************************************************************/
void isr_averageData_SetPriority(uint8 priority)
{
    *isr_averageData_INTC_PRIOR = priority << 5;
}


/*******************************************************************************
* Function Name: isr_averageData_GetPriority
********************************************************************************
*
* Summary:
*   Gets the Priority of the Interrupt.
*
* Parameters:
*   None
*
* Return:
*   Priority of the interrupt, 0 being the highest priority
*    PSoC 3 and PSoC 5LP: Priority is from 0 to 7.
*    PSoC 4: Priority is from 0 to 3.
*
*******************************************************************************/
uint8 isr_averageData_GetPriority(void)
{
    uint8 priority;


    priority = *isr_averageData_INTC_PRIOR >> 5;

    return priority;
}


/*******************************************************************************
* Function Name: isr_averageData_Enable
********************************************************************************
*
* Summary:
*   Enables the interrupt to the interrupt controller. Do not call this function
*   unless ISR_Start() has been called or the functionality of the ISR_Start() 
*   function, which sets the vector and the priority, has been called.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void isr_averageData_Enable(void)
{
    /* Enable the general interrupt. */
    *isr_averageData_INTC_SET_EN = isr_averageData__INTC_MASK;
}


/*******************************************************************************
* Function Name: isr_averageData_GetState
********************************************************************************
*
* Summary:
*   Gets the state (enabled, disabled) of the Interrupt.
*
* Parameters:
*   None
*
* Return:
*   1 if enabled, 0 if disabled.
*
*******************************************************************************/
uint8 isr_averageData_GetState(void)
{
    /* Get the state of the general interrupt. */
    return ((*isr_averageData_INTC_SET_EN & (uint32)isr_averageData__INTC_MASK) != 0u) ? 1u:0u;
}


/*******************************************************************************
* Function Name: isr_averageData_Disable
********************************************************************************
*
* Summary:
*   Disables the Interrupt in the interrupt controller.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void isr_averageData_Disable(void)
{
    /* Disable the general interrupt. */
    *isr_averageData_INTC_CLR_EN = isr_averageData__INTC_MASK;
}


/*******************************************************************************
* Function Name: isr_averageData_SetPending
********************************************************************************
*
* Summary:
*   Causes the Interrupt to enter the pending state, a software method of
*   generating the interrupt.
*
* Parameters:
*   None
*
* Return:
*   None
*
* Side Effects:
*   If interrupts are enabled and the interrupt is set up properly, the ISR is
*   entered (depending on the priority of this interrupt and other pending 
*   interrupts).
*
*******************************************************************************/
void isr_averageData_SetPending(void)
{
    *isr_averageData_INTC_SET_PD = isr_averageData__INTC_MASK;
}


/*******************************************************************************
* Function Name: isr_averageData_ClearPending
********************************************************************************
*
* Summary:
*   Clears a pending interrupt in the interrupt controller.
*
*   Note Some interrupt sources are clear-on-read and require the block 
*   interrupt/status register to be read/cleared with the appropriate block API 
*   (GPIO, UART, and so on). Otherwise the ISR will continue to remain in 
*   pending state even though the interrupt itself is cleared using this API.
*
* Parameters:
*   None
*
* Return:
*   None
*
*******************************************************************************/
void isr_averageData_ClearPending(void)
{
    *isr_averageData_INTC_CLR_PD = isr_averageData__INTC_MASK;
}

#endif /* End check for removal by optimization */


/* [] END OF FILE */
```
