
#include "extern.h"
#include <fcntl.h>
#include <sys/ioctl.h>

/**              Caratteri speciali del protocollo  				  */
#define STX 0x02
#define ETX 0x03
#define ACK 0x06
#define DLE 0x10
#define NAK 0x15
#define ETB 0x17

#define DEBUG_PRINT 1

extern struct protoManage *ProtoConnect ; 								// Variabile di gestione della comunicazione su questo canale

int SimulazioneHWC(int ,int );

SimulazioneHWC(int comando, int cliente)
{
struct connectManage *Connect;											// Utilizzo un area Connect dedicata al gestore di eventi 
unsigned char result[2];
int kk,jj ;
int appo; 

if (cliente == -1) {
	cliente = ProtoConnect->Connect[1]->last_id_send;
}
// printf("Simulazione RISPOSTE HWC a comando %d\n",comando);	
	Connect = ProtoConnect->Connect[0] ;								// La seriale ha solamente una connessione attiva
	kk = 0;
	switch(comando)
	{
		case CMD_Restart:												// Simulazione risposta al riavvio
		  Connect->chk_bus[kk++]=0x00;									// BLOCK 
		  Connect->chk_bus[kk++]=0x01;
		  Connect->chk_bus[kk++]=0x20;									// COMANDO
		  Connect->chk_bus[kk++]=0x0D;
		  Connect->chk_bus[kk++]=0x00;									// LUNGHEZZA IN Bytes
		  Connect->chk_bus[kk++]=0x04;
		  Connect->chk_bus[kk++]=0x00;									// DATA
		  Connect->chk_bus[kk++]=0x00;									// Prima WORD OPTIONS
		  Connect->chk_bus[kk++]=0x00;									// STATUS 0 = SUCCESS 0xFFFF = FAIL
		  Connect->chk_bus[kk++]=0x00;
		break;
		case CMD_ReadDigitalInput:										// Simulazione risposta alla lettura input digitali
		  Connect->chk_bus[kk++]=0x00;									// BLOCK 
		  Connect->chk_bus[kk++]=0x01;
		  Connect->chk_bus[kk++]=0x20;									// COMANDO
		  Connect->chk_bus[kk++]=0x01;
		  Connect->chk_bus[kk++]=0x00;									// LUNGHEZZA IN Bytes
		  Connect->chk_bus[kk++]=0x0C;

		  Connect->chk_bus[kk++]=0x00;									// DATA
		  Connect->chk_bus[kk++]=0x00;									// Prima WORD OPTIONS

		  Connect->chk_bus[kk++]=0x00;									// STATUS 0 = SUCCESS 0xFFFF = FAIL
		  Connect->chk_bus[kk++]=0x00;
		  appo = rnd(65535);											// ritorna un valore tra 0-FFFF
		  Connect->chk_bus[kk++]=((appo>>8)& 0xFF);						// STATUS INPUT 48-63 
		  Connect->chk_bus[kk++]=(appo & 0xFF);
		  appo = rnd(65535);											// ritorna un valore tra 0-FFFF
		  Connect->chk_bus[kk++]=((appo>>8)& 0xFF);						// STATUS INPUT 32-47 
		  Connect->chk_bus[kk++]=(appo & 0xFF);
		  appo = rnd(65535);											// ritorna un valore tra 0-FFFF
		  Connect->chk_bus[kk++]=((appo>>8)& 0xFF);						// STATUS INPUT 16-31 
		  Connect->chk_bus[kk++]=(appo & 0xFF);
		  appo = rnd(65535);											// ritorna un valore tra 0-FFFF
		  Connect->chk_bus[kk++]=((appo>>8)& 0xFF);						// STATUS INPUT 0-15 
		  Connect->chk_bus[kk++]=(appo & 0xFF);

		break;
		case CMD_ReadAnalogInput:										// Simulazione risposta alla lettura input analogici
		  Connect->chk_bus[kk++]=0x00;									// BLOCK 
		  Connect->chk_bus[kk++]=0x01;
		  Connect->chk_bus[kk++]=0x20;									// COMANDO
		  Connect->chk_bus[kk++]=0x02;
		  Connect->chk_bus[kk++]=0x00;									// LUNGHEZZA IN Bytes
		  Connect->chk_bus[kk++]=0x0E;

		  Connect->chk_bus[kk++]=0x00;									// DATA
		  Connect->chk_bus[kk++]=0x00;									// Prima WORD OPTIONS

		  Connect->chk_bus[kk++]=0x00;									// STATUS 0 = SUCCESS 0xFFFF = FAIL
		  Connect->chk_bus[kk++]=0x00;
		  
		  appo = rnd(4096);												// ritorna un valore tra 0-4096
		  Connect->chk_bus[kk++]=((appo>>8)& 0xFF);						// STATUS ANALOG INPUT 0
		  Connect->chk_bus[kk++]=(appo & 0xFF);
		  appo = rnd(4096);												// ritorna un valore tra 0-4096
		  Connect->chk_bus[kk++]=((appo>>8)& 0xFF);						// STATUS ANALOG INPUT 1
		  Connect->chk_bus[kk++]=(appo & 0xFF);
		  appo = rnd(4096);												// ritorna un valore tra 0-4096
		  Connect->chk_bus[kk++]=((appo>>8)& 0xFF);						// STATUS ANALOG INPUT 2
		  Connect->chk_bus[kk++]=(appo & 0xFF);
		  appo = rnd(4096);												// ritorna un valore tra 0-4096
		  Connect->chk_bus[kk++]=((appo>>8)& 0xFF);						// STATUS ANALOG INPUT 3
		  Connect->chk_bus[kk++]=(appo & 0xFF);
		  appo = rnd(4096);												// ritorna un valore tra 0-4096
		  Connect->chk_bus[kk++]=((appo>>8)& 0xFF);						// STATUS ANALOG INPUT 4
		  Connect->chk_bus[kk++]=(appo & 0xFF);


		break;
		case CMD_SetAnalogOutput:										// Simulazione risposta alla scrittura output analogici
		  Connect->chk_bus[kk++]=0x00;									// BLOCK 
		  Connect->chk_bus[kk++]=0x01;
		  Connect->chk_bus[kk++]=0x20;									// COMANDO
		  Connect->chk_bus[kk++]=0x04;
		  Connect->chk_bus[kk++]=0x00;									// LUNGHEZZA IN Bytes
		  Connect->chk_bus[kk++]=0x04;

		  Connect->chk_bus[kk++]=0x00;									// DATA
		  Connect->chk_bus[kk++]=0x00;									// Prima WORD OPTIONS

		  Connect->chk_bus[kk++]=0x00;									// STATUS 0 = SUCCESS 0xFFFF = FAIL
		  Connect->chk_bus[kk++]=0x00;
		  
		break;
		case CMD_SetDCMotor:											// Simulazione risposta alla scrittura DC Motor
		  Connect->chk_bus[kk++]=0x00;									// BLOCK 
		  Connect->chk_bus[kk++]=0x01;
		  Connect->chk_bus[kk++]=0x20;									// COMANDO
		  Connect->chk_bus[kk++]=ProtoConnect->Connect[1]->appo_bus[3];
		  Connect->chk_bus[kk++]=0x00;									// LUNGHEZZA IN Bytes
		  Connect->chk_bus[kk++]=0x04;

		  Connect->chk_bus[kk++]=0x00;									// DATA
		  Connect->chk_bus[kk++]=0x00;									// Prima WORD OPTIONS

		  Connect->chk_bus[kk++]=0x00;									// STATUS 0 = SUCCESS 0xFFFF = FAIL
		  Connect->chk_bus[kk++]=0x00;
		  
		break;
		case CMD_SetDCSolenoid:											// Simulazione risposta alla scrittura DC Solenoid
		  Connect->chk_bus[kk++]=0x00;									// BLOCK 
		  Connect->chk_bus[kk++]=0x01;
		  Connect->chk_bus[kk++]=0x20;									// COMANDO
		  Connect->chk_bus[kk++]=ProtoConnect->Connect[1]->appo_bus[3];
		  Connect->chk_bus[kk++]=0x00;									// LUNGHEZZA IN Bytes
		  Connect->chk_bus[kk++]=0x04;

		  Connect->chk_bus[kk++]=0x00;									// DATA
		  Connect->chk_bus[kk++]=0x00;									// Prima WORD OPTIONS

		  Connect->chk_bus[kk++]=0x00;									// STATUS 0 = SUCCESS 0xFFFF = FAIL
		  Connect->chk_bus[kk++]=0x00;
		  
		break;
		case CMD_SetDigitalOutput:										// Simulazione risposta alla scrittura output digitali
		  Connect->chk_bus[kk++]=0x00;									// BLOCK 
		  Connect->chk_bus[kk++]=0x01;
		  Connect->chk_bus[kk++]=0x20;									// COMANDO
		  Connect->chk_bus[kk++]=0x03;
		  Connect->chk_bus[kk++]=0x00;									// LUNGHEZZA IN Bytes
		  Connect->chk_bus[kk++]=0x04;

		  Connect->chk_bus[kk++]=0x00;									// DATA
		  Connect->chk_bus[kk++]=0x00;									// Prima WORD OPTIONS

		  Connect->chk_bus[kk++]=0x00;									// STATUS 0 = SUCCESS 0xFFFF = FAIL
		  Connect->chk_bus[kk++]=0x00;
		  
		break;
		case CMD_SetStepperMotorSpeed:									// Simulazione risposta alla scrittura Step Motor Start / Stop
		  Connect->chk_bus[kk++]=0x00;									// BLOCK 
		  Connect->chk_bus[kk++]=0x01;
		  Connect->chk_bus[kk++]=0x20;									// COMANDO
		  Connect->chk_bus[kk++]=ProtoConnect->Connect[1]->appo_bus[3]; // Qui rilevo lo START / STOP
		  Connect->chk_bus[kk++]=0x00;									// LUNGHEZZA IN Bytes
		  Connect->chk_bus[kk++]=0x04;

		  Connect->chk_bus[kk++]=0x00;									// DATA
		  Connect->chk_bus[kk++]=0x00;									// Prima WORD OPTIONS

		  Connect->chk_bus[kk++]=0x00;									// STATUS 0 = SUCCESS 0xFFFF = FAIL
		  Connect->chk_bus[kk++]=0x00;
		  
		break;
		
	}
	Connect->flg_car = kk ;
	CalCRC(Connect->chk_bus, Connect->flg_car , result ); 
#ifdef DEBUG_PRINT
printf("calcolare la checksum %X %X\n",result[1],result[0]);			  
#endif
	Connect->result[0] = DLE ; 
	Connect->result[1] = STX ; 
	kk = 2 ;
	for(jj=0;jj<Connect->flg_car;jj++){
         if ((Connect->result[kk++] = Connect->chk_bus[jj]) == DLE) Connect->result[kk++] = DLE ; // Raddoppio il DLE se e' nel data block senza farlo contare nella checksum
    }
	Connect->result[kk++] = DLE ; 					  				// Chiusura pacchetto 
	Connect->result[kk++] = ETX ; 					  				// Fine blocco singolo
	Connect->result[kk++] = result[0];                				// Parte alta della checksum
	Connect->result[kk++] = result[1];                				// Parte bassa  della checksum
	Connect->flg_car = -1 ;											// Ripulisco la variabile
	Connect->last_id_send = cliente ;
#ifdef DEBUG_PRINT
printf("Invio (%d) buffer simulato di risposta HWC ---------------------------------------------------------------------------------------------\n",Connect->last_id_send);			  
#endif
	SPIso2110(Connect,Connect->result,kk,Connect->canale) ;
}

