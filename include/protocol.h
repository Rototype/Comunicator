/********** PROTOTYPE PROTOCOL FUNCTION REFERENCES SECTION ************/
int (* dammi_protocollo(int ))(int, unsigned char *, int, int);
void SendPollCmd(int,int,int ) ;
void InitChannel(int);
void InitConnection(int,struct connectManage *);

/* Funzioni Websocket */
extern int WebSocket(struct connectManage * ,unsigned char *,int,int ) ;
extern int Init_WebSocket(int ) ;
extern int InitAree_WebSocket(int ) ;
extern int Connect_WebSocket(struct connectManage *) ;
extern void SendPoll_WebSocket(int,int,int);

/* Funzioni Iso2110 Master */ 
extern int SPIso2110(int ,unsigned char *,int,int ) ;
extern int Init_SPIso2110(int ) ;
extern int InitAree_SPIso2110(int ) ;
extern int Connect_SPIso2110(struct connectManage *) ;
extern void SendPoll_SPIso2110(int,int,int);

/* Funzioni Iso2110 Slave */
extern int SPIso2110S(int ,unsigned char *,int,int ) ;
extern int Init_SPIso2110S(int ) ;
extern int InitAree_SPIso2110S(int ) ;
extern int Connect_SPIso2110S(struct connectManage *) ;
