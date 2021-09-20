/************************** DEFINE DEFAULT SECTION **********************************/
#define NOME "CHIOSCO Ver." /* Nome del programma e versione */
#define VER 3
/**
 * Ver.1 Primo rilascio
 * Ver.2 Gestione acquisizione e invio immagine invertita (momentaneamente appoggiandola su file)
 * Ver.3 Gestione lowercase durante l'handshake iniziale della connessione WebSocket
 * 
 **/
/* DEFINE DELL'AREA DATI CONDIVISA */
#define     MAX_A		    512  // 512+256+128 LONG
#define     MAX_P		    256  // 
#define     MAX_W		    128  // 
/************************** DEFINE LOW LEVEL PROTOCOL SECTION ******************************/
#define NO_LOW_PROT 0
#define XONXOFF 1
#define RTSCTS 2
#define MAXCONN 6           /* Definisce il numero Massimo di connessioni (-1) da lasciare in coda su TCPIP */
#define MAX_N_PORT 2        /* Definisce il numero Massimo di porte IP da gestire */
#define MAX_PROTO_CONT 10   // Numero di canali di protocollo controllati
#define MAX_DEVICE 10       // Numero di device attaccati all'impianto 
#define MAX_ASSI 10         // Numero di assi massimi dell'impianto 

/************************** DEFINE PROTOCOLLI ******************************/
#define NO_PROTOCOL          0
#define ECHO_PROTOCOL        1 
#define WEBSOCKET            2
#define ISO2110              3
#define ISO2110S             4
#define HOW_MANY_PROT        5    // Deve rimanere l'ultimo per sapere quanti protocolli sono gestiti

/* ################### DEFINE PER LA FUNZIONE READ PARAMETRI ############ */

#define READ_PAR 0
#define NO_DB 0x40
#define WRITE_PAR 0x80

// * Define della funzione di lettura sui file
#define RIAVVIO_FILE    6
#define AUX_FILE        2
#define ASSI_FILE       1
#define IMPIANTO_FILE   0

#define TOT_SGN         24
