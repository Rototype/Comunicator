/* -------------------------------------------------------------------------
   =========================================================================
                            S T R U T T U R . H
        Strutture utilizzate dal sistema comunicator .
   =========================================================================
   ------------------------------------------------------------------------- */ 

struct connectManage	                // Descrittore della singola connessione con il CANALE desiderato
	{
        int client;                     // Id connessione
        int tot_device;                 // Totale devices connessi su questo canale
        int tot_param;                  // numero totale dei parametri da leggere sui dispositivi di questo canale 
        int flg_bus;                    // Sequenziatore per l'interpretazione della risposta
        int flg_car;                    // Numero di bytes contenuti nel pacchetto arrivato
        int flag_stop;                  // 
        
        int id_area ;                   // indice area in scrittura
        int id_area_read ;              // indice area in lettura
        int error_code ;                // errore nel pacchetto ricevuto

        int polcan;                     // Tempo di polling sul canale in microsecondi
        int timer;                      // Timer di accumulo per il polling
        int tipo_protocollo;            // Identificatore del protocollo 1= IP 0 = Seriale
        int sav_handle ;                // Identificatore di linea (handle)
        int proto_client ;              // Protocollo associato alla linea
        int canale ;  			            // Indice del canale
        long port ;                     // Porta di colloquio in caso di connessione IP 

		pthread_t child;

        int timeout;                    // Gestore del timeot sulla singola risposta
        int wait_bus;                   // Attesa risposta da un dispositivo
        int addr_dev;                   // Variabile di appoggio per l'indirizzo del device che sta rispondendo
        int timeout_cnt;                // totale degli errori di timeout verificati
        int last_id_send;               // Ultimo indice device a cui e' stato spedito un messaggio
        int coll;                       // pacchetti OK
        int errore;                     // Errore di timeout sul colloquio del device
        int meta;                       // Variabile di appoggio del primo byte del valore che si sta leggendo
        int power_on;					// Sequenziatore per eventuale inizializzazione 
        int poll_device;                // indirizzo (indice) del device in polling
        int poll_param;                 // indice del parametro in polling su tutti i device del canale
        long len_msg;					// Variabile per la lunghezza di messaggi molto lunghi (invio file binari o immagini, etc ...)
        unsigned int rx_var[1000];       // area di appoggio per variabili che vengono lette durante una richiesta di valori
        unsigned int tx_var[1000];       // area di appoggio per variabili che vengono lette durante una richiesta di valori
        unsigned char appo_bus[4194304];    // area di appoggio del buffer di trasmissione
        unsigned char chk_bus[2000];     // area di appoggio per calcolo della checksum in ricezione

        int coda_socket[20] ;		    // Richieste in coda per il canale

        int id_unique[100][4];          // Gestione massima di 100 device sul singolo canale di colloquio 
                                        // nella posizione [0] = unique_id
                                        // nella posizione [1] = indirizzo sul canale 
                                        //      (che deve comunque essere univoco almeno sul canale)
                                        // nella posizione [2] = indice AsseX della dichiarazione assi.conf
                                        // nella posizione [3] = indice indirizzi struttura per AsseX
        unsigned char result[2000];      // Spazio per i caratteri ricevuti.
	};

// Struttura contenente le variabili per la gestione del protocollo di un canale di colloquio
struct protoManage	                    // Descrittore per il canale di comunicazione
	{
        int tot_device;                 // Totale devices connessi su questo canale
        int tot_param;                  // numero totale dei parametri da leggere sui dispositivi di questo canale 
        int flag_stop;                  // Stop momentaneo del polling sul canale

        int polcan;                     // Tempo di polling sul canale in microsecondi
        int timer;                      // Timer di accumulo per il polling
        int tipo_protocollo;            // Identificatore del protocollo 1= IP 0 = Seriale
        int sav_handle ;                // Identificatore di linea (handle)
        int proto_client ;              // Protocollo associato alla linea
        int canale ;  			        // Indice del canale
        long port ;                     // Porta di colloquio in caso di connessione IP 

        int client[MAXCONN][8] ;        // Buffer per passaggio dei parametri alla funzione di interpretazione dei caratteri in arrivo
										// [MAXCONN][0] = 4 : cliente (e' un progressivo in base agli host connessi)
										// [MAXCONN][1] = 32312 : porta di comunicazione
										// [MAXCONN][2] = 127. :IP Address
										// [MAXCONN][3] = 0.
										// [MAXCONN][4] = 0.
										// [MAXCONN][5] = 1
										// [MAXCONN][6] = 9 : protocollo (TCP_MODBUS)
										// [MAXCONN][7] = x : socket server a cui e' stato risposto
        int sok[MAXCONN];               // Tabella di associazione dei socket attivi al canale 
        int cmd_sok[MAXCONN] ;          // Comando da inviare alla connessione sul canale corrispondente 
        struct connectManage *Connect[MAXCONN] ; // Struttura di gestione della connessione
        long port_proto[4];             // Tabella che contiene l'associazione del socket descriptor,porta,tipo di protocollo e idx di sok associato 
        int coda_socket[20] ;		    // Richieste broadcast in coda per il canale
        int tot_sok_act ;		        // Totale dei client attivi sulla porta IP di questo canale

        int id_unique[100][4];          // Gestione massima di 100 device sul singolo canale di colloquio 
                                        // nella posizione [0] = unique_id
                                        // nella posizione [1] = indirizzo sul canale 
                                        //      (che deve comunque essere univoco almeno sul canale)
                                        // nella posizione [2] = indice AsseX della dichiarazione assi.conf
                                        // nella posizione [3] = indice indirizzi struttura per AsseX
	};

struct Device        					// Descrittore per il dispositivo di interfaccia sul canale di comunicazione
	{
        unsigned char nome_dev[80] ;
        unsigned char indirizzo_dev[200] ;
        unsigned char processo[80] ;
        unsigned char modello_dev[20] ;

// Protocollo associato al device in modo da poter gestire su un canale standard dispositivi diversi (da sviluppare la gestione differenziata del protocollo MODBUS su dispositivi diversi per esempio
        int proto_device ;              // Se non dichiarato e' uguale a quello della connessione

        int unique_id;                	// Indice unico del dispositivo all'interno dell'impianto (sequenziale)
        int address;
        int tot_res;                  	// Totale risorse del dispositivo (massimo 10)
        int variabile[10];            	// Massimo 10 risorse : Asse - IN - OUT - ENC1 - ENC 4
        int areaA[10];
        int bitA[10];
        int maskA[10];
        int cmd ;                     	// Comando da inviare 
        int canale ;                  	// C'e' l'indice della struttura protoManage utilizzata per il colloquio
        int (*interprete)();   			// Funzione per l'interpretazione dei caratteri letti sul canale del dispositivo
        int (*cmd_polling)();         	// Funzione per il comando da inviare in POLLING
        struct wassi * assi_struct;  	// Puntatore alla struttura degli assi
        struct VarDeviceIO * io_struct; // Puntatore alla struttura degli IO del dispositivo
        struct VarDeviceEnc * enc_struct;   // Puntatore alla struttura degli encoder/contatori del dispositivo
	};
	
/** Memoria Condivisa fra i thread per lo scambio di eventi... ********/
#define CODAREA	10														// Massimo messaggi in coda memorizzati
#define MAXP 10	
#define MAXCH 50														// Numero massimo di parametri per comando
struct paramWbSpi{
	int cmdTx;															// Comando da trasmettere
	int rispoRx;														// Cliente che deve ricevere la risposta
	char parstr[MAXP][MAXCH];												// posto per parametri stringhe di 50 bytes max....
	double pardob[MAXP];												// posto per parametri double
	unsigned long parint[MAXP];													// posto per parametri long
};
struct SendArea{
	pthread_mutex_t sema;												// Semaforo di mutua esclusione delle variabili
	pthread_cond_t  trasmettitore;										// Variabile condition per il lettore 
	pthread_cond_t  ricevitori;											// Variabile condition per gli scrittori 
	int accu;	
	int sma;															// Variabili per la gestione del buffer circolare dei comandi
	int accuRx;	
	int smaRx;															// Variabili per la gestione del buffer circolare dei comandi
	int max;															// Massimo numero di elementi nella coda
	struct paramWbSpi *param[CODAREA];									// Puntatore alla struttura dei parametri 
} ;
