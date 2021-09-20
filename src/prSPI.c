/************************** INCLUDE APPLICATION SECTION ***************/
#include "extern.h"
#include <fcntl.h>
#include <sys/ioctl.h>

/*********************** DEFINE CONSTANTS SECTION *********************/
// Caratteri speciali del protocollo
#define STX 0x02
#define ETX 0x03
#define ACK 0x06
#define DLE 0x10
#define NAK 0x15
#define ETB 0x17

//#define DEBUG_PRINT 1                           // Disabilito le stampe di debug
//#define SIMULA_RISPOSTA 1                         // Simula la risposta dalla SPI HWC

/*********************** GLOBAL VARIABLES SECTION *********************/
struct SendArea AreaSpi ;
struct protoManage *ProtoConnect ;                    // Variabile di gestione della comunicazione su questo canale

extern int pkt_rx_ending;

const unsigned int crctab[256] = {
   0x0000, 0x2110, 0x4220, 0x6330, 0x8440, 0xa550, 0xc660, 0xe770,
   0x0881, 0x2991, 0x4aa1, 0x6bb1, 0x8cc1, 0xadd1, 0xcee1, 0xeff1,
   0x3112, 0x1002, 0x7332, 0x5222, 0xb552, 0x9442, 0xf772, 0xd662,
   0x3993, 0x1883, 0x7bb3, 0x5aa3, 0xbdd3, 0x9cc3, 0xfff3, 0xdee3,
   0x6224, 0x4334, 0x2004, 0x0114, 0xe664, 0xc774, 0xa444, 0x8554,
   0x6aa5, 0x4bb5, 0x2885, 0x0995, 0xeee5, 0xcff5, 0xacc5, 0x8dd5,
   0x5336, 0x7226, 0x1116, 0x3006, 0xd776, 0xf666, 0x9556, 0xb446,
   0x5bb7, 0x7aa7, 0x1997, 0x3887, 0xdff7, 0xfee7, 0x9dd7, 0xbcc7,
   0xc448, 0xe558, 0x8668, 0xa778, 0x4008, 0x6118, 0x0228, 0x2338,
   0xccc9, 0xedd9, 0x8ee9, 0xaff9, 0x4889, 0x6999, 0x0aa9, 0x2bb9,
   0xf55a, 0xd44a, 0xb77a, 0x966a, 0x711a, 0x500a, 0x333a, 0x122a,
   0xfddb, 0xdccb, 0xbffb, 0x9eeb, 0x799b, 0x588b, 0x3bbb, 0x1aab,
   0xa66c, 0x877c, 0xe44c, 0xc55c, 0x222c, 0x033c, 0x600c, 0x411c,
   0xaeed, 0x8ffd, 0xeccd, 0xcddd, 0x2aad, 0x0bbd, 0x688d, 0x499d,
   0x977e, 0xb66e, 0xd55e, 0xf44e, 0x133e, 0x322e, 0x511e, 0x700e,
   0x9fff, 0xbeef, 0xdddf, 0xfccf, 0x1bbf, 0x3aaf, 0x599f, 0x788f,
   0x8891, 0xa981, 0xcab1, 0xeba1, 0x0cd1, 0x2dc1, 0x4ef1, 0x6fe1,
   0x8010, 0xa100, 0xc230, 0xe320, 0x0450, 0x2540, 0x4670, 0x6760,
   0xb983, 0x9893, 0xfba3, 0xdab3, 0x3dc3, 0x1cd3, 0x7fe3, 0x5ef3,
   0xb102, 0x9012, 0xf322, 0xd232, 0x3542, 0x1452, 0x7762, 0x5672,
   0xeab5, 0xcba5, 0xa895, 0x8985, 0x6ef5, 0x4fe5, 0x2cd5, 0x0dc5,
   0xe234, 0xc324, 0xa014, 0x8104, 0x6674, 0x4764, 0x2454, 0x0544,
   0xdba7, 0xfab7, 0x9987, 0xb897, 0x5fe7, 0x7ef7, 0x1dc7, 0x3cd7,
   0xd326, 0xf236, 0x9106, 0xb016, 0x5766, 0x7676, 0x1546, 0x3456,
   0x4cd9, 0x6dc9, 0x0ef9, 0x2fe9, 0xc899, 0xe989, 0x8ab9, 0xaba9,
   0x4458, 0x6548, 0x0678, 0x2768, 0xc018, 0xe108, 0x8238, 0xa328,
   0x7dcb, 0x5cdb, 0x3feb, 0x1efb, 0xf98b, 0xd89b, 0xbbab, 0x9abb,
   0x754a, 0x545a, 0x376a, 0x167a, 0xf10a, 0xd01a, 0xb32a, 0x923a,
   0x2efd, 0x0fed, 0x6cdd, 0x4dcd, 0xaabd, 0x8bad, 0xe89d, 0xc98d,
   0x267c, 0x076c, 0x645c, 0x454c, 0xa23c, 0x832c, 0xe01c, 0xc10c,
   0x1fef, 0x3eff, 0x5dcf, 0x7cdf, 0x9baf, 0xbabf, 0xd98f, 0xf89f,
   0x176e, 0x367e, 0x554e, 0x745e, 0x932e, 0xb23e, 0xd10e, 0xf01e
};
/************** PROTOTYPE FUNCTION DECLARATIONS SECTION ***************/
void CalCRC(char *, int , char *);
int Init_SPIso2110(int ) ;
int InitAree_SPIso2110(int ) ;
int SPIso2110(struct connectManage * ,unsigned char *,int,int) ;
int Connect_SPIso2110(struct connectManage *) ;
extern int rispondi(char,struct connectManage *);
void SendPoll_SPIso2110(int,int,int);
void *SendMsg(void *);
int interpreta(struct connectManage *);
#ifdef SIMULA_RISPOSTA
extern int SimulazioneHWC(int,int);                   // Funzione di simulazione risposte dall'HWC
#endif
/************************** FUNCTION DEVELOP SECTION ******************/
/**
 *
 * Thread per la gestione dell'evento di richiesta informazioni verso
 * il canale SPI -> HWC
 * I parametri passano per l'area di scambio AreaSpi
 * Il thread cicla sull'attesa dell'evento dopo aver inizializzato l'area
 * dati
 *
 * */
void *SendMsg(void * arg)                       // Funzione trasformata in thread che sta in attesa di un evento
{
struct connectManage *Connect;                      // Utilizzo un area Connect dedicata al gestore di eventi
int kk ,appo ;
int i;
struct paramWbSpi *p ;
#ifdef SIMULA_RISPOSTA
int simula;
#endif
    
    int* intarg = (int *) arg;
    int cliente = *intarg++;
    int protocollo = *intarg;

    if (protocollo != ISO2110) {
      printf("line %d, protocollo %s : ignoro scrittura su un canale di sola lettura\n",cliente,protocol_name[protocollo]);
      return;
    }

  Connect = (struct connectManage *) malloc(sizeof (struct connectManage)); // Mi alloco la memoria
  ProtoConnect->Connect[1] = Connect ;                // Mi salvo l'indice della struttura nel secondo posto, il primo lo occupa quello delle linea seriale fisica
  
  Connect->sav_handle = ProtoConnect->sav_handle ;          // Recupero l'handle della linea

  // pthread_mutex_init(&AreaSpi.sema,NULL);               // Inizializzo l'Area di scambio eventi per driver SPI
  // AreaSpi.trasmettitore = (pthread_cond_t) PTHREAD_COND_INITIALIZER ;

  // AreaSpi.max = CODAREA ;                       // Gestisco una coda di valori nell'area di memoria

// Inizialmente alloco lo spazio per tutta la coda... questo si potrebbe fare anche dinamico qualora mancassero risorse

  // p = (struct paramWbSpi *) malloc(sizeof(struct paramWbSpi )*AreaSpi.max) ;
  // for(kk = 0 ; kk< AreaSpi.max ; kk++) AreaSpi.param[kk] = p++ ;

#ifdef DEBUG_PRINT
printf("Handle %d Thread gestione messaggi a eventi per Driver SPI Master in wait %ld Connect %X\n",Connect->sav_handle,TAU_PLC,ProtoConnect->Connect[1]);
#endif
  while(1){                             // Loop infinito
//    pthread_mutex_unlock(&AreaSpi.sema);                // Se voglio sbloccare la gestione delle variabili durante l'attesa di altre richieste
//printf("Sono in ATTESA DI UNA CHIAMATA >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %ld\n",TAU_PLC);
    pthread_cond_wait(&AreaSpi.trasmettitore, &AreaSpi.sema);   // Aspetto che qualcuno mi richieda il servizio tramite un evento
//    pthread_mutex_lock(&AreaSpi.sema);                  // Se devo gestire la concorrenza del thread sulla gestione delle variabili
#ifdef DEBUG_PRINT
printf("Handle %d Sono entrato nello smaltimento della coda comandi SPI %ld\n",Connect->sav_handle,TAU_PLC);
#endif
    while (AreaSpi.sma != AreaSpi.accu) {             // Ho dei messaggi da smaltire ?
      kk = AreaSpi.sma++ ;                    // Prendo il primo indice da smaltire
#ifdef SIMULA_RISPOSTA
      simula=kk;
#endif
      if (AreaSpi.sma>=AreaSpi.max) AreaSpi.sma = 0 ;       // Gestisco il puntatore di smaltimento
      p = AreaSpi.param[kk];                    // Punto ai parametri
      Connect->last_id_send = p->rispoRx;             // Mi salvo a chi devo mandare la risposta
      switch(p->cmdTx)                      // Riempio il buffer per la richiesta arrivata
      {
        case CMD_Restart:                     // Ipotesi comando 0x080 con parametro 1 o 2
          kk = 0 ;
          Connect->id_area_read = p->parint[0] ;          // Mi salvo chi ho resettato
        Connect->appo_bus[kk++] = 0x00 ;            // Block 00 01
        Connect->appo_bus[kk++] = 0x01 ;            //
        Connect->appo_bus[kk++] = 0x10 ;            // Opcopde binary: 0001xxxx Comando solo su i 4 bit piu' significativi
        Connect->appo_bus[kk++] = 0x0D ;            // Descrizione Restart HWController/FPGA 0x00D (su dodici bit)
        Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
        Connect->appo_bus[kk++] = 0x02 ;
        Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
        Connect->appo_bus[kk++] = (p->parint[0] & 0xFF);      // 1 = FPGA oppure 2 = HWController
        Connect->flg_car = kk ;
        break;
        case CMD_ReadDigitalInput:
          kk = 0 ;
        Connect->appo_bus[kk++] = 0x00 ;            // Block 00 01
        Connect->appo_bus[kk++] = 0x01 ;            //
        Connect->appo_bus[kk++] = 0x10 ;            // Opcopde binary: 0001xxxx Comando solo su i 4 bit piu' significativi
        Connect->appo_bus[kk++] = 0x01 ;            // Descrizione Get digital input status 0x001 (su dodici bit)
        Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
        Connect->appo_bus[kk++] = 0x02 ;
        Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
        Connect->appo_bus[kk++] = 0x00 ;
        Connect->flg_car = kk ;
        break;
        case CMD_ReadAnalogInput:
          kk = 0 ;
        Connect->appo_bus[kk++] = 0x00 ;            // Block 00 01
        Connect->appo_bus[kk++] = 0x01 ;            //
        Connect->appo_bus[kk++] = 0x10 ;            // Opcopde binary: 0001xxxx Comando solo su i 4 bit piu' significativi
        Connect->appo_bus[kk++] = 0x02 ;            // Descrizione Get analog input status 0x002 (su dodici bit)
        Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
        Connect->appo_bus[kk++] = 0x02 ;
        Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
        Connect->appo_bus[kk++] = 0x00 ;
        Connect->id_area_read = p->parint[0];         // Mi salvo l'ID di chi vuole sapere
        Connect->flg_car = kk ;
        break;
        case CMD_SetAnalogOutput:
          kk = 0 ;
        Connect->appo_bus[kk++] = 0x00 ;            // Block 00 01
        Connect->appo_bus[kk++] = 0x01 ;            //
        Connect->appo_bus[kk++] = 0x10 ;            // Opcopde binary: 0001xxxx Comando solo su i 4 bit piu' significativi
        Connect->appo_bus[kk++] = 0x04 ;            // Descrizione Set analog output status 0x004 (su dodici bit)
        Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
        Connect->appo_bus[kk++] = 0x06 ;
        Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
        Connect->appo_bus[kk++] = 0x00 ;
        Connect->appo_bus[kk++] = ((p->parint[0]>>8) & 0xFF) ;  // Id Analog output parte alta
        Connect->appo_bus[kk++] = (p->parint[0] & 0xFF) ;   // parte bassa
        Connect->appo_bus[kk++] = ((p->parint[1]>>8) & 0xFF) ;  // Value Analog output parte alta
        Connect->appo_bus[kk++] = (p->parint[1] & 0xFF) ;   // parte bassa
        Connect->flg_car = kk ;
        break;
        case CMD_SetDCMotor:
        case CMD_SetDCMotorPWM:
          kk = 0 ;
        Connect->id_area_read = p->parint[4];         // Mi salvo il comando a cui si vuole rispondere
        Connect->appo_bus[kk++] = 0x00 ;            // Block 00 01
        Connect->appo_bus[kk++] = 0x01 ;            //
        Connect->appo_bus[kk++] = 0x10 ;            // Opcopde binary: 0001xxxx Comando solo su i 4 bit piu' significativi
        if ( p->parint[1] == 0 ) {                // Vuole comandare uno STOP ?
          Connect->appo_bus[kk++] = 0x0A;           // Descrizione Stop DC Motor 0x00A (su dodici bit)
                  Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
                  Connect->appo_bus[kk++] = 0x04 ;
                  Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
                  Connect->appo_bus[kk++] = 0x00 ;
                  Connect->appo_bus[kk++] = ((p->parint[0]>>8) & 0xFF) ;// Id DC Motor parte alta
                  Connect->appo_bus[kk++] = (p->parint[0] & 0xFF) ;   // parte bassa
                }
        else {                          // Comando uno START
          Connect->appo_bus[kk++] = 0x09 ;            // Descrizione Start DC Motor 0x009 (su dodici bit)
          Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
          Connect->appo_bus[kk++] = 0x0A ;
          Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
          Connect->appo_bus[kk++] = 0x00 ;
          Connect->appo_bus[kk++] = ((p->parint[0]>>8) & 0xFF) ;// Id DC Motor parte alta
          Connect->appo_bus[kk++] = (p->parint[0] & 0xFF) ;   // parte bassa
          if (p->parint[1] == 0) {
            appo = 0;
          }
          else if ((p->parint[1]>>31) == 0) {
            appo = 1;
          }
          else {
            appo = 2;
          }
          Connect->appo_bus[kk++] = ((appo>>8) & 0xFF) ;// Value set parte alta
          Connect->appo_bus[kk++] = (appo & 0xFF) ;   // parte bassa
          Connect->appo_bus[kk++] = ((p->parint[2]>>8) & 0xFF) ;// PWM:
          Connect->appo_bus[kk++] = (p->parint[2] & 0xFF) ;
          Connect->appo_bus[kk++] = ((p->parint[3]>>8) & 0xFF) ;// Frequency:
          Connect->appo_bus[kk++] = (p->parint[3] & 0xFF) ;
          }
        Connect->flg_car = kk ;
        break;
        case CMD_SetDCSolenoid:
          kk = 0 ;
        Connect->id_area_read = p->parint[4];         // Mi salvo il comando a cui si vuole rispondere
        Connect->appo_bus[kk++] = 0x00 ;            // Block 00 01
        Connect->appo_bus[kk++] = 0x01 ;            //
        Connect->appo_bus[kk++] = 0x10 ;            // Opcopde binary: 0001xxxx Comando solo su i 4 bit piu' significativi
        if ( p->parint[1] == 0 ) {                // Vuole comandare una DISATTIVAZIONE ?
          Connect->appo_bus[kk++] = 0x08;           // Descrizione Solenoid Deactivation 0x008 (su dodici bit)
                  Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
                  Connect->appo_bus[kk++] = 0x04 ;
                  Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
                  Connect->appo_bus[kk++] = 0x00 ;
                  Connect->appo_bus[kk++] = ((p->parint[0]>>8) & 0xFF) ;// Id Solenoid parte alta
                  Connect->appo_bus[kk++] = (p->parint[0] & 0xFF) ;   // parte bassa
                }
        else {                          // Comando una ATTIVAZIONE
          Connect->appo_bus[kk++] = 0x07 ;            // Descrizione Activation Solenoid 0x007 (su dodici bit)
          Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
          Connect->appo_bus[kk++] = 0x08 ;
          Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
          Connect->appo_bus[kk++] = 0x00 ;
          Connect->appo_bus[kk++] = ((p->parint[0]>>8) & 0xFF) ;// Id Solenoid parte alta
          Connect->appo_bus[kk++] = (p->parint[0] & 0xFF) ;   // parte bassa
          Connect->appo_bus[kk++] = ((p->parint[3]>>8) & 0xFF) ;// Initial time : msec 0 - 2000
          Connect->appo_bus[kk++] = (p->parint[3] & 0xFF) ;
          Connect->appo_bus[kk++] = ((p->parint[2]>>8) & 0xFF) ;// PWM: 0 - 100 %
          Connect->appo_bus[kk++] = (p->parint[2] & 0xFF) ;
          }
        Connect->flg_car = kk ;
        break;
        case CMD_SetDigitalOutput:
          kk = 0 ;
        Connect->appo_bus[kk++] = 0x00 ;            // Block 00 01
        Connect->appo_bus[kk++] = 0x01 ;            //
        Connect->appo_bus[kk++] = 0x10 ;            // Opcopde binary: 0001xxxx Comando solo su i 4 bit piu' significativi
        Connect->appo_bus[kk++] = 0x03 ;            // Descrizione Set digital output status 0x003 (su dodici bit)
        Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
        Connect->appo_bus[kk++] = 0x06 ;
        Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
        Connect->appo_bus[kk++] = 0x00 ;
        Connect->appo_bus[kk++] = ((p->parint[0]>>8) & 0xFF) ;  // Id Digital output parte alta
        Connect->appo_bus[kk++] = (p->parint[0] & 0xFF) ;   // parte bassa
        Connect->appo_bus[kk++] = ((p->parint[1]>>8) & 0xFF) ;  // Value on off output parte alta
        Connect->appo_bus[kk++] = (p->parint[1] & 0xFF) ;   // parte bassa
        Connect->flg_car = kk ;
        break;
        case CMD_SetStepperMotorSpeed:
          kk = 0 ;
        Connect->id_area_read = p->parint[6];         // Mi salvo il comando a cui si vuole rispondere
        Connect->appo_bus[kk++] = 0x00 ;            // Block 00 01
        Connect->appo_bus[kk++] = 0x01 ;            //
        Connect->appo_bus[kk++] = 0x10 ;            // Opcopde binary: 0001xxxx Comando solo su i 4 bit piu' significativi
        if ( p->parint[2] == 0 ) {                // Vuole comandare uno STOP motore?
          Connect->appo_bus[kk++] = 0x06;           // Descrizione Stop Motor Step 0x006 (su dodici bit)
                  Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
                  Connect->appo_bus[kk++] = 0x04 ;
                  Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
                  Connect->appo_bus[kk++] = 0x00 ;
                  Connect->appo_bus[kk++] = ((p->parint[0]>>8) & 0xFF) ;// Id Solenoid parte alta
                  Connect->appo_bus[kk++] = (p->parint[0] & 0xFF) ;   // parte bassa
                }
        else {                          // Comando uno START Step Motor
          Connect->appo_bus[kk++] = 0x05 ;            // Descrizione Start Step Motor 0x005 (su dodici bit)
          Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
          Connect->appo_bus[kk++] = 0x18 ;
          Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
          Connect->appo_bus[kk++] = 0x00 ;
          Connect->appo_bus[kk++] = ((p->parint[0]>>8) & 0xFF) ;// Id Step Motor parte alta
          Connect->appo_bus[kk++] = (p->parint[0] & 0xFF) ;   // parte bassa
          Connect->appo_bus[kk++] = ((p->parint[1]>>8) & 0xFF) ;// Resolution 1-2-8-16
          Connect->appo_bus[kk++] = (p->parint[1] & 0xFF) ;
          Connect->appo_bus[kk++] = ((p->parint[4]>>8) & 0xFF) ;// Current load 50 o 100%
          Connect->appo_bus[kk++] = (p->parint[4] & 0xFF) ;
          if((p->parint[2]>>31) == 1) {
            appo = 2 ;                      // Setto direzione ccw
            p->parint[2] = (-p->parint[2]) ;          // Riporto il valore della speed in positivo per il comando
          }
          else appo = 1;                    // Setto direzione cw
          if (p->parint[1] == 0) {
            appo = 0;
          }
          Connect->appo_bus[kk++] = ((appo>>8) & 0xFF) ;    // Command 2 ccw - 1 cw - 0 brake
          Connect->appo_bus[kk++] = (appo & 0xFF) ;
          Connect->appo_bus[kk++] = ((p->parint[2]>>24) & 0xFF) ;// Speed step/sec
          Connect->appo_bus[kk++] = ((p->parint[2]>>16) & 0xFF) ;
          Connect->appo_bus[kk++] = ((p->parint[2]>>8) & 0xFF) ;
          Connect->appo_bus[kk++] = (p->parint[2] & 0xFF) ;
          Connect->appo_bus[kk++] = ((p->parint[3]>>24) & 0xFF) ;// Max acc.
          Connect->appo_bus[kk++] = ((p->parint[3]>>16) & 0xFF) ;
          Connect->appo_bus[kk++] = ((p->parint[3]>>8) & 0xFF) ;
          Connect->appo_bus[kk++] = (p->parint[3] & 0xFF) ;
          if (p->parint[5]==0) appo=0 ;             // Movimento di velocita'
          else if (p->parint[3]==0) appo=1 ;    //fixed speed
          else appo = 2 ;                   // Movimento di spazio perche' ho un valore di steps da percorrere
          Connect->appo_bus[kk++] = ((appo>>8) & 0xFF) ;    // Counter mode : Movimento di spazio=2 o di velocita'=0
          Connect->appo_bus[kk++] = (appo & 0xFF) ;
          Connect->appo_bus[kk++] = ((p->parint[5]>>24) & 0xFF) ;// Steps to do
          Connect->appo_bus[kk++] = ((p->parint[5]>>16) & 0xFF) ;
          Connect->appo_bus[kk++] = ((p->parint[5]>>8) & 0xFF) ;
          Connect->appo_bus[kk++] = (p->parint[5] & 0xFF) ;
          }
        Connect->flg_car = kk ;
        break;

        case CMD_UpdateConfiguration:
          kk = 0 ;
          if (p->parint[1] == 1) {    // se primo blocco
            appo = (p->parint[0]+24);
          }
          else {
            appo = p->parint[0];
          }
          Connect->appo_bus[kk++] = (p->parint[1]>>8 & 0xFF) ;            // Block 00 01
          Connect->appo_bus[kk++] = (p->parint[1] & 0xFF) ;            //
          Connect->appo_bus[kk++] = 0x10 ;            // Opcopde binary: 0001xxxx Comando solo su i 4 bit piu' significativi
          Connect->appo_bus[kk++] = 0x0C ;            // Write HWC Configuration File opcode 0x00C (su dodici bit)
          Connect->appo_bus[kk++] = (appo>>8 & 0xFF); // Lenght in bytes of Data
          Connect->appo_bus[kk++] = (appo & 0xFF) ;

          if (p->parint[1] == 1) {          // se primo blocco inserisco i parametri nel payload
            Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
            Connect->appo_bus[kk++] = 0x00 ;
                                                        //FileName (Unicode), Word[8] Length
            Connect->appo_bus[kk++] = 0x00 ;            /* unicode C */
            Connect->appo_bus[kk++] = 0x43 ;
            Connect->appo_bus[kk++] = 0x00 ;            /* unicode N */
            Connect->appo_bus[kk++] = 0x4E ;
            Connect->appo_bus[kk++] = 0x00 ;            /* unicode F */
            Connect->appo_bus[kk++] = 0x46 ;
            Connect->appo_bus[kk++] = 0x00 ;            /* unicode I */
            Connect->appo_bus[kk++] = 0x49 ;
            Connect->appo_bus[kk++] = 0x00 ;            /* unicode M */
            Connect->appo_bus[kk++] = 0x4D ;
            Connect->appo_bus[kk++] = 0x00 ;            /* unicode A */
            Connect->appo_bus[kk++] = 0x41 ;
            Connect->appo_bus[kk++] = 0x00 ;            /* unicode G */
            Connect->appo_bus[kk++] = 0x47 ;
            Connect->appo_bus[kk++] = 0x00 ;            /* unicode E */
            Connect->appo_bus[kk++] = 0x45 ;

            Connect->appo_bus[kk++] = 0x00 ;            // Reserved for future use
            Connect->appo_bus[kk++] = 0x00 ;
            Connect->appo_bus[kk++] = (p->parint[2]>>24 & 0xFF) ;   // Length in bytes, Word[2] Length
            Connect->appo_bus[kk++] = (p->parint[2]>>16 & 0xFF) ;
            Connect->appo_bus[kk++] = (p->parint[2]>>8 & 0xFF) ;
            Connect->appo_bus[kk++] = (p->parint[2] & 0xFF) ;
          }

          printf(">>>> dati %d : ",p->parint[1]);
          for (i=(p->parint[3]); i<(p->parint[3])+(p->parint[0]); i++) {            // EEPROM content (bytes)
            Connect->appo_bus[kk++] = p->parstr[i/MAXCH][i%MAXCH];
            printf("%c", Connect->appo_bus[kk-1]);
          }
          printf("\n");

          Connect->flg_car = kk ;

        break;

        case CMD_ReadConfiguration:
          kk = 0 ;
          Connect->appo_bus[kk++] = 0x00 ;            // Block 00 01
          Connect->appo_bus[kk++] = 0x01 ;            //
          Connect->appo_bus[kk++] = 0x10 ;            // Opcopde binary: 0001xxxx Comando solo su i 4 bit piu' significativi
          Connect->appo_bus[kk++] = 0x0B ;            // Descrizione Get digital input status 0x001 (su dodici bit)
          Connect->appo_bus[kk++] = 0x00 ;            // Lenght in bytes of Data
          Connect->appo_bus[kk++] = 0x14 ;
          Connect->appo_bus[kk++] = 0x00 ;            // Data : Option una word
          Connect->appo_bus[kk++] = 0x00 ;
                                                      //FileName (Unicode), Word[8] Length
          Connect->appo_bus[kk++] = 0x00 ;            /* unicode C */
          Connect->appo_bus[kk++] = 0x43 ;
          Connect->appo_bus[kk++] = 0x00 ;            /* unicode N */
          Connect->appo_bus[kk++] = 0x4E ;
          Connect->appo_bus[kk++] = 0x00 ;            /* unicode F */
          Connect->appo_bus[kk++] = 0x46 ;
          Connect->appo_bus[kk++] = 0x00 ;            /* unicode I */
          Connect->appo_bus[kk++] = 0x49 ;
          Connect->appo_bus[kk++] = 0x00 ;            /* unicode M */
          Connect->appo_bus[kk++] = 0x4D ;
          Connect->appo_bus[kk++] = 0x00 ;            /* unicode A */
          Connect->appo_bus[kk++] = 0x41 ;
          Connect->appo_bus[kk++] = 0x00 ;            /* unicode G */
          Connect->appo_bus[kk++] = 0x47 ;
          Connect->appo_bus[kk++] = 0x00 ;            /* unicode E */
          Connect->appo_bus[kk++] = 0x45 ;

          Connect->appo_bus[kk++] = 0x00 ;            // Reserved for future use
          Connect->appo_bus[kk++] = 0x00 ;
          Connect->flg_car = kk ;
        break;
      }
      rispondi(0,Connect);                        // Invio richiesta sulla seriale del buffer preparato
#ifdef DEBUG_PRINT
printf("%d/%d Write %d bytes %d\n",AreaSpi.sma,AreaSpi.accu,Connect->last_id_send,kk);
#endif
#ifdef SIMULA_RISPOSTA
  SimulazioneHWC(p->cmdTx,Connect->last_id_send);
#endif
    }
#ifdef DEBUG_PRINT
printf("Coda DRIVER SPI smaltita %d\n",TAU_PLC);
#endif
  }
}
/**############### INTERPRETAZIONE DEL CONTENUTO DATA BLOCK ############
 * Connect->rx_var[0] = Identify msg type
 *  2 = HWC -> MC reply command ccc
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
 * Connect->rx_var[2] = Lunghezza del payload
 * Connect->chk_bus[6....] = Payload <<<<<<<<<< Devo analizzare questo buffer per sapere cosa e' arrivato
 * Connect->addr_dev = numero blocco arrivato
 * Connect->last_id_send = client a cui devo inviare la risposta
 * ###################################################################*/
int interpreta(struct connectManage *Connect)
{
struct paramWbSpi *p ;
int id;
int kk=0;
int i;
int appo, appo2;


if (Connect->last_id_send == -1) {
	Connect->last_id_send = ProtoConnect->Connect[1]->last_id_send;
}

#ifdef DEBUG_PRINT
printf("Interpreto SPI2110: Identify msg type %d \n",Connect->rx_var[0]);
printf("                    Operative code    %X \n",Connect->rx_var[1]);
printf("                    Lunghezza payload %d \n",Connect->rx_var[2]);
printf("                    Numero blocco     %d \n",Connect->addr_dev);
printf("                    Cliente           %d \n",Connect->last_id_send);
#endif
  if(Connect->rx_var[0] == 2) {                   // Ricevuto risposta alla mia ultima domanda
      id = AreaSpi.accuRx ++ ;                      // Occupo una posizione di accumulo
    if (AreaSpi.accuRx>=AreaSpi.max) AreaSpi.accuRx = 0 ;
    p = AreaSpi.param[id];                      // Punto ai parametri
    switch(Connect->rx_var[1])
    {
     case Restart:                          // Restart
        p->cmdTx = CMD_Restart ;
        p->rispoRx = Connect->last_id_send ;
        p->parint[0] = ProtoConnect->Connect[1]->id_area_read ;
     break;
     case Get_Digital_Input_Status:                 // Get Digital Input Status
        p->cmdTx = CMD_ReadDigitalInput ;
        p->rispoRx = Connect->last_id_send ;
        p->parint[0] = (Connect->chk_bus[10]<<24) | (Connect->chk_bus[11]<<16) | (Connect->chk_bus[12]<<8) | (Connect->chk_bus[13]) ;
        p->parint[1] = (Connect->chk_bus[14]<<24) | (Connect->chk_bus[15]<<16) | (Connect->chk_bus[16]<<8) | (Connect->chk_bus[17]) ;
     break;
     case Get_Analog_Input_Status:                  // Get Digital Input Status
        p->cmdTx = CMD_ReadAnalogInput ;
        p->rispoRx = Connect->last_id_send ;
        p->parint[0] = ProtoConnect->Connect[1]->id_area_read ;   // Recupero l'ID dell'ingresso richiesto dalla struttura della gestione Eventi
        p->parint[1] = (Connect->chk_bus[10]<<8) | (Connect->chk_bus[11])  ;
        p->parint[2] = (Connect->chk_bus[12]<<8) | (Connect->chk_bus[13])  ;
        p->parint[3] = (Connect->chk_bus[14]<<8) | (Connect->chk_bus[15])  ;
        p->parint[4] = (Connect->chk_bus[16]<<8) | (Connect->chk_bus[17])  ;
        p->parint[5] = (Connect->chk_bus[18]<<8) | (Connect->chk_bus[19])  ;
     break;
     case Set_Analog_Output:                    // Set Analog Output Status
        p->cmdTx = CMD_SetAnalogOutput ;
        p->rispoRx = Connect->last_id_send ;
     break;
     case Start_DC_Motor:                     // Start DC Motor
        p->cmdTx = ProtoConnect->Connect[1]->id_area_read ;
        p->rispoRx = Connect->last_id_send ;
     break;
     case Stop_DC_Motor:                      // Stop DC Motor
        p->cmdTx = ProtoConnect->Connect[1]->id_area_read ;
        p->rispoRx = Connect->last_id_send ;
     break;
     case Solenoid_activation:                    // Start Solenoid
        p->cmdTx = ProtoConnect->Connect[1]->id_area_read ;
        p->rispoRx = Connect->last_id_send ;
     break;
     case Solenoid_Deactivation:                  // Stop Solenoid
        p->cmdTx = ProtoConnect->Connect[1]->id_area_read ;
        p->rispoRx = Connect->last_id_send ;
     break;
     case Set_Digital_Output:                   // Set Digital Output Status
        p->cmdTx = CMD_SetDigitalOutput ;
        p->rispoRx = Connect->last_id_send ;
     break;
     case Start_Stepper_Motor:                    // Start Step Motor
        p->cmdTx = ProtoConnect->Connect[1]->id_area_read ;
        p->rispoRx = Connect->last_id_send ;
     break;
     case Stop_Stepper_Motor:                   // Stop Step Motor
        p->cmdTx = ProtoConnect->Connect[1]->id_area_read ;
        p->rispoRx = Connect->last_id_send ;
     break;
     case Write_HWC_Configuration_file:
        p->cmdTx = CMD_UpdateConfiguration ;
        p->rispoRx = Connect->last_id_send ;
     break;
     case Read_HWC_Configuration_file:
        p->cmdTx = CMD_ReadConfiguration;
        p->rispoRx = Connect->last_id_send;
        p->parint[1] = Connect->rx_var[2];
        p->parint[2] = Connect->addr_dev;
        if (Connect->addr_dev == 1) {
          p->parint[3] = (Connect->chk_bus[28]<<24) | (Connect->chk_bus[29]<<16) | (Connect->chk_bus[30]<<8) | (Connect->chk_bus[31]) ;     // File Length
          printf("Received File length = %d\n", p->parint[3]);
          appo = 32;
          appo2 = Connect->rx_var[2] - 26;  // data len = payload - 26
        }
        else {
          appo = 6;
          appo2 = Connect->rx_var[2];   // data len = payload
        }
        for (i=0; i<appo2; i++) {
          p->parstr[i/MAXCH][i%MAXCH] = Connect->chk_bus[appo+i];    // File data
        }
     break;
    }
    pthread_cond_signal(&AreaSpi.ricevitori);             // Mando la signal al driver che ne ha fatto richiesta
  }
  else if(Connect->rx_var[0] == 3) {        // Ricevuto Event
    id = AreaSpi.accuRx ++ ;                      // Occupo una posizione di accumulo
    if (AreaSpi.accuRx>=AreaSpi.max) AreaSpi.accuRx = 0 ;
    p = AreaSpi.param[id];                      // Punto ai parametri
    switch(Connect->rx_var[1])
    {
      case Stop_Stepper_Motor:
        p->cmdTx = EVT_StopStepperMotor ;
        p->rispoRx = Connect->last_id_send ;
        p->parint[0] = (Connect->chk_bus[8]<<8) | (Connect->chk_bus[9])  ;    // Memorizzo l'id del motore che ha mandato l'event
#ifdef DEBUG_PRINT
        printf("Ricevuto Event Stop Stepper Motor (id = %ld)\n", p->parint[0]);
#endif
      break;
    }
    pthread_cond_signal(&AreaSpi.ricevitori);             // Mando la signal al driver che ne ha fatto richiesta
  }
  else {                                // Ricevuto un comando non consentito (!=2) che devo fare ?
    trace(__LINE__,__FILE__,599,0,0,"Ricevuto un comando con type diverso da 2 e 3 dal canale SPI Master");
  }

// Faccio l'echo di quello che e' arrivato
/*
  for(kk=0;kk<Connect->rx_var[2]+6;kk++) Connect->appo_bus[kk] = Connect->chk_bus[kk] ;
  Connect->flg_car = kk ;
  rispondi(0,Connect); // Rispondi con il pacchetto preparato su appo_bus lungo flg_car
*/

  return 0 ;
}

static int param[2];

int Connect_SPIso2110(struct connectManage *Connect)
{
#ifdef DEBUG_PRINT
printf("Inizializzo l'area della connessione sul canale %d address %X handle %d/%d\n",Connect->canale,Connect,Connect->sav_handle,ProtoConnect->sav_handle);
#endif
   Connect->timeout      =  0  ;
   Connect->flg_bus      =  0  ;
   Connect->flg_car      = -1  ;
   Connect->wait_bus     =  0  ;
   Connect->addr_dev     = -1  ;
   Connect->timeout_cnt  =  0  ;
   Connect->last_id_send = -1  ;
   Connect->power_on     =  0  ;
   Connect->poll_param   =  0  ;
   Connect->poll_device  =  0  ;
  
   Connect->proto_client = ISO2110;

   if (Connect->polcan == -1 ) {                    // Ha la gestione ad eventi ?
    param[0] = Connect->sav_handle;
    param[1] = Connect->proto_client;
   if ( pthread_create(&Connect->child,NULL,SendMsg,&param) != 0 )  // Installo il thread che viene chiamato sulla generazione di un evento
      printf("Pthread creazione SendMsg errore: %d - %s \n",errno,strerror(errno));
     else {
    pthread_detach(Connect->child);                     //* disassocia dal processo genitore
    printf("Partito il thread che deve catturare gli eventi tid %d\n",Connect->child);
   }
   }
   return(0);
}
int Init_SPIso2110(int pk)
{
   ProtoConnect = ptr[pk] ;                       // Puntatore alla struttura del protoManage : inizializzo il puntatore
  
    pthread_mutex_init(&AreaSpi.sema,NULL);               // Inizializzo l'Area di scambio eventi per driver SPI
    AreaSpi.trasmettitore = (pthread_cond_t) PTHREAD_COND_INITIALIZER ;

    AreaSpi.max = CODAREA ;                       // Gestisco una coda di valori nell'area di memoria
    struct paramWbSpi *p = (struct paramWbSpi *) malloc(sizeof(struct paramWbSpi )*AreaSpi.max) ;
    for(int kk = 0 ; kk< AreaSpi.max ; kk++) AreaSpi.param[kk] = p++ ;

   printf("Inizializzo il canale %d protocollo SPI ISO2110 su connessione %d device %d\n",pk,ProtoConnect->canale, ProtoConnect->tot_device);
   return(0);
}
int InitAree_SPIso2110(int pk)
{
   printf("Inizializzo l'area del protocollo SPI ISO2110 su canale %d connessione %d device %d\n",pk,ProtoConnect->canale, ProtoConnect->tot_device);
   return(0);
}
/**
 * Interpretazione del protocollo ISO2110
 * Le risposte alle richieste inviate sono poi passate al richiedente
 * in Connect->last_id_send
 */
int SPIso2110(struct connectManage *Connect,unsigned char * car,int ncar,int canale)
{
int kk,tt,jj,ii;
unsigned char check[2] ;
/** STAMPA DEI CARATTERI RICEVUTI ( SCOMMENTARE ) * /
char * appo_sch ;
    appo_sch = car ;
    printf("ISO2110 MASTER - %X Ricevuto %d car  = <",Connect,ncar);
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
      Connect->flg_bus = -3 ;                   // Se c'e' un DLE DLE ok altrimenti DLE x => NAK
      }
      Connect->poll_device = 0 ;                  // Segnalo ok DLE DLE
    }
    else if ((Connect->chk_bus[Connect->flg_car++] = *car) == DLE ){ // Bufferizzo per calcolo checksum
      car++;
      Connect->poll_device = 1 ;                  // Segnalo che il prossimo carattere deve essere un DLE altrimenti errore
      continue ;                          // Non faccio niente per questo ciclo con il primo DLE
    }
    }
//printf("FLG_BUS %d- wait %d\n",Connect->flg_bus,Connect->wait_bus );
      switch (Connect->flg_bus)                     // Analizzo un carattere alla volta
      {
        case 0:                             // Carattere iniziale del frame DLE
      Connect->flg_car = -1 ;                     // Non bufferizzare il messaggio
          if(*car != DLE) Connect->flg_bus = -1 ;             // Aspetto un DLE iniziale
          car++;
        break;
        case 1:                             // II Carattere STX
          if(*car++ != STX) Connect->flg_bus = -1 ;           // Aspetto una sequenza DLE STX per iniziare
      else Connect->flg_car = 0 ;                   // Inizio a bufferizzare il messaggio
    break;
    case 2:
      Connect->meta = *car++ ;                    // Parte alta del block number
    break;
    case 3:                             // finito il block number in addr_dev
      Connect->addr_dev = (( Connect->meta << 8 ) & 0xFF00) | (*car++ & 0xFF)  ;  // Parte bassa del block number
    break;
    case 4:                             // Inizia l'OP CODE
      Connect->rx_var[0] = ((*car>>4) & 0xF)  ;             // Identify msg type (Primi 4 bit dell'opcode) in var[0]
      Connect->meta = (*car++ & 0xF)  ;
    break;
    case 5:                             // Fine OP CODE
      Connect->rx_var[1] =  (( Connect->meta << 8 ) & 0xF00) | (*car++ & 0xFF)  ;  // 12 bit OPCODE in var[1]
    break;
        case 6:                             // Primo byte QUANTITA' DI Bytes del payload
          Connect->meta = *car++ ;
        break;
        case 7:                             // Secondo byte QUANTITA' DI Bytes del payload
          Connect->rx_var[2] = (( Connect->meta << 8 ) & 0xFF00) | *car++ ;
          Connect->rx_var[3] = Connect->rx_var[2] + Connect->flg_bus ;  // Quando flg_bus raggiunge questo valore ho ricevuto tutto il payload
#ifdef DEBUG_PRINT
printf("Bytes del pacchetto %d\n",Connect->rx_var[3]);
#endif
      if (Connect->rx_var[3] == Connect->flg_bus){
      CalCRC(Connect->chk_bus,Connect->flg_car,Connect->appo_bus);// Calcola la checksum che deve arrivare in check
#ifdef DEBUG_PRINT
printf("Ricevuto ultimo bytes del payload vado a calcolare la checksum %d %d\n",Connect->appo_bus[0],Connect->appo_bus[1]);
#endif
      Connect->flg_car = -1 ;                   // Non bufferizzare piu' nel chk_bus
      }
        break;
        default:
//printf("Default %d/%d\n",Connect->flg_bus,Connect->rx_var[3]);
      if ( Connect->flg_bus < Connect->rx_var[3]) {car ++ ; break;} // Non fare altro che bufferizzare nel chk_bus
      if ( Connect->flg_bus == Connect->rx_var[3]){         // Ultimo bytes del payload
      CalCRC(Connect->chk_bus,Connect->flg_car,Connect->appo_bus);// Calcola la checksum che deve arrivare in check
#ifdef DEBUG_PRINT
printf("Ricevuto ultimo bytes del payload vado a calcolare la checksum %d %d\n",Connect->appo_bus[0],Connect->appo_bus[1]);
#endif
      car ++ ;
      Connect->flg_car = -1 ;                   // Non bufferizzare piu' nel chk_bus
      }
      else {                            // siamo dopo il payload
      kk = Connect->flg_bus - Connect->rx_var[3] ;
      switch(kk){
        case 1:                         // deve essere DLE
#ifdef DEBUG_PRINT
printf("DLE finale %X\n",*car);
#endif
          if(*car++ != DLE) Connect->flg_bus = -3 ;       // Aspetto un DLE finale altrimenti NAK
        break;
        case 2:                         // deve essere ETX o ETB
#ifdef DEBUG_PRINT
printf("ETX finale %X\n",*car);
#endif
          if(*car != ETX && *car != ETB) Connect->flg_bus = -3 ; // Aspetto un ETX o ETB finale altrimenti NAK
          car++;
        break;
        case 3:                         // prima checksum
#ifdef DEBUG_PRINT
printf("Prima checksum %X ? %X\n",*car,Connect->appo_bus[0]);
#endif
          if(*car++ != Connect->appo_bus[0]) Connect->flg_bus = -3 ; // Aspetto la checksum che ho calcolata altrimenti NAK
        break;
        case 4:                         // seconda checksum
#ifdef DEBUG_PRINT
printf("Seconda checksum %X ? %X\n",*car,Connect->appo_bus[1]);
#endif
          if(*car++ != Connect->appo_bus[1]) Connect->flg_bus = -3 ; // Aspetto la checksum che ho calcolata altrimenti NAK
          else {                        // Pacchetto corretto posso interpretare e poi rispondere
#ifdef DEBUG_PRINT
printf("Rispondo ACK\n");
#endif
            rispondi(ACK,Connect);              // Rispondi ACK pacchetto ricevuto e checksum OK
            interpreta(Connect) ;               // Vado ad interpretare ed eventualmente a rispondere a chi mi aveva chiesto queste info
            Connect->flg_bus = -2 ;             // Fine pacchetto
          }
        break;
      }
      }
        break;
      }
      Connect->flg_bus ++ ;
      if( Connect->flg_bus == -2 ) {                  // Devo rispondere NACK
#ifdef DEBUG_PRINT
printf("Rispondo NAK\n");
#endif
    rispondi(NAK,Connect);                      // Rispondi NACK a pacchetto ricevuto
    Connect->flg_bus ++ ;
    }
      if( Connect->flg_bus == -1 ) {                  // Finito analisi buffer
#ifdef DEBUG_PRINT
printf("Finito analisi buffer\n");
#endif
        Connect->timeout = 0 ;                      // Messaggio arrivato
        Connect->flg_bus = 0 ;
        Connect->flg_car = -1 ;
        Connect->errore = 0 ;
      }
// printf("jj=%d flg=%d \n",jj,Connect->flg_bus);
    }
    return(0);
}


//########################################### CALCOLO CHECKSUM ####################################
void CalCRC(char *strBuffer, int intBufSize, char *result)
{
unsigned int idx;
unsigned char CS1,CS2 ;

  idx = 0xFFFF ;
  while(intBufSize){
    CS1 = idx & 0xFF;                               // Byte Basso
    CS2 = (idx >> 8 ) & 0xFF ;                      // Byte Alto
      idx = ( (CS1 ^ (*strBuffer++) ) & 0x00FF ) ;            // Trovo l'indice della tabella assicurandomi che stia nei 255
      idx = ( (CS2 ^ crctab[idx])   & 0xFFFF ) ;            // Ricalcolo la nuova checksum sulla WORD
      intBufSize --;                                  // Un carattere in meno
                                    // Metto il carttere nel buffer di trasmissione e passo al carattere successivo
  }
  result[1] = (char) (idx & 0xFF);                      // Parte bassa della checksum
  result[0] = (char) ((idx>>8) & 0xFF);                 // Parte alta  della checksum
}

/* ################################### PARTE DA IMPLEMENTARE ################################ */
// Funzione in polling che non dovrebbe essere utilizzata in quanto il protocollo lavora a eventi
// Serve per gestire il timeout sulla non risposta

void SendPoll_SPIso2110(int canale,int connessione,int comando)
{
struct connectManage *Connect;
int kk ;

  Connect = ptr[connessione]->Connect[0];                 // linea seriale puo' avere solo una connessione
#ifdef DEBUG_PRINT
  printf("Poll %X SPIso2110 wait bus %d\n",Connect,Connect->wait_bus);
#endif
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Gestione timeout attesa risposta ACK NAK
  if (Connect->wait_bus >0) {                           // Sto aspettando risposta ?
    Connect->wait_bus -- ;                              // decremento attesa
    if (Connect->wait_bus >0) return;                   // Attesa finita ? NO => continuo l'attesa
    Connect->timeout_cnt ++ ;
    if (Connect->timeout_cnt<5) rispondi(1,Connect);            // rimanda l'ultimo pacchetto per timeout scaduto
    else {
      Connect->wait_bus = 0 ;                     // Non riprovo piu'
      Connect->timeout_cnt = 0 ;
    }
    Connect->timeout ++ ;                               // Incremento i timeout consecutivi
#ifdef DEBUG_PRINT
printf("Timeout %d canale SPIso2110\n",Connect->timeout);
#endif
  }
}


void inviaNPL(int fd, int len) {
  unsigned char npl[SPI_PROTO_NPL_SIZE] = {SPI_PROTO_NPL_ID, len};
#ifdef DEBUG_PRINT
  printf("invio Next Packet Length %d su client %d > %.2X %.2X\n", len, fd, npl[0], npl[1]);
#endif
  int ok = write(fd, npl, SPI_PROTO_NPL_SIZE);
  if (ok < 0) {
    trace(__LINE__,__FILE__,1000,0,0,"Error write Next Packet Length, error %d = %s",errno,strerror(errno));
  }
  else if (ok > SPI_PROTO_NPL_SIZE) {
    trace(__LINE__,__FILE__,1000,0,0,"Next Packet Length write %d bytes (have to be %d), this is weird", ok, SPI_PROTO_NPL_SIZE);
  }
  usleep(SPI_PROTO_TX_INTERVAL_MS*1000);
}

