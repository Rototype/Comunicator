
//   ################### DEFINE CHE VARIANO IN BASE ALL'APPLICATIVO ############################

#define DIR_IMPIANTO    "etc/" 											// Directory dove risiede il file di configurazione
#define DIR_LOG         "log/" 											// Directory dove ci finiscono i file di log del programma con tutti i messaggi di trace
#define DIR_REPORT      "report/"										// Directory dove risiedono i file di report delle attività del programma
#define DIR_PROGRAMMA   "programma/"									// Directory dove risiedono i file dei programmi di lavoro (ricette)
#define DIR_IMG			"img/"									// Directory di salvataggio dei file necessari al trasferimento immagine
/*-----------------* File log dei messaggi di trace per debug sistema *-------------- */
#define FILE_LOG        "file.log"										// i file di log si chiamano con la data data "yy_mm_dd_file.log"
#define FILE_IMPIANTO   "impianto.conf"									// Nome file dichiarazione impianto
#define FILE_RX_IMMAGINE "log/imma"										// Nome file di appoggio per la ricezione di un immagine
#define FILE_TX			 "log/invia"									// Nome file di appoggio per la trasmissione su WebSocket
#define FILE_TX_1		 "log/invia1"									// Nome file di appoggio per la trasmissione su WebSocket

#define FILE_IMG_SERVICE_BIN	"/home/root/update/Comunicator"
#define FILE_IMG_SOC_BIN		"/home/root/update/soc.bin"
#define FILE_IMG_FPGA_BIN		"/home/root/update/fpga.bin"
#define FILE_IMG_RX_BASE64	"img/rx_base64"						// Nome file su cui si riceve da WebSocket l'immagine codificata Base64
#define FILE_IMG_RX_BINARY	"img/rx_binary"						// Nome file su cui si salva l'immagine ricevuta da WebSocket decodificata in binary
#define FILE_IMG_TX_BINARY	"img/tx_binary"						// Nome file su cui si riceve da Zynq l'immagine eleborata in binary
#define FILE_IMG_TX_BASE64	"img/tx_base64"						// Nome file su cui si salva l'immagine elaborata in Base64 per rimandarla al WebSocket

#define FILE_NETWORK_CFG	"/home/root/config/eth0.param"
#define FILE_NETWORK_JSON	"etc/network.json"				// Path del file dove salvare la network config ricevuta in formato json
#define FILE_CONFIG_JSON	"etc/config.json"				// Path del file da utlizzare per salvare la config inviata alla MainUnit in formato json
#define HWC_CONFIG_MAX_BLOCK_SIZE	57
#define HWC_EEPROM_MAX_SIZE_BYTES	4096

#define FILE_DB 		"DBPARA"     	   								// Nome file del DB SQLite3 di default 
#define NOME_TB 		"PARAMETRI"      								// Nome tabella parametri di default

// Definizioni per protocollo SPI
#define SPI_PROTO_NPL_ID			0x55				// idendificativo pacchetto Next Packet Length
#define SPI_PROTO_NPL_SIZE			2					// dimensione pacchetto Next Packet Length
#define SPI_PROTO_MAX_TRANSFER		128					// massimo trasferimento in bytes possibile per scrittura
#define SPI_PROTO_TX_INTERVAL_MS 	2					// intervallo di tempo (in ms) fra invio Next Packet Length e invio dati

// Definizioni per send image
#define IMG_LOCAL_INTERFACE		"192.168.121.2"			// Interfaccia locale da cui inviare l'immagine
#define IMG_LOCAL_PORT			52200					// Porta su cui ricevere l'immagine invertita
#define IMG_ZYNQ_IP_ADDR		"192.168.121.232"		// Indirizzo ip per inviare l'immagine alla zynq
#define IMG_ZYNQ_PORT			5001					// Porta su cui inviare l'immagine alla zynq
#define IMG_SENDCMD_TIMEOUT		5						// Timeout del comando di send (in secondi)
#define IMG_LENBLK_LIMIT		(8000000/57)			// Limite di pacchetti websocket per la ricezione immagine ( ~8 MB)

/************************** DEFINE NOMI FILE E DIRECTORY UTILIZZATE DALL'APPLICATIVO ******************************/
#define PARTOT 9
#define ROOTOBJ 5														// Elenco parametri utilizzati all'interno delle stringhe JSON


// keep sync these look-up table: cmdWbSock + char CMD_tab[CMDTOT]

enum cmdWbSock {
	None,
	CMD_UpdateFirmware,
	CMD_UpdateWebSocketFirmware,
	CMD_Restart,
	CMD_UpdateConfiguration,
	CMD_UpdateNetworkConfiguration,
	CMD_ReadDigitalInput,
	CMD_ReadAnalogInput,
	CMD_SetAnalogOutput,
	CMD_SetDCMotor,
	CMD_SetDCMotorPWM,
	CMD_SetDCSolenoid,
	CMD_SetDCSolenoidPWM,
	CMD_SetDigitalOutput,
	CMD_SetStepperMotorSpeed,
	CMD_SetStepperMotorCountSteps,
	CMD_InvertImage,
	EVT_StopStepperMotor,
	CMD_ReadConfiguration,
    CMDTOT 
};
enum cmdSpi {
	Command_None,
	Get_Digital_Input_Status,
	Get_Analog_Input_Status,
	Set_Digital_Output,
	Set_Analog_Output,
	Start_Stepper_Motor,
	Stop_Stepper_Motor,
	Solenoid_activation,
	Solenoid_Deactivation,
	Start_DC_Motor,
	Stop_DC_Motor,
	Read_HWC_Configuration_file,
	Write_HWC_Configuration_file,
	Restart
};

