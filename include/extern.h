#include "usesys.h"
#include "define.h"
#include "struttur.h"                   // Definizione delle strutture
/**************************** AREA DATI CONDIVISA ************************/
extern unsigned long *A;
extern unsigned long *P;
extern unsigned long *W;
extern char *protocol_name[];

/*-------------------------------*/
extern int dbg_printf ;
extern char nome_mese[];
extern char NOME_DB[];
extern char DATANOME[];
// Default struttura dati I/O 8 uscite e 8 INGRESSI... il file IMPIANTO.CONF puo' riconfigurare questi valori
int nome_sgn[TOT_SGN];
int func_sgn[TOT_SGN];
int resource_id[TOT_SGN];

extern int genliv; 
extern char retValue[];
extern struct DataDevice *pDevice[] ; /* Struttura per indice area dati dispositivi: faccio posto per 255 dispositivi max */
extern struct protoManage *ptr[] ; /* Variabili di RX/TX */
extern int tot_assi,tot_dev,act_client;                       /* Totale clienti attivati */
extern int timer_sec,timer_min;
extern int plcpolltime ;
extern int TAU ;
extern struct Device * ptrDevice[] ; // 100 Puntatori a dispositivi
extern struct wassi *pStructAssi[] ;        // Massimo numero ASSI 
extern int power_on ;
extern char *nome_giorno[];
/************************** PROTOTYPE PROTOCOL FUNCTION REFERENCES SECTION *****************/
extern double rw_valpar(char *,char *,int ) ;
extern double dammiPower(int ) ;
int numGiorno(char *);
// Include dipendenti dall'applicazione ###############################
#include "appext.h"                        

