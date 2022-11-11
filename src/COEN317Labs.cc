Lab 2:

#include <iostream>



#include "xparameters.h"
#include "xil_types.h"

#include "xtmrctr.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"
#include <stdio.h>
using namespace std;

/* Instance of the Interrupt Controller */
XScuGic InterruptController;

/* The configuration parameters of the controller */
static XScuGic_Config *GicConfig;

// Timer Instance
XTmrCtr TimerInstancePtr;

int test = 0;

void Timer_InterruptHandler(void)
{
        cout << "I am the interrupt handler" << endl;
}

int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
{
/*
* Connect the interrupt controller interrupt handler to the hardware
* interrupt handling logic in the ARM processor.
*/
Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
(Xil_ExceptionHandler) XScuGic_InterruptHandler,
XScuGicInstancePtr);
/*
* Enable interrupts in the ARM
*/
Xil_ExceptionEnable();
return XST_SUCCESS;
}

int ScuGicInterrupt_Init(u16 DeviceId,XTmrCtr *TimerInstancePtr)
{
int Status;
/*
* Initialize the interrupt controller driver so that it is ready to
* use.
* */

GicConfig = XScuGic_LookupConfig(DeviceId);
if (NULL == GicConfig)
{
return XST_FAILURE;
}
Status = XScuGic_CfgInitialize(&InterruptController, GicConfig,
GicConfig->CpuBaseAddress);

if (Status != XST_SUCCESS)
{
return XST_FAILURE;
}
/*
* Setup the Interrupt System
* */
Status = SetUpInterruptSystem(&InterruptController);
if (Status != XST_SUCCESS)
{
return XST_FAILURE;
}

// add this line to fix the interrupt only running once
        // Nov. 15, 2017
        // solution found by Anthony Fiorito on Xilinx Forum


XScuGic_CPUWriteReg(&InterruptController, XSCUGIC_EOI_OFFSET, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR);

/*
* Connect a device driver handler that will be called when an
* interrupt for the device occurs, the device driver handler performs
* the specific interrupt processing for the device
*/
Status = XScuGic_Connect(&InterruptController,
XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR,
(Xil_ExceptionHandler)XTmrCtr_InterruptHandler,
(void *)TimerInstancePtr);

if (Status != XST_SUCCESS)
{
return XST_FAILURE;
}
/*
* Enable the interrupt for the device and then cause (simulate) an
* interrupt so the handlers will be called
*/
XScuGic_Enable(&InterruptController, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR);

return XST_SUCCESS;
}


int main()
{
    cout << "Application starts " << endl;
    int xStatus;


        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //Step-1 :AXI Timer Initialization
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        xStatus = XTmrCtr_Initialize(&TimerInstancePtr, XPAR_AXI_TIMER_0_DEVICE_ID);
        if(XST_SUCCESS != xStatus)
        {
                cout << "TIMER INIT FAILED " << endl;
                if(xStatus == XST_DEVICE_IS_STARTED)
                {
                        cout << "TIMER has already started" << endl;
                        cout << "Please power cycle your board, and re-program the bitstream" << endl;
                }
                return 1;
        }





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Step-2 :Set Timer Handler
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //
        // cast second argument to data type XTmrCtr_Handler since in gcc it gave a warning
        // and with g++ for the C++ version it resulted in an error

XTmrCtr_SetHandler(&TimerInstancePtr, (XTmrCtr_Handler)Timer_InterruptHandler, &TimerInstancePtr);



      // intialize time pointer with value from xparameters.h file

 unsigned int* timer_ptr = (unsigned int* )XPAR_AXI_TIMER_0_BASEADDR;

         //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //Step-3 :load the reset value
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *(timer_ptr+ 1) = 0xffffff00;



         //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         //Step-4 : set the timer options
         //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         // from xparameters.h file #define XPAR_AXI_TIMER_0_BASEADDR 0x42800000
         //  Configure timer in generate mode, count up, interrupt enabled
         //  with autoreload of load register

       *(timer_ptr)  = 0x0f4 ;







//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Step-5 : SCUGIC interrupt controller Initialization
//Registration of the Timer ISR
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
xStatus=
ScuGicInterrupt_Init(XPAR_PS7_SCUGIC_0_DEVICE_ID, &TimerInstancePtr);
if(XST_SUCCESS != xStatus)
{
cout << " :( SCUGIC INIT FAILED )" << endl;
return 1;
}




       // Beginning of our main code


//We want to control when the timer starts
char input;
cout << "Press any key to start the timer" << endl;
cin >> input ;
cout << "You pressed "<<  input << endl;
    cout << "Enabling the timer to start" << endl;

        *(timer_ptr) = 0x0d4 ;   // deassert the load 5 to allow the timer to start counting

        // let timer run forever generating periodic interrupts

while(1)
        {
           // wait forever and let the timer generate periodic interrupts
        }


    return 0;
}


Lab 3:

#include "stdbool.h"

#include "xparameters.h"
#include "xil_types.h"
#include "xgpio.h"
#include "xil_io.h"
#include "xil_exception.h"

#include "xtmrctr.h"

#include <iostream>
using namespace std;

int main()

{


    static XGpio GPIOInstance_Ptr;
    u32* Timer_Ptr = (u32*)XPAR_TMRCTR_0_BASEADDR;

    int xStatus;

    cout << "####  Ted counter application starts ####" << endl;


    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //Step-1: AXI GPIO Initialization
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    xStatus = XGpio_Initialize(&GPIOInstance_Ptr, XPAR_AXI_GPIO_FOR_OUTPUT_DEVICE_ID);
    if(xStatus != XST_SUCCESS)
    {
   cout << "GPIO A Initialization FAILED" << endl;
   return 1;
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //Step-2: AXI GPIO Set the Direction
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //XGpio_SetDataDirection(XGpio *InstancePtr, unsigned Channel, u32 DirectionMask);
    //we use only channel 1, and 0 is the the parameter for output

    XGpio_SetDataDirection(&GPIOInstance_Ptr, 1, 0);

    *Timer_Ptr = 0x206; //hex value for generate mode conditions
    *(Timer_Ptr + 4) = 0x206; //hex value added to second timer as well
    //configuring timer for PWM mode

    float dutycyle;
    float period;
    float hightime;

    //using entered inputs to create the repeating signal
    while(true){
    cout << "enter desired hightime in seconds: " << endl;
    cin>>dutycyle;

    cout<< "enter desired total period time in seconds: " << endl;
    cin>>period;

    hightime = dutycyle*(period/100);
    u32 timer1 = (period* 50000000) - 2;
    u32 timer2 = (hightime * 50000000) -2;

    //timer0 sets the period while timer1 sets the hightime
    *(Timer_Ptr +1) = timer1; //duty cycle formula into TLR0
    *(Timer_Ptr + 5) = timer2; //calculates duty cycle based off the percentage given by the user

    //formulas found in data sheet

    *Timer_Ptr = 0x226; //loads values into registers
    *(Timer_Ptr + 4) = 0x226;

    *Timer_Ptr = 0x286; //starts both timers
    *(Timer_Ptr + 4)= 0x286;
    }
    return 0;
}


Lab 4:

#include <iostream>
using namespace std;


#include "xparameters.h"
#include "xil_types.h"

#include "xtmrctr.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"
#include <stdio.h>

/* Instance of the Interrupt Controller */
XScuGic InterruptController;

/* The configuration parameters of the controller */
static XScuGic_Config *GicConfig;

// Timer Instance
XTmrCtr TimerInstancePtr;

int test = 0;

void Timer_InterruptHandler(void)
{

unsigned int* timer_ptr = (unsigned int* )XPAR_AXI_TIMER_0_BASEADDR;


        cout << "I am the interrupt handler" << endl;

        //want to first stop both timers, therefore use the hex value given by ted,
        //then put it into both TCSRs. The timers are set in step 4 of the main

        cout << "value of first control status register is: " << *(timer_ptr) << endl;
        cout << "value of second control status register is: " << *(timer_ptr + 4) << endl;

        //currently get a reading of the TCSRs. In output you can see that the first status
        //register is 468, and the second one is 244.

        if((*(timer_ptr) == 0x1D4)){ //468 in hexadecimal
        cout << "first control status register called the interrupt." << endl;
        }

        if((*(timer_ptr + 4) == 0xF4)){ //244 in hexadecimal
            cout << "second control status register called the interrupt." <<endl;
        }

        // clearing the TINT bit in both TCSR's by writing a 1 to the TINT bit

        *(timer_ptr) = 0x1D4;
        *(timer_ptr + 4) =  0x1D4; // changes output of 2nd TCSR to 212


        //We want to control when the timer starts
        char input;
        cout << "Press any key to start the timer" << endl;
        cin >> input ;
        cout << "You pressed "<<  input << endl;
            cout << "Enabling the timer to start" << endl;

}

int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
{
/*
* Connect the interrupt controller interrupt handler to the hardware
* interrupt handling logic in the ARM processor.
*/
Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
(Xil_ExceptionHandler) XScuGic_InterruptHandler,
XScuGicInstancePtr);
/*
* Enable interrupts in the ARM
*/
Xil_ExceptionEnable();
return XST_SUCCESS;
}

int ScuGicInterrupt_Init(u16 DeviceId,XTmrCtr *TimerInstancePtr)
{
int Status;
/*
* Initialize the interrupt controller driver so that it is ready to
* use.
* */

GicConfig = XScuGic_LookupConfig(DeviceId);
if (NULL == GicConfig)
{
return XST_FAILURE;
}
Status = XScuGic_CfgInitialize(&InterruptController, GicConfig,
GicConfig->CpuBaseAddress);

if (Status != XST_SUCCESS)
{
return XST_FAILURE;
}
/*
* Setup the Interrupt System
* */
Status = SetUpInterruptSystem(&InterruptController);
if (Status != XST_SUCCESS)
{
return XST_FAILURE;
}

// add this line to fix the interrupt only running once
        // Nov. 15, 2017
        // solution found by Anthony Fiorito on Xilinx Forum


XScuGic_CPUWriteReg(&InterruptController, XSCUGIC_EOI_OFFSET, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR);

/*
* Connect a device driver handler that will be called when an
* interrupt for the device occurs, the device driver handler performs
* the specific interrupt processing for the device
*/
Status = XScuGic_Connect(&InterruptController,
XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR,
(Xil_ExceptionHandler)XTmrCtr_InterruptHandler,
(void *)TimerInstancePtr);

if (Status != XST_SUCCESS)
{
return XST_FAILURE;
}
/*
* Enable the interrupt for the device and then cause (simulate) an
* interrupt so the handlers will be called
*/
XScuGic_Enable(&InterruptController, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR);

return XST_SUCCESS;
}


int main()
{
    cout << "Application starts " << endl;
    int xStatus;


        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //Step-1 :AXI Timer Initialization
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        xStatus = XTmrCtr_Initialize(&TimerInstancePtr, XPAR_AXI_TIMER_0_DEVICE_ID);
        if(XST_SUCCESS != xStatus)
        {
                cout << "TIMER INIT FAILED " << endl;
                if(xStatus == XST_DEVICE_IS_STARTED)
                {
                        cout << "TIMER has already started" << endl;
                        cout << "Please power cycle your board, and re-program the bitstream" << endl;
                }
                return 1;
        }





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Step-2 :Set Timer Handler
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //
        // cast second argument to data type XTmrCtr_Handler since in gcc it gave a warning
        // and with g++ for the C++ version it resulted in an error

XTmrCtr_SetHandler(&TimerInstancePtr, (XTmrCtr_Handler)Timer_InterruptHandler, &TimerInstancePtr);



      // intialize time pointer with value from xparameters.h file

unsigned int* timer_ptr = (unsigned int* )XPAR_AXI_TIMER_0_BASEADDR;

         //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //Step-3 :load the reset value
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *(timer_ptr+ 1) = 0xffffff00;



         //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         //Step-4 : set the timer options
         //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         // from xparameters.h file #define XPAR_AXI_TIMER_0_BASEADDR 0x42800000
         //  Configure timer in generate mode, count up, interrupt enabled
         //  with autoreload of load register



       *(timer_ptr)  = 0x0f4 ;

       *(timer_ptr + 4) = 0x0f4 ;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Step-5 : SCUGIC interrupt controller Initialization
//Registration of the Timer ISR
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
xStatus=
ScuGicInterrupt_Init(XPAR_PS7_SCUGIC_0_DEVICE_ID, &TimerInstancePtr);
if(XST_SUCCESS != xStatus)
{
cout << " :( SCUGIC INIT FAILED )" << endl;
return 1;
}

       // Beginning of our main code

    *(timer_ptr) = 0x0d4 ;   // deassert the load 5 to allow the timer to start counting


        // let timer run forever generating periodic interrupts

while( 1)
        {
          //  // wait forever and let the timer generate periodic interrupts
        }


    return 0;
}
