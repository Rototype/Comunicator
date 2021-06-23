/************************** INCLUDE APPLICATION SECTION *****************************/
#include <math.h>
#include "extern.h"
int InitTarature(void);
extern int setta_ora(void);
int logica_sec(void);
int timer_plc(void);

/************************** PROTOTYPE PROTOCOL FUNCTION REFERENCES SECTION *****************/
extern double rw_valpar(char *,char *,int ) ;
/************************** PROTOTYPE FUNCTION DECLARATIONS SECTION *****************/
int init_plc(int opt) // Funzione che inizializza la struttura dell' impianto **************************
{
int kk ;
        InitTarature();
		setta_ora();
}
/***********************************************************************************************************/
int logica_plc(void) // Logica da eseguire ogni TAU *************************************************************
{
}
/***********************************************************************************************************/
int logica_sec(void)  // Logica da eseguire ogni secondo ********************************************************
{
    SEC_ON ++ ; // Secondi passati dalla partenza del programma 
//    printf("Passato %d secondi\n",SEC_ON);
}
/***********************************************************************************************************/
int logica_min() // Logica da eseguire ogni minuto **********************************************************
{
  	setta_ora();
}
/***********************************************************************************************************/
int tau_persi_plc(void) // Logica da eseguire sulla perdita di campionamenti ************************************
{
  if ( power_on > 2 )  printf("TAU persi\n");
}
/***********************************************************************************************************/
int timer_plc(void)  // Gestione dei timer di logica ad ogni TAU ************************************************
{
//printf("TAU timer plc\n");	
}
/***********************************************************************************************************/
int timer_sec_plc(void) // Gestione dei timer di logica ogni secondo ********************************************
{
// printf("timer SEC plc\n");
  logica_sec() ;
  timer_min ++ ; // Passato un altro secondo
  if(timer_min>=60){	// Passato un minuto
  	timer_min = 0 ;
  	logica_min();
  }
}
/***********************************************************************************************************/
int InitTarature(void) // Caricamento dei parametri alla partenza del sistema ***********************************
{
char stringa[30];
long appo ;
printf("Inizializzazione parametri lavoro PLC \n");
    Versione_SWC = VER ;
}
/***********************************************************************************************************/
/* AREA DATI */

int aggiorna_area(int idqar)
{
int tt ;
char appo[20] ;
unsigned long appo1 ;
  
  printf("Aggiorna  A[%d]=%ld = Address %X \n",idqar,A[idqar],A);
  sprintf(appo,"%ld",A[idqar]);

  switch(idqar)
  {
    case 1:        // Variabile A[1]
      appo1 = A[1];
      sprintf(appo,"%ld",appo1);
      rw_valpar("LAVORO/PARAM1", appo , WRITE_PAR )  ;    
    break;  // 
  }
}



