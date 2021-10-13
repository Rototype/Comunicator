#include "extern.h"
#include "ws.h"
#include "sha1.h"
#include "base64.h"
#include "json.h"

// #define DEBUG_PRINT 1  													// Attivo le stampe di debug
//#define DEBUG_INVERSIONE_FILE	 1 										// Sviluppo l'inversione di un file .BMP prima di inviarlo. Se commentato invia il FILE_TX cosi come lo legge

int Init_WebSocket(int ) ;
int InitAree_WebSocket(int ) ;
int WebSocket(struct connectManage * ,unsigned char *,int,int) ;
int Connect_WebSocket(struct connectManage *) ;
void SendPoll_WebSocket(int,int,int);
int SendCmd_WebSocket(int,char *,int);

// Funzioni che servono internamente per la gestione del protocollo
int getHSresponse(char *, char **);
int getHSaccept(char *, unsigned char **);

// Interpretazione dei comandi in arrivo da sviluppare in base alle specifiche di protocollo
void Interpreta_WebSocket(struct connectManage *) ;

// Puntatore alla struttura di gestione del canale di comunicazione
struct protoManage *ProtoConnectWb ;
extern int splitstr(char *str, char *sepa, char ***arr);
extern struct SendArea AreaSpi ;                    // Variabile globale per inviare l'evento al driver SPI

extern int img_lenblk_limit_on;
int send_img_ok = 0;
extern int receivedACK;
extern int pkt_tx_ending;
extern int pkt_rx_ending;
int recv_idx=0;
char recv_cfg[1000];
int recv_cfg_len=0;
int recv_next_pkg=1;
/************************** FUNCTION DEVELOP SECTION ******************/

/**
 *
 * Questa funzione attualmente si limita a fare le printf dei singoli campi
 * ricercando delle parole chiave in un array definito in appglob.h
 * deve essere sviluppata la trattazione delle variabili.... si potrebbe
 * pensare a gestire le variabili all'interno di un database SQLite 3.0
 * sul quale fare poi riferimento...per le variabili di tipo numerico
 * si potrebbe pensare di associare un indice alle parole chiavi e di tenere
 * sia sul DB l'ultimo valore salvato sia in ram sulla memoria condivisa
 * per lavorare runtime...
 *
 **/

int parseJson(char *car)
{
  int i,kk;
  int r,j,trovato;
  jsmn_parser p;
  jsmntok_t t[128];                           // Al massimo non piu' di 128 tokens
  char *JSON_STRING = NULL ;

  JSON_STRING = (char *) malloc(strlen(car)+4);             // Mi alloco lo spazio per la stringa JSON

  if(car[0]!='{') sprintf(JSON_STRING,"{%s}",car);            // Accetto stringhe sia con che senza {} contenitive
  else sprintf(JSON_STRING,"%s",car);

  jsmn_init(&p);                            // Inizializzo per una nuova interpretazione
  r = jsmn_parse(&p, JSON_STRING, strlen(JSON_STRING), t, sizeof(t) / sizeof(t[0]));
  if (r < 0) {
    printf("Sintassi JSON non riconosciuta nella stringa errore: %d\n", r);
    return 1;
  }

  /** Mi aspetto una stringa che inizia e finisce con {.......} **/
  if (r < 1 || t[0].type != JSMN_OBJECT) {
    printf("Oggetto JSON non riconosciuto\n");
    return 1;
  }

  /** Ricerco fra le parole chiave registrate nel vettore jsonObj **/
  for (i = 1; i < r; i++) {
    for ( trovato = kk = 0 ; kk< ROOTOBJ ; kk++ ){            // Cerco fra le parole chiave "singole"
    if (jsoneq(JSON_STRING, &t[i], jsonObj[kk]) == 0) {
      printf("%s: %.*s\n",jsonObj[kk], t[i + 1].end - t[i + 1].start,JSON_STRING + t[i + 1].start);
      i++;
      trovato ++ ;
      break;
    }
  }
  if(!trovato) {                            // Altrimenti
/** Da implementare
      for ( trovato = kk = 0 ; kk< ARRAYOBJ ; kk++ ){         // Cerco fra le parole chiave "array"
      if (jsoneq(JSON_STRING, &t[i], jsonArr[kk]) == 0) {     // Si puo' pensare a delle parole chiave per delle variabili indicizzate (vettori)
      printf("- Gruppo:\n");
      if (t[i + 1].type != JSMN_ARRAY) {
        continue;                         // Ci si aspetta un vettore di stringhe
      }
      for (j = 0; j < t[i + 1].size; j++) {
        jsmntok_t *g = &t[i + j + 2];
        printf("  * %.*s\n", g->end - g->start, JSON_STRING + g->start);
      }
      i += t[i + 1].size + 1;
      trovato ++ ;
      break;
      }
    }
**/
    if(!trovato) {                          // Parola chiave non riconosciuta....
      printf("Parola chiave non codificata: %.*s\n", t[i].end - t[i].start,JSON_STRING + t[i].start);
      }
  }
  }
  free(JSON_STRING);                          // Libero la memoria
  return 0;
}

void SendEvent(void * arg)                        // La gestione degli eventi gli serve per mandare le risposte ai CLIENT che hanno chiesto un servizio
{
struct connectManage *Connect;
int kk ;
int i;
int appo;
struct paramWbSpi *p;

    Connect = (struct connectManage *) malloc(sizeof (struct connectManage)); // Mi alloco la memoria
    Connect->sav_handle = ProtoConnectWb->sav_handle ;        // Recupero l'handle della linea
#ifdef DEBUG_PRINT
    printf("Handle %d Thread gestione messaggi a eventi per Driver WebSocket in wait %ld\n",Connect->sav_handle,TAU_PLC);
#endif
  while(1){
//    pthread_mutex_unlock(&AreaSpi.Sema);                // Se voglio sbloccare la gestione delle variabili durante l'attesa di altre richieste
    pthread_cond_wait(&AreaSpi.ricevitori, &AreaSpi.sema);      // Aspetto che qualcuno mi richieda il servizio
//    pthread_mutex_lock(&AreaSpi.sema);                  // Se devo gestire la concorrenza del thread sulla gestione delle variabili
#ifdef DEBUG_PRINT
    printf("Sono entrato nello smaltimento della coda risposte ai comandi WEBSOCKET %ld\n",TAU_PLC);
#endif
    while (AreaSpi.smaRx != AreaSpi.accuRx) {           // Ho dei messaggi da smaltire ?
      kk = AreaSpi.smaRx++ ;                    // Prendo il primo indice da smaltire
      if (AreaSpi.smaRx>=AreaSpi.max) AreaSpi.smaRx = 0 ;     // Gestisco il puntatore di smaltimento
      p = AreaSpi.param[kk] ;                   // Punto all'area dei parametri di scambio
      Connect->client = p->rispoRx;               // Recupero il client a cui devo mandare la risposta
//      trace(__LINE__,__FILE__,104,0,0,"Rispondo ai CLIENT del WebSocket");
      int opt = 2;
      switch(p->cmdTx)                      // Riempio il buffer per la richiesta arrivata
      {
        case CMD_Restart:
          if(p->parint[0]>2 || p->parint[0]<=0) p->parint[0] = 1 ;// Controllo l'indice che abbia un valore tra 1 e 2
		  sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_Restart],SlaveUnit[p->parint[0]]);
        break;
        case CMD_ReadDigitalInput:
          sprintf(Connect->appo_bus,"%s@%s(0x%8.8X%8.8X)",CMD_tab[CMD_ReadDigitalInput],SlaveUnit[0],p->parint[0],p->parint[1]);
        break;
        case CMD_ReadAnalogInput:
          if(p->parint[0]>4 || p->parint[0]<0) p->parint[0] = 0 ;// Controllo l'indice che abbia un valore tra 1 e 5
          sprintf(Connect->appo_bus,"%s@%s(%ld)",CMD_tab[CMD_ReadAnalogInput],SlaveUnit[0],p->parint[p->parint[0]+1]); // Ritorno solo un valore ?! (pur avendoli tutti e 5 disponibili.... )
        break;
        case CMD_SetAnalogOutput:
          sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_SetAnalogOutput],SlaveUnit[0]);
        break;
        case CMD_SetDCMotor:
          sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_SetDCMotor],SlaveUnit[0]);
        break;
        case CMD_SetDCMotorPWM:
          sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_SetDCMotorPWM],SlaveUnit[0]);
        break;
        case CMD_SetDCSolenoid:
          sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_SetDCSolenoid],SlaveUnit[0]);
        break;
        case CMD_SetDCSolenoidPWM:
          sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_SetDCSolenoidPWM],SlaveUnit[0]);
        break;
        case CMD_SetDigitalOutput:
          sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_SetDigitalOutput],SlaveUnit[0]);
        break;
        case CMD_SetStepperMotorSpeed:
          sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_SetStepperMotorSpeed],SlaveUnit[0]);
        break;
        case CMD_SetStepperMotorCountSteps:
          sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_SetStepperMotorCountSteps],SlaveUnit[0]);
        break;
        case EVT_StopStepperMotor:
          sprintf(Connect->appo_bus,"%s@%s(%ld)",CMD_tab[EVT_StopStepperMotor],SlaveUnit[0],p->parint[0]);
          opt = 1; // setto opt=1 perchè opt=3 risulta occupato dalla gestione risposte contenenti file
        break;
        case CMD_UpdateConfiguration:
          sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_UpdateConfiguration],SlaveUnit[2]);
        break;
        case CMD_ReadConfiguration:
          if (p->parint[2] == recv_next_pkg) {
            if (p->parint[2] == 1) {       // se primo blocco
              appo = p->parint[1] - 26;     // data len = payload - 26
              recv_cfg_len = p->parint[3];   // salvo tot data length
            }
            else {
              appo = p->parint[1];        // data len = payload
              AreaSpi.smaRx--;
              if (AreaSpi.smaRx < 0) AreaSpi.smaRx = AreaSpi.max-1;
            }
            // printf(">>> data %d (len %d) : ", p->parint[2], appo);
            for (i=0; i<appo; i++) {
              recv_cfg[recv_idx+i] = p->parstr[i/MAXCH][i%MAXCH];     // copy to recv buffer
              // printf(" %.2X('%c')", p->parstr[i/MAXCH][i%MAXCH], p->parstr[i/MAXCH][i%MAXCH]);
            }
            recv_idx += appo;
            // printf("\n");

            if (pkt_rx_ending == 0x03) {   // se ETX
              sprintf(Connect->appo_bus,"%s@%s",CMD_tab[CMD_ReadConfiguration],SlaveUnit[2]);
              kk=strlen(Connect->appo_bus);
              for (i=0; i<recv_cfg_len; i++) {
                Connect->appo_bus[kk++] = recv_cfg[i];
              }
              Connect->appo_bus[kk++] = 0x00;
              recv_idx = 0;
              recv_next_pkg = 1;
              if (p->parint[2] > 1) {
                AreaSpi.smaRx++;
                if (AreaSpi.smaRx>=AreaSpi.max) AreaSpi.smaRx = 0 ;
              }
            }
            else {
              recv_next_pkg++;
              continue;  // skip sending message to WebSocket
            }
          }
          else {
            printf("Ricevuto blocco %d, atteso %d\n", p->parint[2], recv_next_pkg);
            continue;  // skip sending message to WebSocket
          }
        break;
      }
      SendCmd_WebSocket(Connect->client,Connect->appo_bus,opt);   // Invio risposta al Client WebSocket che aveva fatto richiesta
      // printf("smaRx %d , accuRx = %d\n",AreaSpi.smaRx, AreaSpi.accuRx);
      recv_next_pkg = 1;
    }
#ifdef DEBUG_PRINT
printf("Coda WEB SOCKET smaltita %ld\n",TAU_PLC);
#endif
  }
}
void Interpreta_WebSocket(struct connectManage *Connect)
{
int kk, appo;
char **brr = NULL ;
static int out_fd ;
static long lenBlk = 0 ;
static int typeBlk = 0 ;
char configStr[1000];

int rc = 0;
int charPresent = 0;
int brr_cnt = 0;

  Connect->appo_bus[Connect->addr_dev] = 0 ;              				// Metto il fine stringa
  if(typeBlk!=0){
	  switch(typeBlk){
		case CMD_InvertImage:
			lenBlk ++ ;
			for(kk=0;kk<Connect->addr_dev;kk++) 
        if(Connect->appo_bus[kk]==']') 
          break;
			if(kk<=Connect->addr_dev-1){ 								// Trovato il fine file
// printf("Ultimo bytes <%X>\n",Connect->appo_bus[Connect->addr_dev-1]);		
				Connect->power_on = 2;
				typeBlk =  0; 											// Fine di accumulo nel buffer
     			write(out_fd, Connect->appo_bus, kk) ;					// Copio tutto sul file meno la parentesi
				close(out_fd);
// printf("Chiudo il file \n");
        if (lenBlk < IMG_LENBLK_LIMIT) {
          // printf("lenBlk = %ld\n", lenBlk);
          send_img_ok = SendImage();
          if (send_img_ok == 0) {
            sprintf(Connect->appo_bus,"%s@%s[",CMD_tab[CMD_InvertImage],SlaveUnit[0]);
            SendCmd_WebSocket(Connect->client,Connect->appo_bus,3); // Invio risposta al Client WebSocket
            for(rc=0;kk<Connect->addr_dev;kk++) 
              Connect->appo_bus[rc]=Connect->appo_bus[kk];
            Connect->addr_dev = rc;
          }
          else {
            printf("Error in SendImage() : %d\n", send_img_ok);
          }
          CleanupImgFiles();
        }
        else {
          printf("IMG_LENBLK_LIMIT reached\n");
          img_lenblk_limit_on = 1;
        }
			}
			else {
     			write(out_fd, Connect->appo_bus, Connect->addr_dev) ;	// Copio tutto sul file
// printf("Accumulo[%d] nel file %d bytes\n",lenBlk,Connect->addr_dev);		
				Connect->addr_dev = 0 ;									// Ho gia' interpretato tutto il buffer ricevuto
			}
		break;
	  }
  }
  if(Connect->addr_dev) {
#ifdef DEBUG_PRINT
printf("Letto %d caratteri\n",Connect->addr_dev); // ,Connect->appo_bus);
#endif
    charPresent = 0;
    rc = 0;
    brr_cnt = 0;


	  charPresent = instring(&Connect->appo_bus,'{');                			// Standard Json
	  if (charPresent) {                                                         
#ifdef DEBUG_PRINT                                                      
printf("JSON msg\n");                                                   
#endif                                                                  
		brr_cnt = splitstr(Connect->appo_bus , "@#{}", &brr ) ;        		// Separo i vari campi
	  }                                                                 
	  else 
    {                                                            
		  charPresent = instring(&Connect->appo_bus,'(');              			// Campi ASCII
		  if (charPresent) {                                                       
#ifdef DEBUG_PRINT                                                      
        printf("ASCII cmd msg\n");                                              
#endif                                                                  
		    brr_cnt = splitstr(Connect->appo_bus , "@#(),", &brr ) ;     		// Separo i vari campi
		  }                                                               
		  else 
      {                                                          
  		  charPresent = instring(&Connect->appo_bus,'[');            			// Campo binario Base64
	  	  if (charPresent) {                                                     
#ifdef DEBUG_PRINT                                                      
          printf("Base64 Binary Encode msg[%d]\n",rc);                            
#endif                                                                  
			    brr_cnt = splitstr(Connect->appo_bus , "@[]", &brr ) ;    		// Separo i vari campi
		    }
		    else 
        {
			    charPresent = instring(&Connect->appo_bus,'!');          			// Campo binario puro
			    if (charPresent) {
			      brr_cnt = splitstr(Connect->appo_bus , "@!", &brr ) ; 			// Separo i vari campi
#ifdef DEBUG_PRINT
            printf("Binary msg[%d]\n",rc);
#endif
			    }
		      else 
          {
#ifdef DEBUG_PRINT
            printf("No parameters msg\n");
#endif
			      brr_cnt = splitstr(Connect->appo_bus , "@#", &brr ) ;  		// Separo i tre campi tra @ e #
			    }
		    }
	    }
	  }
	  if(brr_cnt){ // Se ho letto qualcosa
	  for (kk=0 ; kk<CMDTOT ; kk++) 
      if(!(strcmp(brr[0],CMD_tab[kk]))) 
        break; // Trovo il comando fra quelli conosciuti
#ifdef DEBUG_PRINT
printf("Comando %s \n",CMD_tab[kk]);
#endif
	  switch(kk){                             // Sviluppo il comando trovato fra quelli riconosciuti
		case CMD_UpdateFirmware:
		break;
		case CMD_UpdateWebSocketFirmware:
		break;
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		case CMD_Restart:
		if(!(strcmp(brr[1],SlaveUnit[0]))){               // Accetto Main unit
		  trace(__LINE__,__FILE__,404,0,0,"Richiesto Restart Main");
		  sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_Restart],SlaveUnit[0]);
		  SendCmd_WebSocket(Connect->client,Connect->appo_bus,2);   // Invio risposta al Client WebSocket
		  system("sudo reboot");                    // Eseguo un restart software
		}
		else {
		  if(!(strcmp(brr[1],SlaveUnit[1]))){             // Accetto FPGA unit
			trace(__LINE__,__FILE__,404,0,0,"Richiesto Restart FPGA");
			//Connect->tx_var[0] = 1 ;                // Indico restart FPGA
			//SendEventRequest(Connect,CMD_Restart);          // Invio evento richiesta alla SPI
        sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_Restart],SlaveUnit[1]);
		    SendCmd_WebSocket(Connect->client,Connect->appo_bus,2);   // Invio risposta al Client WebSocket
        system("sh resetFPGA.sh &");
		  }
		  if(!(strcmp(brr[1],SlaveUnit[2]))){             // Accetto HWController unit
			trace(__LINE__,__FILE__,404,0,0,"Richiesto Restart HWController");
			//Connect->tx_var[0] = 2 ;                // Indico restart HWController
			//SendEventRequest(Connect,CMD_Restart);          // Invio evento richiesta alla SPI
        sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_Restart],SlaveUnit[2]);
		    SendCmd_WebSocket(Connect->client,Connect->appo_bus,2);   // Invio risposta al Client WebSocket
        system("sh resetZynq.sh &");
		  }
		}
		break;
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		case CMD_UpdateConfiguration:
		if(!(strcmp(brr[1],SlaveUnit[0]))){               // Accetto Main unit
		  trace(__LINE__,__FILE__,404,0,0,"Richiesto Update Configuration Main");
		  if(!parseJson(brr[2]))  {                 // Interpreto la stringa JSON per gestire i parametri
        FILE* configfile = fopen(FILE_CONFIG_JSON, "w");
        fwrite("{", sizeof(unsigned char), 1, configfile);
        fwrite(brr[2], sizeof(unsigned char), strlen(brr[2]), configfile);
        fwrite("}", sizeof(unsigned char), 1, configfile);
        fclose(configfile);
        sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_UpdateConfiguration],SlaveUnit[0]);
        SendCmd_WebSocket(Connect->client,Connect->appo_bus,2); // Invio risposta al Client WebSocket
		  }
		  else{                           // In caso di errore non risponde
			trace(__LINE__,__FILE__,404,0,0,"Errore stringa JSON %s",brr[2]);
		  }
		}
		else {
		  if(!(strcmp(brr[1],SlaveUnit[2]))){             // Accetto HWController unit
			trace(__LINE__,__FILE__,404,0,0,"Richiesto Update Configuration HWController");
			if(!parseJson(brr[2])){                 // Interpreto la stringa JSON per gestire i parametri, ritorna 1 per errore di interpretazione
			  // sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_UpdateConfiguration],SlaveUnit[2]);
			  // SendCmd_WebSocket(Connect->client,Connect->appo_bus,2); // Invio risposta al Client WebSocket
        sprintf(Connect->appo_bus,"{%s}",brr[2]);
        SendEventRequest(Connect,CMD_UpdateConfiguration);        // Invio evento richiesta alla SPI
			}
			else{                           // In caso di errore non risponde
			  trace(__LINE__,__FILE__,404,0,0,"Errore stringa JSON %s",brr[2]);
			}
		  }
		  else {
			trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando Update Configuration non riconosciuta");
		  }
		}
		break;
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		case CMD_ReadConfiguration:
		if(!(strcmp(brr[1],SlaveUnit[0]))){               // Accetto Main unit
		  trace(__LINE__,__FILE__,404,0,0,"Richiesto Read Configuration Main");
      int fileSize = dammiSize(FILE_CONFIG_JSON);
      FILE* configfile = fopen(FILE_CONFIG_JSON, "r");
      fread(configStr, sizeof(unsigned char), fileSize, configfile);
      fclose(configfile);
      sprintf(Connect->appo_bus,"%s@%s%s",CMD_tab[CMD_ReadConfiguration],SlaveUnit[0],configStr);
      kk = strlen(CMD_tab[CMD_ReadConfiguration]) + 1 + strlen(SlaveUnit[0]) + fileSize;
      Connect->appo_bus[kk] = 0x00;
      SendCmd_WebSocket(Connect->client,Connect->appo_bus,2); // Invio risposta al Client WebSocket
		}
		else {
		  if(!(strcmp(brr[1],SlaveUnit[2]))){             // Accetto HWController unit
			trace(__LINE__,__FILE__,404,0,0,"Richiesto Read Configuration HWController");
			// if(!parseJson(brr[2])){                 // Interpreto la stringa JSON per gestire i parametri, ritorna 1 per errore di interpretazione
			  // sprintf(Connect->appo_bus,"%s@%s{TODO}",CMD_tab[CMD_ReadConfiguration],SlaveUnit[2]);
			  // SendCmd_WebSocket(Connect->client,Connect->appo_bus,2); // Invio risposta al Client WebSocket
        // sprintf(Connect->appo_bus,"{%s}",brr[2]);
         SendEventRequest(Connect,CMD_ReadConfiguration);       // Invio evento richiesta alla SPI
			// }
			// else{                           // In caso di errore non risponde
			//   trace(__LINE__,__FILE__,404,0,0,"Errore stringa JSON %s",brr[2]);
			// }
		  }
		  else {
			trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando Update Configuration non riconosciuta");
		  }
		}
		break;
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		case CMD_UpdateNetworkConfiguration:
		if(!(strcmp(brr[1],SlaveUnit[0]))){               // Accetto solo Main unit
		  trace(__LINE__,__FILE__,404,0,0,"Richiesto Update Network Configuration");
		  if(!parseJson(brr[2])){                   // Interpreto la stringa JSON per gestire i parametri
        FILE* networkfile = fopen(FILE_NETWORK_JSON, "w");
        fwrite("{", sizeof(unsigned char), 1, networkfile);
        fwrite(brr[2], sizeof(unsigned char), strlen(brr[2]), networkfile);
        fwrite("}", sizeof(unsigned char), 1, networkfile);
        fclose(networkfile);
  			sprintf(Connect->appo_bus,"%s@%s#",CMD_tab[CMD_UpdateNetworkConfiguration],SlaveUnit[0]);
	  		SendCmd_WebSocket(Connect->client,Connect->appo_bus,2); // Invio risposta al Client WebSocket
		  }
		  else{                           // In caso di errore non risponde
			trace(__LINE__,__FILE__,404,0,0,"Errore stringa JSON %s",brr[2]);
		  }
		}
		else {
		  trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando Update Network Configuration non riconosciuta");
		}
		break;
	//######################################################################
		case CMD_ReadDigitalInput:                    // Richiesta lettura di tutti gli input digitali
		if(!(strcmp(brr[1],SlaveUnit[0]))){               // Accetto solo Main unit
		  trace(__LINE__,__FILE__,404,0,0,"Richiesto Read Digital Input");
		  SendEventRequest(Connect,CMD_ReadDigitalInput);       // Invio evento richiesta alla SPI
		}
		else {
		  trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando Read Digital Input non riconosciuta %s",brr[1]);
		}
		break;
		case CMD_ReadAnalogInput:
		  if(!(strcmp(brr[1],SlaveUnit[0]))){             // Accetto solo Main unit
			trace(__LINE__,__FILE__,404,0,0,"Richiesto Read Analog Input %s",brr[2]); // Indice dell'ingresso da leggere
			Connect->tx_var[0] = atoi(brr[2]) ;             // Mi porto dietro l'indice dell'Input da leggere da 1 a 5 valori accettati se diverso ritorna il valore di in 1
			SendEventRequest(Connect,CMD_ReadAnalogInput);        // Invio evento richiesta alla SPI
		  }
		  else {
			trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando Read Analog Input non riconosciuta");
		  }
		break;
		case CMD_SetAnalogOutput:
		  if(!(strcmp(brr[1],SlaveUnit[0]))){             // Accetto solo Main unit
			trace(__LINE__,__FILE__,404,0,0,"Richiesto Write Analog Output %s=%s",brr[2],brr[3]); // Indice dell'uscita da scrivere e valore
			Connect->tx_var[0] = atoi(brr[2]) ;             // Mi porto dietro i parametri necessari
			Connect->tx_var[1] = atoi(brr[3]) ;             // Mi porto dietro i parametri necessari
			SendEventRequest(Connect,CMD_SetAnalogOutput);        // Invio evento richiesta alla SPI
		  }
		  else {
		  trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando Write Analog Output non riconosciuta");
		  }
		break;
	//######################################################################
		case CMD_SetDCMotor:
		  if(!(strcmp(brr[1],SlaveUnit[0]))){             // Accetto solo Main unit
		  if(brr_cnt<4) {
			trace(__LINE__,__FILE__,404,0,0,"Parametri del comando CMD_SetDCMotor mancanti");
			break;
		  }
		  trace(__LINE__,__FILE__,404,0,0,"Richiesto Set DC Motor %s=%s",brr[2],brr[3]); // Indice dell'uscita da scrivere e valore
		  Connect->tx_var[0] = atoi(brr[2]) ;             // Mi porto dietro i parametri necessari
		  Connect->tx_var[1] = -2 ;                 // Ipotesi di Parametro errato
		  Connect->tx_var[2] = 100 ;                  // Forzo il valore del PWM al 100% di default
      Connect->tx_var[3] = 20 ;    // empty
		  Connect->tx_var[4] = CMD_SetDCMotor ;           // Mi salvo il comando a cui rispondere
	/** Possibili parametri come stringa in arrivo
	 * off,open,0,brake oppure on,1,cw,clockwise,ccw,counterclockwise,-1
	 **/
		  if (brr[3][0]=='-' || brr[3][0]=='0' || brr[3][0]=='1' ) Connect->tx_var[1] = atoi(brr[3]) ;
		  else {
			rc = traduci(brr[3],1);                 // Trasformo la stringa in un valore
			if(rc != PAR_val[PARTOT-1]) Connect->tx_var[1] = rc ;   // Se stringa trovata
		  }
			if(Connect->tx_var[1]==-1 || Connect->tx_var[1]==0 || Connect->tx_var[1]==1 )     // Se valore compreso fra -1 0 1
			  SendEventRequest(Connect,CMD_SetDCMotor);       // Invio evento richiesta alla SPI
			else
			  trace(__LINE__,__FILE__,404,0,0,"Parametro del comando CMD_SetDCMotor non riconosciuto <%s>",brr[3]);

		  }
		  else  trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando CMD_SetDCMotor non riconosciuta");
		break;
		case CMD_SetDCMotorPWM:
		  if(!(strcmp(brr[1],SlaveUnit[0]))){             // Accetto solo Main unit
			  if(brr_cnt<5) {
				trace(__LINE__,__FILE__,404,0,0,"Parametri del comando CMD_SetDCMotorPWM mancanti");
				break;
			  }
			  trace(__LINE__,__FILE__,404,0,0,"Richiesto Set DC MotorPWM %s=%s pwm %s perc. %s freq.",brr[2],brr[3],brr[4],brr[5]); // Indice dell'uscita da scrivere e valore
			  Connect->tx_var[0] = atoi(brr[2]) ;             				// Mi porto dietro i parametri necessari
			  Connect->tx_var[1] = -2 ;                 					// Ipotesi di Parametro errato
			  Connect->tx_var[2] = atoi(brr[4]);              				// Leggo il valore del PWM tra 0% e 100%
        Connect->tx_var[3] = atoi(brr[5]);                  // Leggo frequency
			  Connect->tx_var[4] = CMD_SetDCMotorPWM ;          			// Mi salvo il comando a cui rispondere
	/** Possibili parametri come stringa in arrivo
	 * off,open,0,brake oppure on,1,cw,clockwise,ccw,counterclockwise,-1
	 **/
			  if (brr[3][0]=='-' || brr[3][0]=='0' || brr[3][0]=='1' ) Connect->tx_var[1] = atoi(brr[3]) ;
			  else {
				rc = traduci(brr[3],1);                 // Trasformo la stringa in un valore
				if(rc != PAR_val[PARTOT-1]) Connect->tx_var[1] = rc ;   // Se stringa trovata
			  }
        printf("freq pwm = %d\n", Connect->tx_var[3]);
			  if((Connect->tx_var[1]==-1 || Connect->tx_var[1]==0 || Connect->tx_var[1]==1 ) && Connect->tx_var[2]>=0 && Connect->tx_var[2]<=100 && Connect->tx_var[3]>=20 && Connect->tx_var[3]<=100 && (Connect->tx_var[3] % 5) == 0)
				  // Se valore compreso fra -1 0 1      e percentuale di PWM tra 0 e 100      e frequenza PWM fra 20 e 100 kHz  e frequenza a step di 5 kHz
				SendEventRequest(Connect,CMD_SetDCMotorPWM);       // Invio evento richiesta alla SPI
			  else trace(__LINE__,__FILE__,404,0,0,"Parametro del comando CMD_SetDCMotorPWM non riconosciuto <%s><%s>",brr[3],brr[4]);
		  }
		  else trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando CMD_SetDCMotorPWM non riconosciuta");
		break;
	//######################################################################
		case CMD_SetDCSolenoid:
		  if(!(strcmp(brr[1],SlaveUnit[0]))){             // Accetto solo Main unit
		  if(brr_cnt<4) {
			trace(__LINE__,__FILE__,404,0,0,"Parametri del comando CMD_SetDCSolenoid mancanti");
			break;
		  }
		  trace(__LINE__,__FILE__,404,0,0,"Richiesto Set DC Solenoid %s=%s",brr[2],brr[3]); // Indice dell'uscita da scrivere e valore
		  Connect->tx_var[0] = atoi(brr[2]) ;             // Mi porto dietro i parametri necessari
		  Connect->tx_var[1] = -2 ;                 // Ipotesi di Parametro errato
		  Connect->tx_var[2] = 100 ;                  // Forzo il valore del PWM al 100% di default
		  Connect->tx_var[3] = 0 ;                  // Forzo il valore dei millisecondi iniziali a 0
		  Connect->tx_var[4] = CMD_SetDCSolenoid ;            // Mi salvo il comando a cui rispondere
	/** Possibili parametri come stringa in arrivo
	 * off,0 oppure on,1
	 **/
		  if (brr[3][0]=='0' || brr[3][0]=='1' ) Connect->tx_var[1] = brr[3][0]-0x30 ;
		  else {
			rc = traduci(brr[3],1);                 // Trasformo la stringa in un valore
			if(rc != PAR_val[PARTOT-1]) Connect->tx_var[1] = rc ;   // Se stringa trovata
		  }
		  if(Connect->tx_var[1]>= 0 && Connect->tx_var[1]<=1)       // Se valore compreso fra 0 1
			SendEventRequest(Connect,CMD_SetDCSolenoid);      // Invio evento richiesta alla SPI
		  else trace(__LINE__,__FILE__,404,0,0,"Parametro del comando CMD_SetDCSolenoid non riconosciuto <%s>",brr[3]);
		  }
		  else trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando CMD_SetDCSolenoid non riconosciuta");
		break;
		case CMD_SetDCSolenoidPWM:
		  if(!(strcmp(brr[1],SlaveUnit[0]))){             // Accetto solo Main unit
		  if(brr_cnt<6) {
			trace(__LINE__,__FILE__,404,0,0,"Parametri del comando CMD_SetDCSolenoidPWM mancanti");
			break;
		  }
		  trace(__LINE__,__FILE__,404,0,0,"Richiesto Set DC SolenoidPWM %s=%s",brr[2],brr[3]); // Indice dell'uscita da scrivere e valore
		  Connect->tx_var[0] = atoi(brr[2]) ;             // Mi porto dietro i parametri necessari
		  Connect->tx_var[1] = -2 ;                 // Ipotesi di Parametro errato
		  Connect->tx_var[2] = atoi(brr[4])  ;            // Forzo il valore del PWM al 100% di default
		  Connect->tx_var[3] = atoi(brr[5])  ;            // Forzo il valore dei millisecondi iniziali a 0
		  Connect->tx_var[4] = CMD_SetDCSolenoidPWM ;         // Mi salvo il comando a cui rispondere
	/** Possibili parametri come stringa in arrivo
	 * off,0 oppure on,1
	 **/
		  if (brr[3][0]=='0' || brr[3][0]=='1' ) Connect->tx_var[1] = brr[3][0]-0x30 ;
		  else {
			rc = traduci(brr[3],1);                 // Trasformo la stringa in un valore
			if(rc != PAR_val[PARTOT-1]) Connect->tx_var[1] = rc ;   // Se stringa trovata
		  }
		  if((Connect->tx_var[1]>=0 && Connect->tx_var[1]<=1)  &&   // Se valore compreso fra 0 1
			 (Connect->tx_var[2]>=0 && Connect->tx_var[2]<=100)&&   // Se valore compreso fra 0 - 100
			 (Connect->tx_var[3]>=0 && Connect->tx_var[3]<=2000))   // Se valore compreso fra 0 - 2000
			SendEventRequest(Connect,CMD_SetDCSolenoid);      // Invio evento richiesta alla SPI
		  else trace(__LINE__,__FILE__,404,0,0,"Parametro del comando CMD_SetDCSolenoidPWM non riconosciuto <%s>",brr[3]);

		  }
		  else trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando CMD_SetDCSolenoidPWM non riconosciuta");
		break;
	//######################################################################
		case CMD_SetDigitalOutput:
		  if(!(strcmp(brr[1],SlaveUnit[0]))){             // Accetto solo Main unit
		  trace(__LINE__,__FILE__,404,0,0,"Richiesto Write Digital Output %s=%s",brr[2],brr[3]); // Indice dell'uscita da scrivere e valore
		  Connect->tx_var[0] = atoi(brr[2]) ;             // Mi porto dietro i parametri necessari
	/** Possibili parametri come stringa in arrivo
	 * off,0 oppure on,1
	 **/
		  if (brr[3][0]=='0' || brr[3][0]=='1' ) Connect->tx_var[1] = brr[3][0]-0x30 ;
		  else {
			rc = traduci(brr[3],1);                 // Trasformo la stringa in un valore
			if(rc != PAR_val[PARTOT-1]) Connect->tx_var[1] = rc ;   // Se stringa trovata
		  }
		  if(Connect->tx_var[1]>=0 && Connect->tx_var[1]<=1)      // Se valore compreso fra 0 1
			SendEventRequest(Connect,CMD_SetDigitalOutput);     // Invio evento richiesta alla SPI
		  }
		  else {
			trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando Write Digital Output non riconosciuta");
		  }
		break;
	//######################################################################
		case CMD_SetStepperMotorSpeed:                  // id, mode = brake;0;1;2;8;16 , speed = -n 0 +n , maxacc.= steps/s2 , load = 50 o 100 %
		  if(!(strcmp(brr[1],SlaveUnit[0]))){             // Accetto solo Main unit
		  if(brr_cnt<6) {
			trace(__LINE__,__FILE__,404,0,0,"Parametri del comando CMD_SetStepperMotorSpeed mancanti");
			break;
		  }
		  trace(__LINE__,__FILE__,404,0,0,"Richiesto Set Stepper Motor Speed %s=%s",brr[2],brr[3]); // Indice del motore
		  Connect->tx_var[0] = atoi(brr[2]) ;             // Mi porto dietro i parametri necessari
		  Connect->tx_var[1] = -2 ;                 // mode Ipotesi di Parametro errato
		  Connect->tx_var[2] = atoi(brr[4])  ;            // valore della speed con segno
		  Connect->tx_var[3] = atoi(brr[5])  ;            // valore della maxacceleration
		  Connect->tx_var[4] = atoi(brr[6])  ;            // 50 o 100 % di carico
		  Connect->tx_var[5] = 0;                   // FORZO il valore degli STEP
		  Connect->tx_var[6] = CMD_SetStepperMotorSpeed ;       // Mi salvo il comando a cui rispondere
		  if (brr[3][0]>='0' && brr[3][0]<='9' ) Connect->tx_var[1] = atoi(brr[3]) ;
		  else {
			rc = traduci(brr[3],1);                 // Trasformo la stringa in un valore
			if(rc != PAR_val[PARTOT-1]) Connect->tx_var[1] = rc ;   // Se stringa trovata
		  }
		  if((Connect->tx_var[1]>=0 && Connect->tx_var[1]<=16) &&   // Se valore compreso fra 0 16
			 (Connect->tx_var[4]==50 || Connect->tx_var[4]==100))   // Se perc = 50 o 100
			SendEventRequest(Connect,CMD_SetStepperMotorSpeed);     // Invio evento richiesta alla SPI
		  else {
			trace(__LINE__,__FILE__,404,0,0,"Parametro del comando CMD_SetStepperMotorSpeed non riconosciuto <%s>",brr[3]);
		  }
		  }
		  else {
		  trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando CMD_SetStepperMotorSpeed non riconosciuta");
		  }
		break;
		case CMD_SetStepperMotorCountSteps:
		  if(!(strcmp(brr[1],SlaveUnit[0]))){             // Accetto solo Main unit
			  if(brr_cnt<7) {
				trace(__LINE__,__FILE__,404,0,0,"Parametri del comando CMD_SetStepperMotorCountSteps mancanti");
				break;
			  }
			  trace(__LINE__,__FILE__,404,0,0,"Richiesto Set Stepper Motor Speed Count steps %s=%s",brr[2],brr[3]); // Indice del motore
			  Connect->tx_var[0] = atoi(brr[2]) ;             // Mi porto dietro i parametri necessari
			  Connect->tx_var[1] = -2 ;                 // mode Ipotesi di Parametro errato
			  Connect->tx_var[2] = atoi(brr[4])  ;            // valore della speed con segno
			  Connect->tx_var[3] = atoi(brr[5])  ;            // valore della maxacceleration
			  Connect->tx_var[4] = atoi(brr[6])  ;            // 50 o 100 % di carico
			  appo = atoi(brr[7])  ;                          // numero di steps da eseguire
        if (appo < 0) {
          Connect->tx_var[2] = -Connect->tx_var[2];
          Connect->tx_var[5] = -appo;
        }
        else {
          Connect->tx_var[5] = appo;
        }
			  Connect->tx_var[6] = CMD_SetStepperMotorCountSteps ;        // Mi salvo il comando a cui rispondere
			  if (brr[3][0]>='0' && brr[3][0]<='9' ) Connect->tx_var[1] = atoi(brr[3]) ;
			  else {
				rc = traduci(brr[3],1);                 // Trasformo la stringa in un valore
				if(rc != PAR_val[PARTOT-1]) Connect->tx_var[1] = rc ;   // Se stringa trovata
			  }
			  if((Connect->tx_var[1]>=0 && Connect->tx_var[1]<=16) &&   // Se valore compreso fra 0 16
				 (Connect->tx_var[4]==50 || Connect->tx_var[4]==100))   // Se perc = 50 o 100
				   SendEventRequest(Connect,CMD_SetStepperMotorSpeed);     // Invio evento richiesta alla SPI
			  else trace(__LINE__,__FILE__,404,0,0,"Parametro del comando CMD_SetStepperMotorCountSteps non riconosciuto <%s>",brr[3]);
		  }
		  else trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando CMD_SetStepperMotorCountSteps non riconosciuta");

		break;
	//######################################################################
		case CMD_InvertImage:
		  if(!(strcmp(brr[1],SlaveUnit[0]))){             				// Accetto solo Main unit
			Connect->power_on = 3 ;
   		    lenBlk = 1 ;
			typeBlk = CMD_InvertImage ;									// Mi ricordo che sto bufferizzando un immagine
			rc = Connect->addr_dev - strlen(CMD_tab[CMD_InvertImage]) - strlen(SlaveUnit[0]) - 2 ;  // Tolgo il comando di riconoscimento 
/*	* /		
for(out_fd=0;out_fd<Connect->addr_dev;out_fd++) printf("%X.",brr[2][out_fd]);			
printf("\n");
/* */
//printf("Arrivati %d caratteri File \n",rc);			
  	     // brr[2] punta sul primo dato binario : Nel caso di .bmp file i primi due bytes sono BM
			out_fd = open(FILE_IMG_RX_BASE64, O_WRONLY | O_CREAT | O_TRUNC);  // Apro in sovrascrittura
			if ( out_fd <= 0) break;
//printf("ultimo car <%c> <%X>\n",	brr[2][rc-1] ,brr[2][rc-1] );
			if(brr[2][rc-1]==']') {
			  typeBlk = 0 ;												// Gia' finito bufferizzazione immagine
			  write(out_fd, brr[2], rc-1) ;								// Copio tutto sul file
			  close(out_fd);
			  send_img_ok = SendImage();
        if (send_img_ok == 0) {
          sprintf(Connect->appo_bus,"%s@%s[",CMD_tab[CMD_InvertImage],CMD_tab[CMD_InvertImage]);
  			  SendCmd_WebSocket(Connect->client,Connect->appo_bus,3);   // Invio risposta al Client WebSocket
        }
        else {
          printf("Error in SendImage() : %d\n", send_img_ok);
        }
        CleanupImgFiles();
			}
			else {
			  write(out_fd, brr[2], rc) ;								// Copio tutto sul file
			}  
		  }
		  else trace(__LINE__,__FILE__,404,0,0,"SlaveUnit del comando CMD_InvertImage non riconosciuta");
		break;
		default:
      if (img_lenblk_limit_on || send_img_ok < 0) break;
		trace(__LINE__,__FILE__,498,0,0,"Comando non riconosciuto %s",Connect->appo_bus);
	//    SendCmd_WebSocket(Connect->client,Connect->appo_bus,0);
		break;
	  }
    for (int i = 0; i != brr_cnt; i++)
  	  free(brr[i]);
	  free(brr); // Libero l'area di split del comando
	  }
	  else {
      if (img_lenblk_limit_on || send_img_ok < 0) return;
		trace(__LINE__,__FILE__,498,0,0,"Comando non riconosciuto %s",Connect->appo_bus);
	//    SendCmd_WebSocket(Connect->client,Connect->appo_bus,0);
	  }
  } // Chiudo else typeBLK
}
/**
 * FUNZIONE CHE SPEDISCE UN EVENTO VERSO IL CANALE SPI
 */
void SendEventRequest(struct connectManage *Connect , int command)
{
char msg[100];
int idArea;
int i, j;
int appo, appo2;
int blk_len, read_idx;
char configstr[1000];

    idArea = AreaSpi.accu++;                      // Mi prendo una posizione da riempire del buffer circolare
    if (AreaSpi.accu >= AreaSpi.max) AreaSpi.accu = 0 ;         // Gestisco il puntatore di accumulo
  AreaSpi.param[idArea]->cmdTx = command ;              // Mi copio il comando che devo inviare al driver SPI
  AreaSpi.param[idArea]->rispoRx = Connect->client ;          // Salvo il client che deve ricevere la risposta
  switch(command){                          // Se, sulla base del comando, devo dare altre info al driver le smaltisco sotto lo switch
    case CMD_Restart:
      AreaSpi.param[idArea]->parint[0] = Connect->tx_var[0];    // Parametro da passare: id =1 FPGA id=2 HWController
    break;
    case CMD_ReadAnalogInput:
      AreaSpi.param[idArea]->parint[0] = Connect->tx_var[0];    // Parametro da passare: id dell'input analogico richiesto
    break;
    case CMD_SetAnalogOutput:
      AreaSpi.param[idArea]->parint[0] = Connect->tx_var[0];    // Parametro da passare: id dell'output analogico richiesto
      AreaSpi.param[idArea]->parint[1] = Connect->tx_var[1];    // Parametro da passare: valore analogico richiesto
    break;
    case CMD_SetDCMotor:
    case CMD_SetDCMotorPWM:
      AreaSpi.param[idArea]->parint[0] = Connect->tx_var[0];    // Parametro da passare: id del motore
      AreaSpi.param[idArea]->parint[1] = Connect->tx_var[1];    // Parametro da passare: valore STATUS da settare
      AreaSpi.param[idArea]->parint[2] = Connect->tx_var[2];    // Parametro da passare: valore pwm %
      AreaSpi.param[idArea]->parint[3] = Connect->tx_var[3];    // Parametro da passare: valore frequency
      AreaSpi.param[idArea]->parint[4] = Connect->tx_var[4];    // Mi ricordo il comando in ingresso
    break;
    case CMD_SetDCSolenoid:
      AreaSpi.param[idArea]->parint[0] = Connect->tx_var[0];    // Parametro da passare: id del motore
      AreaSpi.param[idArea]->parint[1] = Connect->tx_var[1];    // Parametro da passare: valore STATUS da settare
      AreaSpi.param[idArea]->parint[2] = Connect->tx_var[2];    // Parametro da passare: valore pwm %
      AreaSpi.param[idArea]->parint[3] = Connect->tx_var[3];    // Parametro da passare: valore msec iniziali
      AreaSpi.param[idArea]->parint[4] = Connect->tx_var[4];    // Mi ricordo il comando in ingresso
    break;
    case CMD_SetDigitalOutput:
      AreaSpi.param[idArea]->parint[0] = Connect->tx_var[0];    // Parametro da passare: id dell'output digitale richiesto
      AreaSpi.param[idArea]->parint[1] = Connect->tx_var[1];    // Parametro da passare: valore on off richiesto
    break;
    case CMD_SetStepperMotorSpeed:
      AreaSpi.param[idArea]->parint[0] = Connect->tx_var[0];    // Parametro da passare: id del motore
      AreaSpi.param[idArea]->parint[1] = Connect->tx_var[1];    // Parametro da passare: valore Mode da settare
      AreaSpi.param[idArea]->parint[2] = Connect->tx_var[2];    // Parametro da passare: valore speed
      AreaSpi.param[idArea]->parint[3] = Connect->tx_var[3];    // Parametro da passare: valore max acceleration
      AreaSpi.param[idArea]->parint[4] = Connect->tx_var[4];    // Parametro da passare: valore load 50 o 100 %
      AreaSpi.param[idArea]->parint[5] = Connect->tx_var[5];    // Parametro da passare: valore numero di steps
      AreaSpi.param[idArea]->parint[6] = Connect->tx_var[6];    // Mi ricordo il comando in ingresso
    break;
    case CMD_UpdateConfiguration:
      appo = strlen(Connect->appo_bus);     // lunghezza della config da scrivere
      for (i=0; i<appo; i++) {
          configstr[i] = Connect->appo_bus[i];
        }
      read_idx = 0;
      if (appo < 33) {    // calcolo numero totale blocchi
        appo2 = 1;
      }
      else {
        appo2 = 1 + ((appo-33)/HWC_CONFIG_MAX_BLOCK_SIZE) + 1;
      }
      for (int blk=1; blk<=appo2; blk++) {
        if (blk>1) AreaSpi.sma--;
        if (AreaSpi.sma < 0) AreaSpi.sma = AreaSpi.max-1;
        blk_len = appo - read_idx;
        if (blk_len > HWC_CONFIG_MAX_BLOCK_SIZE) {
          if (blk == 1) {
            blk_len = HWC_CONFIG_MAX_BLOCK_SIZE-24;   // nel primo blocco ci stanno meno byte del file per via dei vari parametri
          }
          else {
            blk_len = HWC_CONFIG_MAX_BLOCK_SIZE;
          }
        }
        if (blk == 1 && appo2 > 1) {    // if first block of multi
          pkt_tx_ending = 0x17;  // ETB
        }
        if (blk == appo2) {    // if last block
          pkt_tx_ending = 0x03;  // ETX
        }
        printf("blk = %d , blk_len = %d , tot_len = %d\n", blk, blk_len, appo);
        AreaSpi.param[idArea]->parint[0] = blk_len;
        AreaSpi.param[idArea]->parint[1] = blk;
        AreaSpi.param[idArea]->parint[2] = appo;
        AreaSpi.param[idArea]->parint[3] = read_idx;
        for (i=(blk-1)*HWC_CONFIG_MAX_BLOCK_SIZE; i<(blk)*HWC_CONFIG_MAX_BLOCK_SIZE; i++) {
          AreaSpi.param[idArea]->parstr[i/MAXCH][i%MAXCH] = configstr[i];
        }
        
        pthread_cond_signal(&AreaSpi.trasmettitore);            // Mando la signal al driver
        if (blk == 1) {
          read_idx += HWC_CONFIG_MAX_BLOCK_SIZE-24;
        }
        else {
          read_idx += HWC_CONFIG_MAX_BLOCK_SIZE;
        }
        printf("Attendo ACK\n");
        receivedACK = 0;
        while (receivedACK == 0) {
          usleep(10000);    // sleep 10 ms
        }
        if (receivedACK < 0) {
          printf("ACK non ricevuto, sospendo invio configurazione\n");
          return;
        }
        printf("ACK ricevuto, continuo con il blocco successivo %d/%d\n", blk, appo2);
      } // end of for multiple blocks
      printf("Finito invio configurazione\n");
    return;// break;

    case CMD_ReadConfiguration:
      // no extra parameters to pass to SPI
    break;

    default:
    break;
  }
#ifdef DEBUG_PRINT
printf("INVIA CHIAMATA A DRIVER SPI ################################################################## %ld\n",TAU_PLC);
#endif
  pthread_cond_signal(&AreaSpi.trasmettitore);            // Mando la signal al driver
}
int Connect_WebSocket(struct connectManage *Connect)
{
#ifdef DEBUG_PRINT
printf("Inizializzo l'area della connessione %d address %X\n",Connect->canale,Connect);
#endif
   Connect->timeout      =  0  ;
   Connect->flg_bus      =  0  ;
   Connect->flg_car      = -1  ;
   Connect->wait_bus     =  0  ;
   Connect->addr_dev     =  0  ;
   Connect->timeout_cnt  =  0  ;
   Connect->last_id_send = -1  ;
   Connect->poll_param   =  0  ;
   Connect->poll_device  =  0  ;
   Connect->power_on     =  0  ;  // Ricordati di fare l'inizializzazione del colloquio all'arrivo dei primi caratteri: HANDSHAKE INIZIALE

   if (Connect->polcan == -1 ){   // Ha la gestione degli eventi questa connessione ?
     if ( pthread_create(&Connect->child, NULL, SendEvent, NULL) != 0 )  // Installo la funzione che gestisce gli eventi
       printf("Pthread creazione SendEvent errore: %d - %s \n",errno,strerror(errno));
     else {
       pthread_detach(Connect->child);
       printf("Connect WebSocket : thread che deve catturare gli eventi tid %ld\n",Connect->child);
     }
   }

   // reset lenblk_limit flag
   img_lenblk_limit_on = 0;
   
   // reset send image error code
   send_img_ok = 0;

   return(0);
}
int Init_WebSocket(int pk)
{
   ProtoConnectWb = ptr[pk] ; // Puntatore alla struttura del protoManage : inizializzo il puntatore
   printf("Inizializzo l'area del protocollo WebSocket su canale %d connessione %d device %d\n", pk, ProtoConnectWb->canale, ProtoConnectWb->tot_device);
   return(0);
}
int InitAree_WebSocket(int pk)
{
   printf("Inizializzo l'area del protocollo WebSocket su canale %d device %d\n",pk, ProtoConnectWb->tot_device);
   return(0);
}

int SendCmd_WebSocket(int fd, char *msg,int opt) 						// Mascheratura del buffer da inviare
{
unsigned char *response;  												/* Response data.  */
unsigned char frame[10];  												/* Frame.          */
uint8_t idx_first_rData;  												/* Index data.     */
uint64_t length;          												/* Message length. */
int idx_response;         												/* Index response. */
int output,i;               											/* Bytes sent.     */
long sizeFile;
FILE * fin;

  if (! opt ) {                            								// Devo inviare un ACK alla richiesta
    msg[0]='A';
    msg[1]='C';
    msg[2]='K';
    sizeFile = 0;
  } else if ( opt == 2 ){                          							// Devo inviare un END alla richiesta
    msg[0]='E';
    msg[1]='N';
    msg[2]='D';
    sizeFile = 0;
  } else if (opt==3)  {
    msg[0]='E';
    msg[1]='N';
    msg[2]='D';
    sizeFile = dammiSize(FILE_IMG_TX_BASE64)+1;							// Invio del file invia + ']'
  } else {
    sizeFile = 0;
  }
//printf("Size file %d\n",sizeFile);  
  length = strlen( (const char *) msg) + sizeFile ;

  frame[0] = (WS_FIN | WS_FR_OP_TXT);

  if (length <= 125)													/* Split the size between octects. */
  {
    frame[1] = length & 0x7F;
    idx_first_rData = 2;
  }
  else if (length >= 126 && length <= 65535)							/* Size between 126 and 65535 bytes. */
  {
    frame[1] = 126;
    frame[2] = (length >> 8) & 255;
    frame[3] = length & 255;
    idx_first_rData = 4;
//printf("Dimensione tra 126 e 65535\n");    
  }
  else                                                                  /* More than 65535 bytes. */
  {
    frame[1] = 127;
    frame[2] = (unsigned char) ((length >> 56) & 255);
    frame[3] = (unsigned char) ((length >> 48) & 255);
    frame[4] = (unsigned char) ((length >> 40) & 255);
    frame[5] = (unsigned char) ((length >> 32) & 255);
    frame[6] = (unsigned char) ((length >> 24) & 255);
    frame[7] = (unsigned char) ((length >> 16) & 255);
    frame[8] = (unsigned char) ((length >> 8) & 255);
    frame[9] = (unsigned char) (length & 255);
    idx_first_rData = 10;
  }

  /* Add frame bytes. */
  idx_response = 0;
//printf("Malloc di %d\n",sizeof(unsigned char) * (idx_first_rData + length + 1));
  response = malloc( sizeof(unsigned char) * (idx_first_rData + length + 1) );
  for (i = 0; i < idx_first_rData; i++)
  {
    response[i] = frame[i];
    idx_response++;
  }

  /* Add data bytes. */
  for (i = 0; i < strlen( (const char *) msg) ; i++)
  {
    response[idx_response] = msg[i];
    idx_response++;
  }
  
  /* Add file bytes */
  if (opt==3){
	sizeFile--;  
//printf("Apro il file id %d\n",idx_response);
    fin = fopen(FILE_IMG_TX_BASE64, "r");											// Apro il file da leggere
    fread(&response[idx_response], sizeof(unsigned char), sizeFile, fin);// leggo il file in memoria
    fclose(fin);														// Chiudo il file
// printf("base64 = %s\n", &response[idx_response]);
    idx_response += sizeFile ;											// Aggiorno il puntatore dei caratteri caricati in memoria
//printf("Chiudo il file id %d\n",idx_response);
    response[idx_response++] = ']';
  }
  response[idx_response] = '\0';
//  output = write(fd,response,idx_response);                   			// Invio richiesta sulla linea

  output = sendsock(fd, response, idx_response,0);
printf("SendSock %d tot car %d %s\n",fd,idx_response,response + 1);

  free(response);
  return (output);
}

/** ##################### INTERPRETAZIONE DEL PACCHETTO RICEVUTO ##################
 * Nel primo carattere del nuovo frame c'e'
 *          WS_FIN = 0x80 alto se il pacchetto contiene tutto il messaggio che e' stato spedito.
 *          Se e' basso questo bit allora seguiranno piu' pacchetti da gestire con 
 *          le solite machere
 *          Se il resto dei bit sono a 0 indica che il pacchetto e' successivo ad uno
 * 			gia' avviato altrimenti ha il valore di TEXT = 1 o binario = 2
 * Nel secondo carattere c'e' il tipo di codifica e la lunghezza del messaggio
 * 			se inferiore a 0x7E (126) indica la lunghezza del msg e quindi le maschere 
 *          cominciano dal carattere successivo
 * 			se vale 0x7E (126) allora i prossimi due bytes indicano la lunghezza del messaggio
 *          e le maschere cominciano dopo i prossimi 2 bytes (la lunghezza)
 * 			se vale 0x7F (127) allora i prossimi otto bytes indicano la lunghezza del messaggio
 *          e le maschere cominciano dopo i prossimi 8 bytes (la lunghezza)
 * Dopo la lunghezza ci sono 4 bytes di maschere e poi cominciano i dati
 * 
 * I dati devono essere smascherati facendo un EXOR (or esclusivo) con il carattere
 * di maschera corrispondente alla posizione in modulo 4.
 ** ###############################################################################
 */
int WebSocket(struct connectManage *Connect,unsigned char * car,int ncar,int canale)
{
int kk,tt,jj,ii;
char *appo ;
unsigned char pingpong[2];
/* Scommentare se si vuole vedere a video i caratteri in Hexadecimal che arrivano sul socket * /
char * appo_sch ;
    appo_sch = car ;
    printf("WEBSOCKET - %X stringa <%s> Ricevuto %d car  = <",car,Connect,ncar);
    for (jj=1;jj<=ncar;jj++) { printf("%X-",((*appo_sch++) & 0xFF) ); } // Esadecimali separati da -
    printf("> bus %d flg_car %d\n",Connect->flg_bus,Connect->flg_car);
/* */
  if (Connect->power_on == 0 ) {              // Devo fare l'handshake di una nuova connessione ?
      int hsok = getHSresponse( (char *) car, &appo);          // Controllo richiesta di handshake, mi ritorna la risposta da mandare
#ifdef DEBUG_PRINT
    printf("Handshaked, response: \n %s \n -------- \n ",(char *)&appo);
    printf("SendSock %d tot car %d\n",Connect->client,strlen(appo));
#endif
      sendsock(Connect->client,appo,strlen(appo),0);        		    // Invio richiesta sulla linea

//    sendsock(Connect->client, appo, strlen(appo));    // Rispondo per terminare l'handshake iniziale
      Connect->power_on = 1 ;               // Handshake OK ! Non devo piu' rifarlo....
      Connect->timeout = 0 ;                // Risposta arrivata
      Connect->flg_bus = 0 ;                // Mi preparo per il prossimo pacchetto
      Connect->flg_car = -1;                // Resetto richiesta comando
      Connect->errore  = 0 ;                // Nessun errore
      Connect->addr_dev= 0 ;                // Riparti dall'inizio a bufferizzare
      Connect->rx_var[8] = 0;				// Residuo del buffer precedente 
      if (hsok == 0) {
        free(appo);                       // Libero la memoria che e' stata utilizzata per preparare la risposta all'handshake iniziale che non mi servira' piu'
      }
  }
  else { // Handshake gia' effettuato => interpreto i comandi che mi arrivano
	if(Connect->power_on == 3 && Connect->len_msg>0) { // Sto ricevendo un blocco dati successivo 
		Connect->flg_bus = 2 ;
		Connect->addr_dev = 0 ; 
		Connect->flag_stop = ncar ;
//printf("Segue contenuto %d x bytes %d\n",Connect->flg_bus,Connect->flag_stop);
	}
/*  * /
for(jj=0;jj<ncar;jj++) printf("%X.",car[jj]);			
printf("\n");
/* */
  if (img_lenblk_limit_on || send_img_ok < 0) return;

    for (jj=1;jj<=ncar;jj++){
  //printf("FLG_BUS %d- wait %d\n",Connect->flg_bus,Connect->wait_bus );
      switch (Connect->flg_bus) // Analizzo un carattere alla volta
      {
        case 0: // Primo carattere del frame
			Connect->flg_car = *car++;
			if (Connect->flg_car == (WS_FIN | WS_FR_OP_PING) ){   		// Messagio tipo PING senza payload
			  trace(__LINE__,__FILE__,100,0,0,"Ricevuto un comando PING sul WebSocket");
			  pingpong[0] = (WS_FIN | WS_FR_OP_PONG) & 0xFF ;       	// Rispondo con il PONG
			  pingpong[1] = 0 ;
//			  write(Connect->client,pingpong,2);                  		// Invio risposta
              sendsock(Connect->client,pingpong,2,0);
			  Connect->flg_bus = -2 ;                 // Fine analisi se fosse un Ping con payload e' da trattare diversamente
			}
			else if (Connect->flg_car == (WS_FIN | WS_FR_OP_PONG) ) { // Messaggio tipo PONG
  			  trace(__LINE__,__FILE__,101,0,0,"Ricevuto un comando PONG sul WebSocket");
/* si dovrebbe liberare il buffer del client */
			}
			else if (Connect->flg_car == (WS_FIN | WS_FR_OP_CONT) ) { // Continuo di un messaggio
			  trace(__LINE__,__FILE__,101,0,0,"Ricevuto un comando di seconda parte del messaggio sul WebSocket");
			}
			else if (Connect->flg_car == (WS_FIN | WS_FR_OP_CLSE) ) {
			  trace(__LINE__,__FILE__,101,0,0,"Ricevuto un comando di chiusura canale di colloquio sul WebSocket");
			  return (1) ;// Messaggio di richiesta chiusura connessione
			}
			else if (Connect->flg_car != (WS_FIN | WS_FR_OP_TXT) ){
//			  trace(__LINE__,__FILE__,101,0,0,"Tipo di frame %X non GESTITO sul WebSocket",Connect->flg_car);
// 			  Connect->flg_bus = -3 ;                // Tipo frame non gestito
			}
		break;
		case 1: // Secondo carattere del frame per decodificare le eventuali maschere dei caratteri e la lunghezza del pacchetto
          Connect->meta  = *car ++;                 					// Leggo il secondo carattere
          Connect->rx_var[6] = 2;                 						// Default indice della chiave di maschera = 2
          Connect->flag_stop = (int) (Connect->meta & 0x7F);      		// Lunghezza del pacchetto
// printf("flag_stop %d %X\n",Connect->flag_stop,Connect->meta & 0x7F);
          if (Connect->flag_stop == 126) Connect->rx_var[6] = 4;  		// Se la lunghezza e' 126 allora l'indice della chiave diventa 4
          else if (Connect->flag_stop == 127) Connect->rx_var[6] = 10;  // Se 127 indice = 10
          Connect->rx_var[5] = Connect->rx_var[6] + 4;        			// Di conseguenza l'indice dei dati
          if( Connect->rx_var[8] ) Connect->flag_stop = Connect->rx_var[8] - Connect->rx_var[5];       		// Lunghezza del pacchetto
          else if (Connect->flag_stop >= 126) Connect->flag_stop = ncar - Connect->rx_var[5];       		// Lunghezza del pacchetto
          else{
  		    Connect->rx_var[7] = 1;	
 		    Connect->len_msg = Connect->flag_stop;	
//printf("ncar %d -len %d\n",ncar,Connect->flag_stop); 		    
		  }
          Connect->addr_dev = 0 ;                   					// Indice in cui inizia a bufferizzare il pacchetto smascherato
          Connect->last_id_send = 0 ;                   				// Indice della sequenza di smascheramento
#ifdef DEBUG_PRINT
printf("Secondo carattere %X id mask %d numero caratteri %d/%d id dati %d\n",Connect->meta,Connect->rx_var[6],Connect->flag_stop,ncar,Connect->rx_var[5]);
printf("%X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",*car,*(car+1),*(car+2),*(car+3),*(car+4),*(car+5),*(car+6),*(car+7),*(car+8),*(car+9),*(car+10),*(car+11),*(car+12),*(car+13));
#endif
      break;
      default:
        if (Connect->flg_bus >= Connect->rx_var[5] || (Connect->power_on==3 && Connect->len_msg>0 && Connect->rx_var[7] == 0) ) {    // Inizio del pacchetto dati
          Connect->appo_bus[Connect->addr_dev] = (*car) ^ Connect->rx_var[Connect->last_id_send % 4];
          Connect->last_id_send ++ ;
          Connect->addr_dev ++ ;										// in addr_dev ci trovo il numero di bytes ricevuti
          Connect->flag_stop -- ;
		  if( Connect->len_msg - Connect->addr_dev == 0) {				// Se ho finito il buffer precedente e ne sto cominciando uno nuovo
// printf("sopra %d Pacchetto lungo %d tot [%d] diff %d (last %d)\n",Connect->flag_stop,Connect->addr_dev,Connect->len_msg,Connect->len_msg-Connect->addr_dev,Connect->last_id_send);			
  		     Connect->last_id_send = Connect->len_msg = 0;
			 Interpreta_WebSocket(Connect);              				// Passo il buffer completo all'interprete dei messaggi custom 
			 Connect->flg_bus = -1;
			 Connect->rx_var[8] = Connect->flag_stop ;					// residuo dei caratteri con il nuovo blocco
			 Connect->addr_dev =  0;
		  }
        }
        else if (Connect->flg_bus == Connect->rx_var[6]+0 ) Connect->rx_var[0] = * car ;  // Maschere dei byte
        else if (Connect->flg_bus == Connect->rx_var[6]+1 ) Connect->rx_var[1] = * car ;
        else if (Connect->flg_bus == Connect->rx_var[6]+2 ) Connect->rx_var[2] = * car ;
        else if (Connect->flg_bus == Connect->rx_var[6]+3 ) { Connect->rx_var[3] = * car ; Connect->rx_var[7] = 0 ;}
        else {
		  Connect->rx_var[7] = 1;	
		  if(Connect->flg_bus==2) Connect->len_msg = *car ;
		  else Connect->len_msg = (Connect->len_msg << 8) | * car ;
//printf("[%d]=%X => %X\n",Connect->flg_bus,*car,Connect->len_msg);
		}
        car ++ ;
        if((!Connect->flag_stop) && (Connect->addr_dev>0)) {
 //         trace(__LINE__,__FILE__,104,0,0,"Interpreto comando sul WebSocket");
//printf("sotto Pacchetto lungo %d tot [%d] diff %d (last %d)\n",Connect->addr_dev,Connect->len_msg,Connect->len_msg-Connect->addr_dev,Connect->last_id_send);			
		  Connect->len_msg -= Connect->addr_dev ;
          Interpreta_WebSocket(Connect);              // Passo il buffer completo all'interprete dei messaggi custom
          Connect->flg_bus = -1  ;                // Fine pacchetto
          Connect->rx_var[8] = 0 ;
        }
      break;
      }
      Connect->flg_bus ++ ;
      if( Connect->flg_bus == -2 ) {                // Frame errato o non gestito
#ifdef DEBUG_PRINT
printf("Rispondo Errore Frame ricevuto non riconosciuto\n");
#endif
		Connect->flg_bus ++ ;
      }
      if( Connect->flg_bus == -1 ) {                // Finito analisi buffer
#ifdef DEBUG_PRINT
printf("Finito analisi buffer\n");
#endif
      Connect->timeout = 0 ;                    // Risposta arrivata
      Connect->flg_bus = 0 ;                    // Mi preparo per il prossimo pacchetto
      Connect->flg_car = -1;                    // Resetto richiesta comando
      Connect->errore  = 0 ;                    // Nessun errore
      Connect->addr_dev= 0 ;                    // Riparti dall'inizio a bufferizzare
      }
// printf("jj=%d flg=%d \n",jj,Connect->flg_bus);
    }
#ifdef DEBUG_PRINT
printf("ncar=%d flg=%d stop=%d\n",ncar,Connect->flg_bus,Connect->flag_stop);
#endif
  }
  if( Connect->power_on == 1 ) Connect->power_on ++ ;         // Porto a 2 power_on che significa che quello che ho letto non e' l'handshake
    return(0);                              // Tutto OK aspetto il prossimo pacchetto
}
/**
 * FUNZIONE CHE SPEDISCE IN POLLING (non utilizzata in quanto interfaccia SERVER)
 */
void SendPoll_WebSocket(int linea ,int conn ,int command)
{
char msg[100];

  switch(command){
    default:
      sprintf(msg,"%s@%s#",CMD_tab[CMD_ReadDigitalInput],SlaveUnit[0]);
	  SendCmd_WebSocket(ptr[linea]->Connect[conn]->client,msg,-1);   	// Invio richiesta al Server WebSocket
/*
  	  sprintf(msg,"%s@%s[",CMD_tab[CMD_InvertImage],CMD_tab[CMD_InvertImage]);
	  SendCmd_WebSocket(ptr[linea]->Connect[conn]->client,msg,1);   	// Invio richiesta al Server WebSocket
*/
printf("Send poll %s\n",msg);
    break;
  }
}

// ##########################################################################################################
//  ############### Funzioni per l'handshake iniziale di una nuova richiesta di connessione ################
/**
 * @param hsrequest  richiesta del Client
 * @param hsresponse risposta del Server da inviare al Client che ha fatto richiesta
 */
int getHSresponse(char *hsrequest, char **hsresponse)
{
  char *s;
  unsigned char *accept;
  char appo[1024];

  for (s = strtok(hsrequest, "\r\n"); s != NULL; s = strtok(NULL, "\r\n") ){
	strcpy(appo,s);  
	lower(appo);
    if (strstr(appo , WS_HS_REQ) != NULL) break;
  }
  s = strtok(s,    " ");
  s = strtok(NULL, " ");

  if (s == NULL) {
    printf("Error: wsKey is NULL\n");
    return -1;
  }
  getHSaccept(s, &accept);

  *hsresponse = malloc(sizeof(char) * WS_HS_ACCLEN);
  strcpy(*hsresponse, WS_HS_ACCEPT);
  strcat(*hsresponse, (const char *)accept);
  strcat(*hsresponse, "\r\n\r\n");

  free(accept);
  return (0);
}
int getHSaccept(char *wsKey, unsigned char **dest)
{
  SHA1Context ctx;
  char *str ;
  unsigned char hash[SHA1HashSize];

  str = malloc( sizeof(char) * (WS_KEY_LEN + WS_MS_LEN + 1) );
  strcpy(str, wsKey);
  strcat(str, MAGIC_STRING);

  SHA1Reset(&ctx);
  SHA1Input(&ctx, (const uint8_t *)str, WS_KEYMS_LEN);
  SHA1Result(&ctx, hash);

  *dest = base64_encode(hash, SHA1HashSize, NULL);
  *(*dest + strlen((const char *)*dest) - 1) = '\0';
  free(str);
  return (0);
}
//  ################################ Base64 encode e decode ########################################
static const unsigned char base64_table[65] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
unsigned char * base64_encode(const unsigned char *src, size_t len,size_t *out_len)
{
  unsigned char *out, *pos;
  const unsigned char *end, *in;
  size_t olen;
  int line_len;

  olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
  olen += olen / 72; /* line feeds */
  olen++; /* nul termination */
  if (olen < len)
    return NULL; /* integer overflow */
  out = malloc(olen);
  if (out == NULL)
    return NULL;

  end = src + len;
  in = src;
  pos = out;
  line_len = 0;
  while (end - in >= 3) {
    *pos++ = base64_table[in[0] >> 2];
    *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
    *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
    *pos++ = base64_table[in[2] & 0x3f];
    in += 3;
    line_len += 4;
    if (line_len >= 72) {
      *pos++ = '\n';
      line_len = 0;
    }
  }

  if (end - in) {
    *pos++ = base64_table[in[0] >> 2];
    if (end - in == 1) {
      *pos++ = base64_table[(in[0] & 0x03) << 4];
      *pos++ = '=';
    } else {
      *pos++ = base64_table[((in[0] & 0x03) << 4) |
                (in[1] >> 4)];
      *pos++ = base64_table[(in[1] & 0x0f) << 2];
    }
    *pos++ = '=';
    line_len += 4;
  }

  if (line_len)
    *pos++ = '\n';

  *pos = '\0';
  if (out_len)
    *out_len = pos - out;
  return out;
}
/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
unsigned char * base64_encode_nocr(const unsigned char *src, size_t len,size_t *out_len)
{
  unsigned char *out, *pos;
  const unsigned char *end, *in;
  size_t olen;
  int line_len;

  olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
//  olen += olen / 72; /* line feeds */
  olen++; /* nul termination */
  if (olen < len)
    return NULL; /* integer overflow */
  out = malloc(olen);
  if (out == NULL)
    return NULL;

  end = src + len;
  in = src;
  pos = out;
  line_len = 0;
  while (end - in >= 3) {
    *pos++ = base64_table[in[0] >> 2];
    *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
    *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
    *pos++ = base64_table[in[2] & 0x3f];
    in += 3;
    line_len += 4;
    if (line_len >= 72) {
//      *pos++ = '\n';
      line_len = 0;
    }
  }

  if (end - in) {
    *pos++ = base64_table[in[0] >> 2];
    if (end - in == 1) {
      *pos++ = base64_table[(in[0] & 0x03) << 4];
      *pos++ = '=';
    } else {
      *pos++ = base64_table[((in[0] & 0x03) << 4) |
                (in[1] >> 4)];
      *pos++ = base64_table[(in[1] & 0x0f) << 2];
    }
    *pos++ = '=';
    line_len += 4;
  }

//  if (line_len) *pos++ = '\n';

  *pos = '\0';
  if (out_len)
    *out_len = pos - out;
  return out;
}


/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
unsigned char * base64_decode(const unsigned char *src, size_t len,size_t *out_len)
{
  unsigned char dtable[256], *out, *pos, block[4], tmp;
  size_t i, count, olen;
  int pad = 0;

  memset(dtable, 0x80, 256);
  for (i = 0; i < sizeof(base64_table) - 1; i++)
    dtable[base64_table[i]] = (unsigned char) i;
  dtable['='] = 0;

  count = 0;
  for (i = 0; i < len; i++) {
    if (dtable[src[i]] != 0x80)
      count++;
  }

  if (count == 0 || count % 4) return NULL;
  olen = count / 4 * 3;
  pos = out = malloc(olen);
  if (out == NULL) return NULL;
  count = 0;
  for (i = 0; i < len; i++) {
    tmp = dtable[src[i]];
    if (tmp == 0x80)
      continue;

    if (src[i] == '=')
      pad++;
    block[count] = tmp;
    count++;
    if (count == 4) {
      *pos++ = (block[0] << 2) | (block[1] >> 4);
      *pos++ = (block[1] << 4) | (block[2] >> 2);
      *pos++ = (block[2] << 6) | block[3];
      count = 0;
      if (pad) {
        if (pad == 1)
          pos--;
        else if (pad == 2)
          pos -= 2;
        else {
          /* Invalid padding */
          free(out);
          return NULL;
        }
        break;
      }
    }
  }

  *out_len = pos - out;
  return out;
  
}
//  ################################ Secure Hashing Algoritm ########################################
/*
 *  Description:
 *      This file implements the Secure Hashing Algorithm 1 as
 *      defined in FIPS PUB 180-1 published April 17, 1995.
 *
 *      The SHA-1, produces a 160-bit message digest for a given
 *      data stream.  It should take about 2**n steps to find a
 *      message with the same digest as a given message and
 *      2**(n/2) to find any two messages with the same digest,
 *      when n is the digest size in bits.  Therefore, this
 *      algorithm can serve as a means of providing a
 *      "fingerprint" for a message.
 *
 *  Portability Issues:
 *      SHA-1 is defined in terms of 32-bit "words".  This code
 *      uses <stdint.h> (included via "sha1.h" to define 32 and 8
 *      bit unsigned integer types.  If your C compiler does not
 *      support 32 bit unsigned integers, this code is not
 *      appropriate.
 *
 *  Caveats:
 *      SHA-1 is designed to work with messages less than 2^64 bits
 *      long.  Although SHA-1 allows a message digest to be generated
 *      for messages of any number of bits less than 2^64, this
 *      implementation only works with messages with a length that is
 *      a multiple of the size of an 8-bit character.
 *
 */

/*
 *  Define the SHA1 circular left shift macro
 */
#define SHA1CircularShift(bits,word) \
                (((word) << (bits)) | ((word) >> (32-(bits))))

/* Local Function Prototyptes */
void SHA1PadMessage(SHA1Context *);
void SHA1ProcessMessageBlock(SHA1Context *);

/*
 *  SHA1Reset
 *
 *  Description:
 *      This function will initialize the SHA1Context in preparation
 *      for computing a new SHA1 message digest.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to reset.
 *
 *  Returns:
 *      sha Error Code.
 *
 */
int SHA1Reset(SHA1Context *context)
{
    if (!context)
    {
        return shaNull;
    }

    context->Length_Low             = 0;
    context->Length_High            = 0;
    context->Message_Block_Index    = 0;

    context->Intermediate_Hash[0]   = 0x67452301;
    context->Intermediate_Hash[1]   = 0xEFCDAB89;
    context->Intermediate_Hash[2]   = 0x98BADCFE;
    context->Intermediate_Hash[3]   = 0x10325476;
    context->Intermediate_Hash[4]   = 0xC3D2E1F0;

    context->Computed   = 0;
    context->Corrupted  = 0;

    return shaSuccess;
}

/*
 *  SHA1Result
 *
 *  Description:
 *      This function will return the 160-bit message digest into the
 *      Message_Digest array  provided by the caller.
 *      NOTE: The first octet of hash is stored in the 0th element,
 *            the last octet of hash in the 19th element.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to use to calculate the SHA-1 hash.
 *      Message_Digest: [out]
 *          Where the digest is returned.
 *
 *  Returns:
 *      sha Error Code.
 *
 */
int SHA1Result( SHA1Context *context,
                uint8_t Message_Digest[SHA1HashSize])
{
    int i;

    if (!context || !Message_Digest)
    {
        return shaNull;
    }

    if (context->Corrupted)
    {
        return context->Corrupted;
    }

    if (!context->Computed)
    {
        SHA1PadMessage(context);
        for(i=0; i<64; ++i)
        {
            /* message may be sensitive, clear it out */
            context->Message_Block[i] = 0;
        }
        context->Length_Low = 0;    /* and clear length */
        context->Length_High = 0;
        context->Computed = 1;

    }

    for(i = 0; i < SHA1HashSize; ++i)
    {
        Message_Digest[i] = context->Intermediate_Hash[i>>2]
                            >> 8 * ( 3 - ( i & 0x03 ) );
    }

    return shaSuccess;
}

/*
 *  SHA1Input
 *
 *  Description:
 *      This function accepts an array of octets as the next portion
 *      of the message.
 *
 *  Parameters:
 *      context: [in/out]
 *          The SHA context to update
 *      message_array: [in]
 *          An array of characters representing the next portion of
 *          the message.
 *      length: [in]
 *          The length of the message in message_array
 *
 *  Returns:
 *      sha Error Code.
 *
 */
int SHA1Input(    SHA1Context    *context,
                  const uint8_t  *message_array,
                  unsigned       length)
{
    if (!length)
    {
        return shaSuccess;
    }

    if (!context || !message_array)
    {
        return shaNull;
    }

    if (context->Computed)
    {
        context->Corrupted = shaStateError;

        return shaStateError;
    }

    if (context->Corrupted)
    {
         return context->Corrupted;
    }
    while(length-- && !context->Corrupted)
    {
    context->Message_Block[context->Message_Block_Index++] =
                    (*message_array & 0xFF);

    context->Length_Low += 8;
    if (context->Length_Low == 0)
    {
        context->Length_High++;
        if (context->Length_High == 0)
        {
            /* Message is too long */
            context->Corrupted = 1;
        }
    }

    if (context->Message_Block_Index == 64)
    {
        SHA1ProcessMessageBlock(context);
    }

    message_array++;
    }

    return shaSuccess;
}

/*
 *  SHA1ProcessMessageBlock
 *
 *  Description:
 *      This function will process the next 512 bits of the message
 *      stored in the Message_Block array.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:

 *      Many of the variable names in this code, especially the
 *      single character names, were used because those were the
 *      names used in the publication.
 *
 *
 */
void SHA1ProcessMessageBlock(SHA1Context *context)
{
    const uint32_t K[] =    {       /* Constants defined in SHA-1   */
                            0x5A827999,
                            0x6ED9EBA1,
                            0x8F1BBCDC,
                            0xCA62C1D6
                            };
    int           t;                 /* Loop counter                */
    uint32_t      temp;              /* Temporary word value        */
    uint32_t      W[80];             /* Word sequence               */
    uint32_t      A, B, C, D, E;     /* Word buffers                */

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++)
    {
        W[t] = context->Message_Block[t * 4] << 24;
        W[t] |= context->Message_Block[t * 4 + 1] << 16;
        W[t] |= context->Message_Block[t * 4 + 2] << 8;
        W[t] |= context->Message_Block[t * 4 + 3];
    }

    for(t = 16; t < 80; t++)
    {
       W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->Intermediate_Hash[0];
    B = context->Intermediate_Hash[1];
    C = context->Intermediate_Hash[2];
    D = context->Intermediate_Hash[3];
    E = context->Intermediate_Hash[4];

    for(t = 0; t < 20; t++)
    {
        temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);

        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    context->Intermediate_Hash[0] += A;
    context->Intermediate_Hash[1] += B;
    context->Intermediate_Hash[2] += C;
    context->Intermediate_Hash[3] += D;
    context->Intermediate_Hash[4] += E;

    context->Message_Block_Index = 0;
}

/*
 *  SHA1PadMessage
 *

 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64
 *      bits represent the length of the original message.  All bits in
 *      between should be 0.  This function will pad the message
 *      according to those rules by filling the Message_Block array
 *      accordingly.  It will also call the ProcessMessageBlock function
 *      provided appropriately.  When it returns, it can be assumed that
 *      the message digest has been computed.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to pad
 *      ProcessMessageBlock: [in]
 *          The appropriate SHA*ProcessMessageBlock function
 *  Returns:
 *      Nothing.
 *
 */

void SHA1PadMessage(SHA1Context *context)
{
    /*
     *  Check to see if the current message block is too small to hold
     *  the initial padding bits and length.  If so, we will pad the
     *  block, process it, and then continue padding into a second
     *  block.
     */
    if (context->Message_Block_Index > 55)
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 64)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }

        SHA1ProcessMessageBlock(context);

        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }
    else
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 56)
        {

            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }

    /*
     *  Store the message length as the last 8 octets
     */
    context->Message_Block[56] = context->Length_High >> 24;
    context->Message_Block[57] = context->Length_High >> 16;
    context->Message_Block[58] = context->Length_High >> 8;
    context->Message_Block[59] = context->Length_High;
    context->Message_Block[60] = context->Length_Low >> 24;
    context->Message_Block[61] = context->Length_Low >> 16;
    context->Message_Block[62] = context->Length_Low >> 8;
    context->Message_Block[63] = context->Length_Low;

    SHA1ProcessMessageBlock(context);
}

int traduci(char *line,int type)
{
int kk;
  for(kk=0;kk<CMDTOT;kk++) if(!(strcmp(line,PAR_str[kk]))) return(PAR_val[kk]);
  return(PAR_val[CMDTOT-1]);
}

int SendImage()
{	
#ifdef DEBUG_PRINT
  printf("inside SendImage()\n");
#endif

	// ------- Converti il file da BASE64 a binario con un header x y profondita' ------- 

  int offset;
  int fileSize = dammiSize(FILE_IMG_RX_BASE64);								// File di ingresso da decodificare 
  FILE* fin = fopen(FILE_IMG_RX_BASE64, "r");
// printf("fin is %s\n", fin != NULL ? "OK" : "NO");
  unsigned char* encod = (unsigned char *) malloc(fileSize);  							// Mi alloco la memoria per caricare l'immagine
  int bytes_read = fread(encod, sizeof(unsigned char), fileSize, fin); 					// carico in un colpo solo l'immagine in memoria

  if (bytes_read != fileSize) {
    printf("Something wrong reading file: read %d bytes of %d\n", bytes_read, fileSize);
    return;
  }

#ifdef DEBUG_PRINT
  printf("Chiamo la decode %d\n",fileSize);
#endif
  unsigned char* img = base64_decode(encod, fileSize, &offset) ;
  // printf("img %X encod %X offset %d\n",img,encod,offset);
  fclose(fin);	
/*
  printf("encod = %s\n",encod);
  printf("img = ");
  for (int i=0; i<offset; i++) printf(" %.2x",img[i]);
  printf("\n");
*/
  if (offset == 0 || img == NULL) {
    printf("Error decoding file\n");
    return -1;
  }

  FILE* fout = fopen(FILE_IMG_RX_BINARY, "wb");									// File di uscita
// printf("fout is %s\n", fout != NULL ? "OK" : "NO");
  int bytes_write = fwrite(img, sizeof(unsigned char), offset, fout); 										// Scrivo in un colpo solo il file codificato
  fclose(fout);
  free(img);
  free(encod);

#ifdef DEBUG_PRINT
  printf("bytes read = %d/%d   bytes write = %d/%d\n",bytes_read, fileSize, bytes_write, offset);
#endif

  if (bytes_write != offset) {
    printf("Something wrong writing decoded data\n");
    return -2;
  }

  //  ------- Invia tramite SOCKET allo Zynq ------- 
  //  ------- Ricevi dal SOCKET un immagine in binario con header x y e profondita' ------- 
  // InvertiImage();

  char curlcmd[256];
  sprintf(curlcmd, "curl -o %s --http0.9 --data-binary @%s --local-port %d --interface %s %s:%d -H User-Agent: -H Accept: -H Host: -H Content-Length: -H Content-Type: -H Expect: --max-time %d", FILE_IMG_TX_BINARY, FILE_IMG_RX_BINARY, IMG_LOCAL_PORT, IMG_LOCAL_INTERFACE, IMG_ZYNQ_IP_ADDR, IMG_ZYNQ_PORT, IMG_SENDCMD_TIMEOUT);
  //#ifdef DEBUG_PRINT
    printf("curlcmd: %s\n", curlcmd);
  //#endif
  int curl_ok = system(curlcmd);

  if ( WEXITSTATUS(curl_ok) != 0 && WEXITSTATUS(curl_ok) != 56) {
    printf("curl error : %d\n", WEXITSTATUS(curl_ok));
    return -3;
  }
  
  //  ------- Converti in BASE64 e rispondi al WebSocket richiedente  ------- 

  fileSize = dammiSize(FILE_IMG_TX_BINARY);
  fin = fopen(FILE_IMG_TX_BINARY, "rb");
  img = (unsigned char *) malloc(fileSize);  							// Mi alloco la memoria per caricare l'immagine
  fread(img, sizeof(unsigned char), fileSize, fin); 					// carico in un colpo solo l'immagine in memoria
  offset = fileSize;
#ifdef DEBUG_PRINT
  printf("Chiamo la encode %d\n",fileSize);
#endif
    encod = base64_encode_nocr(img, fileSize, &offset) ;
    fout = fopen(FILE_IMG_TX_BASE64, "w");									// File di uscita
    if (fout <0 )
      printf("Errore apertura file in scrittura %d, %d\n",fout,errno);
    else {
#ifdef DEBUG_PRINT
  printf("Write del file %d\n",offset);
#endif
      fwrite(encod, sizeof(unsigned char), offset, fout); 										// Scrivo in un colpo solo il file codificato
      fclose(fout);
    }
    free (encod) ;														// Libero la memoria allocata
    free(img);

#ifdef DEBUG_PRINT
  printf("outside SendImage()\n");
#endif

  return 0;
}


InvertiImage()
{
int i,fileSize;
int offset;
unsigned char * img;
unsigned char * encod;
FILE * fin;
FILE * fout;
/*
printf("Inverti Immagine\n");
    fileSize = dammiSize(FILE_RX_IMMAGINE);								// File di ingresso da decodificare 
printf("Size file %d\n",fileSize);
    fin = fopen(FILE_RX_IMMAGINE, "r");
	encod = (unsigned char *) malloc(fileSize);  							// Mi alloco la memoria per caricare l'immagine
printf("encod %X size %d\n",encod,fileSize);
    fread(encod, sizeof(unsigned char), fileSize, fin); 					// carico in un colpo solo l'immagine in memoria
printf("Chiamo la decode %d\n",fileSize);
    img = base64_decode(encod, fileSize, &offset) ;
    fclose(fin);	
printf("img %X encod %X offset %d\n",img,encod,offset);
//    free(encod);
*/   
  fileSize = dammiSize(FILE_IMG_RX_BINARY);
  fin = fopen(FILE_IMG_RX_BINARY, "rb");
  img = (unsigned char *) malloc(fileSize);  							// Mi alloco la memoria per caricare l'immagine
  fread(img, sizeof(unsigned char), fileSize, fin); 					// carico in un colpo solo l'immagine in memoria
  offset = fileSize;

	if (img!=NULL) {
// printf("Decodificato il file %X %d\n",img,offset); // ,img[0],img[1]);
/*	
    fileSize = dammiSize(FILE_RX_IMMAGINE) - 54;						// File di ingresso BMP
    fin = fopen(FILE_RX_IMMAGINE, "r");
    fread(info, sizeof(unsigned char), 54, fin); 						// leggo i 54-bytes di header
    memcpy(&offset, info + 10, sizeof(int));
	img = (unsigned char *) malloc(fileSize);  							// Mi alloco la memoria per caricare l'immagine
*/

	    fileSize = offset - 54 ;
printf("img+10 %X offset %d @ %X size %d\n",img+10,offset,&offset,sizeof(int));
		memcpy(&offset,img+10, sizeof(int));
printf("offset %d\n",offset);
		encod = img + 54;
printf("img %X encod %X\n",img,encod);
		
printf("Apro il file di uscita\n");
#ifdef DEBUG_INVERSIONE_FILE
 		for(i = offset-54; i < fileSize; i += 3) 						// Manipolo i dati dell'immagine saltando l'eventuale tavolozza 
		{
			encod[i]  = 255-encod[i] ;											// Inverto i valori di RGB
			encod[i+1]= 255-encod[i+1] ;
			encod[i+2]= 255-encod[i+2] ;
		}
#endif
		fileSize += 54 ;

// printf("Chiamo la encode %d\n",fileSize);
		//encod = base64_encode_nocr(img, fileSize, &offset) ;
    offset = fileSize;
		fin = fopen(FILE_IMG_TX_BINARY, "wb");									// File di uscita
		if (fin <0 )
printf("Errore apertura file in scrittura %d, %d\n",fin,errno);
		else {
printf("Write del file %d\n",offset);
			fwrite(img, sizeof(unsigned char), offset, fin); 										// Scrivo in un colpo solo il file codificato
			fclose(fin);
		}
		free (img) ;														// Libero la memoria allocata
printf("Rigirato il file\n");
	}
	else {
printf("Errori di decodifica BASE64\n");
	}
    return ;
}

void CleanupImgFiles() {
#ifdef DEBUG_PRINT
  printf("cleanup image files\n");
#endif
  FILE* f;
  f = fopen(FILE_IMG_RX_BASE64, "w");
  fclose(f);
  f = fopen(FILE_IMG_RX_BINARY, "wb");
  fclose(f);
  f = fopen(FILE_IMG_TX_BINARY, "wb");
  fclose(f);
  f = fopen(FILE_IMG_TX_BASE64, "w");
  fclose(f);
}