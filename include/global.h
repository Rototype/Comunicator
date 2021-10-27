#include "usesys.h"
#include "define.h"
#include "struttur.h"                   // Definizione delle strutture
/**************************** AREA DATI CONDIVISA ************************/
unsigned long *A;
unsigned long *P;
unsigned long *W;
//unsigned int *TT; /* Puntatore all'area hardware del touch da passare alla GUI */
/*----------------------------------------------------------------------*/
/*** Dati per debug e log file ***/
int dbg_printf ;
int genliv=10; /* Livello attuale di trace ( default = 5 ) 10 li vedo tutti 0 solo gli errori importanti */
char retValue[256];
extern void trace(int,char *,int,int,int,char *, ...);
/*-------------------------------*/
/*** Dati per gestione canali di comunicazione ***/
int tot_canali_act=0;
struct sockaddr_in server_addr;        	/* Indirizzo del Server */
struct sockaddr_in client_addr;        	/* Indirizzo del client */
struct sockaddr_in addr ;
int act_client=0;                       /* Totale clienti attivati */
int client_len;    						/* Dimensione dell'indirizzo client */
int sd;    							  	/* Identificatori del canale pubblico */
struct sigaction sasok[MAX_N_PORT];                 /* Definition of signal action */
struct sigaction saplc;                 /* Definition of signal action */
char *protocol_name[];
struct protoManage *ptr[MAX_PROTO_CONT] ; /* Variabili di RX/TX */
struct Device * ptrDevice[MAX_DEVICE] ; // 100 Puntatori a dispositivi
struct wassi *pStructAssi[MAX_ASSI] ;        // Massimo numero ASSI 
int tot_assi = 0 ;
int tot_dev = 0 ;
int act_dev = 0 ;
int TAU = 20 ;
int plcpolltime ;
int power_on = 0 ;
int timer_sec,timer_min,rimba_plc=0;
char NOME_DB[50];
char DATANOME[50];
/*----------------------------------------------*/
int img_lenblk_limit_on = 0;
/*----------------------------------------------*/
char *nome_giorno[]={
"LUNEDI",
"MARTEDI",
"MERCOLEDI",
"GIOVEDI",
"VENERDI",
"SABATO",
"DOMENICA"
};
char *nome_mese[]={
"GENNAIO",
"FEBBRAIO",
"MARZO",
"APRILE",
"MAGGIO",
"GIUGNO",
"LUGLIO",
"AGOSTO",
"SETTEMBRE",
"OTTOBRE",
"NOVEMBRE",
"DICEMBRE"
};
char *protocol_name[]={
"DoNothing",
"Echo",
"WebSocket",
"Iso2110",
"Iso2110Slave",
""
};

char speed_table[8][8]={"2400","4800","9600","19200","38400","57600","115200",""};
speed_t vs_table[7]={B2400,B4800,B9600,B19200,B38400,B57600,B115200};
// Default struttura dati I/O 8 uscite e 8 INGRESSI... il file IMPIANTO.CONF puo' riconfigurare questi valori
int nome_sgn[TOT_SGN];
int func_sgn[TOT_SGN];
int resource_id[TOT_SGN];

/************************** PROTOTYPE FUNCTION DECLARATIONS SECTION *****************/
int openScheduling(unsigned long);
void plc(int);
int openSocket(int,int);
void sig_io(int ,siginfo_t *, void * );
void richiedi_socket(int,int,int) ;
char *Buff2Hex(char * ,int) ;
unsigned long strip2long(char *);
char *long2strip(unsigned long);
unsigned long StrHex2Value(char * );
int rw_parametro(char *,char *,int );
void rw_par_lavoro(int );
int EsisteFile(int ) ;
double rw_valpar(char *,char *,int ) ;
char * memory(int ,int );
int openSeriale(char *,speed_t,int,int,char,int,int);
void *SerialLine (void* );
void rw_strpar(char *,char *,char *,int );
int crea_tabella ( int  );
int numGiorno(char *); 
void trim(char * );
long dammiSec(char *) ;
/************************** PROTOTYPE REFERENCES FUNCTION DECLARATIONS SECTION *****************/
extern int (* dammi_protocollo(int ))();
extern void SendPollCmd(int ,int ,int );
extern void InitChannel(int );
// Include dipendenti dall'applicazione ###############################
#include "appglob.h"                        
