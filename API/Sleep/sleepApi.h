
#ifndef SLEEP_API_H
#define SLEEP_API_H

#include "Device_ctrl.h"
#include "em_dbg.h"
#include "em_emu.h"
#include "cdc.h"

DC_return_t SLEEPAPI_sleep(DC_modemMode_t modemMode, uint32_t sleep_period); //Go to Sleep
DC_return_t SLEEPAPI_sleep_sec(uint32_t sleep_period); //Sleep on sec

#endif

