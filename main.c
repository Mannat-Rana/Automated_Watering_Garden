#include <project.h>
#include <stdio.h>

/*******************************************************************************
* Macros
*******************************************************************************/
#define FALSE     0u
#define TRUE      1u
#define LED_OFF   1u
#define LED_ON    0u
#define Ch1Period	(9000)
#define SAMPLESIZE (4) 

#define WDT_COUNTER                                   (CY_SYS_WDT_COUNTER1)
#define WDT_COUNTER_MASK                              (CY_SYS_WDT_COUNTER1_MASK)
#define WDT_INTERRUPT_SOURCE                          (CY_SYS_WDT_COUNTER1_INT) 
#define WDT_COUNTER_ENABLE                            (1u)
#define WDT_1SEC                                      (32767u)


/*Global Variables*/
uint8 authReq;
uint8 notificationsEnabled=FALSE;
uint16 temperature;
volatile uint32 WDTInterrupt;
uint8 temp[2];
uint8 camp[2];
static int intFlag = 0;
int MoistureLevel;
static volatile uint32 Channel_1_Count;


/***************************************
*        Function Declaration
***************************************/
void StackEventhandler(uint32 event,void * eventParam);

void StackEventhandler(uint32 event,void * eventParam)
{
    CYBLE_GATTS_WRITE_REQ_PARAM_T *wrReqParam;
    
    switch(event)
    {
        case CYBLE_EVT_STACK_ON:
                /*Start Advertising*/
                CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            break;
        case CYBLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
                if(CyBle_GetState()==CYBLE_STATE_DISCONNECTED)
                {/*Start advertising if time out happens*/
                    CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
                }
                if(CyBle_GetState()==CYBLE_STATE_ADVERTISING)
                {
                    Advertising_LED_Write(LED_ON);
                }
            break;       
        case CYBLE_EVT_GAP_DEVICE_CONNECTED:
            break;  
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:             
                notificationsEnabled=FALSE;
                /*Start advertising again*/
                CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            break;                                
        case CYBLE_EVT_GATTS_WRITE_REQ:                
                wrReqParam=(CYBLE_GATTS_WRITE_REQ_PARAM_T*) eventParam;                 
                /*If notification is enabled*/
                if(wrReqParam->handleValPair.attrHandle==
                    CYBLE_CUSTOM_SERVICE_CUSTOM_CHARACTERISTIC_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE)
                {
                    if(*wrReqParam->handleValPair.value.val==0x01)
                    {
                       notificationsEnabled=TRUE;
                    }
                    else if(*wrReqParam->handleValPair.value.val==0x00)
                    {
                        notificationsEnabled=FALSE;
                    }
                }  
                CyBle_GattsWriteRsp(cyBle_connHandle);
            break;               
        default:
            break;
    }
}
//interupt procedure
CY_ISR(isr_1_Handler) {
    intFlag = Timer1_ReadStatusRegister();	//	Read and clear interrupt status
    if(intFlag & Timer1_STATUS_CAPTURE) {
        //Channel_1_Count[i++] = Ch1Period - Timer1_ReadCapture();
        Channel_1_Count = Ch1Period - Timer1_ReadCapture();       
    }	
    Timer1_Stop();                         // stop and setup for next round
    Timer1_WriteCounter(Ch1Period);		//	Rewrite counter for next cycle
    
}
/*******************************************************************************
* Function Name: main
********************************************************************************
*
* Summary:
*  Main function.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Theory:
* This function initializes the BLE component and then procesthe BLE events routinely
*******************************************************************************/
int main()
{
    CYBLE_GATTS_HANDLE_VALUE_NTF_T serverTemp; 
    CyGlobalIntEnable;
    isr_1_StartEx(isr_1_Handler);
    Timer1_Start();
    CyBle_Start(StackEventhandler); 
    for(;;)
    {       
        CyBle_ProcessEvents();  
        if(CyBle_GetState()==CYBLE_STATE_CONNECTED && notificationsEnabled==TRUE)
        {          
            temp[1]= 1;
            temp[0]= 0;
            camp[1]= 0;
            temp[0]= 0;
            
            switch (WaterLevel_Read()) {
            case 1:
            serverTemp.attrHandle=CYBLE_CUSTOM_SERVICE_CUSTOM_CHARACTERISTIC_CHAR_HANDLE;
            serverTemp.value.val=temp;
            serverTemp.value.len=sizeof(temp);
            Trigger1_Write(0);
            Trigger1_Write(1);
            CyDelay(1);
            if (intFlag) {
            Timer1_Start();
            
            }
            if (Channel_1_Count < 1750) {
                MoistureLevel = 1;
            }
            else {
                MoistureLevel = 0;
            }
                    
            break;
            case 0:
            serverTemp.attrHandle=CYBLE_CUSTOM_SERVICE_CUSTOM_CHARACTERISTIC_CHAR_HANDLE;
            serverTemp.value.val=camp;
            serverTemp.value.len=sizeof(camp);    
            
            Pump1_Write(0);
            break;
            }
            
            if(CYBLE_ERROR_OK==CyBle_GattsNotification(cyBle_connHandle,&serverTemp))
            {
                WDTInterrupt=FALSE;
            }
        }
         if (MoistureLevel == 1) {
            Pump1_Write(1);
            } 
        else {
        Pump1_Write(0);
        }   
    }
}

