/****************** INCLUDE APPLICATION SECTION ***********************/
#include "extern.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include "define.h" // CROSS_DEBUG

/**              Caratteri speciali del protocollo  				  */
#define STX 0x02
#define ETX 0x03
#define ACK 0x06
#define DLE 0x10
#define NAK 0x15
#define ETB 0x17

#ifdef CROSS_DEBUG
  #define DEBUG_PRINT 1                           // Disabilito le stampe di debug
  #define SIMULA_RISPOSTA 1                         // Simula la risposta dalla SPI HWC
  #define DISABLE_ACK 1										// Disabilita l'invio di ACK e NACK (solo per motivi di testing/debugging)
  #define DISABLE_ACK_TIMEOUT_HANDLER 1						// Disabilita la ritrasmissione dell'ultimo comando in caso di timeout ACK
#endif

#define TIMEOUT_ACK 			(500/TAU) 					// Timeout a 500 msec sull'attesa di risposta ACK NAK
#define MAX_RETRANSMISSIONS		5							// Numero massimo di ritrasmissioni comandi in caso di mancata ricezione ACK / NAK	

/************ PROTOTYPE PROTOCOL FUNCTION REFERENCES SECTION **********/
extern void CalCRC(char *, int , char *);

/************** PROTOTYPE FUNCTION DECLARATIONS SECTION ***************/
int Init_SPIso2110S(int ) ;
int InitAree_SPIso2110S(int ) ;
int SPIso2110S(struct connectManage * ,unsigned char *,int,int) ;
int Connect_SPIso2110S(struct connectManage *) ;
int interpretaS(struct connectManage *);
int rispondi(char,struct connectManage *);

struct protoManage *ProtoConnectSpiS ;									// Variabile di gestione della comunicazione su questo canale

int last_pkt_len = -1;											// copia del valore di lunghezza dell'ultimo pacchetto inviato
unsigned char* last_pkt_buf = NULL;								// puntatore al buffer dati dell'ultimo pacchetto inviato
int receivedACK = 0;
int pkt_tx_ending = ETX;
int pkt_rx_ending = 0;

/************************** FUNCTION DEVELOP SECTION ******************/
int Connect_SPIso2110S(struct connectManage *Connect) 
{
#ifdef DEBUG_PRINT
printf("Inizializzo l'area della connessione sul canale %d address %X\n",Connect->canale,Connect);
#endif
   Connect->timeout      = 0  ;
   Connect->flg_bus      = 0  ;
   Connect->flg_car      = -1 ;
   Connect->wait_bus     = 0  ;
   Connect->addr_dev     = -1 ;
   Connect->timeout_cnt  = 0  ;
   Connect->last_id_send = -1 ;
   Connect->poll_param   = 0  ;
   Connect->power_on     = 0  ;
   Connect->poll_device  = 0  ; 
   return(0);
}
int Init_SPIso2110S(int pk)
{
  ProtoConnectSpiS = ptr[pk] ; 											// Puntatore alla struttura del protoManage : inizializzo il puntatore
printf("Inizializzo il canale %d SPI ISO2110 su connessione %d device %d\n",pk,ProtoConnectSpiS->canale, ProtoConnectSpiS->tot_device);
  return(0);
}
int InitAree_SPIso2110S(int pk)
{
printf("Inizializzo l'area del protocollo SPI ISO2110 SERVER (slave) su canale %d connessione %d device %d\n",pk,ProtoConnectSpiS->canale, ProtoConnectSpiS->tot_device);
  return(0);
}
/***********   Interpretazione del protocollo ISO2110 Slave ************
 * Questo protocollo serve per stare in ascolto sui messaggi spontanei.
 * Non viene trasmesso nulla se non l'handshake della gestione della 
 * risposta prevista dal protocollo.
 **********************************************************************/
int SPIso2110S(struct connectManage *Connect,unsigned char * car,int ncar,int canale) 
{
int kk,tt,jj,ii;
unsigned char check[2] ;

	// risposta di lunghezza = 1 , dovrebbe essere un ACK / NAK
	if (ncar == 1) {
		if (car[0] == ACK) {					// se ACK
#ifdef DEBUG_PRINT
			printf("Ricevuto ACK, stop timeout handler (timeout = %d)\n", Connect->wait_bus);
#endif
			Stop_ACKTimeoutHandler();
		}
		else if (car[0] == NAK) {				// se NAK
#ifdef DEBUG_PRINT
			printf("Ricevuto NAK, invio nuovamente il comando e faccio ripartire il timeout handler\n");
#endif
			// Stop_ACKTimeoutHandler();				// il vecchio timeout handler finisce
			rispondi(1,Connect);				// rimanda l'ultimo pacchetto per timeout scaduto
			Set_ACKTimeout(TIMEOUT_ACK);	// reset timeout handler
#ifndef DISABLE_ACK_TIMEOUT_HANDLER
			// Start_ACKTimeoutHandler();	// lancio un nuovo timeout handler
#endif
		}
		else {
			printf("Ricevuto pacchetto di lunghezza 1 diverso da ACK/NAK : %.2X\n", car[0]);
		}

		return 0;								// fine gestione pacchetto di lunghezza 1
	}

/** STAMPA DEI CARATTERI RICEVUTI ( SCOMMENTARE ) * /
char * appo_sch ; 
    appo_sch = car ;
    printf("ISO2110 SLAVE - %X Ricevuto %d car  = <",Connect,ncar);
    for (jj=1;jj<=ncar;jj++) { printf("%X-",((*appo_sch++) & 0xFF) ); }
    printf("> bus %d flg_car %d\n",Connect->flg_bus,Connect->flg_car);
/**********************************************************************/

    for (jj=1;jj<=ncar;jj++){
      if (Connect->flg_car >= 0) {
		if(Connect->poll_device){ 
  		  if ( *(car) != DLE ) {
#ifdef DEBUG_PRINT
printf("Trovata sequenza DLE scorretta flg_car %d flg_bus %d car %X\n",Connect->flg_car,Connect->flg_bus,*car);				
#endif
			Connect->flg_bus = -3 ; 									// Se c'e' un DLE DLE ok altrimenti DLE x => NAK 
		  }
		  Connect->poll_device = 0 ; 									// Segnalo ok DLE DLE
		}
		else if ((Connect->chk_bus[Connect->flg_car++] = *car) == DLE ){ // Bufferizzo per calcolo checksum
		  car++;
		  Connect->poll_device = 1 ; 									// Segnalo che il prossimo carattere deve essere un DLE altrimenti errore
		  continue ; 													// Non faccio niente per questo ciclo con il primo DLE
		}
	  }
//printf("FLG_BUS %d- wait %d\n",Connect->flg_bus,Connect->wait_bus ); 	  
      switch (Connect->flg_bus) 										// Analizzo un carattere alla volta
      {
        case 0: 														// Carattere iniziale del frame DLE
		  Connect->flg_car = -1 ;  										// Non bufferizzare il messaggio
          if(*car != DLE) Connect->flg_bus = -1 ; 						// Aspetto un DLE iniziale
          car++;
        break;
        case 1: 														// II Carattere STX
          if(*car++ != STX) Connect->flg_bus = -1 ; 					// Aspetto una sequenza DLE STX per iniziare
		  else Connect->flg_car = 0 ;  									// Inizio a bufferizzare il messaggio
		break;
		case 2:
		  Connect->meta = *car++ ; 										// Parte alta del block number
		break;
		case 3: 														// finito il block number in addr_dev
		  Connect->addr_dev = (( Connect->meta << 8 ) & 0xFF00) | (*car++ & 0xFF)  ;  // Parte bassa del block number
		break;
		case 4: 														// Inizia l'OP CODE
		  Connect->rx_var[0] = ((*car>>4) & 0xF)  ;    					// Identify msg type (Primi 4 bit dell'opcode) in var[0]
		  Connect->meta = (*car++ & 0xF)  ;
		break;
		case 5: 														// Fine OP CODE
		  Connect->rx_var[1] =  (( Connect->meta << 8 ) & 0xF00) | (*car++ & 0xFF)  ;  // 12 bit OPCODE in var[1]
		break;
        case 6: 														// Primo byte QUANTITA' DI Bytes del payload
          Connect->meta = *car++ ;
        break;    
        case 7: 														// Secondo byte QUANTITA' DI Bytes del payload
          Connect->rx_var[2] = (( Connect->meta << 8 ) & 0xFF00) | *car++ ;
          Connect->rx_var[3] = Connect->rx_var[2] + Connect->flg_bus ; 	// Quando flg_bus raggiunge questo valore ho ricevuto tutto il payload
#ifdef DEBUG_PRINT
printf("Bytes del pacchetto %d\n",Connect->rx_var[3]);
#endif
		  if (Connect->rx_var[3] == Connect->flg_bus){
			CalCRC(Connect->chk_bus,Connect->flg_car,Connect->appo_bus);// Calcola la checksum che deve arrivare in check
#ifdef DEBUG_PRINT
printf("Ricevuto ultimo bytes del payload vado a calcolare la checksum %d %d\n",Connect->appo_bus[0],Connect->appo_bus[1]);			  
#endif
			Connect->flg_car = -1 ; 									// Non bufferizzare piu' nel chk_bus
		  }
        break;    
        default:
//printf("Default %d/%d\n",Connect->flg_bus,Connect->rx_var[3]);
		  if ( Connect->flg_bus < Connect->rx_var[3]) {car ++ ; break;} // Non fare altro che bufferizzare nel chk_bus
		  if ( Connect->flg_bus == Connect->rx_var[3]){					// Ultimo bytes del payload
			CalCRC(Connect->chk_bus,Connect->flg_car,Connect->appo_bus);// Calcola la checksum che deve arrivare in check
#ifdef DEBUG_PRINT
printf("Ricevuto ultimo bytes del payload vado a calcolare la checksum %d %d\n",Connect->appo_bus[0],Connect->appo_bus[1]);			  
#endif
			car ++ ;
			Connect->flg_car = -1 ; 									// Non bufferizzare piu' nel chk_bus
		  }
		  else { 														// siamo dopo il payload
			kk = Connect->flg_bus - Connect->rx_var[3] ;
			switch(kk){
				case 1: 												// deve essere DLE
#ifdef DEBUG_PRINT
printf("DLE finale %X\n",*car);				
#endif
					if(*car++ != DLE) Connect->flg_bus = -3 ; 			// Aspetto un DLE finale altrimenti NAK
				break;
				case 2: 												// deve essere ETX o ETB
#ifdef DEBUG_PRINT
printf("ETX finale %X\n",*car);				
#endif
					if(*car != ETX && *car != ETB) Connect->flg_bus = -3 ; // Aspetto un ETX o ETB finale altrimenti NAK
					pkt_rx_ending = *car;
					car++;
				break;
				case 3: 												// prima checksum
#ifdef DEBUG_PRINT
printf("Prima checksum %X ? %X\n",*car,Connect->appo_bus[0]);				
#endif
					if(*car++ != Connect->appo_bus[0]) Connect->flg_bus = -3 ; // Aspetto la checksum che ho calcolata altrimenti NAK
				break;
				case 4: 												// seconda checksum
#ifdef DEBUG_PRINT
printf("Seconda checksum %X ? %X\n",*car,Connect->appo_bus[1]);				
#endif
					if(*car++ != Connect->appo_bus[1]) Connect->flg_bus = -3 ; // Aspetto la checksum che ho calcolata altrimenti NAK
					else { 												// Pacchetto corretto posso interpretare e poi rispondere
#ifdef DEBUG_PRINT
printf("Rispondo SLAVE ACK\n");		  
#endif
						rispondi(ACK,Connect); 							// Rispondi ACK pacchetto ricevuto e checksum OK
						// interpretaS(Connect) ;  						// Vado ad interpretare ed eventualmente a rispondere
						interpreta(Connect) ;  						// Vado ad interpretare ed eventualmente a rispondere
						Connect->flg_bus = -2 ; 						// Fine pacchetto
					}
				break;
			}
		  }
        break;
      }
      Connect->flg_bus ++ ;
      if( Connect->flg_bus == -2 ) { 									// Devo rispondere NACK
#ifdef DEBUG_PRINT
printf("Rispondo SLAVE NAK\n");		  
#endif
		rispondi(NAK,Connect);  										// Rispondi NACK a pacchetto ricevuto
		Connect->flg_bus ++ ;
	  }
      if( Connect->flg_bus == -1 ) { 									// Finito analisi buffer
#ifdef DEBUG_PRINT
printf("Finito analisi buffer\n");		  
#endif
        Connect->timeout = 0 ; 											// Messaggio arrivato
        Connect->flg_bus = 0 ;
        Connect->flg_car = -1 ;
        Connect->errore = 0 ;
      }
// printf("jj=%d flg=%d \n",jj,Connect->flg_bus);
    }
    return(0);
}

/**############### INTERPRETAZIONE DEL CONTENUTO DATA BLOCK ############
 * Connect->rx_var[0] = Identify msg type 
 *  1 = MC -> HWC command ccc
 *  2 = HWC -> MC reply command ccc
 *  3 = HWC -> MC event command eee
 * Connect->rx_var[1] = Operative code 
 * -ccc
 *  001 Get Digital Input Status
 *  002 Get Analog  Input Status
 *  003 Set Digital Output 
 *  004 Set Analog  Output 
 *  005 Start Stepper Motor
 *  006 Stop Stepper Motor
 *  007 Solenoid activation
 *  008 Solenoid Deactivation
 *  009 Start DC Motor
 *  00A Stop DC Motor
 *  00B Read HWC Configuration file
 *  00C Write HWC Configuration file
 * -eee 
 *  006 Stepper Motor Self Stop (after step countdown)
 * Connect->rx_var[2] = Lunghezza del payload
 * Connect->chk_bus[6....] = Payload <<<<<<<<<< Devo analizzare questo buffer per sapere cosa e' arrivato
 * Connect->addr_dev =  numero blocco arrivato
 * Connect->last_id_send = ultimo blocco processato OK
 * ###################################################################*/
int interpretaS(struct connectManage *Connect)
{
int kk=0;
#ifdef DEBUG_PRINT
printf("Interpreto SPI2110S: Identify msg type %d \n",Connect->rx_var[0]);
printf("                     Operative code    %X \n",Connect->rx_var[1]);
printf("                     Lunghezza payload %d \n",Connect->rx_var[2]);
printf("                     Numero blocco     %d \n",Connect->addr_dev);
printf("                     Ultimo blocco OK  %d \n",Connect->last_id_send);
#endif

int opcode = Connect->rx_var[1];
int cmd = 0;
switch (opcode) {
	case Get_Digital_Input_Status : 
		cmd = CMD_ReadDigitalInput;
		break;
	case Get_Analog_Input_Status :
		cmd = CMD_ReadAnalogInput;
		break;
	case Set_Digital_Output :
		cmd = CMD_SetDigitalOutput;
		break;
	case Set_Analog_Output :
		cmd = CMD_SetAnalogOutput;
		break;
}

if (cmd > 0) {
	printf("Simulo risosta per opcode 0x0%.2x  cmd = %s\n", opcode, CMD_tab[cmd]);
	SimulazioneHWC(cmd, Connect->last_id_send);
}
else {
	printf("Opcode non gestito : 0x0%.2x\n", opcode);
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ DA IMPLEMENTARE L'INTERPRETAZIONE DEI CARATTERI RICEVUTI ed eventuale risposta se prevista.... 	

// Faccio l'echo di quello che e' arrivato
/*	
	for(kk=0;kk<Connect->rx_var[2]+6;kk++) Connect->appo_bus[kk] = Connect->chk_bus[kk] ;  
	Connect->flg_car = kk ;
	rispondi(0,Connect); // Rispondi con il pacchetto preparato su appo_bus lungo flg_car
*/	
return 0 ;
}
// Se ok diverso da 0 deve mandare solamente ACK o NAK altrimenti manda il buffer appo_bus
int rispondi(char ok,struct connectManage *Connect) 					
{
	int fd_tx = Connect->sav_handle;
	// se non sono SPI Master, recupera il file descriptor del SPI Master per poter trasmettere
	if (Connect->proto_client != ISO2110) {
		for (int i=0; i<act_client; i++) {
			if (ptr[i]->proto_client == ISO2110) {
				fd_tx = ptr[i]->sav_handle;
			}
		}
	}

unsigned char result[2];
int kk,jj ;
	if(ok) {															// Se ok diverso da 0 puo' essere invio di ACK o NACK oppure 1 = Ripetizione ultimo comando
#ifdef DEBUG_PRINT
//   printf("Rispondo %d\n",ok);
	printf("Rispondo %d su fd %d\n",ok, fd_tx);
#endif
	  if(ok==1) {
		if (last_pkt_len > 0 && last_pkt_buf != NULL) {
			inviaNPL(fd_tx, last_pkt_len);		    // Invio Next Packet Length dell'ultimo comando
			write(fd_tx,last_pkt_buf,last_pkt_len);// Ripeto l'ultimo comando
			Set_ACKTimeout(TIMEOUT_ACK); 								// attendo una risposta
#ifdef DEBUG_PRINT
printf("Invio nuovamente l'ultimo comando (%d car)  = <",last_pkt_len);
for (jj=0;jj<last_pkt_len;jj++) { printf("%.2X ",(last_pkt_buf[jj] & 0xFF) ); }
printf(">\n");
#endif
		}
		else {
			printf("Ricevuta richiesta di ritrasmissione ultimo pacchetto senza dati\n");
		}
	  }
	  else {
	    Connect->appo_bus[0] = ok ;
#ifndef DISABLE_ACK
		inviaNPL(fd_tx, 1);							// Invio Next Packet Length = 1
	    write(fd_tx,Connect->appo_bus,1);               	// Invio risposta ACK -NACK
#else
		printf("DISABLE_ACK is active, no ACK/NACK are send\n");
#endif
	  }
	}
	else {																// Se ok = 0 invio un messaggio preparato nel buffer appo_bus lungo flg_car
		CalCRC(Connect->appo_bus, Connect->flg_car, result ); 
#ifdef DEBUG_PRINT
printf("calcolare la checksum %X %X\n",result[1],result[0]);			  
#endif
		Connect->chk_bus[0] = DLE ; 
		Connect->chk_bus[1] = STX ; 
		kk = 2 ;
		for(jj=0;jj<Connect->flg_car;jj++){
          if ((Connect->chk_bus[kk++] = Connect->appo_bus[jj]) == DLE) Connect->chk_bus[kk++] = DLE ; // Raddoppio il DLE se e' nel data block senza farlo contare nella checksum
        }
		Connect->chk_bus[kk++] = DLE ; 					  				// Chiusura pacchetto 
		//Connect->chk_bus[kk++] = ETX ; 					  				// Fine blocco singolo
		Connect->chk_bus[kk++] = pkt_tx_ending ; 					  				// Fine blocco
		Connect->chk_bus[kk++] = result[0];                				// Parte alta della checksum
		Connect->chk_bus[kk++] = result[1];                				// Parte bassa  della checksum
		Connect->poll_param = kk ;
		if (kk < SPI_PROTO_MAX_TRANSFER) {
			inviaNPL(fd_tx, kk);								// Invio Next Packet Length
			write(fd_tx,Connect->chk_bus,Connect->poll_param);// Invio risposta 
			// Connect->wait_bus = TIMEOUT_ACK ; 								// attendo una risposta per 500 msec (in TAU)
			Set_ACKTimeout(TIMEOUT_ACK);
#ifndef DISABLE_ACK_TIMEOUT_HANDLER
			Start_ACKTimeoutHandler();
#endif
			
#ifdef DEBUG_PRINT
printf("SPIso2110 client %d - Invio %d car  = <",fd_tx,kk);
for (jj=0;jj<kk;jj++) { printf("%.2X ",(Connect->chk_bus[jj] & 0xFF) ); }
printf("> Address %X\n",Connect);
#endif
			last_pkt_len = Connect->poll_param;
			last_pkt_buf = Connect->chk_bus;
		}
		else {
			printf("Error: trying to send packet with size (%d) greater than spi_max_transfer (%d)\n", kk, SPI_PROTO_MAX_TRANSFER);
		}
		
	}
}



void ACKTimeoutHandler()
{
	struct connectManage *Connect;
	for (int i=0; i<act_client; i++) {
		if (ptr[i]->proto_client == ISO2110S) {
			Connect = ptr[i]->Connect[0];
			break;
		}
	}
#ifdef DEBUG_PRINT
  printf("Inizio attesa timeout ACK (%d ms)\n",Connect->wait_bus*TAU);
#endif
  while (Connect->wait_bus >0) {                           // Sto aspettando risposta ?
    usleep(TAU*1000);                            // sleep per TAU ms
    Connect->wait_bus -- ;                              // decremento attesa
// printf("%d ms\n",Connect->wait_bus*TAU);
    if (Connect->wait_bus >0) continue;                   // Attesa finita ? NO => continuo l'attesa
#ifdef DEBUG_PRINT
  printf("Timeout ACK #%d\n",Connect->timeout_cnt);
#endif
    if (Connect->timeout_cnt < MAX_RETRANSMISSIONS) {
		Connect->wait_bus = 1;			// resetto il timeout restando attivo, verrà settato nuovamente dentro la rispondi()
		rispondi(1,Connect);            // rimanda l'ultimo pacchetto per timeout scaduto
    	Connect->timeout_cnt ++ ;
	}
    else {
#ifdef DEBUG_PRINT
  printf("Max ritrasmissioni raggiunte %d / %d\n",Connect->timeout_cnt, MAX_RETRANSMISSIONS);
#endif
    //   Connect->wait_bus = 0 ;                     // Non riprovo piu'
    //   Connect->timeout_cnt = 0 ;
    }
    Connect->timeout ++ ;                               // Incremento i timeout consecutivi
  }
#ifdef DEBUG_PRINT
  printf("Fine gestione timeout ACK (%d)\n",Connect->timeout);
#endif
	receivedACK = -1;
	Connect->wait_bus = 0 ;                     // Non riprovo piu'
    Connect->timeout_cnt = 0 ;
	
	pthread_exit(0);
}


void Start_ACKTimeoutHandler() {
	receivedACK = 0;
	pthread_t child;
	if ( pthread_create(&child, NULL, ACKTimeoutHandler, NULL) != 0 )
    {
       trace(__LINE__,__FILE__,1000,0,0,"Pthread creation error: %d - %s",errno,strerror(errno));/* Messaggio di errore */
    }
    else pthread_detach(child);
}

void Set_ACKTimeout(int t) {
	for (int i=0; i<act_client; i++) {
		if (ptr[i]->proto_client == ISO2110S) {
			ptr[i]->Connect[0]->wait_bus = t;
			return;
		}
	}
}

void Stop_ACKTimeoutHandler() {
	receivedACK = 1;
	for (int i=0; i<act_client; i++) {
		if (ptr[i]->proto_client == ISO2110S) {
			ptr[i]->Connect[0]->wait_bus = 0;
			ptr[i]->Connect[0]->timeout_cnt = MAX_RETRANSMISSIONS;
			return;
		}
	}
}