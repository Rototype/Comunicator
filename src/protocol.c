/************************** INCLUDE APPLICATION SECTION *****************************/
#include "extern.h"
#include "protocol.h"													// Definizione dei protocolli utilizzati
int (* dammi_protocollo(int ))(int, unsigned char *, int, int);
void SendPollCmd(int,int,int ) ;
void InitChannel(int);
void InitConnection(int,struct connectManage *);

/************************** FUNCTION DEVELOP SECTION *****************/
int (* dammi_protocollo(int protocollo))()
{
int (*Interprete)();

  switch(protocollo){
      case WEBSOCKET:
        Interprete = WebSocket ;
      break;
      case ISO2110:
        Interprete = SPIso2110 ;
      break;
      case ISO2110S:
        Interprete = SPIso2110S ;
      break;
  }
  return(Interprete);
}
/************************** FUNCTION DEVELOP SECTION *****************/

/*---------------------------------------------*
 * Inizializzazione dispositivi dell'impianto *
 *---------------------------------------------*/

void InitChannel(int id_act)
{
printf("Init Channel %d = %s\n",id_act,protocol_name[ptr[id_act]->proto_client]);
  // * In base al protocollo invio il comando di polling relativo *
  switch(ptr[id_act]->proto_client){
      case WEBSOCKET:
    	  Init_WebSocket(id_act) ;
      break;
      case ISO2110:
    	  Init_SPIso2110(id_act) ;
      break;
      case ISO2110S:
    	  Init_SPIso2110S(id_act) ;
      break;
  }
}
/*---------------------------------------------*
 * Inizializzazione dispositivi dell'impianto *
 *---------------------------------------------*/

void InitAreeChannel(int id_act)
{
printf("Init Aree Channel %d = %s\n",id_act,protocol_name[ptr[id_act]->proto_client]);
  switch(ptr[id_act]->proto_client){
      case WEBSOCKET:
    	  InitAree_WebSocket(id_act) ;
      break;
      case ISO2110:
    	  InitAree_SPIso2110(id_act) ;
      break;
      case ISO2110S:
    	  InitAree_SPIso2110S(id_act) ;
      break;
  }
}
/*---------------------------------------------*
 * Inizializzazione dispositivi dell'impianto *
 *---------------------------------------------*/

void InitConnection(int id_act,struct connectManage * tcpConnect)
{
printf("Init Connection %d = %s\n",id_act,protocol_name[ptr[id_act]->proto_client]);

  switch(ptr[id_act]->proto_client){
      case WEBSOCKET:
    	  Connect_WebSocket(tcpConnect) ;
      break;
      case ISO2110:
    	  Connect_SPIso2110(tcpConnect) ;
      break;
      case ISO2110S:
    	  Connect_SPIso2110S(tcpConnect) ;
      break;
  }
}

/*
 * ************************* Polling sui comandi da inviare ai dispositivi ****************
 * @param canale, num di connessione del canale, comando da inviare
 * @return niente
 */

void SendPollCmd(int linea,int id_act,int opt) 
{
  /* In base al protocollo invio il comando di polling relativo */
  switch(ptr[id_act]->proto_client){
      case WEBSOCKET:
		SendPoll_WebSocket(linea,id_act,opt);
      break;
      case ISO2110:
		SendPoll_SPIso2110(linea,id_act,opt);
      break;
      case ISO2110S: // Non ha polling questo protocollo bisogna mettere a 0 il tempo di polling in taraura
      break;
  }
}
int dammi_canale(int protocollo)
{
int kk;   
  for(kk = 0 ; kk< act_client ; kk ++ ){
    if (ptr[kk]->proto_client == protocollo ) return(kk);
  }
  return (-1);
}
