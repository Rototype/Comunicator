/************************** INCLUDE MAIN APPLICATION SECTION *****************************/
#include "global.h"


// backported (obsolete function)
#include <errno.h>
int stime (const time_t *when)
{
  struct timeval tv;
  if (when == NULL)
    {
      errno = EINVAL;
      return -1;
    }
  tv.tv_sec = *when;
  tv.tv_usec = 0;
  return settimeofday (&tv, (struct timezone *) 0);
}

/************************** PROTOTYPE INTERNAL FUNCTION DECLARATED  **********************/
void msec_sleep(int);
int checkDb(int);
int openClientSocket(int,int,char *);
int DammiGPIObit(char *);
int set_config(char *,char *, char *);
int get_config(char *,char *, char *);
int SecondiDiff(char *,char *);
int caricaFileNelDB(char *);
int fileCpy(char *,char *);
int fileTxtCpy(char *,char *);
int setta_ora(void);

extern void InitConnection(int,struct connectManage * );
extern int init_plc(int);
extern int logica_plc(void);
extern int tau_persi_plc(void);
extern int timer_plc(void);
extern int timer_sec_plc(void);

int param_wait[10];
int coda_wait ;
/************************** MAIN FUNCTION SECTION ***********************************/
int main(int argc,char **argv)
{
struct tm *dt;
time_t t;
char * shared_memory ;
FILE *fp ;
int rc,ii,valappo,db,sb,linea,kk,kdev,kres,tot_res,k_con,tot_dev_can,ksig;
speed_t bd;
int prot,last_sec,tot_con;
char stringa[200],strappo[201],nome_con[200],nome_dev[200],linea_ser[40],parita[10],nome_res[200];
char gwy[20],mask[20],mpd_serv[20];
char *token ;
long pp ;
int sys_slave = 0 ; // 0 = server o 1 = slave: servizio da server -> Per default ipotesi server
char **brr = NULL ;
struct wassi * ptrax;
double fappo;

/*
 	InvertiImage();
	exit(0);
*/
  
  openlog("Comunicator", 0, LOG_USER);                          		// Vedere i messaggi con il comando: <grep Comunicator /var/log/user.log>

																		// Controllo l'ora se ha un settaggio accettabile
  time(&t);
  dt = localtime(&t);
  if (dt->tm_year<100){                                     // Se anno inferiore al 2000 setto 1 Gen 2020 ore 8:00
    dt->tm_year = 120;                                      // 2020
    dt->tm_mday = 1;                                        // Giorno
    dt->tm_mon  = 1;                                        // Mese
    dt->tm_hour = 8;                                        // Ora
    dt->tm_min  = 0;                                        // Min
    dt->tm_sec  = 0;                                        // Secondi
    t= mktime(dt);
    rc = stime(&t);                                         // Settaggio ora
    if ( rc !=0 ) trace(__LINE__,__FILE__,1100,0,0,"Settata ora %d errno = %d : %s",rc,errno,strerror(errno));
    else trace(__LINE__,__FILE__,1100,0,0,"Settata ora %2.2d:%2.2d %2.2d/%2.2d/%2.2d",dt->tm_hour,dt->tm_min,dt->tm_mday,dt->tm_mon,(dt->tm_year-100));

  }

  checkDb(0);                                             // Prepara il DB se non esiste crealo ripartendo dal file di tarature dei parametri fabbrica

  dbg_printf = rw_valpar("IMPIANTO/TraceLevel", "1" , READ_PAR | IMPIANTO_FILE ); // OKKIO attivo un pò di printf

  trace(__LINE__,__FILE__,3000,0,0,"INIZIO PROGRAMMA %s%d",NOME,VER);   // Da un riferimento sull'avvio dell'applicativo

// ###################### ALLOCO LA MEMORIA ###########################################################

  ii = rw_valpar("IMPIANTO/SharedMemory", "0" , READ_PAR | IMPIANTO_FILE ); // Chiave di identificazione del segmento shared
  if (ii){
    kk = ( MAX_A * sizeof(int) ) +( MAX_P * sizeof(int) )+ ( MAX_W * sizeof(int) );
    sprintf(stringa,"%d",kk);
    rw_valpar("IMPIANTO/SizeMemory", stringa , WRITE_PAR | IMPIANTO_FILE ); // Dimensione in bytes del segmento condiviso
    shared_memory = memory(ii,kk);
    trace(__LINE__,__FILE__,3001,0,0,"--------------- Allocata memoria condivisa nome %d size %d address %lX",ii,kk,shared_memory);
//printf("size short %d size int %d size int %d\n",sizeof(short),sizeof(int),sizeof(int));
    A = (unsigned int *) shared_memory ; // Area condivisa dei device
    P = (unsigned int *)(shared_memory + (MAX_A*sizeof(int))) ;
    W = (unsigned int *)(shared_memory + (MAX_A*sizeof(int)) + (MAX_P*sizeof(int))) ;
  }
  else {
    A = malloc(MAX_A*sizeof(int)) ;
    P = malloc(MAX_P*sizeof(int)) ;
    W = malloc(MAX_W*sizeof(int)) ;
    trace(__LINE__,__FILE__,3001,0,0,"------------------------------------------------ Allocata memoria non condivisa");
    shared_memory = (char *) -1 ;
  }
  memset(A,0,MAX_A*sizeof(int));
  memset(P,0,MAX_P*sizeof(int));
  memset(W,0,MAX_W*sizeof(int));
  trace(__LINE__,__FILE__,3002,0,0,"------------------------------------------------ Azzerata memoria A - M - P");
  trace(__LINE__,__FILE__,3003,0,0,"Address area A %lX - P %lX - W %lX",A,P,W);
  trace(__LINE__,__FILE__,3004,0,0,"A[274]=%lX A[512]=%lX P[0]=%lX P[256]=%lX W[0]=%lX W[256]=%lX",&A[274],&A[512],&P[0],&P[256],&W[0],&W[256]);

// #######################################################################################################################################

  tot_con = rw_valpar("IMPIANTO/Connections", "0" , READ_PAR | IMPIANTO_FILE );

  TAU = rw_valpar("IMPIANTO/Tau", "20" , READ_PAR | IMPIANTO_FILE );    // Default 20 msec

  trace(__LINE__,__FILE__,3000,0,0,"TOTALE CONNESSIONI DICHIARATE %d",tot_con);

  tot_dev = 0 ;                                           // Azzero il totale dei device presenti
  if (tot_con){                                           // Se ho dichiarato delle connessioni allora vado a parametrizzare i canali
    for(k_con=1;k_con<=tot_con;k_con++) {
        sprintf(stringa,"IMPIANTO/Connect%d",k_con);
        rw_strpar(nome_con,stringa, "" , READ_PAR | IMPIANTO_FILE ) ;
//printf("Connessione %s = %s parziale device %d\n",stringa,nome_con,tot_dev);
        sprintf(stringa,"%s/Enable",nome_con);
        valappo = rw_valpar(stringa, "0" , READ_PAR | IMPIANTO_FILE );  // Connessione abilitata

        if (valappo){                                       // Si > allora vado a parametrizzare il canale
printf("\n###################################################\n");
printf("Connessione abilitata %s\n",nome_con);
          ptr[act_client] =  malloc(sizeof (struct protoManage));       // Mi alloco la memoria per i buffer di TX/RX
          memset(ptr[act_client],0,sizeof(struct protoManage));       // La pulisco
          sprintf(stringa,"%s/Indirizzo",nome_con);
                                    // Se fosse un server seriale RTU Modbus ha un indirizzo di identificazione a cui deve rispondere
          ptr[act_client]->id_unique[0][1] = rw_valpar(stringa, "0" , READ_PAR | IMPIANTO_FILE ) ;
          sprintf(stringa,"%s/Protocollo",nome_con);
          rw_strpar(strappo,stringa, "DoNothing" , READ_PAR | IMPIANTO_FILE ) ;
          for (prot=0,ii=0;protocol_name[ii][0]!=0;ii++) if (!strcmp(strappo,protocol_name[ii])) prot = ii;
          ptr[act_client]->proto_client = prot ;                  // Mi memorizzo il protocollo

          sprintf(stringa,"%s/Polltime",nome_con);                  // Polltime="1000"  OPPURE Polltime="event" oppure "-1" # Tempo di rinfresco dati oppure invio evento di richiesta

          rw_strpar(strappo,stringa,"100",READ_PAR | IMPIANTO_FILE ) ;  // Guardo se c'e' la gestione in polling o a eventi
          if ( !strcmp(strappo,"event") || !strcmp(strappo,"-1"))  ptr[act_client]->polcan = -1 ;  // gestione a eventi
          else ptr[act_client]->polcan = 1000 * rw_valpar(stringa, "100" , READ_PAR | IMPIANTO_FILE ) ; // Espresso in usec

          sprintf(stringa,"%s/Port",nome_con);                    // Con questo capisco che tipo di canale e' richiesto
          rw_strpar(strappo,stringa, "" , READ_PAR | IMPIANTO_FILE ) ;

          if(!strcmp(strappo,"gpio") ){
printf("Dichiarato canale GPIO questa configurazione e' valida solo per CustomBoard\n");
            ptr[act_client]->tipo_protocollo = 3 ;                // Segnalo protocollo GPIO
            ptr[act_client]->sav_handle = 0 ;                     // Handle a 0
            ptr[act_client]->canale = act_client ;                // Mi memorizzo l'indice del canale
            ksig = 0 ;                                      // Numero dei segnali dichiarati in impianto.conf
          }
          else {                                          // Non gpio => o seriale o IP
            for (linea_ser[0]=0,ii=0;ii<strlen(strappo);ii++) if (strappo[ii]=='/') strcpy(linea_ser,strappo);

            if (linea_ser[0]==0){                               // Socket TCP
//printf("Connessione TCP\n");
              ptr[act_client]->tipo_protocollo = 1 ;                // Segnalo protocollo socket Client IP
              valappo=atol(strappo);
              ptr[act_client]->port = valappo ;                   // Mi memorizzo la porta
              ptr[act_client]->canale = act_client ;                // Mi memorizzo l'indice del canale
              sprintf(stringa,"%s/Databit",nome_con);
              rw_strpar(strappo,stringa, "TCP" , READ_PAR | IMPIANTO_FILE ) ; // Server TCP 
              if(!strcmp(strappo,"UDP")) ii = 1 ; // Connessione tipo UDP
              else ii = 0 ; // Connessione tipo TCP
              sprintf(stringa,"%s/Baudrate",nome_con);
              rw_strpar(strappo,stringa, "ethernet" , READ_PAR | IMPIANTO_FILE ) ;
              InitChannel(ptr[act_client]->canale);
              if (!strcmp(strappo,"ethernet")){                    // Connessione come server
                ptr[act_client]->tipo_protocollo = 2 ;// Segnalo protocollo socket IP tipo server
                if(ii == 1) sd = openUDPSocket(valappo,prot); // Connessione tipo SERVER UDP
                else sd = openSocket(valappo,prot); // Connessione tipo SERVER TCP
			  }
              else {
                if(ii == 1) sd = openClientUDPSocket(valappo,prot,strappo); // Connessione tipo client UDP
				else sd = openClientSocket(valappo,prot,strappo); // Connessione tipo client TCP
			  }
              ptr[act_client]->sav_handle = sd ;                  // Mi memorizzo l'handle
              trace(__LINE__,__FILE__,4010,6,0,"Open socket TCPIP porta %ld - socket descriptor %d",valappo,sd);
            }
            else if (strstr(linea_ser, "spidev") != NULL) {       // Device SPI con driver spidev
              ptr[act_client]->tipo_protocollo = 4 ;              // Segnalo protocollo SPI
              sprintf(stringa,"%s/Baudrate",nome_con);          // Baudrate="clk20000000" contiene speed (Hz)
              rw_strpar(strappo,stringa, "clk1000000" , READ_PAR | IMPIANTO_FILE ) ;
              if (strstr(strappo, "clk") != NULL ) bd = atoi(&strappo[3]);
              sprintf(stringa,"%s/Databit",nome_con);           // Databit="cpha0" contiene clock phase
              rw_strpar(strappo,stringa, "cpha0" , READ_PAR | IMPIANTO_FILE ) ;
              if (strstr(strappo, "cpha") != NULL ) db = atoi(&strappo[4]);
              sprintf(stringa,"%s/Stopbit",nome_con);           // Stopbit="cpol0" contiene clock polarity
              rw_strpar(strappo,stringa, "cpol0" , READ_PAR | IMPIANTO_FILE ) ;
              if (strstr(strappo, "cpol") != NULL ) sb = atoi(&strappo[4]);
              sprintf(stringa,"%s/Parita",nome_con);          // Parita="csh0" contiene chip select active high
              rw_strpar(strappo,stringa, "csh0" , READ_PAR | IMPIANTO_FILE ) ;
              if (strstr(strappo, "csh") != NULL ) parita[0] = atoi(&strappo[3]);
              ptr[act_client]->canale = act_client ;                // Mi memorizzo l'indice del canale
              InitChannel(ptr[act_client]->canale);
              linea = openSPI(linea_ser,bd     ,db ,sb ,parita[0],prot);
              ptr[act_client]->sav_handle = linea ;                 // Mi memorizzo l'handle
              trace(__LINE__,__FILE__,4010,6,0,"Open su %s %d %d %d %d %d",linea_ser,bd,db,sb,parita[0],linea);
            }
            else {                                          // Linea seriale
//printf("Connessione Seriale\n");
              ptr[act_client]->tipo_protocollo = 0 ;                // Segnalo protocollo seriale
              sprintf(stringa,"%s/Databit",nome_con);
              db=rw_valpar(stringa, "8" , READ_PAR | IMPIANTO_FILE ) ;
              sprintf(stringa,"%s/Stopbit",nome_con);
              sb=rw_valpar(stringa, "1" , READ_PAR | IMPIANTO_FILE ) ;
              sprintf(stringa,"%s/Baudrate",nome_con);
              rw_strpar(strappo,stringa, "19200" , READ_PAR | IMPIANTO_FILE ) ;
              for (bd=B19200,ii=0;speed_table[ii][0]!=0;ii++) if (!strcmp(strappo,speed_table[ii])) bd= vs_table[ii];
              sprintf(stringa,"%s/Parita",nome_con);
              rw_strpar(parita,stringa, "n" , READ_PAR | IMPIANTO_FILE ) ;
              ptr[act_client]->canale = act_client ;                // Mi memorizzo l'indice del canale
              InitChannel(ptr[act_client]->canale);
              linea = openSeriale(linea_ser,bd     ,db ,sb ,parita[0],NO_LOW_PROT,prot); //NO_LOW_PROT
              ptr[act_client]->sav_handle = linea ;                 // Mi memorizzo l'handle
              trace(__LINE__,__FILE__,4010,6,0,"Open su %s %s %d %d %c %d",linea_ser,strappo,db,sb,parita[0],linea);
            }                                             // Chiusura tipo canale
          }
          trace(__LINE__,__FILE__,3010,0,0,"Handle %d, Protocollo %s, Memoria %X, Cliente %d",ptr[act_client]->sav_handle ,protocol_name[prot],ptr[act_client], act_client);                // Da un riferimento sull'avvio del servizio
          sprintf(stringa,"%s/Devices",nome_con);
          tot_dev_can = rw_valpar(stringa, "0" , READ_PAR | IMPIANTO_FILE ); // Dispositivi presenti su questa connessione
		  trace(__LINE__,__FILE__,3000,1,0,"Totale devices dichiarati %d sul canale %s canale n.%d",tot_dev_can,nome_con,act_client);
          if (tot_dev_can){ // Si > allora vado a parametrizzare i dispositivi su questo canale
            act_dev = 0 ;
            for(kdev=1;kdev<=tot_dev_can;kdev++) {
              sprintf(stringa,"%s/Device%d",nome_con,kdev);
              rw_strpar(nome_dev,stringa, "" , READ_PAR | IMPIANTO_FILE ) ;
              sprintf(stringa,"%s/Enable",nome_dev);
              valappo = rw_valpar(stringa, "0" , READ_PAR | IMPIANTO_FILE ); // Dispositivo abilitato
              if (valappo){                                   // Si > allora vado a parametrizzare il dispositivo
        		trace(__LINE__,__FILE__,3000,1,0,"Nome device %s abilitato",nome_dev);
                ptrDevice[tot_dev] =  malloc(sizeof (struct Device));   // Mi alloco la memoria per il dispositivo
                memset(ptrDevice[tot_dev],0,sizeof(struct Device));
                strcpy(ptrDevice[tot_dev]->nome_dev,nome_dev);
        		trace(__LINE__,__FILE__,3000,1,0,"Allocata memoria per device %s",nome_dev);
                sprintf(stringa,"%s/Protocollo",nome_dev);
                                    // Protocollo del dispositivo : Se non e' dichiarato lo imposto uguale al protocollo del canale
                rw_strpar(strappo,stringa, protocol_name[prot], READ_PAR | IMPIANTO_FILE ) ;
                for (kk=0,ii=0;protocol_name[ii][0]!=0;ii++) if (!strcmp(strappo,protocol_name[ii])) kk = ii;
                ptrDevice[tot_dev]->proto_device = kk ;
        trace(__LINE__,__FILE__,3000,1,0,"Protocollo device %s = %s",nome_dev,protocol_name[kk]);
// #########################################################################################################
// ASSEGNAZIONE INDICE SEQUENZIALE UNIQUE ID DEVICE PER IDENTIFICARE IL DEVICE ALL'INTERNO DELL'IMPIANTO
// #########################################################################################################
                ptrDevice[tot_dev]->unique_id = tot_dev ;             // Indice univoco del dispositivo
                ptr[act_client]->id_unique[act_dev][0] = tot_dev ;
                ptr[act_client]->tot_device ++ ;                  // Incremento il numero di dispositivi su questo canale
                ptrDevice[tot_dev]->canale = act_client ;             // Associo il dispositivo al canale
       			trace(__LINE__,__FILE__,3000,1,0,"UNIQUE ID = %d Device %d del canale %d",tot_dev,act_dev,act_client);

                sprintf(stringa,"%s/Modello",nome_dev);             // E' un EXE ?
                rw_strpar(strappo,stringa, "" , READ_PAR | IMPIANTO_FILE ) ; // Modello tipo del dispositivo
                if (!strcmp(strappo,"EXE")){
                  ptrDevice[tot_dev]->tot_res = 1 ;
                  sprintf(stringa,"%s/Indirizzo",nome_dev);
                  rw_strpar(ptrDevice[tot_dev]->indirizzo_dev,stringa, "0" , READ_PAR | IMPIANTO_FILE ) ; // Indirizzo del dispositivo
                  ptrDevice[tot_dev]->canale = act_client ;           // Associo il dispositivo al canale
                  strcpy(ptrDevice[tot_dev]->modello_dev,"EXE");
				  trace(__LINE__,__FILE__,3000,1,0,"\tDevice EXE : Address = %s \n", ptrDevice[tot_dev]->indirizzo_dev );
                }
                else if (!strcmp(strappo,"CUSTOM")){ 
                  sprintf(stringa,"%s/Resources",nome_dev);
                  tot_res = rw_valpar(stringa, "0" , READ_PAR | IMPIANTO_FILE ); // Risorse del dispositivo
				  trace(__LINE__,__FILE__,3000,1,0,"Totale risorse %d sul dispositivo %s\n",tot_res,nome_dev);
                  if (tot_res){ // Si > allora vado a parametrizzare le risorse del dispositivo
//.....................
                    InitChannel(ptr[act_client]->canale);                 // Inizializzo il canale
                  }
				}
                act_dev ++ ;                                  // Incremento il numero di dispositivi del canale dichiarati
                tot_dev ++ ;                                  // Incremento il numero di dispositivi totali dichiarati
              }                                             // Chiusura Enable DEVICE
            }                                               // Chiusura FOR Devices
            printf("Device abilitati totali %d sul canale %d\n", ptr[act_client]->tot_device, act_client  );
          }                                                 // Chiusura if Devices > 0
          act_client++ ;
        }                                                   // Chiusura Enable CONNECTION
    }                                                       // Chiusura for connections
  }                                                         // Chiusura if connections
  act_dev = 0 ;
  trace(__LINE__,__FILE__,3001,1,0,"\n-------------------------------------------------------- \n\n");
  trace(__LINE__,__FILE__,3001,0,0,"TOTALE CANALI ABILITATI %d - DISPOSITIVI ABILITATI NELL' IMPIANTO %d",act_client,tot_dev);

  /*
      PARAMETRIZZO IL POLLING DELLO SCHEDULER **********************************************
  */
/*
 * Disabilito scheduling per problemi con la read() del SPI

  plcpolltime = 1000 * TAU ;
  openScheduling(plcpolltime);                                // Avvio un programma a intervalli regolari di tempo pari a plcpolltime usec.

  trace(__LINE__,__FILE__,3001,0,0,"Scheduling impianto ogni %d usec",plcpolltime);
*/
  dbg_printf &= 0xFFFD ;

  for(act_dev=0;act_dev<tot_dev;act_dev++){                       // Lancio tutti gli eseguibili
    if (!strcmp(ptrDevice[act_dev]->modello_dev,"EXE")){
        trace(__LINE__,__FILE__,3000,1,0,"Lancio programma %s \n", ptrDevice[act_dev]->indirizzo_dev );
        strcpy(strappo,ptrDevice[act_dev]->indirizzo_dev);
        token = strtok(strappo, "/");                           // Separatore
        while (token) {                                     // continua fino a che trovi il separatore
          strcpy(ptrDevice[act_dev]->processo,token);
          token = strtok(NULL, "/");
        }
        sprintf(strappo,"%s&",ptrDevice[act_dev]->indirizzo_dev);
	    trace(__LINE__,__FILE__,3000,1,0,"Lancio %s\n",strappo);
        system(strappo); 									// Lancio l'eseguibile in background
    }
  }

  // check che la cartella di salvataggio immagini esista, altrimenti creala
  printf("Controllo che la cartella '%s' esista = ", DIR_IMG);
  struct stat dir_img_sb;
  if (stat(DIR_IMG, &dir_img_sb) == 0 && S_ISDIR(dir_img_sb.st_mode)) {
      printf("YES\n");
  } else {
      printf("NO\n");
      // printf("creating dir\n");
      if (mkdir(DIR_IMG, S_IRWXU) < 0){
        printf("Errore %d nella creazione della cartella : %s\n",errno,strerror(errno));
      }
      else {
        printf("Cartella '%s' creata\n", DIR_IMG);
      }
  }

  while(1){
    pthread_exit(0); // S'addormenta per sempre .......
//    msec_sleep(60000);                                      // S'addormenta per 1 minuto e poi ritorna qui .....
  }

}
/*====================================================================================================================
  ====================================================================================================================
  ====================================================================================================================
  ====================================================================================================================

                                             F I N E   D E L    M A I N

  ====================================================================================================================
  ====================================================================================================================
  ====================================================================================================================
  ====================================================================================================================*/


/********************************************************************************************************************
 *                                                                                                                  *
 *                                                        P L C                                                     *
 *                                                                                                                  *
 ********************************************************************************************************************/
void *func_wait(void *arg)
{
int *appo;
int milliseconds;
struct timeval tv;
int ret,timeout;

  appo= (int *) arg;
  milliseconds = *(appo+coda_wait);

  tv.tv_sec = (int)(milliseconds/1000);
  tv.tv_usec = (int)((milliseconds%1000) * 1000);
// printf("attesa %d.%d\n",tv.tv_sec,tv.tv_usec);
  timeout = milliseconds/TAU ;
  do {                                  // Lascia le risorse per fare altro e rientra dopo il tempo di timeout......
//  printf("tout %d attesa %d.%d in coda %d \n",timeout,tv.tv_sec,tv.tv_usec,coda_wait);
    ret = select(1,NULL,NULL,NULL,&tv);
    timeout -- ;
  }
  while((ret == -1) && (errno == EINTR) && timeout>0);
}

void msec_sleep(int milliseconds) // Aspetto un po' di tempo lasciando fare altro alla CPU
{
pthread_t child;

       if (coda_wait > 8) coda_wait = -1 ;                // Gestisce un buffer circolare di 10 richieste in rapida successione
       param_wait[++coda_wait] = milliseconds ;
       if ( pthread_create(&child, NULL, func_wait, &param_wait) != 0 ) // Creo il thread che non verra' disturbato dall'interrupt del TAU del PLC
       {
         trace(__LINE__,__FILE__,1000,0,0,"Pthread creation error: %d : %s",errno,strerror(errno));
       }
       pthread_join(child,NULL );                     // E aspetto la morte del thread lasciando libera la CPU
}

/*----------------------- Lancio scheduling dell'impianto ----------------------*/
int openScheduling(unsigned long period)
{
struct itimerval periodo;

    bzero(&saplc, sizeof(saplc));
    saplc.sa_handler = plc;
    saplc.sa_flags  = SA_RESTART ;
    if ( sigaction(SIGALRM, &saplc, 0) < 0 ){
        trace(__LINE__,__FILE__,1000,0,0,"SigAlarm per Scheduling macchina fail %d : %s",errno,strerror(errno));
    }
    periodo.it_value.tv_sec = 0 ;                               /* Parte fra 0 secondi */
    periodo.it_value.tv_usec = 100000 ;                         /* e 100 msec */
    periodo.it_interval.tv_sec = 0 ;                            /* Con una cadenza di 0 secondi */
    periodo.it_interval.tv_usec = period ;                        /* e period microsecondi = period/100 msec */
// Segnali ITIMER_REAL ne puo' esistere solamente UNO , all'interno della funzione si può contare la base dei tempi e fare altri scheduling
    setitimer( ITIMER_REAL, &periodo, NULL );                     // Faccio partire il timer REAL TIME
    return(0);
}

/*----------------------- Funzione di controllo impianto --------------------------------*/
void plc(int signum)
{
long int count=0;
int ii,kk ;

    TAU_PLC ++ ;                                          // Incremento ad ogni tau
//printf("%ld\n",TAU_PLC);
    if(rimba_plc == 3 ) return ;                              // Richiedo al plc di stare fermo

    if(rimba_plc==0) {
    rimba_plc = 1 ;                                         // Antiripetizione sulla gestione della logica
/* ------------------------------------------------------------------ */
/* Cose da fare al primo giro (INIZIALIZZAZIONE) */
      if (!power_on){
        power_on = 1 ;                                      // Primo giro da effettuare

        init_plc(1);                                        // Configura l'impianto power on

        for(ii=1,kk=0;kk<act_client;kk++) {                 // Inizializzo i timer in modo che lavorino uno alla volta e non tutti sullo stesso TAU
          if(ptr[kk]->polcan>0){                            // Se negativo vuol dire che va ad eventi con il gestore dello smaltimento di code
            ptr[kk]-> polcan = ptr[kk]-> polcan / plcpolltime ;     // Riporto i tempi di polling sui canali da usec a TAU
            if ( ptr[kk]->polcan == 1)                  // Se timer uguali a TAU li faccio partire allineati a 0
                 ptr[kk]->timer = 0 ;
            else ptr[kk]->timer = ii++ ;                /* altrimenti sfalzo i TAU in cui vengono gestiti (avanti di uno)  */
          }
printf("canale %d timer = %d polcan in TAU = %d\n",kk,ptr[kk]->timer,ptr[kk]->polcan );
        }
        power_on = 2; // Primo giro effettuato
      }
/* ------------------------------------------------------------------ */
/* Cose da fare ad ogni TAU PLC */
      logica_plc() ;
      rimba_plc =  2 ;
    }
    else tau_persi_plc();

/* ------------------------------------------------------------------ */
/* INCREmento i contatori di polling e i timer del plc */

    timer_sec ++ ;

    timer_plc() ;

/* ------------------------------------------------------------------ */
/*            Polling canali                  */
    for(kk=0;kk<act_client;kk++){
//printf("canale %d timer = %d polcan = %d\n",kk,ptr[kk]->timer,ptr[kk]->polcan );
      if (ptr[kk]->polcan <= 0 ) continue;                // Salto il polling per quelli che vanno a evento(-1) o non vogliono il polling(0)
      if (ptr[kk]->timer >= ptr[kk]->polcan ){              // xx msec
            ptr[kk]->timer = 1 ;                    // Riparto da 1 altrimenti perdo un TAU se passo da 0
                                    // ######################   Polling TCP SOCKET
      if ( ptr[kk]->tipo_protocollo == 2 || ptr[kk]->tipo_protocollo == 1 ){ /* E' un canale TCP che può avere piu' clienti  */
// trace(__LINE__,__FILE__,3000,1,0,"canale IP %d polcan = %d TAU numero %ld",kk,ptr[kk]->polcan,TAU_PLC );
                                    // Gestione delle code verso i clienti connessi su SOCKET
                                    // Devo svuotare una coda di messaggi ? Assegno direttamente perche' se e' 0 va bene il reset se != 0 svuoto

//trace(__LINE__,__FILE__,3000,1,0,"count= %d ,ptr[%d]->coda_socket[0]=%d (%X)",count,kk,ptr[kk]->coda_socket[0],ptr);
          if( count = ptr[kk]->coda_socket[0] ) {
             for(ii=0;ii<(sizeof(ptr[kk]->coda_socket)/sizeof(int))-2;ii++) ptr[kk]->coda_socket[ii] = ptr[kk]->coda_socket[ii+1]; // Copio il 2 nel 1, il 3 nel 2, ..etc
             ptr[kk]->coda_socket[ii] = 0 ;               // Azzero l'ultimo
             richiedi_socket(count,kk,0) ;                // Faccio la prima richiesta della coda
             count = 0 ;
          }
//printf("COUNT=%d , sok[ii: %d ] [count: %d] = %d\n",count,ii,count,sok[ii][count]);
//          ii = ptr[kk]->canale ;
            for(count=0;count<MAXCONN;count++) {
              if ( (count<MAXCONN) && (ptr[kk]->sok[count]>0) ) {     // GESTISCO IL COLLOQUIO SUI SOCKET TCP APERTI e CONNESSI
//printf("Invio comando socket %d comando = %d - count =%d - kk = %d\n",ptr[kk]->sok[count],ptr[kk]->cmd_sok[count],count,kk);
                SendPollCmd(kk,count,ptr[kk]->cmd_sok[count]);      // Canale,connessione,comando
                ptr[kk]->cmd_sok[count]=0;                // Resetto eventuali richieste particolari di invio
              }
            }
        }
        else {                              // ########################   Polling su seriali
//trace(__LINE__,__FILE__,3000,1,0,"Invio comando seriale %d",ptr[kk]->sav_handle);
//trace(__LINE__,__FILE__,3000,1,0,"canale seriale %d polcan = %d TAU numero %ld",kk,ptr[kk]->polcan,TAU_PLC );

           if ( ptr[kk]->sav_handle>=0 ) SendPollCmd(ptr[kk]->sav_handle,kk, 0);
        }
      }
      else ptr[kk]->timer ++ ;                      // INCREMENTO I TIMER DI POLLING DEI CANALI ATTIVI
    }
/* ------------------------------------------------------------------ */
/*        Cose da fare ogni Secondo                 */
    if (timer_sec >= (1000000/plcpolltime) ){               /* 1000 msec  */
      timer_sec = 0 ;
      lampeggio = lampeggio ^ 1 ;
      lamp_vel = lamp_vel ^ 1 ;
      timer_sec_plc() ;                         // Lancio i timer del plc che devono essere serviti ogni secondo
    }
/* ------------------------------------------------------------------ */
/*        Cose da fare ogni Mezzo secondo             */
    if (timer_sec == (500000/plcpolltime) ){ /* 500 msec  */
      lamp_vel = lamp_vel ^ 1 ;
    }

    if( rimba_plc == 2 ) rimba_plc = 0 ;                // Finita l'esecuzione del PLC ritorna in attesa della sveglia a interrupt real time di sistema.

/* ------------------------------------------------------------------ */
}

int DammiGPIObit(char *nome_sig)                    // Funzione di servizio che restituisce un indice in base ad un nome ASCII del Segnale definito nelle variabili utilizzate
{
 int kk;
  for(kk=0;kk<32;kk++) if ( !strcmp(nome_sig,gpio_sgn[kk]) ) return(kk) ;
  return(33);
}
/***********************************************************************
 *                                                                     *
 *                         M E M O R Y                                 *
 *                                                                     *
 * ################################################################### *
 * ################################################################### *
 * #############                                        ############## *
 * ############# Funzione per la creazione          ############## *
 * ############# di aree condivise                  ############## *
 * ############# Nome segmento                      ############## *
 * ############# Size area memoria                  ############## *
 * ############# Ritorna puntatore                  ############## *
 * #############                                    ############## *
 * ################################################################### *
 * ################################################################### *
*/
char * memory(int mem_name,int mem_size)
{
int shmid;
key_t key;
char *shm;

    key = (key_t) mem_name;
    if ((shmid = shmget(key, mem_size, IPC_CREAT | 0666)) < 0) {
            trace(__LINE__,__FILE__,3010,0,0,"Shared memory Get error %d",errno);
            return((char *)(-1) ) ;
    }
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
       trace(__LINE__,__FILE__,3010,0,0,"Shared memory Attached error %d",errno);
       return((char *)(-1)) ;
    }
    memset(shm,0,mem_size); // Inizializzo la memoria a 0
    return(shm) ;
}
/***********************************************************************
 *                                                                     *
 *                         S E R I A L E                               *
 *                                                                     *
 **********************************************************************/
/******************* INCLUDE APPLICATION SECTION **********************/


/************* PROTOTYPE PROTOCOL FUNCTION REFERENCES SECTION *********/
static int param[2];                               /* Parametri da passare al thread : ser_d - protocollo software */
                                    // [0] = cliente
                                    // [1] = 2 : Protocollo (per es. MODBUS_PROTOCOL)
/*************** PROTOTYPE FUNCTION DECLARATIONS SECTION **************/
/*
 * Server della linea seriale.
 * Cattura i caratteri ricevuti dalla linea
 * e chiama l'interprete dei comandi
 */
void *SerialLine (void* arg)
{
int bytes_read,kk;
int *appo,esci,last_error;
int  cliente,protocollo;
int (*InterpretaProtocollo)(struct connectManage *, unsigned char * , int, int);
struct connectManage *  ptr_act ;

    appo= (int *) arg;
    cliente = *appo++;
    protocollo = *appo;
    InterpretaProtocollo = dammi_protocollo(protocollo);

    ptr_act =  (struct connectManage *) malloc(sizeof (struct connectManage)); /* Mi alloco la memoria per i buffer di TX/RX */

                                    // Cerco i parametri da copiare nella struttura del nuovo cliente

// printf("Trovato su canale %d[%d] con campionamento = %d\n",sd,ptr_act->canale,ptr_act->polcan);

    for (kk=0;kk<=act_client;kk++){
// trace(__LINE__,__FILE__,3000,1,0,"sav_handle[%d]=%d == cliente=%d\n",kk,ptr[kk]->sav_handle,cliente);
      if(ptr[kk]->sav_handle==cliente){                 // Client gia' registrato posso onorare l'interpretazione dei caratteri
         ptr_act->canale = ptr[kk]->canale ;                // Copio l'indice del canale di colloquio
         ptr_act->polcan = ptr[kk]->polcan ;                // Copio il tempo di campionamento del canale
         ptr_act->sav_handle = cliente ;                    // Copio l'id client nella struttura di gestione
         ptr[kk]->Connect[0] = ptr_act ;                  // Copio l'indirizzo della struttura della singola connessione nella struttura di gestione (la seriale ne ha una sola di connessioni)
      }
    }
// printf("SERIAL : Trovato su canale %d[%d] con campionamento = %d memoria strutt %X\n",sd,ptr_act->canale,ptr_act->polcan,ptr_act);
    InitConnection(ptr_act->canale,ptr_act);

    esci = 1;
    last_error = 0 ;
    do {
      bytes_read = read(cliente, ptr_act -> result, sizeof(ptr_act -> result)); /* Legge continuamente la linea seriale */
      if (bytes_read > 0){
        if (last_error ){
          trace(__LINE__,__FILE__,3000,0,0,"Reset Serial line %d, protocollo %s Mem.(%X)\n",cliente,protocol_name[protocollo],(ptr_act ));
          last_error = 0 ; /* reset segnalazione errore uguale */
        }
        InterpretaProtocollo(ptr_act, ptr_act -> result, bytes_read, ptr_act->canale) ;
      }
      else {
        if (last_error != errno && errno != 11) {           // Segnala il primo errore di questo tipo poi smette fino a che non se ne verificano di differenti
          trace(__LINE__,__FILE__,1000,0,0,"Error Serial line %d, protocollo %s read error %d = %s,Mem(%X)",cliente,protocol_name[protocollo],errno,strerror(errno),(ptr_act));
          last_error = errno ;
        }
      }
    }
    while (esci);
    trace(__LINE__,__FILE__,1000,0,0,"Chiusura della linea seriale %d",cliente);
    close(cliente);
    return arg;
}


void *SPILine (void* arg)
{
int bytes_read,kk;
int *appo,esci,last_error;
int  cliente,protocollo;
int (*InterpretaProtocollo)(struct connectManage *, unsigned char * , int, int);
struct connectManage *  ptr_act ;

    appo= (int *) arg;
    cliente = *appo++;
    protocollo = *appo;

    InterpretaProtocollo = dammi_protocollo(protocollo);

    ptr_act =  (struct connectManage *) malloc(sizeof (struct connectManage)); /* Mi alloco la memoria per i buffer di TX/RX */

                                    // Cerco i parametri da copiare nella struttura del nuovo cliente

// printf("Trovato su canale %d[%d] con campionamento = %d\n",sd,ptr_act->canale,ptr_act->polcan);

    for (kk=0;kk<=act_client;kk++){
// trace(__LINE__,__FILE__,3000,1,0,"sav_handle[%d]=%d == cliente=%d\n",kk,ptr[kk]->sav_handle,cliente);
      if(ptr[kk] != NULL && ptr[kk]->sav_handle==cliente){                 // Client gia' registrato posso onorare l'interpretazione dei caratteri
         ptr_act->canale = ptr[kk]->canale ;                // Copio l'indice del canale di colloquio
         ptr_act->polcan = ptr[kk]->polcan ;                // Copio il tempo di campionamento del canale
         ptr_act->sav_handle = cliente ;                    // Copio l'id client nella struttura di gestione
         ptr[kk]->Connect[0] = ptr_act ;                  // Copio l'indirizzo della struttura della singola connessione nella struttura di gestione (la seriale ne ha una sola di connessioni)
      }
    }
// printf("SERIAL : Trovato su canale %d[%d] con campionamento = %d memoria strutt %X\n",sd,ptr_act->canale,ptr_act->polcan,ptr_act);
    InitConnection(ptr_act->canale,ptr_act);

    if (protocollo != ISO2110S) {
      printf("line %d, protocollo %s : ignoro lettura su un canale di sola scrittura\n",cliente,protocol_name[protocollo]);
      pthread_exit(0);
    }

    esci = 1;
    last_error = 0 ;
    unsigned char npl_packet[SPI_PROTO_NPL_SIZE];
    do {
      bytes_read = read(cliente, npl_packet, SPI_PROTO_NPL_SIZE); /* Legge continuamente la linea seriale */
      if (bytes_read > 0){
        if (last_error ){
          trace(__LINE__,__FILE__,3000,0,0,"Reset SPI line %d, protocollo %s Mem.(%X)\n",cliente,protocol_name[protocollo],(ptr_act ));
          last_error = 0 ; /* reset segnalazione errore uguale */
        }
        if (npl_packet[0] == SPI_PROTO_NPL_ID) {
          // ricevuto pacchetto Next Packet Length
          int length = npl_packet[1];
          printf("line %d, protocollo %s : ricevuto packet length = %d\n",cliente,protocol_name[protocollo], length);
          // ricevi pacchetto
          bytes_read = read(cliente, ptr_act -> result, length);
          if (bytes_read > 0) {
            printf("line %d, protocollo %s : recv >",cliente,protocol_name[protocollo]);
            for (int i=0; i<length; i++) printf(" %.2X", ptr_act->result[i]);
            printf("\n");
            InterpretaProtocollo(ptr_act, ptr_act -> result, bytes_read, ptr_act->canale) ;
          }
          else {
            if (last_error != errno && errno != 11) {           // Segnala il primo errore di questo tipo poi smette fino a che non se ne verificano di differenti
              trace(__LINE__,__FILE__,1000,0,0,"Error SPI line %d, protocollo %s read data packet error %d = %s,Mem(%X)",cliente,protocol_name[protocollo],errno,strerror(errno),(ptr_act));
              last_error = errno ;
            }
          }
        }
        else {
          printf("line %d, protocollo %s : unknown packet %.2X %.2X\n",cliente,protocol_name[protocollo], ptr_act->result[0], ptr_act->result[1]);
        }
      }
      else {
        if (last_error != errno && errno != 11) {           // Segnala il primo errore di questo tipo poi smette fino a che non se ne verificano di differenti
          trace(__LINE__,__FILE__,1000,0,0,"Error SPI line %d, protocollo %s read error %d = %s,Mem(%X)",cliente,protocol_name[protocollo],errno,strerror(errno),(ptr_act));
          last_error = errno ;
        }
      }
    }
    while (esci);
    trace(__LINE__,__FILE__,1000,0,0,"Chiusura della linea SPI %d",cliente);
    close(cliente);
    return arg;
}


/*
 * Apre la seriale con i parametri passati
 * Lancia un thread che gestisce la comunicazione sulla linea seriale
 * Ritorna il filedescriptor della linea se si ha bisogno comunque di
 * accederci direttamente.
 * Parametri:
 * Nome della linea, BaudRate, DataBit, StopBit, Parita', LowProtocol, Protocollo
 * dove:
 *    LowProtocol indica se e' attivo un handshake CTS-RTS o XonXoff o nessuno
 *    Protocollo e' il livello logico delle informazioni quindi MDOBUS o ....
 */

int openSeriale(char * line ,speed_t speed,int data,int stop,char parity,int low_proto,int sw_proto)
{
struct termios port;
pthread_t child;
int ser_d;
long DATABITS;
long STOPBITS;
long PARITYON;
long PARITY;
char appo[200];
int HardFlow ;
int SoftFlow ;

/*             Open serial interface, device                          */
  sprintf(appo,"sudo chmod 777 %s\n",line);
  system(appo);
//printf("%s\n",appo);
//sleep(5);

      SoftFlow = 0 ;
      HardFlow = 0 ;
      switch (low_proto) {
         case 1:        // Controllo Xon Xoff
            SoftFlow = IXON | IXOFF ;
            port.c_cc[VSTART] = 17 ;  //DC1;
            port.c_cc[VSTOP]  = 19 ;  //DC3;
         break;
         case 2:        // Controllo RTS CTS
            HardFlow = CRTSCTS;
         break;
      }
      switch (data)
      {
         case 8:
         default:
            DATABITS = CS8;
         break;
         case 7:
            DATABITS = CS7;
         break;
         case 6:
            DATABITS = CS6;
         break;
         case 5:
            DATABITS = CS5;
         break;
      }  //end of switch data_bits
      switch (stop)
      {
         case 1:
         default:
            STOPBITS = 0;
            break;
         case 2:
            STOPBITS = CSTOPB;
            break;
      }  //end of switch stop bits
      switch (parity)
      {
         case 'n':
         case 'N':
         default:                       //none
            PARITYON = 0;
            PARITY = 0;
            break;
         case 'o':                        //odd
         case 'd':                        //dispari
         case 'O':                        //dispari
         case 'D':                        //dispari
            PARITYON = PARENB;
            PARITY = PARODD;
            break;
         case 'e':                        //even
         case 'p':                        //pari
         case 'E':                        //even
         case 'P':                        //pari
            PARITYON = PARENB;
            PARITY = 0;
            break;
      }  //end of switch parity
      ptr[act_client]->sav_handle= open(line, O_RDWR | O_NOCTTY ); //  | O_NONBLOCK );
      ser_d = ptr[act_client]->sav_handle;
//printf("Open %d\n",ser_d);
      if(ser_d<=0){
        trace(__LINE__,__FILE__,1000,0,0,"Error open line %s Ret:%d - errno %d : %s",line,ser_d,errno,strerror(errno));
      }
      else
      {

/* Stampa dei valori attuali
    HardFlow = tcgetattr (ser_d, &port);
    if (HardFlow<0 ) printf("Errore nella get parametri\n");
    printf("c_cflag=%X\n",port.c_cflag);
    printf("c_iflag=%X\n",port.c_iflag);
    printf("c_oflag=%X\n",port.c_oflag);
    printf("c_lflag=%X\n",port.c_lflag);
    printf("c_cc[VMIN]=%X\n",port.c_cc[VMIN]);
    printf("c_cc[VTIME]=%X\n",port.c_cc[VTIME]);
    printf("c_line=%X\n",port.c_line);
    printf("Speed IN =%X\n",cfgetispeed(&port));
    printf("Speed OUT =%X\n",cfgetospeed(&port));
*/

    tcgetattr (ser_d, &port); // Carico tutti i parametri attuali

  // resetto le opzioni di default

    port.c_cflag &= ~(CSIZE | CSTOPB | PARENB | PARODD | CRTSCTS);
    port.c_iflag &= ~( INPCK | ISTRIP | IGNPAR | IXON | IXOFF | IXANY | ICRNL | INLCR );
    port.c_lflag &= ~( ICANON | ECHO | ECHOE | ISIG );

    port.c_cflag = DATABITS | PARITYON | PARITY | STOPBITS | CLOCAL | CREAD | HUPCL | HardFlow;

    port.c_iflag = 0;                           // Richiedo una lettura binaria in modo che non venga filtrato nessun carattere dal driver

    port.c_iflag |= SoftFlow;                       // Xon-Xoff se richiesto

    if ( PARITYON == PARENB ) port.c_iflag |= INPCK;          // Parita'
    else                  port.c_iflag |= IGNPAR;

    port.c_oflag = 0;                           // Scrittura binaria, nessuna interpretazione del driver...
    port.c_lflag = 0;                           // disattiva il canonical input
    port.c_cc[VMIN] = 1;                        //  block until 1 character, i.e. don't block
    port.c_cc[VTIME] = 1;                         // don't use inter-character timer
    port.c_cc[VSTART]=17;                         //DC1;
    port.c_cc[VSTOP]=19;                          //DC3;
    port.c_line = 0;                          // line discipline: TTY

    cfsetispeed(&port,speed);
    cfsetospeed(&port,speed);

    tcflush(ser_d, TCIFLUSH);                     // Pulisci il canale

    if ( tcsetattr(ser_d, TCSANOW, &port) == -1) {            // attiva new settings
      trace(__LINE__,__FILE__,1000,0,0,"Errore nel settaggio parametri della linea = %d",ser_d);  // Messaggio di trace
    }

    param[0] = ser_d;
    param[1] = sw_proto;

    trace(__LINE__,__FILE__,1000,0,0,"Creato il thread della linea seriale - serial line descriptor = %d with Protocol %s",ser_d,protocol_name[sw_proto]);// Messaggio di trace

    if ( pthread_create(&child, NULL, SerialLine, &param) != 0 )
    {
       trace(__LINE__,__FILE__,1000,0,0,"Pthread creation error: %d - %s",errno,strerror(errno));/* Messaggio di errore */
    }
    else pthread_detach(child);
  }
  return(ser_d);
}


int openSPI(char * line ,int clk,int cpha,int cpol,int csh, int sw_proto)
{
  pthread_t child;
  int spi_d;
  uint8_t mode;
  uint8_t bits = 8;
  int ret = 0;
  int chk_mode  = 0;
  int chk_bits  = 0;
  int chk_clock = 0;

/*             Open and Setup SPI interface                          */
  if (cpha) mode |= SPI_CPHA;
  if (cpol) mode |= SPI_CPOL;
  if (csh)  mode |= SPI_CS_HIGH;

  ptr[act_client]->sav_handle= open(line, O_RDWR);
  spi_d = ptr[act_client]->sav_handle; 
  
  if(spi_d<=0){
        trace(__LINE__,__FILE__,1000,0,0,"Error open line %s Ret:%d - errno %d : %s",line,spi_d,errno,strerror(errno));
      }
      else
      {
/*
   if ( fcntl(sok_d, F_SETFL, O_ASYNC | O_NONBLOCK) < 0 )  {
     trace(__LINE__,__FILE__,livello,7,0,"Asincronous fail %d : %s",errno,strerror(errno));
   }
*/

        // spi mode
        ret = ioctl(spi_d, SPI_IOC_WR_MODE, &mode);
        if (ret == -1) {
          trace(__LINE__,__FILE__,1000,0,0,"Errore nel settaggio spi mode della linea = %d",spi_d);  // Messaggio di trace
        }

        // bits per word
        ret = ioctl(spi_d, SPI_IOC_WR_BITS_PER_WORD, &bits);
        if (ret == -1) {
          trace(__LINE__,__FILE__,1000,0,0,"Errore nel settaggio spi bits per word della linea = %d",spi_d);  // Messaggio di trace
        }

         // max speed hz
        ret = ioctl(spi_d, SPI_IOC_WR_MAX_SPEED_HZ, &clk);
        if (ret == -1) {
          trace(__LINE__,__FILE__,1000,0,0,"Errore nel settaggio spi max speed (Hz) della linea = %d",spi_d);  // Messaggio di trace
        }


        // ----- check spi settings -------

        ret = ioctl(spi_d, SPI_IOC_RD_MODE, &chk_mode);
        if (ret == -1) {
          trace(__LINE__,__FILE__,1000,0,0,"Errore nella lettura spi mode della linea = %d",spi_d);  // Messaggio di trace
        }

        ret = ioctl(spi_d, SPI_IOC_RD_BITS_PER_WORD, &chk_bits);
        if (ret == -1) {
          trace(__LINE__,__FILE__,1000,0,0,"Errore nella lettura spi bits per word della linea = %d",spi_d);  // Messaggio di trace
        }

        ret = ioctl(spi_d, SPI_IOC_RD_MAX_SPEED_HZ, &chk_clock);
        if (ret == -1) {
          trace(__LINE__,__FILE__,1000,0,0,"Errore nella lettura spi max speed (Hz) della linea = %d",spi_d);  // Messaggio di trace
        }

        printf("%s: spi mode 0x%x, %d bits per word, %d Hz max\n", line, chk_mode, chk_bits, chk_clock);

      param[0] = spi_d;
      param[1] = sw_proto;

      trace(__LINE__,__FILE__,1000,0,0,"Creato il thread della linea SPI - line descriptor = %d with Protocol %s",spi_d,protocol_name[sw_proto]);// Messaggio di trace

      if ( pthread_create(&child, NULL, SPILine, &param) != 0 )
      {
        trace(__LINE__,__FILE__,1000,0,0,"Pthread creation error: %d - %s",errno,strerror(errno));/* Messaggio di errore */
      }
      else pthread_detach(child);
  }
  return(spi_d);
}


/***********************************************************************
 *                                                                     *
 *                                                                     *
 *                                                                     *
 *                                                                     *
 *                         S O C K E T                                 *
 *                                                                     *
 *                                                                     *
 *                                                                     *
 *                                                                     *
 *                                                                     *
***********************************************************************/

/*********** PROTOTYPE PROTOCOL FUNCTION REFERENCES SECTION ***********/
// int client[MAXCONN][8] // Spostata nella struttura del protoManage
// [MAXCONN][0] = 4 : cliente (e' un progressivo in base agli host connessi)
// [MAXCONN][1] = 32312 : porta di comunicazione
// [MAXCONN][2] = 127. :IP Address
// [MAXCONN][3] = 0.
// [MAXCONN][4] = 0.
// [MAXCONN][5] = 1
// [MAXCONN][6] = 9 : protocollo (TCP_MODBUS)
// [MAXCONN][7] = x : socket server a cui e' stato risposto

/******************** GLOBAL VARIABLE SECTION *************************/

/********************** DEVELOP FUNCTION SECTION **********************/
/*-------------------------------------------------------------------*/
/*--- Server del socket sulla porta di ascolto                    ---*/
/*-------------------------------------------------------------------
 * Questo e' il programma di gestione delle richieste che vengono dai
 * client sulla porta associata.
 * Gestisce un dialogo "a domanda rispondo" e poi la chiusura del socket
 * da parte del client.
 * Attualmente non e' prevista la chiusura per inutilizzo
 * (quindi con un timeout)
 */
void* SocketLine(void* arg)
{
int bytes_read,kk,ii;
int *appo,esci,last_error=0;
int add1 , add2 , add3 , add4 , cliente , port, protocollo, sd ;
int (*InterpretaProtocollo)(struct connectManage *, unsigned char * , int , int );
struct connectManage * ptr_act ;

    appo        = (int *)arg;
    cliente     = *appo++;                          					// [0] = cliente
    port        = *appo++;                            					// [1] = porta di comunicazione dinamica in base alla connessione
    add1        = *appo++;                            					// [2] = 127.
    add2        = *appo++;                            					// [3] = 0.
    add3        = *appo++;                            					// [4] = 0.
    add4        = *appo++;                            					// [5] = 1
    protocollo  = *appo++;                        						// [6] = MODBUS_PROTOCOL : 2
    sd          = *appo;                            					// [7] = Canale server : x

    InterpretaProtocollo = dammi_protocollo(protocollo);

    ptr_act =  (struct connectManage *) malloc(sizeof (struct connectManage)); /* Mi alloco la memoria per i buffer di TX/RX */

    // Cerco i parametri da copiare nella struttura del nuovo cliente
    ptr_act->canale = ptr[sd]->canale ;                 // Copio l'indice del canale di colloquio
    ptr_act->polcan = ptr[sd]->polcan ;                 // Copio il tempo di campionamento del canale
    ptr_act->client = cliente ;                         // Copio l'id client nella struttura di gestione
    ptr_act->sav_handle = cliente ;
    for(kk=0;kk<MAXCONN;kk++){                        // Copio l'indirizzo della struttura della connessione nella struttura del canale
      if ( ptr[sd]->sok[kk] == cliente ){
        ptr[sd]->Connect[kk]=ptr_act;
        break;
      }
    }
printf("SOCKET: Trovato su canale %d[%d] con campionamento = %d memoria strutt %X\n",sd,ptr_act->canale,ptr_act->polcan,ptr_act);
    InitConnection(sd,ptr_act);
    esci = 0;
    do {
// printf("Riattendo un pacchetto\n");
        bytes_read = recv(cliente,  ptr_act -> result, sizeof(ptr_act -> result), 0); // Leggo al massimo a pacchetti di sizeof[result]
        if (bytes_read > 0)
        {
          if (last_error ) {
            trace(__LINE__,__FILE__,1000,1,0,"Reset last error %d IP line %d, protocollo %s read error %d = %s,Mem(%X)",last_error,cliente,protocol_name[protocollo],errno,strerror(errno),(ptr_act));
            last_error = 0 ;
          }
/* Log dei caratteri ricevuti (solo per debug) * /
          trace(__LINE__,__FILE__,1000,1,0,"Protocollo %d su TCPIP %d sul canale %d cliente %d",protocollo,bytes_read,ptr_act->canale,cliente);
          trace(__LINE__,__FILE__,1000,0,0,"Ricevuti %d caratteri = %s",bytes_read,ptr_act -> result);
/* */
          esci = InterpretaProtocollo(ptr_act, ptr_act->result, bytes_read, ptr_act->canale) ;
        }
        else {                              // Ricevuto un errore sulla lettura del canale: devo capire che errore e' o se e' una chiusura di connessione
          if (last_error != errno && errno != 11) {
            trace(__LINE__,__FILE__,1000,0,0,"Error IP line %d, protocollo %s read error %d = %s,Mem(%X)",cliente,protocol_name[protocollo],errno,strerror(errno),(ptr_act));
            last_error = errno ;
            if(errno == 104) esci = 1 ;                 // Connessione resettata dal punto
          }
          else if( !errno || last_error == errno ) esci = 1 ;     // Se ricevo 0 caratteri => perdita di connessione => uscita
        }
    }
    while(!esci);
//printf("Chiusura dal client %d.%d.%d.%d con port: %d\n",add1,add2,add3,add4,ntohs(port));

    trace(__LINE__,__FILE__,1000,0,0,"Chiusura dal client %d.%d.%d.%d con port: %d",add1,add2,add3,add4,ntohs(port));
    for(ii=0;ii<act_client;ii++){
      for(kk=0;kk<MAXCONN;kk++){
        if ( ptr[ii]->sok[kk] == cliente ){
            ptr[ii]->sok[kk]=0;
            ptr[ii]->Connect[kk]=(struct connectManage *) NULL ; // Cancello l'indirizzo della memoria
            ptr[ii]->tot_sok_act --;
trace(__LINE__,__FILE__,1000,1,0,"Cancellato ptr[%d]->sok[%d]=%d totale %d cliente %d",ii,kk,ptr[ii]->sok[kk],ptr[ii]->tot_sok_act,cliente);
            kk = MAXCONN+1;
        }
      }
    }
    free(ptr_act); // Libero la memoria per lo scambio dati
    close(cliente);
    return arg;
}
/*--------------------------------------------------------------------*/
/*--- Signal capture: SIGIO - connect to client.                   ---*/
/*--------------------------------------------------------------------
 * Una volta catturata la richiesta di IO si accetta la richiesta solo
 * se non si e' raggiunto il max numero di connessioni disponibili.
 * Si crea un thread che servirà le richieste del client che ha fatto
 * richiesta di connessione. Si "stacchera'" il figlio dal padre il quale
 * (figlio) vivra' finche' resta alta la connessione dopodiche' morira'
 * lasciando libero il posto per un altra connessione... il padre continuera'
 * ad aspettare altre richieste di connessione e quindi generando tanti
 * figli quante sono le richieste fino ad un massimo di MAXCONN.
  siginfo_t :
  int si_signo;
  int si_code;
  union sigval si_value;
  int si_errno;
  pid_t si_pid;
  uid_t si_uid;
  void *si_addr;
  int si_status;
  int si_band;
 */

void sig_io(int signo,siginfo_t *info, void * context)
{

int kk,jj,ii,fin,sok_d;
char appo[20],appo1[20];
pthread_t child;
int kk_can ,tot_ana_can ;

   kk_can = 0 ;
   tot_ana_can = 0 ;

prova_altra_porta:
   while(ptr[kk_can]->tipo_protocollo != 2 ) kk_can ++ ;        // Cerco canale IP server

   if (ptr[kk_can]->tot_sok_act < MAXCONN){
   kk = 0 ;
   while(ptr[kk_can]->sok[kk]!=0 && kk<MAXCONN) kk ++ ;
   client_len=sizeof(client_addr);                            // Dimensione dell'indirizzo client

   /* ******************************************
    * Cerco nell'elenco delle porte in ascolto *
    * ******************************************/

   if (tot_ana_can <tot_canali_act) sok_d = ptr[kk_can]->sav_handle ;
   else {
      trace(__LINE__,__FILE__,1,0,0,"Connessione per la porta richiesta non attiva");
      return ;
   }
   trace(__LINE__,__FILE__,1,0,0,"Ha richiesto una accept il socket descriptor %d",sok_d);

   ptr[kk_can]->sok[kk] = accept(sok_d, (struct sockaddr*)&client_addr, &client_len);
   trace(__LINE__,__FILE__,1,1,0,"Risposta accept %d al canale %d",ptr[kk_can]->sok[kk],kk_can);
   if (ptr[kk_can]->sok[kk]>0){

     ptr[kk_can]->cmd_sok[kk] = 1 ;                   // Richiedi l'invio dei dati di inizializzazione a questo socket
     sprintf(appo,"%s",inet_ntoa(client_addr.sin_addr));        // Preparo i parametri da passare al thread che servira' le richieste di questo socket a questo client
     ptr[kk_can]->client[ptr[kk_can]->tot_sok_act][0] = ptr[kk_can]->sok[kk] ; // Salvo il puntatore al socket
     ptr[kk_can]->client[ptr[kk_can]->tot_sok_act][1] = client_addr.sin_port;  // Salvo la porta del client
     for (jj=kk=0,ii=2,fin=strlen(appo);kk<=fin;kk++){          // Salvo l'IP address del client
       appo1[kk]=appo[kk];
       if ( appo[kk]=='.' || appo[kk]==0) {
         appo[kk]=0;
         ptr[kk_can]->client[ptr[kk_can]->tot_sok_act][ii] = atoi(&appo[jj]);
         jj=kk+1;
         ii++;
       }
     }
     appo1[kk]=0;
     ptr[kk_can]->client[ptr[kk_can]->tot_sok_act][ii++] = ptr[kk_can]->proto_client ;
       ptr[kk_can]->client[ptr[kk_can]->tot_sok_act][ii--] = kk_can ;  // Canale di riferimento
       trace(__LINE__,__FILE__,1,0,0,"Ho accettato una connessione sul canale %d dal client %s con port: %d con protocollo %s", kk_can, appo1, ntohs(client_addr.sin_port) , protocol_name[ptr[kk_can]->client[ptr[kk_can]->tot_sok_act][ii]] );

       if ( pthread_create(&child, NULL, SocketLine, &ptr[kk_can]->client[ptr[kk_can]->tot_sok_act][0]) != 0 )
       {
         trace(__LINE__,__FILE__,1,0,0,"Pthread creation error: %d - %s",errno,strerror(errno));
       }
       else pthread_detach(child);                    //* disassocia dal processo genitore
// printf("Creato il thread %d\n",kk_can);
       ptr[kk_can]->timer = ptr[kk_can]->polcan ;             // Faccio partire subito il primo messaggio
       ptr[kk_can]->tot_sok_act ++ ;
// printf("Inserito totale %d\n",ptr[kk_can]->tot_sok_act);
     }
     else {
       ptr[kk_can]->sok[kk]=0 ;
       if (tot_ana_can ++ < tot_canali_act ){ kk_can++ ; goto prova_altra_porta ;}
       trace(__LINE__,__FILE__,1,0,0,"%d accept socket error %d: %s",sd,errno,strerror(errno));
     }
   }
   else {
     trace(__LINE__,__FILE__,1,0,0,"Richiesta connessioni contemporanee maggiore di quella consentita (%d)",MAXCONN-1);
     kk = accept(sd, (struct sockaddr*)&client_addr, &client_len); /* Elimino comunque la richiesta */
     trace(__LINE__,__FILE__,1,0,0,"Rifiuto la connessione dal client %s con port: %d",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
     close(kk);
   }
}
// ################################## TCP SOCKET #######################
/*
 * Installa un gestore di segnale SIGIO che viene svegliato quando
 * c'e' una richiesta di connessione alla porta che e'
 * stata aperta come parametro di chiamata.
 */
int openSocket(int  listen_port,int sw_proto)
{
int kk,sok_d ;
int livello = 1001;

  if (tot_canali_act>=MAX_N_PORT && tot_canali_act<0) {
    trace(__LINE__,__FILE__,livello,0,0,"Non ho trovato spazio nella tabella per la porta %d : con protocollo %s",listen_port,protocol_name[sw_proto]);
    return(-1);
  }

  bzero(&ptr[act_client]->sok[0], MAXCONN);
  bzero(&sasok[tot_canali_act], sizeof(sasok[0]) );
  ptr[act_client]->tot_sok_act = 0 ;

  sigemptyset (&sasok[tot_canali_act].sa_mask);
  sasok[tot_canali_act].sa_sigaction = sig_io;
  sasok[tot_canali_act].sa_flags = SA_SIGINFO ;
  if ( sigaction(SIGIO, &sasok[tot_canali_act], 0) < 0 ){
    trace(__LINE__,__FILE__,livello,2,0,"SigIO per Socket Connection fail %d : %s",errno,strerror(errno));
  }

   /* Inizializza l'indirizzo server per accettare connessione sulla porta PORT e da qualunque indirizzo IP (INADDR_ANY) */
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons((u_short) listen_port); /* htons: host to network conversion, short */
   server_addr.sin_addr.s_addr = INADDR_ANY;

   /* Crea il canale pubblico */
   sok_d = socket(AF_INET,SOCK_STREAM,0 );
   if (sok_d < 0 ){
     trace(__LINE__,__FILE__,livello,3,0,"Socket non creato %d : %s",errno,strerror(errno));
     return(sok_d);
   }

   if( setsockopt(sok_d, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) <0 ){
     trace(__LINE__,__FILE__,livello,4,0,"setsockopt(SO_REUSEADDR) failed %d : %s",errno,strerror(errno));
   }

   /* Lega al socket l'indirizzo del server */
   bind(sok_d,(struct sockaddr*) &server_addr,sizeof(server_addr));

   /* Imposta il numero Massimo di connessioni in coda */
   listen(sok_d,MAXCONN+1); /* Il+1 e' per rifiutare l'ennesima richiesta di connessione che viene accettata e poi chiusa subito per non lasciarla pendente */

   trace(__LINE__,__FILE__,livello,5,0,"Mi metto in ascolto sulla porta: %d", ntohs(server_addr.sin_port));

   /* allow the process to receive SIGIO */
   if ( fcntl(sok_d, F_SETOWN, getpid()) < 0 ){
    trace(__LINE__,__FILE__,livello,6,0,"Proprietario del SIGIO fail %d : %s",errno,strerror(errno));
   }
    /* Make the file descriptor asynchronous */
   if ( fcntl(sok_d, F_SETFL, O_ASYNC | O_NONBLOCK) < 0 )  {
     trace(__LINE__,__FILE__,livello,7,0,"Asincronous fail %d : %s",errno,strerror(errno));
   }
   tot_canali_act ++ ;                          // Mi preparo per un'altro canale
   return(sok_d);
}


int sendsock(int accept_socket,char *myDmsg,int lenDmsg,int opt)
{
int select_return ;
struct timeval tv ;
fd_set write_mask;
// Potrebbero esserci piu' socket contemporanei che vanno in timeout ma si segnala solo il primo
static int firsttout = 0 ;

  tv.tv_sec = 0;                            // ritorno immediato dalla select
  tv.tv_usec = 0;
  FD_ZERO(&write_mask);
  FD_SET(accept_socket, &write_mask);
                                    // handle del pacchetto da spedire
  select_return = select(accept_socket+1, (fd_set *)0, &write_mask, (fd_set *)0, &tv);
  if(select_return < 0) {                         // valori di rientro: timeout=0, -1= ERROR
    trace(__LINE__,__FILE__,1000,0,0,"Select Socket %d in errore %d : %s",accept_socket, errno,strerror(errno));
  }
  else if(select_return == 0) {
    if ( ! firsttout ) {                          // Non ho segnalazioni gia' attive di timeout ?
      trace(__LINE__,__FILE__,1000,0,0,"Socket %d in timeout",accept_socket);
      firsttout = accept_socket ;                     // Mi ricordo il canale che e' int timeout
    }
  }
  else {
//    printf("inviati %d bytes\n",lenDmsg);
    if( send(accept_socket, myDmsg, lenDmsg, opt) < 0) {
      trace(__LINE__,__FILE__,1000,0,0,"Send Socket %d in errore %d : %s",accept_socket, errno,strerror(errno));
    }
    if ( firsttout == accept_socket ) firsttout = 0 ;           // Ho spedito sul canale che era in timeout ? allora elimino segnalazione timeout attiva
  }
}

void richiedi_socket(int opt,int canale,int priorita)
{
int kk,ln,ii ;
// printf("Richiedi socket\n");
  if (canale==-1) {                           // Metto la richiesta su tutti i  canali IP
    for (ln=0;ln<act_client;ln++){                    // Giro tutti i canali dichiarati
      if (ptr[ln]->tipo_protocollo == 1 || ptr[ln]->tipo_protocollo == 2){ // Canale tipo IP
        if (ptr[ln]->cmd_sok[0]!=0){                  // Se c'e' una richiesta pendente accodo
          for (kk=0;kk<(sizeof(ptr[ln]->coda_socket)/sizeof(int))-1;kk++) {
            if (ptr[ln]->coda_socket[kk]==0) {
              ptr[ln]->coda_socket[kk]=opt ;
            break;
            }
          }
        }
        else {                              // Se non ci sono richieste pendenti metto subito per la prossima richiesta.
          for(kk=0;kk<MAXCONN;kk++) {
            ptr[ln]->cmd_sok[kk]=opt;                   // Richiedi l'invio su tutti i socket possibili del canale
          }
        }
      }
    }
  }
  else {
    if (ptr[canale]->cmd_sok[0]!=0){                  // Se c'e' una richiesta pendente accodo
      for (kk=0;kk<(sizeof(ptr[canale]->coda_socket)/sizeof(int))-1;kk++) {
        if (ptr[canale]->coda_socket[kk]==0) {
          ptr[canale]->coda_socket[kk]=opt ;
          break;
        }
      }
    }
    else {
      for(ln=0;ln<MAXCONN;ln++) {                     // GESTISCO IL COLLOQUIO SUI SOCKET TCP APERTI e CONNESSI
        ptr[canale]->cmd_sok[ln]=opt;                   // Richiedi l'invio su tutti i socket possibili del canale
      }
    }
  }
  if(priorita) ptr[canale]->timer = ptr[canale]->polcan ;
// printf("Fine richiedi socket\n");
}
int openClientSocket(int  client_port,int sw_proto,char * ipaddress)
{
int livello = 1000;
int kk,ii,jj,fin,sok_d ;
pthread_t child;
char appo[20],appo1[20];
struct sockaddr_in	 sercli_addr; 

printf("OPEN CLIENT TCP ------------------------------------- >\n");

   /* Inizializza l'indirizzo del server per creare una connessione sulla porta client_port */
   bzero((char *) &sercli_addr, sizeof(sercli_addr));
   sercli_addr.sin_family = AF_INET;
   sercli_addr.sin_port = htons((u_short) client_port); /* htons: host to network conversion, short */
   sercli_addr.sin_addr.s_addr = inet_addr(ipaddress);

   /* Crea il canale pubblico */
   sok_d = socket(AF_INET,SOCK_STREAM,0 );
   if (sok_d < 0 ){
     trace(__LINE__,__FILE__,livello,2,0,"Socket non creato %d : %s",errno,strerror(errno));
     return(sok_d);
   }

   /* Connettiti al server */
   if (connect(sok_d,(struct sockaddr*) &sercli_addr,sizeof(sercli_addr))<0){
     trace(__LINE__,__FILE__,livello,3,0,"Tentata connessione al server %s:%d",inet_ntoa(sercli_addr.sin_addr),ntohs(sercli_addr.sin_port));
     trace(__LINE__,__FILE__,livello,4,0,"Connessione al server fallita %d : %s",errno,strerror(errno));
     return(sok_d);
   }

   trace(__LINE__,__FILE__,livello,5,0,"Connesso al server %s con la porta:%d",ipaddress, ntohs(sercli_addr.sin_port));

// Metto i dati nella struttura
   ptr[act_client]->tot_sok_act = 1 ;                 // Il client ha solo un socket attivo
   ptr[act_client]->sav_handle = sok_d ;              // Handle del socket
   ptr[act_client]->sok[0] = sok_d ;                  // Impegno il primo socket per lavorare
   ptr[act_client]->client[ptr[act_client]->tot_sok_act][0] = ptr[act_client]->sok[0] ;         // Salvo il puntatore al socket
   ptr[act_client]->client[ptr[act_client]->tot_sok_act][1] = sercli_addr.sin_port;             // Salvo la porta del server
   for (jj=kk=0,ii=2,fin=strlen(ipaddress);kk<=fin;kk++){         // Salvo l'IP address del server
     appo1[kk]=ipaddress[kk];
     if ( ipaddress[kk]=='.' || ipaddress[kk]==0) {
       ipaddress[kk]=0;
       ptr[act_client]->client[ptr[act_client]->tot_sok_act][ii] = atoi(&ipaddress[jj]);
       jj=kk+1;
       ii++;
     }
   }
   appo1[kk]=0;
   ptr[act_client]->client[ptr[act_client]->tot_sok_act][ii++] = ptr[act_client]->proto_client ;
   ptr[act_client]->client[ptr[act_client]->tot_sok_act][ii] = act_client ;               // Canale di riferimento

   trace(__LINE__,__FILE__,livello,6,0,"Creato il thread della connessione IP al server = %d with Protocol %s",sok_d,protocol_name[sw_proto]);// Messaggio di trace

   if ( pthread_create(&child, NULL, SocketLine, &ptr[act_client]->client[ptr[act_client]->tot_sok_act][0]) != 0 )
   {
     trace(__LINE__,__FILE__,livello,7,0,"Pthread creation error: %d - %s",errno,strerror(errno));
   }
   else pthread_detach(child);
   return(sok_d);
}

// ################################## UDP SOCKET #######################
/* DA SCRIVERE ... COPIATO IL SOCKET TCP AL MOMENTO
 * Installa un gestore di segnale SIGIO che viene svegliato quando
 * c'e' una richiesta di connessione alla porta che e'
 * stata aperta come parametro di chiamata.
 */
int openUDPSocket(int  listen_port,int sw_proto)
{
int kk,sok_d ;

  if (tot_canali_act>=MAX_N_PORT && tot_canali_act<0) {
    trace(__LINE__,__FILE__,1000,0,0,"Non ho trovato spazio nella tabella per la porta %d : con protocollo %s",listen_port,protocol_name[sw_proto]);
    return(-1);
  }

  bzero(&ptr[act_client]->sok[0], MAXCONN);
  bzero(&sasok[tot_canali_act], sizeof(sasok[0]) );
  ptr[act_client]->tot_sok_act = 0 ;

  sigemptyset (&sasok[tot_canali_act].sa_mask);
  sasok[tot_canali_act].sa_sigaction = sig_io;
  sasok[tot_canali_act].sa_flags = SA_SIGINFO ;
  if ( sigaction(SIGIO, &sasok[tot_canali_act], 0) < 0 ){
    trace(__LINE__,__FILE__,1000,0,0,"SigIO per Socket Connection fail %d : %s",errno,strerror(errno));
  }

   /* Inizializza l'indirizzo server per accettare connessione sulla porta PORT e da qualunque indirizzo IP (INADDR_ANY) */
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons((u_short) listen_port); /* htons: host to network conversion, short */
   server_addr.sin_addr.s_addr = INADDR_ANY;

   /* Crea il canale pubblico */
   sok_d = socket(AF_INET,SOCK_DGRAM,0 );
   if (sok_d < 0 ){
      trace(__LINE__,__FILE__,1000,0,0,"Socket non creato %d : %s",errno,strerror(errno));
      return(sok_d);
   }
   /* Lega al socket l'indirizzo del server */
   bind(sok_d,(struct sockaddr*) &server_addr,sizeof(server_addr));

   /* Imposta il numero Massimo di connessioni in coda */
   listen(sok_d,MAXCONN+1); /* Il+1 e' per rifiutare l'ennesima richiesta di connessione che viene accettata e poi chiusa subito per non lasciarla pendente */
/*
   kk = 1 ;
   setsockopt(sok_d, SOL_SOCKET, SO_REUSEADDR, &kk, sizeof(int));
*/
   trace(__LINE__,__FILE__,1000,0,0,"Mi metto in ascolto sulla porta: %d", ntohs(server_addr.sin_port));

   /* allow the process to receive SIGIO */
  if ( fcntl(sok_d, F_SETOWN, getpid()) < 0 ){
    trace(__LINE__,__FILE__,1000,0,0,"Proprietario del SIGIO fail %d : %s",errno,strerror(errno));
  }
    /* Make the file descriptor asynchronous */
    if ( fcntl(sok_d, F_SETFL, O_ASYNC | O_NONBLOCK) < 0 )  {
    trace(__LINE__,__FILE__,1000,0,0,"Asincronous fail %d : %s",errno,strerror(errno));
  }
  tot_canali_act ++ ; // Incremento il numero di canali SERVER IP aperti
  return(sok_d);
}
/*---------------------------------------------------------------------*/
/*--- Funzione di ricezione dati dal socket UDP sulla porta di ascolto */
/*----------------------------------------------------------------------
 * Questo e' il programma di gestione delle risposte che vengono dal server UDP
 * sulla porta associata.
 * Gestisce un dialogo "a domanda rispondo" .
 * Attualmente non e' prevista la chiusura per inutilizzo (quindi con un timeout)
 */
void* SocketUDPLine(void* arg)
{
int bytes_read,kk,ii;
int *appo,esci,last_error;
int add1 , add2 , add3 , add4 , cliente , port, protocollo, sd ;
int (*InterpretaProtocollo)(struct connectManage *, unsigned char * , int , int );
struct connectManage * ptr_act ;
struct sockaddr	 servaddr; 
socklen_t len;

    appo        = (int *)arg;
    cliente     = *appo++;                          					// [0] = cliente
    port        = *appo++;                            					// [1] = porta di comunicazione dinamica in base alla connessione
    add1        = *appo++;                            					// [2] = 127.
    add2        = *appo++;                            					// [3] = 0.
    add3        = *appo++;                            					// [4] = 0.
    add4        = *appo++;                            					// [5] = 1
    protocollo  = *appo++;                        						// [6] = MODBUS_PROTOCOL : 2
    sd          = *appo;                            					// [7] = Canale server : x

    InterpretaProtocollo = dammi_protocollo(protocollo);

    ptr_act =  (struct connectManage *) malloc(sizeof (struct connectManage)); /* Mi alloco la memoria per i buffer di TX/RX */

    // Cerco i parametri da copiare nella struttura del nuovo cliente
    ptr_act->canale = ptr[sd]->canale ;                 // Copio l'indice del canale di colloquio
    ptr_act->polcan = ptr[sd]->polcan ;                 // Copio il tempo di campionamento del canale
    ptr_act->client = cliente ;                         // Copio l'id client nella struttura di gestione
    ptr_act->sav_handle = cliente ;
    for(kk=0;kk<MAXCONN;kk++){                        // Copio l'indirizzo della struttura della connessione nella struttura del canale
      if ( ptr[sd]->sok[kk] == cliente ){
        ptr[sd]->Connect[kk]=ptr_act;
        break;
      }
    }
    trace(__LINE__,__FILE__,3000,0,0,"Partito thread di ascolto cliente %d(%d) su canale %d con campionamento = %d\n",cliente,sd,ptr_act->canale,ptr_act->polcan);
    InitConnection(sd,ptr_act);
    esci = 0;
    do {
        bytes_read = recvfrom(cliente,  ptr_act -> result, sizeof(ptr_act -> result), 0, (struct sockaddr *)&servaddr, &len);
        if (bytes_read > 0)
        {
          if (last_error ) {
            trace(__LINE__,__FILE__,3000,1,0,"Reset last error %d IP line %d, protocollo %s read error %d = %s,Mem(%X)",last_error,cliente,protocol_name[protocollo],errno,strerror(errno),(ptr_act));
            last_error = 0 ;
          }
/* Log dei caratteri ricevuti (solo per debug) * /
          trace(__LINE__,__FILE__,1000,1,0,"Protocollo %d su TCPIP %d sul canale %d cliente %d",protocollo,bytes_read,ptr_act->canale,cliente);
          trace(__LINE__,__FILE__,1000,0,0,"Ricevuti %d caratteri = %s",bytes_read,ptr_act -> result);
/* */
          esci = InterpretaProtocollo(ptr_act, ptr_act->result, bytes_read, ptr_act->canale) ;
        }
        else {                              // Ricevuto un errore sulla lettura del canale: devo capire che errore e' o se e' una chiusura di connessione
          if (last_error != errno && errno != 11) {
            trace(__LINE__,__FILE__,1000,0,0,"Error IP line %d, protocollo %s read error %d = %s,Mem(%X)",cliente,protocol_name[protocollo],errno,strerror(errno),(ptr_act));
            last_error = errno ;
            if(errno == 104) esci = 1 ;                 // Connessione resettata dal punto
          }
          else if( !errno || last_error == errno ) esci = 1 ;     // Se ricevo 0 caratteri => perdita di connessione => uscita
        }
    }
    while(!esci);
    trace(__LINE__,__FILE__,1000,0,0,"Chiusura dal client %d.%d.%d.%d con port: %d",add1,add2,add3,add4,ntohs(port));
    for(ii=0;ii<act_client;ii++){
      for(kk=0;kk<MAXCONN;kk++){
        if ( ptr[ii]->sok[kk] == cliente ){
            ptr[ii]->sok[kk]=0;
            ptr[ii]->Connect[kk]=(struct connectManage *) NULL ; // Cancello l'indirizzo della memoria
            ptr[ii]->tot_sok_act --;
trace(__LINE__,__FILE__,1000,1,0,"Cancellato ptr[%d]->sok[%d]=%d totale %d cliente %d",ii,kk,ptr[ii]->sok[kk],ptr[ii]->tot_sok_act,cliente);
            kk = MAXCONN+1;
        }
      }
    }
    free(ptr_act); // Libero la memoria per lo scambio dati
    close(cliente);
    return arg;
}
int openClientUDPSocket(int  client_port,int sw_proto,char * ipaddress)
{
int livello = 1000;
int kk,ii,jj,fin,sok_d ;
pthread_t child;
char appo[20],appo1[20];

printf("OPEN CLIENT UDP  ------------------------------------- >\n");

   /* Crea il canale pubblico */
   sok_d = socket(AF_INET,SOCK_DGRAM,0 );
   if (sok_d < 0 ){
     trace(__LINE__,__FILE__,livello,2,0,"Socket UDP non creato %d : %s",errno,strerror(errno));
     return(sok_d);
   }

// Metto i dati nella struttura
   ptr[act_client]->tot_sok_act = 1 ;                 // Il client ha solo un socket attivo
   ptr[act_client]->sav_handle = sok_d ;              // Handle del socket
   ptr[act_client]->sok[0] = sok_d ;                  // Impegno il primo socket per lavorare
   ptr[act_client]->client[ptr[act_client]->tot_sok_act][0] = ptr[act_client]->sok[0] ;         // Salvo il puntatore al socket
   ptr[act_client]->client[ptr[act_client]->tot_sok_act][1] = htons((u_short) client_port);     // Salvo la porta del server
   for (jj=kk=0,ii=2,fin=strlen(ipaddress);kk<=fin;kk++){         // Salvo l'IP address del server
     appo1[kk]=ipaddress[kk];
     if ( ipaddress[kk]=='.' || ipaddress[kk]==0) {
       ipaddress[kk]=0;
       ptr[act_client]->client[ptr[act_client]->tot_sok_act][ii] = atoi(&ipaddress[jj]);
       jj=kk+1;
       ii++;
     }
   }
   appo1[kk]=0;
   ptr[act_client]->client[ptr[act_client]->tot_sok_act][ii++] = ptr[act_client]->proto_client ;
   ptr[act_client]->client[ptr[act_client]->tot_sok_act][ii] = act_client ;               // Canale di riferimento

   trace(__LINE__,__FILE__,livello,6,0,"Creato il thread della connessione UDP IP al server = %d with Protocol %s",sok_d,protocol_name[sw_proto]);// Messaggio di trace

   if ( pthread_create(&child, NULL, SocketUDPLine, &ptr[act_client]->client[ptr[act_client]->tot_sok_act][0]) != 0 )
   {
     trace(__LINE__,__FILE__,livello,7,0,"Pthread creation error: %d - %s",errno,strerror(errno));
   }
   else pthread_detach(child);
   return(sok_d);
}
int sendUDPsock(int pk,char *myDmsg,int lenDmsg,int opt)
{
int select_return ;
struct timeval tv ;
fd_set write_mask;
// Potrebbero esserci piu' socket contemporanei che vanno in timeout ma si segnala solo il primo
static int firsttout = 0 ;
struct sockaddr_in servaddr; 
char ipaddress[20];

  tv.tv_sec = 0;                            // ritorno immediato dalla select
  tv.tv_usec = 0;
  FD_ZERO(&write_mask);
  FD_SET(ptr[pk]->sok[0], &write_mask);
                                    // handle del pacchetto da spedire
  select_return = select(ptr[pk]->sok[0]+1, (fd_set *)0, &write_mask, (fd_set *)0, &tv);
  if(select_return < 0) {                         // valori di rientro: timeout=0, -1= ERROR
    trace(__LINE__,__FILE__,1000,0,0,"Select Socket %d in errore %d : %s",ptr[pk]->sok[0], errno,strerror(errno));
  }
  else if(select_return == 0) {
    if ( ! firsttout ) {                          // Non ho segnalazioni gia' attive di timeout ?
      trace(__LINE__,__FILE__,1000,0,0,"Socket %d in timeout",ptr[pk]->sok[0]);
      firsttout = ptr[pk]->sok[0] ;                     // Mi ricordo il canale che e' int timeout
    }
  }
  else {
 	memset(&servaddr, 0, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(ptr[pk]->port) ; 
	sprintf(ipaddress,"%d.%d.%d.%d",ptr[pk]->client[ptr[pk]->tot_sok_act][2],ptr[pk]->client[ptr[pk]->tot_sok_act][3],ptr[pk]->client[ptr[pk]->tot_sok_act][4],ptr[pk]->client[ptr[pk]->tot_sok_act][5]); 
	servaddr.sin_addr.s_addr = inet_addr(ipaddress) ; 

//    printf("inviati %d bytes\n",lenDmsg);
    if( sendto(ptr[pk]->sok[0], myDmsg, lenDmsg, 0, (struct sockaddr_in *)&servaddr, sizeof(struct sockaddr_in)) < 0) {
      trace(__LINE__,__FILE__,1000,0,0,"Send UDP Socket %d in errore %d : %s",ptr[pk]->sok[0], errno,strerror(errno));
    }
    if ( firsttout == ptr[pk]->sok[0] ) firsttout = 0 ;           // Ho spedito sul canale che era in timeout ? allora elimino segnalazione timeout attiva
  }
}
/***********************************************************************
 *                                                                     *
 *                           F U N C T I O N               			   *
 *                                                                     *
 **********************************************************************/

/***************** FUNCTION DEVELOPMENT *******************************/
// --------------------- Generazione numero pseudo random -------------
int rnd( int max ) {
   return (rand() % max) + 1;
}
/**--------------------------------------------------------------------*
 *
 * Traccia dei messaggi generati dal Comunicator nel file
 *
 *              /var/log/user.log
 *
 * Variabili statiche:
 * dbg_printf = 0 Non fa printf di debug, piu' sale il valore più printf
 *          di debug vengono fatte
 *---------------------------------------------------------------------*
 *
 * Stringa contiene il messaggio che verra' memorizzato con data e ora
 *
 * - codice segue questa strategia:
 *     0 -  999  Messaggi di errore
 *  1000 - 1999  Messaggi di warning
 *  2000 - 2999  Messaggi di stato
 *  3000 - 3999  Messaggi di informazione
 *  4000 - 4999  Messaggi di trace
 *  5000 - 5999  Errori SQLite3
 *
 * - livello indica l'importanza della segnalazione
 *     0     Salvo sempre nel file e faccio sempre anche la printf
 *     1     Faccio sempre solo la printf
 *     2....   Faccio la printf solo se il livello di DEBUG e' >
 *    0x80       Esegue la printf di debug di tutti i msg che
 *         arrivano da un file specificato
 *
 * - act_lev serve per cambiare il livello di debug
 * - stringa con variabili a seguire in stile printf
 *--------------------------------------------------------------------*/
void trace(int Criga,char * Cfile,int codice,int livello,int act_lev, char *stringa, ...)
{
va_list ap;
int rc=0;
int kk,jj;
int accu,idf;
static int last_codice=0,last_livello=0;
static int dbg_printf=0;                        						// Default livello di debug a zero (nessuan stampa di debug)
static char nf[40];                           							// Mi salvo il nome del file di cui voglio stampare tutti i messaggi che arrivano
char dest[1024]={""};
char appo[20]={""};
char fmt[20]={""};

  if (act_lev>0){                           							// Se ha il bit 0x80 alto mi memorizzo il nome del file da cui provengono i messaggi che voglio stampare
  if (act_lev & 0x80) {
    Cfile[39] = 0 ;                         							// Tronco a 40 caratteri il nome del file da cui provengono i messaggi
    strcpy(nf,Cfile);                         							// Me lo salvo
  }
  dbg_printf = act_lev ;                        						// Se richiesta variazione di livello di debug lo cambio
  }

  if (livello > 1) {                          							// printf di debug
  if(dbg_printf & 0x80){                        						// Se ho attive le stampe di debug di un file specifico stampo tutti i msg che arrivano da quel file
    if (!(strcmp(nf,Cfile))) rc =  1 ;
  }
  if((dbg_printf & 0x7F)>=livello) rc = 1;              				// Debug impostato ad un livello superiore rispetto al valore del livello del messaggio ?
  }
  else {                                								// printf di trace
    rc = 1;
  if (!livello && (last_codice != codice || last_livello != Criga)){  	// Salvo nel file di log del sistema solo se codice diverso dal precedente o riga di provenienza
      rc = 2 ;
      last_codice = codice;                                     		// Mi salvo l'ultimo messaggio per non ripetere lo stesso piu' volte consecutivamente !
      last_livello = Criga;
    }
  }
  if(rc){                                 								// Devo comporre la stringa
  for(kk=jj=0;stringa[jj]!=0;jj++ )                 					// Conto i %dsf della stringa per sapere quante variabili ci sono
    if(stringa[jj]=='%' && (stringa[jj+1]=='d' || stringa[jj+1]=='s' || stringa[jj+1]=='c' || stringa[jj+1]=='X' || stringa[jj+1]=='x' || stringa[jj+1]=='f' || stringa[jj+1]=='.' || ( stringa[jj+1]>='0' && stringa[jj+1]<='9'))) kk++ ;
  va_start(ap,kk);                          							// Avvio la lettura dei parametri dinamici sapendo che ho kk variabili
  accu = 0 ;
  for(jj=0,kk=0;stringa[jj]!=0;jj++ ) {               					// Rilevo il formato di lettura dal carattere dopo il % della stringa :  %d= int  %s= char * %f= double
    if(stringa[jj]=='%') {
      memcpy(&dest[accu],&stringa[kk],jj-kk);
      accu = accu + (jj-kk) ;             								// Posiziono l'indice sul primo carattere libero
      dest[accu]=0;                       								// Metto il fine stringa momentaneo
      fmt[0]='%';
      for(idf=1;!(stringa[jj+idf]=='X' || stringa[jj+idf]=='x' || stringa[jj+idf]=='c' || stringa[jj+idf]==0 || stringa[jj+idf]=='d' || stringa[jj+idf]=='f' || stringa[jj+idf]=='s');idf++) {
         fmt[idf]=stringa[jj+idf];
	  }
      fmt[idf]=stringa[jj+idf]; 										// Ci metto il tipo
      fmt[++idf] = 0 ; 													// Fine stringa sul formato della variabile
      kk = jj+idf ;                         							// Porto il prossimo inizio della costante stringa dopo il %d/%s/%f
      appo[0] = 0 ;
      if (fmt[idf-1]=='X' || fmt[idf-1]=='x' ) sprintf(appo,fmt,va_arg(ap,int));	// Accodo il valore intero
      else if (fmt[idf-1]=='d')  sprintf(appo,fmt,va_arg(ap,int)); 					// Accodo il valore intero
      else if (fmt[idf-1]=='f') sprintf(appo,fmt,va_arg(ap,double)); 				// Accodo il valor float
      else if (fmt[idf-1]=='c') sprintf (appo,fmt,(char)(va_arg(ap,int)));			// Accodo il valore char
      else if (fmt[idf-1]=='s') strcat (dest,(va_arg(ap,char *))); 					// Accodo la stringa
      else kk --;                         								// Se non c'e' uno dei tre caratteri ritorno indietro di uno per copiarci quello che ci sara'
      if(appo[0]!=0) strcat (dest,appo);
      accu = strlen(dest);                    							// Aggiorno la lunghezza del buffer di uscita
    }
  }
  memcpy(&dest[accu],&stringa[kk],jj-kk);                               // Copio la fine del buffer
  accu = accu + (jj-kk) ;                                               // Posiziono l'indice sul primo carattere libero
  dest[accu]=0;                                                         // Metto il fine stringa momentaneo
  va_end(ap);                                                           // Termino lettura dei parametri della funzione
//  printf("Cod.%d = %s {%s riga %d}\n",codice,dest,Cfile,Criga);     	// Faccio la printf a video
    printf("%s\n",dest);                                                // Faccio la printf a video
    if(rc==2){                                                          // Salvo nel file di log di sistema
      replace(dest,'\n','\000');                                        // Levo evenutali CR/LF
      syslog(LOG_INFO, "Cod.%4.4X = %s {%s riga %d}\n",codice,dest,Cfile,Criga);
    }
  }
  return;
}

// ################################################### FUNZIONI PER LETTURA FILES DI TARATURA ######################################################
/**
 * Ritorna un valore double del parametro nella funzione
 * **/
double rw_valpar(char *stringa,char *valore,int opt)
{
char buffer[200],appo1[80],appo2[80];
int kk,reale;

  if(opt & NO_DB) {                           // Leggo da file
    strcpy(buffer,valore);
// printf("Leggo da file\n");
    rw_parametro(stringa, buffer , opt );

// printf("Letto valore %s\n",buffer);

    reale =0  ;                             // Ipotesi valore intero
    for(kk=0;kk<strlen(buffer);kk++) {
      if ( buffer[kk]=='.' || buffer[kk]==',') {
        reale = 1 ;
        break;
      }
    }
    if (reale ) return(atof(buffer));
    else return(atoi(buffer));
  }
  else                                  // Leggo da DB
  {
     reale = strlen(stringa);
     for (kk=0 ; kk<reale ; kk ++ ) {
       if (stringa[kk] == '/' ) {
         appo1[kk] = 0 ;
         strcpy(appo2,&stringa[kk+1]) ;
         break;
       }
       else appo1[kk] = stringa[kk] ;
     }
     strcpy(buffer,valore);
     if (opt & WRITE_PAR)                         // Devo scriverlo ?
       set_config(appo1,appo2, buffer );
     else{
//printf("get config %s,%s,%s\n",appo1,appo2,buffer);
       get_config(appo1,appo2, buffer );
     }
     reale =0  ;                            // Ipotesi valore intero
     for(kk=0;kk<strlen(buffer);kk++) {
       if ( buffer[kk]=='.' || buffer[kk]==',') {
         reale = 1 ;
         break;
       }
     }
     if (reale ) return(atof(buffer));
     else return(atoi(buffer));
  }
}

/*-----------------------------------------------------------------------------------------------*
 * Ritorna una stringa del parametro nella variabile passata
 *-----------------------------------------------------------------------------------------------*/
void rw_strpar(char *result,char *stringa,char *valore,int opt)
{
char buffer[200],appo1[80],appo2[80];
int kk,reale;

  if(opt & NO_DB) {                           // Leggo da file
    strcpy(buffer,valore);
// printf("Leggo da file <%s> <%s>\n",stringa,valore);
    rw_parametro(stringa , buffer, opt );
    strcpy(result,buffer);
  }
  else
  {
     reale = strlen(stringa);
     for (kk=0 ; kk<reale ; kk ++ ) {
       if (stringa[kk] == '/' ) {
         appo1[kk] = 0 ;
         strcpy(appo2,&stringa[kk+1]) ;
         break;
       }
       else appo1[kk] = stringa[kk] ;
     }
     strcpy(buffer,valore);
     if (opt & WRITE_PAR)                         // Devo scriverlo ?
       set_config(appo1,appo2, buffer );
     else
       get_config(appo1,appo2, buffer );
     strcpy(result,buffer) ;
  }

}

/*-----------------------------------------------------------------------------------------------*
 * Lettura/Scrittura del parametro con il valore passato nel file di tarature
 *-----------------------------------------------------------------------------------------------*/
/*
   Leggi e scrivi il file con la sintassi:
        [ARGOMENTO_1]
        Parametro_1="Valore"
        .....
        [ARGOMENTO_2]
        Parametro_1="Valore"
        .....
   str_lbl indica l'ASCII del parametro da cercare dentro il file con la sintassi "ARGOMENTO_1/Parametro_1"
   str_val contiene il valore di default o da scrivere
   opt indica se scrivere o leggere il parametro ed il file in cui scrivere
   bit 7 = 0 lettura nel file
   bit   = 1 scrittura nel file
   bit 0 = 0 file 0
         = 1 file 1
*/
int rw_parametro(char *str_lbl,char *str_val,int opt)
{
FILE *fp;
FILE *newfile;
int   k,kk,conta,i,jj;
char  stringa[200];
char  appo[200];
char  nf[400];
char argome[200];
char param[200];
char last_line[200];
int trovato ;
int doppapici;

  kk = opt & ~WRITE_PAR ;
  kk = kk & ~NO_DB ;
  switch ( kk ){
      default:
        sprintf(appo,"IMPIANTO/ParamFile%d",kk);
        rw_strpar(stringa,appo, "" , READ_PAR | IMPIANTO_FILE ) ;
        if( stringa[0]!=0 ) strcpy(nf,stringa);
        else {
          strcpy(nf,DIR_IMPIANTO);
          strcat(nf,FILE_IMPIANTO);
        }
      break;
      case IMPIANTO_FILE:
        strcpy(nf,DIR_IMPIANTO);
        strcat(nf,FILE_IMPIANTO);
      break;
      case RIAVVIO_FILE:
        rw_strpar(stringa,"IMPIANTO/FileRestart", "riavvio" , READ_PAR | IMPIANTO_FILE ) ;
        strcpy(nf,stringa);
      break;
  }
  fp = fopen(nf,"r");
// printf("Aperto file %s\n",nf);
  if(fp <= (FILE *) 0) {                        // Se il file non esiste
    if (opt & WRITE_PAR) {                        // Devo scriverlo ?
      fp = fopen(nf,"w");                         // Ne apro uno vuoto
    }
    else{
      trace(__LINE__,__FILE__,1000,0,0,"File di tarature <%s> non trovato",nf);
      return(-1);
    }
  }
  if( fp ){
    newfile = fopen("tmp.file","w");                  // Apro un file di appoggio per aggiornare i valori
    conta = 0;
    trovato = 0 ;                             // Ipotesi di argomento e parametri non trovati
    for(k=0;k<200;k++) {
       if(*(str_lbl+k)=='/') {
         memcpy(argome,str_lbl,k);
         argome[k]=0;
         i=strlen(str_lbl) - k  ;
         memcpy(param,str_lbl+k+1,i-1);
         param[i-1]=0;
         break;
       }
     }
/*
trace(__LINE__,__FILE__,4002,6,0,"Ricerca  [%s] : <%s>",argome,param);
);
*/

    while(fgets(stringa,160,fp)){

      if ( trovato == 2 ) {
        if (newfile) fputs(stringa,newfile);              // Scarico il resto del file in quello nuovo
        continue ;                            // Aspetto la fine senza controllare nulla
      }
      strcpy(last_line,stringa);
      conta ++ ;
      memset(appo,0,sizeof(appo));
/*

trace(__LINE__,__FILE__,4002,6,0,"Linea %d = %s",conta,stringa);
*/
      if(stringa[0]=='*' || stringa[0] =='#' ) {
        if (newfile) fputs(last_line,newfile);
        continue ;                            // Asterisco o graticola in prima posizione commento
      }
                                    // Compattazione dei blank
      doppapici = 0 ;
      for(k=0;k<160 && stringa[k] != 0 ;k++) {
        if(stringa[k]=='"') {
            if(doppapici==0)  doppapici = 1;
            else doppapici=0;
        }
        if(stringa[k]==' ' && doppapici==0) {
          for(jj=k;stringa[jj]!=0;jj++) stringa[jj]=stringa[jj+1] ;   //non usare : memcpy(&stringa[k],&stringa[k+1],160-(k+1) ); // Puo' dare errori
          k--;
        }
        if( stringa[k]==0xD || stringa[k]==0xA ) {            // Eliminazione del CR LF
          stringa[k] = 0 ;
          break;
        }
      }

      if(strlen(stringa) == 0 ) {
        if ( newfile ) fputs(last_line,newfile);
        continue ;                            // Se riga vuota salta
      }

      if(stringa[0]=='[' ) {                      // Argomento ?
        for(k=0;k<80;k++) {                       // Tolgo le parentesi
          if(stringa[k]==']') {
            stringa[k]=0;
            strcpy(appo,&stringa[1]);
            break;
          }
        }
        if (!strcmp(appo,argome)) {                     // Argomento trovato ora devo cercare il parametro
          if (newfile) fputs(last_line,newfile);
          trovato = 1 ;
        }
        else {
          if (trovato) {
            if (strlen(str_val)) {                    // Ho un valore di default da settare
              sprintf(stringa,"%s=\"%s\"\n",param,str_val);       // Rimpiazzo l'attuale riga con questo nuovo parametro
              fputs(stringa,newfile);
              fputs(last_line,newfile);                 // Metto l'attuale riga subito sotto
              trovato = 2 ;                       // Dichiaro che il paramentro non l'ho trovato ma l'ho scritto
            }
            else {
              trovato = 3 ;
              break;                            // Se no ho finito l'argomento allora parametro non trovato posso finire l'analisi del file
            }
          }
          else {
            if (newfile) fputs(last_line,newfile);
          }
        }
      }
      else {                              // Campo parametro
        if (trovato != 1){
          if (newfile) fputs(last_line,newfile);
          continue ;                          // Se argomento non trovato non controllo i parametri
        }
        for(k=0;k<80;k++) {
          if(stringa[k]=='=') {
            stringa[k]=0;
            strcpy(appo,&stringa[k+1]);
           break;
          }
        }
        if (!strcmp(stringa,param)){                      // Parametro trovato
          if (opt & WRITE_PAR) {                    // Devo scriverlo ?
            sprintf(stringa,"%s=\"%s\"\n",param,str_val);         // Rimpiazzo l'attuale riga con questo nuovo parametro
            if (newfile) fputs(stringa,newfile);
            trovato = 2 ;                         // Scrivi tutto il resto del file
          }
          else {                            // Lettura del parametro
            for(k=1;k<160;k++) {
               if(appo[k]=='"') {
                 appo[k]=0;
                 strcpy(appo,&appo[1]);
                 break;
               }
             }
             strcpy(str_val,appo) ;
             trovato = 4 ;
             break;
          }
        }
        else {
          if (newfile) fputs(last_line,newfile);
        }
      }
    }
    if( trovato == 0 ){                         // Se non ho trovato niente inserisco questo argomento con il relativo parametro
        sprintf(stringa,"[%s]\n",argome);                 // riga con questo nuovo parametro
        fputs(stringa,newfile);
        sprintf(stringa,"%s=\"%s\"\n",param,str_val);           // riga con questo nuovo parametro
        fputs(stringa,newfile);
        trovato = 2 ;                           // Segnalo rigo inserito
    }
    if (trovato == 1 ) {                        // Se ho trovato l'argomento ma non il parametro ed e' finito il file
        sprintf(stringa,"%s=\"%s\"\n",param,str_val);           // riga con questo nuovo parametro
        fputs(stringa,newfile);
        trovato = 2 ;                           // Segnalo rigo inserito
    }
    fclose (fp);
    fclose (newfile);
  }
  else
  {
      trace(__LINE__,__FILE__,1000,0,0,"Errore nella gestione del file <%s>",nf);
      return(-1);
  }

  if (trovato == 2){                          // Devo sostituire il file
    if(!rename(nf,"appo")) {                      // Rinomino il vecchio file in uno di appoggio
      if (!rename("tmp.file",nf))                     // Rinomino il nuovo file con il suo nome corretto
           remove("appo");                            // se tutto OK cancello il vecchio di appoggio
      else rename("appo",nf) ;                        // Se va male il rename ripristino il vecchio
    }
  }
/* PER DEBUGGER DEL FILE DI TARATURA *
  printf("%s = %s\n",str_lbl,str_val);
**/
  return(0);
}
// -------------------------------------------------- FINE FUNZIONI LETTURA FILE DI TARATURA ---------------------------------------------

// ################################################### FUNZIONI GESTIONE DATA E ORA ######################################################
/**
 *  0=ora 1=minuti 2=giorno della settimana 0=Lun 6=Dom
 * **/
int dammitempo(int opz)
{
struct tm *dt;
time_t t;
int ret ;

  time(&t);
  dt = localtime(&t);
  switch(opz){
    case 0: ret = dt->tm_hour; break;                   				// Ora
    case 1: ret = dt->tm_min;  break;                   				// Min
    case 2:
      ret = dt->tm_wday;                        						// Trasformo da 0 = Domenica 6 = Sabato
      ret -- ;
      if ( ret < 0  ) ret = 6 ;                     					// a 0 = Lunedi 6 = Domenica
    break;
  }
  return(ret);

}
/**
 *  Da due stringhe ritorna i secondi di differenza tra la prima e la seconda
 *  Le stringhe sono in formato hh:mm:ss
 * **/
int SecondiDiff( char * start_ora , char * act_ora )            		
{
long ini,fine;
char uppo[10] ;
char * token ;

  strcpy(uppo,start_ora);
  token = strtok(uppo,":");
  ini = atoi(token) * 60 ; 												// ore trasformate in minuti
  token = strtok(NULL,":") ;
  ini = ini + atoi(token); 												// ci sommo i minuti
  ini = ini * 60 ; 														// Diventano secondi
  token = strtok(NULL,":") ;
  ini = ini + atoi(token); 												// ci sommo i secondi

  strcpy(uppo,act_ora);
  token = strtok(uppo,":");
  fine = atoi(token) * 60 ; 											// ore trasformate in minuti
  token = strtok(NULL,":") ;
  fine = fine + atoi(token); 											// ci sommo i minuti
  fine = fine * 60 ; 													// Diventano secondi
  token = strtok(NULL,":") ;
  fine = fine + atoi(token); 											// ci sommo i secondi

  ini= fine-ini ;
  if (ini<0 ) ini = ini + 86400 ; 										// Ho scavallato la mezzanotte ?
  return(ini);

}
/**
* opt = 0 Ritorna l'ora nel formato hh:mm:ss
*       1 Ritorna data nel formato  yy/mm/dd
*       2 Ritorna l'ora nel formato hh:mm
*       3 Ritorna data nel formato  dd/mm/yy
*       4 Ritorna data nel formato  yy_mm_gg
*       5 Ritorna data nel formato  yyyy/mm/dd
*       6 Ritorna data nel formato gg mese anno
*       7 Ritorna data nel formato yymmddhhmmss
*       8 Ritorna data nel formato yy/mm/dd hh:mm:ss
*       9 Ritorna il giorno della settimana
*      10 Ritorna il 1 gennaio dell'anno in corso nel formato yyyy/mm/dd
*      11 Ritorna data nel formato  yyyy-mm-dd
*      20 Ritorna il numero del giorno
*      21 Ritorna il numero del mese
*      22 Ritorna il numero dell'anno
*      23 Ritorna il numero del giorno della settimana 0 = LUNEDI 6 = DOMENICA
*      24 Ritorna la data nel formato yymmddhhmmss prima di result secondi
*      25 Ritorna la data nel formato yymmdd prima di result mesi
*      29 Ritorna data nel formato  yyyy-mm-dd hh:mm:ss.000 formato SQL timedate
* **/
int dataora(int opz,char * result)
{
struct tm *dt;
time_t t;
int mese,kk,anno,giorno ;
//              GenFebMarAprMagGiuLugAgoSetOttNovDic
int giorni[12]={31,28,31,30,31,30,31,31,30,31,30,31};

  time(&t);
  dt = localtime(&t);
  mese = dt->tm_mon+1;
  switch(opz){
    case 0:                               // Ritorna l'ora nel formato hh:mm:ss
      sprintf(result,"%2.2d:%2.2d:%2.2d",dt->tm_hour,dt->tm_min,dt->tm_sec);
    break;
    case 1:                               // Ritorna data nel formato  yy/mm/dd
      sprintf(result,"%2.2d/%2.2d/%2.2d",dt->tm_year-100,mese,dt->tm_mday);
    break;
    case 2:                               // Ritorna l'ora nel formato hh:mm
      sprintf(result,"%2.2d:%2.2d",dt->tm_hour,dt->tm_min);
    break;
    case 3:                               // Ritorna data nel formato  dd/mm/yy
      sprintf(result,"%2.2d/%2.2d/%2.2d",dt->tm_mday,mese,dt->tm_year-100);
    break;
    case 4:                               // Ritorna data nel formato  yy_mm_dd
      sprintf(result,"%2.2d_%2.2d_%2.2d",dt->tm_year-100,mese,dt->tm_mday);
    break;
    case 5:                               // Ritorna data nel formato  yyyy/mm/dd
      sprintf(result,"%4.4d/%2.2d/%2.2d",dt->tm_year+1900,mese,dt->tm_mday);
    break;
    case 6:                               // Ritorna data nel formato   gg mese anno
      sprintf(result,"%d %s %d",dt->tm_mday,nome_mese[dt->tm_mon],dt->tm_year+1900);
    break;
    case 7:                               // Ritorna data nel formato  yymmddhhmmss
      sprintf(result,"%2.2d%2.2d%2.2d%2.2d%2.2d%2.2d",dt->tm_year-100,mese,dt->tm_mday,dt->tm_hour,dt->tm_min,dt->tm_sec);
    break;
    case 8:                               // Ritorna data nel formato  yy/mm/dd hh:mm:ss
      sprintf(result,"%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",dt->tm_year-100,mese,dt->tm_mday,dt->tm_hour,dt->tm_min,dt->tm_sec);
    break;
    case 9:                               // Ritorna il giorno della settimana LUNEDI....
      if(dt->tm_wday) mese = dt->tm_wday-1 ;
      else mese = 6 ;
      sprintf(result,"%s",nome_giorno[mese]);
    break;
    case 10:                              // Ritorna il 1 gennaio dell'anno in corso nel formato yyyy/mm/dd
      sprintf(result,"%4.4d/01/01",dt->tm_year+1900);
    break;
    case 11:                              // Ritorna data nel formato  yyyy/mm/dd
      sprintf(result,"%4.4d-%2.2d-%2.2d",dt->tm_year+1900,mese,dt->tm_mday);
    break;
    case 20:                              // Ritorna il numero del giorno
      return (dt->tm_mday) ;
    break;
    case 21:                              // Ritorna il numero del mese
      return (mese) ;
    break;
    case 22:                              // Ritorna il numero dell'anno
      return (dt->tm_year+1900) ;
    break;
    case 23:                              // Ritorna il numero del giorno della settimana LUNEDI....
      if(dt->tm_wday) mese = dt->tm_wday-1 ;
      else mese = 6 ;
      return(mese);                           // Da 0 a 6 = LUN - DOM
    break;
    case 24:                              // Fa la differenza tra la data attuale e i secondi nella variabile passata
      mese = atoi (result);
      kk = mese % 60 ;                          // secondi
      kk = dt->tm_sec - kk ;
      if (kk<0){
        dt->tm_sec = kk+60 ;
        kk = (mese % 3600) + 1 ;                    // minuti
      }
      else{
        dt->tm_sec = kk ;
        kk = (mese % 3600) ;                      // minuti
      }

      kk = dt->tm_min - kk ;
      if (kk<0){
        dt->tm_min = kk+60 ;
        kk = (mese / 3600) + 1 ;                    // ore
      }
      else{
        dt->tm_min = kk ;
        kk = (mese / 3600) ;                      // ore
      }
      kk = dt->tm_hour - kk ;
      if (kk<0){
        kk += 24 ;
        dt->tm_hour = kk ;
        kk = dt->tm_mday - 1 ;
        if(kk<=0){
          kk = dt->tm_mon - 1 ;
          if ( kk < 0 ) kk = 0 ;                    // Non considero lo scavallamento dell'anno indietro
          dt->tm_mon = kk  ;
          dt->tm_mday = giorni[kk] ;                    // Non considero l'anno bisestile
        }
        else dt->tm_mday = kk ;
      }
      else dt->tm_hour = kk ;
      sprintf(result,"%2.2d%2.2d%2.2d%2.2d%2.2d%2.2d",dt->tm_year-100,dt->tm_mon+1,dt->tm_mday,dt->tm_hour,dt->tm_min,dt->tm_sec);
    break;
    case 25:                              // Fa la differenza tra la data attuale e i mesi nella variabile passata
      mese = atoi (result);
      kk = dt->tm_mon - mese ;                      // Mesi da 0 a 11
      if ( kk < 0 ){                          // Considero lo scavallamento dell'anno indietro
        kk += 12 ;
        dt->tm_year -= 1;
      }
      sprintf(result,"%2.2d%2.2d%2.2d",dt->tm_year-100,kk+1,dt->tm_mday);
    break;
    case 29: // Ritorna data nel formato  yyyy-mm-dd hh:mm:ss.000 formato SQL timedate
      sprintf(result,"%4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d.000",dt->tm_year+1900,mese,dt->tm_mday,dt->tm_hour,dt->tm_min,dt->tm_sec);
    break;
    case 100: // Ritorna la data yymmdd rispetto ad oggi - result giorni
      giorno = dt->tm_yday + atoi(result) ;
      anno = dt->tm_year-100 ;
      if(giorno>=0 && giorno<365) giorno++ ; // Vado da 1 a 366
      else{
        if(giorno >= 365) { anno++ ; giorno -=364 ; }
        if(giorno < 0)   { anno-- ; giorno +=366 ; }
      }
      for (mese=0 ; mese < 12 ; mese++) {
        if(giorno > giorni[mese]) {
          giorno -= giorni[mese];
        }
        else break;
      }
      mese ++ ;
      sprintf(result,"%02d%02d%02d",anno,mese,giorno) ;
      return(giorno); // Nella funzione ritorna il valore del giorno dell'anno 1-365
    break;
  }
  return (0);
}
/**
 *   Ritorna data di ieri nel formato  yy_mm_gg
 * **/
void yesterday(char * result)
{
struct tm *ts;
time_t t;

  time(&t);
  ts = localtime(&t);
  /**
   * If today is the 1st, subtract 1 from the month
   * and set the day to the last day of the previous month
   */
  if (ts->tm_mday == 1)
  {
    /**
     * If today is Jan 1st, subtract 1 from the year and set
     * the month to Dec.
     */
    if (ts->tm_mon == 0)
    {
      ts->tm_year--;
      ts->tm_mon = 11;
    }
    else
    {
      ts->tm_mon--;
    }

    /**
     * Figure out the last day of the previous month.
     */
    if (ts->tm_mon == 1)
    {
      /**
       * If the previous month is Feb, then we need to check
       * for leap year.
       */
      if (ts->tm_year % 4 == 0 && ts->tm_year % 400 == 0)
        ts->tm_mday = 29;
      else
        ts->tm_mday = 28;
    }
    else
    {
      /**
       * It's either the 30th or the 31st
       */
      switch(ts->tm_mon)
      {
         case 0: case 2: case 4: case 6: case 7: case 9: case 11:
           ts->tm_mday = 31;
           break;

         default:
           ts->tm_mday = 30;
      }
    }
  }
  else
  {
    ts->tm_mday--;
  }
  sprintf(result,"%2.2d_%2.2d_%2.2d",ts->tm_year-100,ts->tm_mon+1,ts->tm_mday);
  return;
}
/**
 * Ritorno il giorno della settimana
 * **/
int numGiorno(char *weekday)
{
int kk;
  for (kk=0;kk<7;kk++)  if (!strcmp(nome_giorno[kk],weekday)) return(kk+1);
}
/**
 * Legge l'ora di sistema e la pubblica nell'area di memoria
 * **/
int setta_ora(void)
{
struct tm *dt;
time_t t;
  time(&t);
  dt = localtime(&t);													// Pubblico nelle aree la data e l'ora
  ora_act = dt->tm_hour ;
  min_act = dt->tm_min ;
  timer_min = dt->tm_sec ;                        						// Sincronizzo i minuti del PLC al secondo + o -
  gg_act = dt->tm_mday ;
  mm_act = dt->tm_mon+1 ;
  anno_act = dt->tm_year - 100 ;                      					// anni dal 1900 (es.2015 = 115)
  wday_act = dt->tm_wday;                         						// 0 = Domenica - 6 = Sabato
// printf("Esco giorno %d mese %d gg %d ora %d min %d sec %d setta ora\n",wday_act,mm_act,gg_act,ora_act,min_act,timer_min);
}
// ---------------------------------------------- FINE FUNZIONI GESTIONE DATA E ORA  -------------------------------------------
// ################################################### FUNZIONI SUI FILES ######################################################

/**
 *
 * Conversione da stringa esadecimale ASCII a valore numerico binario
 *
 * **/
unsigned long StrHex2Value(char * str)
{
int kk;
unsigned long ret_val;

    kk = strlen(str)-1;
    for(kk=0,ret_val=0;str[kk]!=0;kk++){
        if(str[kk]>='0' && str[kk]<='9') {
            ret_val = ret_val << 4 ;
            ret_val |= (unsigned long)(str[kk]-'0');
        }
        else {
            if(str[kk]>='a' && str[kk]<='f') {
                ret_val = ret_val << 4 ;
                ret_val |= (unsigned long)(str[kk]-'a'+10);
            }
            else if(str[kk]>='A' && str[kk]<='F') {
                ret_val = ret_val << 4 ;
                ret_val |= (unsigned long)(str[kk]-'A'+10);
            }
        }
    }
    return(ret_val);
}
/**
 *
 * Conversione da valore numerico binario a stringa esadecimale ASCII
 *
 * **/
char *Value2StrHex(unsigned long valo)
{
int jj,kk,ii;

  for(jj=ii=0;ii<8;ii++){
    kk = ((valo>>(28-(4*ii))) & 0xF);                 // Prendo un nibble alla volta
    if(kk>9) retValue[ii] = kk+0x37;                  // Lo riporto in ASCII HEX
      else retValue[ii] =kk+0x30;
  }
  retValue[ii]=0;                           // Fine stringa
    return(retValue);
}

/**
 *
 * Conversione da stringa con valore numerico decimale a stringa con valore numerico esadecimale
 *
 * **/
char *Buff2Hex(char * str,int nbyte)
{
int kk,jj;
  for(jj=kk=0;kk<nbyte;kk++){
    retValue[jj]=(str[kk]>>4) & 0xF;
    if(retValue[jj]>9) retValue[jj] += 0x37;
    else retValue[jj] += 0x30;
    jj ++ ;
    retValue[jj]=str[kk] & 0xF;
    if(retValue[jj]>9) retValue[jj] += 0x37;
    else retValue[jj] += 0x30;
    jj ++;
    retValue[jj++]=0x2e;
  }
  retValue[--jj]=0;
  return(retValue);
}

/**
 * Funzione che indica la presenza di un certo carattere
 * ritorna 0 se non c'e' oppure la posizione nella stinga sul primo carattere trovato
 * stringa da controllare
 * carattere ricercato
 * **/
int instring(char *str, char c)                     
{                                   
    int count = 0;
    char *p;
    p = str;

    while (*p != '\0')
    {
        if (*p == c) return(count);
        count++;
        p++;
    }
    return(0); // Non trovato
}
/**
 * Funzione che separa una stringa in un vettore di strighe 
 * controllando il separatore indicato
 * stringa da separare
 * carattere separatore
 * array di stringhe con i singoli valori separati
 * Una volta che la funzione rientra dopo aver adoperato l'array di stringhe
 * si deve liberare la memoria con una free(arr) ;
 * */
int split (char *str, char c, char ***arr)                
{
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;

    p = str;
    while (*p != '\0')
    {
        if (*p == c)
            count++;
        p++;
    }

    *arr = (char**) malloc(sizeof(char*) * count);
    if (*arr == NULL)
        return(0);

    p = str;
    while (*p != '\0')
    {
        if (*p == c)
        {
            (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
            if ((*arr)[i] == NULL)
                return(0);

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
    if ((*arr)[i] == NULL)
        return(0);

    i = 0;
    p = str;
    t = ((*arr)[i]);
    while (*p != '\0')
    {
        if (*p != c && *p != '\0')
        {
            *t = *p;
            t++;
            *t = '\0'; // ipotesi che ci sara' un fine stringa
        }
        else
        {
            *t = '\0';
            i++;
            t = ((*arr)[i]);
        }
        p++;
    }
    return count;
}
/***********************************************************************
 * stringa da separare
 * stringa di separatori
 * array di stringhe con i singoli valori separati
 * Una volta che la funzione rientra dopo aver adoperato l'array di stringhe
 * si deve liberare la memoria con una free(arr) ;
 * */
int splitstr(char *str, char *sepa, char ***arr)
{
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;
  int kk,jj,trov ;

  kk = strlen(sepa) ;                         							// Quanti separatori devo gestire ?
    p = str;
    while (*p != '\0')
    {
    for(jj=0;jj<kk;jj++) if (*p == sepa[jj] ) count++;
        p++;
    }

    *arr = (char**) malloc(sizeof(char*) * count);            			// Mi alloco il numero di puntatori che mi serviranno
    if (*arr == NULL) return(0);

    p = str;
    while (*p != '\0')
    {
    for(trov=jj=0;jj<kk;jj++) if(*p == sepa[jj]){trov=1;break;}
        if ( trov )
        {
          (*arr)[i] = (char*) malloc( sizeof(char) * token_len );   	// Mi alloco lo spazio per i singoli array
          if ((*arr)[i] == NULL) return(0);
      token_len = 0;
          i++;
        }
        p++;
        token_len++;
    }

  if(token_len>1) {
    (*arr)[i] = (char*) malloc( sizeof(char) * token_len );     		// Mi alloco lo spazio per l'ultima stringa se non c'e' un separatore come ultimo carattere
    if ((*arr)[i] == NULL) return(0);
  }
  else count--;                           								// Altrimenti ho un array in meno

    i = 0;
    p = str;
    t = ((*arr)[i]);
    while (*p != '\0')
    {
    for(trov=jj=0;jj<kk;jj++) if(*p == sepa[jj]){trov=1;break;}

        if (!(trov) && (*p != '\0'))
        {
            *t = *p;
            t++;
            *t = '\0';                          						// ipotesi che ci sara' un fine stringa
        }
        else
        {
      if(*(p+1)!='\0'){                     							// Se non sono sul fine stringa apro un altro array
        *t = '\0';
        i++;
        t = ((*arr)[i]);
      }
        }
        p++;
    }
    return count;
}
/**
 *  Conversione da long ad indirizzo IP ASCII
 * **/
char *long2strip(unsigned long indirizzo)
{
  sprintf(retValue,"%ld.%ld.%ld.%ld",((indirizzo/0x1000000)& 0xFF),
                     ((indirizzo/0x10000)& 0xFF),
                     ((indirizzo/0x100)& 0xFF),
                      (indirizzo & 0xFF) );
  return(retValue);
}
/**
 *  Conversione da indirizzo IP ASCII a long
 * **/
unsigned long strip2long(char *stringa)
{
int k,kk,i,jj,ii;
char appo[4];
unsigned long p1,retVal;

  retVal = 0 ;
  ii = strlen(stringa);

trace(__LINE__,__FILE__,4000,5,0,"%s - %d",stringa,ii);


  for(jj=0x1000000,kk=i=k=0;k<ii;k++) {
    if(stringa[k]=='.' && i<=2) {
      stringa[k]=0;
      strcpy(appo,&stringa[kk]);
      kk = k+1 ;
      i ++;
      p1 = atoi(appo);
      retVal += (p1 * jj);


trace(__LINE__,__FILE__,4000,5,0,"%ld - %ld",retVal,p1);

      jj /= 0x100 ;
      if (i == 3){
        strcpy(appo,&stringa[kk]);
        p1 = atoi(appo);
        retVal += (p1 * jj);
        return(retVal);
      }
    }
  }
  return(0);
}
/**
 *  Sostituisco un carattere con un altro all'interno della stringa
 *  se NULL => compatto la stringa eliminando i caratteri ricercati
 * **/
void replace(char * strstr,char s,char d)
{
int kk,jj ;
  for (kk = 0 ;strstr[kk]!=0 ;kk++) {
    if(strstr[kk]==s) {                         // Sostituzione del carattere ricercato
      if(d==NULL){                            // Se si vuole togliere
        for(jj=kk;strstr[jj]!=0;jj++) strstr[jj]=strstr[jj+1] ;     // Compatto la stringa eliminando il carattere
        kk--;
      }
      else strstr[kk] = d ;                       // Altrimenti sostituisco uno a uno
    }
  }
}
/**
 *  Elimina i blank, CR e LF dalla stringa
 * **/
void trim(char * strstr)
{
int kk,jj ;
      for (kk = 0 ;strstr[kk]!=0 ;kk++) {
        if(strstr[kk]==' ') {                       					// Compattazione dei blank
          for(jj=kk;strstr[jj]!=0;jj++) strstr[jj]=strstr[jj+1] ;     	// non usare : memcpy(&strstr[k],&strstr[k+1],160-(k+1) );
          kk--;
        }
        if( strstr[kk]==0xD || strstr[kk]==0xA ) {            			// Eliminazione del CR LF
          strstr[kk] = 0 ;
          break;
        }
      }
}
void lower(char *pstr){for(char *p = pstr;*p;++p) *p=*p>='A'&&*p<='Z'?*p|0x60:*p;}
void upper(char *pstr){for(char *p = pstr;*p;++p) *p=*p>='a'&&*p<='z'?*p&0xDF:*p;}
// --------------------------------------------------------- FINE FUNZIONI SULLE STRINGHE -----------------------------------------------------
// ################################################### FUNZIONI SUI FILES ######################################################
int EsisteFile(int opt)
{
FILE *fp;
char  nf[400];
  switch ( opt ){
      case IMPIANTO_FILE:
        strcpy(nf,DIR_IMPIANTO);
        strcat(nf,FILE_IMPIANTO);
      break;
  }
  fp = fopen(nf,"r");
  if(fp <= (FILE *) 0) return(0); // Se il file non esiste
  else {
    fclose (fp) ;
    return(1);
  }
}

long int dammiSize(const char *file_name)
{
struct stat st;
    if(stat(file_name,&st)==0) return (st.st_size);
    else return (-1) ;
}
int fileCpy(char *src,char *dst)
{
int in_fd ;
int out_fd ;
char buf[8192];
int result ;

  strcpy(buf,DIR_PROGRAMMA);
  strcat(buf,src);
  in_fd = open(buf, O_RDONLY);
  if(in_fd <= 0) return(1);                       // File sorgente non esiste
  strcpy(buf,DIR_PROGRAMMA);
  strcat(buf,dst);
  out_fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC);           // Apro in sovrascrittura
  if ( out_fd <= 0) {
    return(2);                              // Error file destinazione
  }
  while (1) {
    result = read(in_fd, &buf[0], sizeof(buf));
    if (result<=0) break;
    write(out_fd, &buf[0], result) ;
  }
  close(in_fd);
  close(out_fd);
}
int fileTxtCpy(char *src,char *dst)
{
FILE * in_fd , * out_fd ;
char buf[200];

  strcpy(buf,DIR_PROGRAMMA);
  strcat(buf,src);
//printf("open source %s\n",buf);
  in_fd = fopen(buf, "r");
  if(in_fd <= 0) return(1);                       // File sorgente non esiste
  strcpy(buf,DIR_PROGRAMMA);
  strcat(buf,dst);
  out_fd = fopen(buf,"w");                        // Apro in sovrascrittura
//printf("open dest %s\n",buf);
  if ( out_fd <= 0) {
    return(2);                              // Error file destinazione
  }
  while (fgets(buf,160,in_fd)){
//printf("Letto %s\n",buf);
   fputs(buf,out_fd);
  }
  fclose(in_fd);
  fclose(out_fd);
//printf("close all \n");


}
// Funzione che modifica il file di risoluzione del dns per il server e il localname
/*

127.0.0.1       localhost hal9ce1ec  : il nome si risolve come HAL e gli ultimi 3 byte del mac address
::1             localhost ip6-localhost ip6-loopback
fe00::0         ip6-localnet
ff00::0         ip6-mcastprefix
ff02::1         ip6-allnodes
ff02::2         ip6-allrouters

10.0.0.241      server
*/

void aggiorna_hosts(char * nuovoip,int tipo)
{
FILE *fp,*newfile;
int trovato ;
char nf[]="/etc/hosts";
char stringa[200],local_name[20];

  strcpy(local_name,"crono");
  if (tipo==1) strcpy(local_name,"hal");
  dammi_mac(stringa);
  strcat(local_name,stringa);
  aggiorna_hostname(local_name);

  fp = fopen(nf,"r");
  newfile = fopen("tmp.file","w");                    // Apro un file di appoggio per aggiornare i valori
  if(fp<=0) {                               // Se il file non esiste
      trace(__LINE__,__FILE__,1000,0,0,"File di risoluzione dns <%s> non trovato, lo creo di default",nf);
      sprintf(stringa,"127.0.0.1       localhost %s\n",local_name);
      if (newfile) fputs(stringa,newfile);
      sprintf(stringa,"::1             localhost ip6-localhost ip6-loopback\n");
      if (newfile) fputs(stringa,newfile);
      sprintf(stringa,"fe00::0         ip6-localnet\n");
      if (newfile) fputs(stringa,newfile);
      sprintf(stringa,"ff00::0         ip6-mcastprefix\n");
      if (newfile) fputs(stringa,newfile);
      sprintf(stringa,"ff02::1         ip6-allnodes\n");
      if (newfile) fputs(stringa,newfile);
      sprintf(stringa,"ff02::2         ip6-allrouters\n");
      if (newfile) fputs(stringa,newfile);
      sprintf(stringa,"%s      server\n",nuovoip);
      if (newfile) fputs(stringa,newfile);
  }
  else {
    trovato = 0 ;
    while(fgets(stringa,160,fp)){                     // Leggo il file sorgente
      if ( strstr(stringa,"127.0.0.1")   ){               // Modifico il local name
        sprintf(stringa,"127.0.0.1       localhost %s\n",local_name);
        if (newfile) fputs(stringa,newfile);
      }
      else{
        if ( strstr(stringa,"server")   ){                // Modifico l'ip del server
          sprintf(stringa,"%s      server\n",nuovoip);
          if (newfile) fputs(stringa,newfile);
        }
        else{
          if (newfile) fputs(stringa,newfile);              // Scarico il resto del file in quello nuovo
        }
      }
    }
    fclose (fp);
    fclose (newfile);
    if(!rename(nf,"appo")) {                      // Rinomino il vecchio file in uno di appoggio
      if (!rename("tmp.file",nf)) {                   // Rinomino il nuovo file con il suo nome corretto
        sprintf(stringa,"sudo chmod 777 %s",nf);
        system(stringa);
        remove("appo");                             // se tutto OK cancello il vecchio di appoggio
      }
      else rename("appo",nf) ;                        // Se va male il rename ripristino il vecchio
    }
  }
}
void aggiorna_hostname(char * nuovonome)
{
FILE *newfile;
int trovato ;
char nf[]="/etc/hostname";
char stringa[200];
  newfile = fopen("tmp.file","w");                    // Apro un file di appoggio per aggiornare i valori
  if (newfile) {
    sprintf(stringa,"%s\n",nuovonome);
    fputs(stringa,newfile);
    fclose (newfile);

    if(!rename(nf,"appo")) {                      // Rinomino il vecchio file in uno di appoggio
      if (!rename("tmp.file",nf)) {                   // Rinomino il nuovo file con il suo nome corretto
        sprintf(stringa,"sudo chmod 777 %s",nf);
        system(stringa);
        remove("appo");                             // se tutto OK cancello il vecchio di appoggio
      }
      else rename("appo",nf) ;                        // Se va male il rename ripristino il vecchio
    }
  }
}

void aggiorna( char * nuovoip, char * subnet, char *gway )
{
FILE *fp,*newfile;
int trovato ;
char nf[]=NETWORK_FILE;
char stringa[200];

  fp = fopen(nf,"r");

  if(fp<=0) {                               // Se il file non esiste
      trace(__LINE__,__FILE__,1000,0,0,"File di configurazione rete <%s> non trovato",nf);
      return;
  }
  if( fp ){
    newfile = fopen("tmp.file","w");                  // Apro un file di appoggio per aggiornare i valori
    trovato = 0 ;
    while(fgets(stringa,160,fp)){                     // Leggo il file sorgente
        if ( ( strstr(stringa,"iface eth0 inet") == NULL)  ){
          if (newfile) fputs(stringa,newfile);              // Scarico la parte prima del file in quello nuovo
        }
        else{
          if(strlen(nuovoip )>1) {
            sprintf(stringa,"iface eth0 inet static\n");
            if (newfile) fputs(stringa,newfile);
            sprintf(stringa,"     address %s\n",nuovoip);
            if (newfile) fputs(stringa,newfile);
            sprintf(stringa,"     netmask %s\n",subnet);
            if (newfile) fputs(stringa,newfile);
            sprintf(stringa,"     gateway %s\n",gway);
          }
          else strcpy(stringa,"iface eth0 inet dhcp\n");
          if (newfile) fputs(stringa,newfile);              // Scrive
          strcpy(stringa,"     dns-nameservers 8.8.8.8\n");
          if (newfile) fputs(stringa,newfile);              // Scrive
          trovato = 1 ;
          break;
        }
    }
    fclose (fp);
    fclose (newfile);
    if (trovato == 1){                          // Devo sostituire il file
      if(!rename(nf,"appo")) {                      // Rinomino il vecchio file in uno di appoggio
        if (!rename("tmp.file",nf)) {                   // Rinomino il nuovo file con il suo nome corretto
             sprintf(stringa,"sudo chmod 777 %s",nf);
             system(stringa);
             remove("appo");                            // se tutto OK cancello il vecchio di appoggio
        }
        else rename("appo",nf) ;                        // Se va male il rename ripristino il vecchio
      }
    }
  }
  else
  {
      trace(__LINE__,__FILE__,1000,0,0,"Errore nella gestione del file di configurazione rete");
      return;
  }

}

void dammi_mac(char *address)
{
  struct ifreq s;
  int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

  strcpy(s.ifr_name, "eth0");
  if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) {
      sprintf(address,"%02x%02x%02x", (unsigned char) s.ifr_addr.sa_data[3],(unsigned char) s.ifr_addr.sa_data[4],(unsigned char) s.ifr_addr.sa_data[5]);
//    printf("%s\n",address);
  }
  else strcpy(address,"000000");
  return ;
}

int dammiNomeDbSQLite(char * nome)
{
FILE *fp;
char nome_db[100];
int ret ;

  rw_strpar(nome_db,"IMPIANTO/SQLData", "" , NO_DB | READ_PAR | IMPIANTO_FILE ) ;
  if (nome_db[0]==0) {
    strcpy(nome,FILE_DB);                         // Se database non specificato prendo il nome di default
    ret = 1 ;                               // Indico DB Default
  }
  else {
    strcpy(nome,nome_db);                       // Altrimenti lavoro con quello impostato nel file
    ret = 2;                                // Indico DB dal file di configurazione
  }
  fp = fopen(nome,"r");                         // Provo ad aprire il file in lettura per vedere se esiste
  if(fp <= (FILE *) 0)                          // Se il file del DATA BASE non esiste
    ret = - ret ;                             // Indico che il file non esiste
  else
    fclose (fp) ;                             // Richiudo
  return ( ret );
}
int checkDb(int opz)
{
FILE *fp;
char nome_db[100],nf[100],stringa[100];
int kk,jj ;

  kk = dammiNomeDbSQLite(NOME_DB) ;                   // Se negativo non esiste il file

// printf("Apro il file SQLite DB\n");
  if ( opz == 1 && kk > 0 ) {
    remove(NOME_DB);                          // Se forzata un rigenerazione del file
    kk = (- kk) ;                             // Adesso non esiste piu' il file
  }

  if (kk < 0 ) {
    crea_tabella(1);                          // Creo la tabella che contiene tutti i valori.
                                    // Carico il file di tarature
    strcpy(nf,DIR_IMPIANTO);
    strcat(nf,FILE_IMPIANTO);
printf("Inizio caricamento file %s \n",nf);
    caricaFileNelDB(nf);
printf("Finito caricamento file %s \n",nf);
                                    // Controllo se ci sono altri file da caricare nel DB
    jj = rw_valpar("IMPIANTO/ParamFiles", "0" , READ_PAR | IMPIANTO_FILE ); // Default nessun file in piu' di tarature
    for(kk = 0 ; kk< jj ; kk ++ ) {
      sprintf(stringa,"IMPIANTO/ParamFile%d",kk+1);
      rw_strpar(nf,stringa, "" , READ_PAR | IMPIANTO_FILE ) ;
printf("Inizio caricamento file %s\n",nf);
      caricaFileNelDB(nf);
printf("Finito caricamento file %s\n",nf);
    }
  }
  return(0) ;
}
int caricaFileNelDB(char * nome_file)
{
FILE *fp;
int reterr = 0;
char stringa[200];
char argomento[200];
char descri[200];
char valore[200];

int kk,jj ;

   if ( dammiNomeDbSQLite(NOME_DB) < 0 ){
      reterr = 2 ;                            // DB non creato dal file di configurazione
    }
    else {
//printf("CaricaFileNelDB : %s\n", NOME_DB);
      fp = fopen(nome_file,"r");
      while(fgets(stringa,160,fp)){
        for (kk = 0 ;stringa[kk]!=0 ;kk++) {
          if(stringa[kk]==' ') {                    // Compattazione dei blank
            for(jj=kk;stringa[jj]!=0;jj++) stringa[jj]=stringa[jj+1] ;  // non usare il : memcpy(&stringa[kk],&stringa[kk+1],160-(kk+1) ); // Puo' dare errore
            kk--;
          }
          if( stringa[kk]==0xD || stringa[kk]==0xA ) {          // Eliminazione del CR LF
            stringa[kk] = 0 ;
            break;
          }
        }
        if (strlen(stringa)==0) continue ;                // Salta le righe vuote
        if (stringa[0]=='#') continue ;                 // Salta le righe di commento

//printf("RIPULITO STRINGA = %s\n",stringa);
        if(stringa[0]=='[') {
          for (kk =1 ;stringa[kk]!=']';kk++) argomento[kk-1]=stringa[kk];
          argomento[kk-1]=0;                      // Fine argomento
//printf("NUOVO ARGOMENTO = %s\n",argomento);
          continue;
        }
        else
        {
//printf("ELABORO = %s\n",argomento);

          for (kk=0 ;stringa[kk]!='=';kk++) descri[kk]=stringa[kk];
          descri[kk]=0;                         // Fine descrizione
//printf("DESCRIZIONE = %s\n",descri);

          kk++;                             // Passo dopo l'uguale
          for (jj=0;stringa[kk]!=0;kk++) if(stringa[kk]!='"') valore[jj++]=stringa[kk];
          valore[jj]=0;                         // Fine valore

//printf("NUOVO RECORD %s,%s,%s\n",argomento,descri,valore);
          set_config(argomento,descri,valore);
//printf("[%s] %s = %s\n",argomento,descri,valore);
        }
      }
    }
    reterr = 1 ;                            // DB creato dal file di configurazione
    fclose(fp);
    sprintf(stringa,"sudo chmod 777 %s",NOME_DB);
    system(stringa);
    sprintf(stringa,"whoami>temp");
    system(stringa);
    fp=fopen("temp","r");
    if (fp>0) {                             // Cerco l'utente
      fgets(argomento,160,fp);
      trim(argomento);
      fclose(fp);
    }
    else strcpy(argomento,"develop");                 // Metto l'utente di default
    sprintf(stringa,"sudo chown %s.%s %s",argomento,argomento,NOME_DB); // Assegno la proprieta' all'utente rilevato
printf("system %s\n",stringa);
    system(stringa);
    return(reterr) ;
}

/********************************************************************************************************************
 *                                                                                                                  *
                                         F U N C T I O N     S Q L I T E 3
 *                                                                                                                  *
 ********************************************************************************************************************/

int onlyone = 0 ;														// Flag che garantisce l'accesso id un thread alla volta al DBSqlite3

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

int crea_tabella ( int opt )
{
   sqlite3 *db;
   char *zErrMsg = 0;
   int  rc;
   char sql[2001];
   /* Open database */
   if (NOME_DB[0]==0)  dammiNomeDbSQLite(NOME_DB) ;
//printf("Provo ad aprire il db %s\n",NOME_DB);
   rc = SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE ;//| SQLITE_OPEN_URI ;
   rc = sqlite3_open_v2(NOME_DB, &db,rc,zErrMsg);
   if( rc ){
      printf("Can't open database: %s\n", sqlite3_errmsg(db));
      return(-1);
   }
//   else   printf("Open database: %s\n", NOME_DB);

/* Create SQL statement */
   sprintf(sql,"CREATE TABLE %s(id INTEGER PRIMARY KEY AUTOINCREMENT,argomento VARCHAR(50) NOT NULL,descrizione VARCHAR(50) NOT NULL,valore VARCHAR(50) NOT NULL);",NOME_TB);
/* */
//printf("SQL COMMAND:%s\n",sql);
   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      printf("Create SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   else{
      printf("DataBase rigenerato\n");
   }
   sqlite3_close(db);

   return (0);

}
int find_rec_sql( char * search )
{
   sqlite3 *db;
   sqlite3_stmt *res ;
   const char *zErrMsg = 0;
   int rc;

   /* Open database */
   if (NOME_DB[0]==0)  dammiNomeDbSQLite(NOME_DB) ;
//printf("Find Rec SQL %s\n",search);
   rc = SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE ;//| SQLITE_OPEN_URI ;
   rc = sqlite3_open_v2(NOME_DB, &db,rc,zErrMsg);
//printf("Apertura DB %d\n",rc);
   if( rc ){
     printf("Can't open database: %s\n", sqlite3_errmsg(db));
     search[0]=0;
     return (1) ;
   }
   /* Execute SQL statement */
   rc = sqlite3_prepare_v2(db, search, strlen(search), &res, &zErrMsg); // Apri la query
   if( rc != SQLITE_OK ){
      printf("Find SQL error: %s\n", zErrMsg);
      do rc = sqlite3_prepare_v2(db, search, strlen(search), &res, &zErrMsg); // Apri la query
      while( rc != SQLITE_OK );
   }
   if ( sqlite3_step(res) == SQLITE_ROW) sprintf(search,"%s",sqlite3_column_text(res, 0)); // Prendo solo il primo risultato che c'è
   else {
     search[0]=0;
     sqlite3_finalize(res);  // Chiudi la query
     sqlite3_close(db); // Chiudi il DB
     return(3);
   }
   sqlite3_finalize(res);  // Chiudi la query
   sqlite3_close(db); // Chiudi il DB
   return (0);
}
int upd_rec( char * sql)
{
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;

   /* Open database */
   if (NOME_DB[0]==0)  dammiNomeDbSQLite(NOME_DB) ;
   rc = SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE ;// | SQLITE_OPEN_URI ;
   rc = sqlite3_open_v2(NOME_DB, &db,rc,zErrMsg);
   if( rc ){
      printf("Update Can't open database: %s\n", sqlite3_errmsg(db));
      printf("Query: %s\n", sql);
      return (1);
   }

   /* Execute SQL statement */

   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
     printf("Update SQL error: %s\n", zErrMsg);
     printf("Query: %s\n", sql);
     do rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
     while( rc != SQLITE_OK );
   }

   sqlite3_close(db);

   return (0);
}
int get_config(char * argomento, char * descri, char * valore)
{
char sql_query[1000];
int reterr =  0 ;
int num_fields,i ;

  while (onlyone ==1 ) msec_sleep(10);
//printf("Semaforo %d\n",onlyread);
  onlyone = 1 ;

/* Ricerco il parametro */
  sprintf(sql_query,"SELECT valore FROM %s WHERE argomento='%s' AND descrizione='%s';",NOME_TB,argomento,descri);
//printf("SQL :%s\n",sql_query);
  if ( find_rec_sql(sql_query) == 0 ){ // Se valore esiste
//printf("Valore esiste\n");
// printf("campi %d \n",num_fields ) ;
    sprintf(valore,"%s",sql_query ) ; // In valore ritorno il valore letto
//printf("valore trovato %s\n",valore);
  }
  else{
//printf("Valore non esiste\n");
        sprintf(sql_query,"INSERT into %s (argomento,descrizione,valore)VALUES('%s','%s','%s');",NOME_TB,argomento,descri,valore);
// printf("Query %s\n",sql_query);
        if ( upd_rec(sql_query) ) reterr = 2; // Errore nell'inserimento
//printf("valore inserito %s\n",valore);
  }
  onlyone = 0 ;
  return (reterr) ;
}
int set_config(char * argomento, char * descri, char * valore)
{
char sql_query[1000];
int reterr =  0 ;

  while (onlyone == 1) msec_sleep(10);
//printf("Semaforo %d\n",onlyone);
  onlyone = 1 ;

/* Ricerco il parametro */
//printf("Set config\n");
  sprintf(sql_query,"SELECT valore FROM %s WHERE argomento='%s' AND descrizione='%s';",NOME_TB,argomento,descri);
//printf("%s\n",sql_query);
  if ( find_rec_sql(sql_query) == 0 ){ // Se valore esiste
//printf("valore esiste\n");
    sprintf(sql_query,"UPDATE %s set valore='%s' WHERE argomento='%s' AND descrizione='%s';",NOME_TB,valore,argomento,descri);
    if ( upd_rec(sql_query) ) reterr = 1; // Errore nell'aggiornamento
  }
  else // Se valore non esiste allora lo inserisco in tabella
  {
//printf("Valore non esiste\n");
    sprintf(sql_query,"INSERT into %s (argomento,descrizione,valore)VALUES('%s','%s','%s');",NOME_TB,argomento,descri,valore);
    if ( upd_rec(sql_query) ) reterr = 2; // Errore nell'inserimento
  }
  onlyone = 0 ;
  return (reterr) ; // OK
}
