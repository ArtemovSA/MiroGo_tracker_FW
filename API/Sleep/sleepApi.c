
#include "sleepApi.h"

extern USBReceiveHandler USB_Handler;

//--------------------------------------------------------------------------------------------------
//Sleep on sec
DC_return_t SLEEPAPI_sleep_sec(uint32_t sleep_period)
{
  //UART_RxDisable(); // Disable IRQ
  //MGT_reset(); //Reset task

  DC_ledStatusSet(1);
  vTaskDelay(5);
  
  if (sleep_period > 0){
    sleepMode = 1; //Sleep flag mode
    DC_set_RTC_timer_s(sleep_period); //Set RTC timer
    if (DBG_Connected())
    {
      DC_debugOut("Sleep on: %d\r\n", sleep_period);
      
      switch(DC_taskCtrl.DC_modeCurrent){
      case DC_MODE_AGPS_UPDATE: DC_debugOut("Next Function: DC_MODE_AGPS_UPDATE\r\n"); break;
      case DC_MODE_SEND_DATA: DC_debugOut("Next Function: DC_MODE_SEND_DATA\r\n"); break;
      case DC_MODE_REPEATED: DC_debugOut("Next Function: DC_MODE_REPEATED\r\n"); break;
      case DC_MODE_GNSS_DATA: DC_debugOut("Next Function: DC_MODE_GNSS_DATA\r\n"); break;
      };

      vTaskDelay(sleep_period*1000);
      
    }else{
      EMU_EnterEM2(true);
      
      USB_Driver_Init(USB_Handler);
    }
  }
  
  sleepMode = 0; //Sleep flag mode
  DC_ledStatusSet(0);
  
  return DC_OK;
}
//--------------------------------------------------------------------------------------------------
//Go to Sleep
DC_return_t SLEEPAPI_sleep(DC_modemMode_t modemMode, uint32_t sleep_period)
{
  DC_setModemMode(modemMode); //Set Modem mode DC_MODEM_ALL_IN_ONE
  
  globalSleepPeriod = sleep_period;
  
  SLEEPAPI_sleep_sec(sleep_period);
  
  if (DC_taskCtrl.DC_mes_status & (1<<DC_MES_INT_ACC))
    DC_taskCtrl.DC_modeCurrent = DC_MODE_SEND_DATA;
  
  return DC_OK;
}