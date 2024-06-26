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
* Summary:
*  main() performs following functions:
*  1: Starts the ADC and UART components.
*  2: Checks for ADC end of conversion.  Stores latest result in output
*     if conversion complete.
*  3: Checks for UART input.
*     On 'C' or 'c' received: transmits the last sample via the UART.
*     On 'S' or 's' received: continuously transmits samples as they are completed.
*     On 'X' or 'x' received: stops continuously transmitting samples.
*     On 'E' or 'e' received: transmits a dummy byte of data.
*
* Parameters:
*  None.
*
* Return:
*  None.
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
