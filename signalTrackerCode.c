/* ============================================

This program get signal strength of antenna Ubiquiti Nano Loco M5 using 4 gpios of 
router TLMR3420 with OpenWrt Chaos Calmer. The antenna should find other signal and 
looking the best position where the signal is more strength. First, scan 0-180° and 
set the signal strength, After search this signal and try to keep this posistion.

===============================================
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "define_vars.h"


FILE *GpioLan1,*GpioLan2;

void Write_Gpio( int v);/*Function to write values in output gpio WPS and RESET*/
void Read_Input_Switch(int *a, int *b,FILE *c, FILE *d);/* Function to read values of gpio inputs */
void Motor_Sequence(int a, int b, int c);/* Function to send to led_lan1 and led_lan2 motor sequence */
void Init_Gpio_IO(FILE *a,FILE *b);/*Function to initialize led_lan1, led_lan2, resetButton and wpsButton*/
int Signal_Request();/* Function to Get signal strength*/


void Write_Gpio(int Data){ /*Write 0 or 1 in gpio lan1 or lan2 to control steps and direction driver motor*/

  char ValueToWriteGpio;

	if(Data & 0x01)
		ValueToWriteGpio = '0';
  else
		ValueToWriteGpio = '1';
	
	fputc(ValueToWriteGpio,GpioLan1);
	fflush(GpioLan1);

	if((Data >> 1) & 0x01)
		ValueToWriteGpio = '0';
  else
		ValueToWriteGpio = '1';
	
	fputc(ValueToWriteGpio,GpioLan2);
	fflush(GpioLan2);

}

void Read_Input_Switch(int *SwitchStart, int *SwitchEnd,FILE *GpioResetButton, FILE *GpioWpsButton){ /*REad value directly gpio reset and wps to identify if is near to 0 or 180 degrees*/
  char ValueSwitchWps,ValueSwitchReset;

  ValueSwitchReset = fgetc(GpioResetButton);
  ValueSwitchWps = fgetc(GpioWpsButton);
  rewind(GpioResetButton);
  rewind(GpioWpsButton);

  if(ValueSwitchWps  == '0'){ // Switch WPS
    *SwitchStart = 1;
  }else{
    *SwitchStart = 0;
  }
  if(ValueSwitchReset == '1'){ // Switch RESET
    *SwitchEnd = 1;
  }else{
    *SwitchEnd = 0;
  }
}

void Motor_Sequence(int StepsMotor, int DirectionMotor, int TimeMotor){/*Send value to write of gpios lan1 and lan2 */
  int j;
  for(j=0; j < StepsMotor; j++){
    Write_Gpio(DirectionMotor + 2);
    usleep(TimeMotor/2); 
    Write_Gpio(DirectionMotor);
    usleep(TimeMotor/2);
  }
}

void Init_Gpio_IO(FILE *GpioResetButton,FILE *GpioWpsButton){/*Open all gpios as a files to write 0 or 1*/

     GpioLan1 = fopen("/sys/class/leds/tp-link:green:lan1/brightness","w");
     if (GpioLan1 == NULL)
       printf("No se pudo abrir GpioLan1\n" );
     else
       printf("Se pudo abrir GpioLan1\n" );

     GpioLan2= fopen("/sys/class/leds/tp-link:green:lan2/brightness","w");
     if (GpioLan2== NULL)
       printf("No se pudo abrir gpiolan2\n" );
     else
       printf("Se pudo abrir gpiolan2\n" );

     GpioResetButton = fopen("/etc/hotplug.d/button/resetButton","r");
     if (GpioResetButton == NULL)
       printf("No se pudo abrir GpioResetButton\n" );
     else
       printf("Se pudo abrir GpioResetButton\n" );

     GpioWpsButton = fopen("/etc/hotplug.d/button/wpsButton","r");
     if (GpioWpsButton == NULL)
       printf("No se pudo abrir GpioWpsButton\n" );
     else
       printf("Se pudo abrir GpioWpsButton\n" );
}

int Signal_Request()/*Get signal strength*/
{
  FILE *PtrSignal; 
  char SignalValue[4],SignalStrength = 0;

  PtrSignal = popen("curl -L -F 'username=ubnt' -F 'password=ubnt' -F 'Submit=Login' -F 'uri=/signal.cgi'  'http://192.168.1.22/login.cgi' -H 'Expect:' -c c.txt -b c.txt | grep signal | sed -e 's/.*\"signal\"\\:.-\\([0-9]*\\).*/\\1/'", "r");	/*Get signal value of http AirOS*/

  if (PtrSignal == NULL){
       printf("Failed to run command\n" );
       exit;
  }
  fgets(SignalValue, sizeof(SignalValue)-1, PtrSignal);
  printf("\nCurrent dB=%s", SignalValue);
  SignalStrength = atoi(SignalValue);
  pclose(PtrSignal);
  
  return SignalStrength;
}


int main( ){
  FILE *GpioResetButton,*GpioWpsButton;
  int SwitchStartValue, SwitchEndValue,MaxSignalValue = 0, ReferencePosition = 0, SignalStatus = 0, CurrentGainSignal=-1, PreviousGainSignal = 0, SignalGain= 0,  DirectionMotor=1, tsleep=T_SLEEP_LONG, SignalError = 0 ;
  int  ConnectDev = 0;
  char ContPosition, ContPositionMaxSignal = 0;

  Init_Gpio_IO(GpioResetButton,GpioWpsButton);  /*Inicialize Switchs*/
  while(1){
    Read_Input_Switch(&SwitchStartValue, &SwitchEndValue,GpioResetButton,GpioWpsButton);/*Inicialize Switchs*/
    if(DirectionMotor && SwitchStartValue){   /*Start path with Right direction*/
      DirectionMotor = RIGHT_DIR ;
    }
    if(!DirectionMotor && SwitchEndValue){  /*Start path with Left direction*/
      DirectionMotor = LEFT_DIR;
    }
      Motor_Sequence(START_STEP, DirectionMotor, T_STEP);  /*Start sequence with direction previusly set*/
    sleep(tsleep);
    CurrentGainSignal = Signal_Request();  /*Request current signal*/

    if(CurrentGainSignal != 0){
      if(!ConnectDev){
        ConnectDev = 1;
        tsleep = T_SLEEP_SHORT;
      }
      while( SwitchStartValue == 0){  /* Set start position at home*/
        Motor_Sequence(START_STEP,LEFT_DIR,TQ_STEP);
        Read_Input_Switch(&SwitchStartValue, &SwitchEndValue,GpioResetButton,GpioWpsButton);
      }
      usleep(T_SLEEP_LONG);

      for(ContPositionMaxSignal = 0;ContPositionMaxSignal <= INIT_STEP; ContPositionMaxSignal++){ /* 180 path to scan signal and get signal strength*/
         CurrentGainSignal = Signal_Request(); 
         printf("\nPrevious dB= %d\n", PreviousGainSignal);
         if(ContPositionMaxSignal == 0){
	    PreviousGainSignal = CurrentGainSignal;
            MaxSignalValue = PreviousGainSignal;
         }
         if(ContPositionMaxSignal > 0){
            if(PreviousGainSignal < CurrentGainSignal & PreviousGainSignal < MaxSignalValue){ 
                MaxSignalValue = PreviousGainSignal;
            }
            if( MaxSignalValue > CurrentGainSignal){
               MaxSignalValue = CurrentGainSignal;  
	       ReferencePosition = ContPosition;
            }
         }
         PreviousGainSignal = CurrentGainSignal; /*Set strength signal value and set this as a reference for the control*/
         printf("Referencia dB =%d\n", MaxSignalValue);
//       printf("Muestra:%d\n", i);
         printf("Posicion: %d\n", ReferencePosition);
         Motor_Sequence(START_STEP,RIGHT_DIR ,T_STEP);
    }
    Motor_Sequence((INIT_STEP-ReferencePosition)*START_STEP,LEFT_DIR,TQ_STEP);/*Move after calculate steps left searching signal reference */
    sleep(2);
    while(1){
      Read_Input_Switch(&SwitchStartValue, &SwitchEndValue,GpioResetButton,GpioWpsButton);
      SignalStatus = Signal_Request();
      SignalError = MaxSignalValue - SignalStatus; /*Control error*/
      printf("\nReferencia%d\n", MaxSignalValue);
      printf("Error:%d\n", SignalError);
      SignalGain = SignalError + 1;  /*Gain*/
      printf("gp %d\n", SignalGain);
      if( MaxSignalValue != SignalStatus){
          if( SignalGain > 0 && DirectionMotor ){  
	            Motor_Sequence(SignalGain*HALF_STEP,RIGHT_DIR, T_STEP);/*Move half steps to right searching signal reference*/
              DirectionMotor = 0;
          }
          if(SignalGain > 0 && DirectionMotor != 1 ){
	           Motor_Sequence(SignalGain*HALF_STEP, RIGHT_DIR, T_STEP);/*Move half steps to right searching signal reference*/
             DirectionMotor = 0;
          }
          if( SignalGain < 0 && DirectionMotor ){
	           Motor_Sequence(abs(SignalGain)*HALF_STEP, RIGHT_DIR, T_STEP);
            DirectionMotor = RIGHT_DIR ;
            }else if( SignalGain < 0 && !DirectionMotor ){
		                 Motor_Sequence(abs(SignalGain)*HALF_STEP,RIGHT_DIR , T_STEP);
                     DirectionMotor = LEFT_DIR;
                   }
      }else
	         Write_Gpio(0x00);
       
    if (abs(SignalError) > 10) break;/*If error is more than 10 restart the process to get the signal strength*/
    }
    }else{
      if(ConnectDev){
        DirectionMotor = !DirectionMotor;
        ConnectDev = 0;
        tsleep = T_SLEEP_LONG;
      }
      CurrentGainSignal = 100;
    }
    ContPosition++;
    PreviousGainSignal = CurrentGainSignal;
  }
  return 0;
}
