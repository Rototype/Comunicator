#include "appdef.h"                        
#include "appstrutt.h"
#include "areadati.h"

char CMD_tab[CMDTOT][33]={
	"CMD_UpdateFirmware",
	"CMD_UpdateWebSocketFirmware",
	"CMD_Restart",
	"CMD_UpdateConfiguration",
	"CMD_UpdateNetworkConfiguration",
	"CMD_ReadDigitalInput",
	"CMD_ReadAnalogInput",
	"CMD_SetAnalogOutput",
	"CMD_SetDCMotor",
	"CMD_SetDCMotorPWM",
	"CMD_SetDCSolenoid",
	"CMD_SetDCSolenoidPWM",
	"CMD_SetDigitalOutput",
	"CMD_SetStepperMotorSpeed",
	"CMD_SetStepperMotorCountSteps",
	"CMD_InvertImage",
	"EVT_StopStepperMotor",
	"CMD_ReadConfiguration"
};
char SlaveUnit[4][33]={
	"Main",
	"FPGA",
	"HWController",
	"LastUnit"
};
char PAR_str[PARTOT][33]={
	"on",
	"off",
	"open",
	"clockwise",
	"counterclockwise",
	"cw",
	"ccw",
	"brake",
	"last_parameter"
};
int PAR_val[PARTOT]={
	1  ,
	0  , 
	0  ,
	1  ,
   -1  ,
	1  ,
   -1  ,
    0  ,
	0xDEAD
};
char jsonObj[ROOTOBJ][33]={
	 "IpAddress",
	 "DefaultGateway",
	 "DHCP",
	 "MechanicalOffset1",
	 ""
};

