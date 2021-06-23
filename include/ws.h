/*
Copyright (C) 2019 STELIRE ADVISER di Stefano Pria <priastefano@stelire.it>
*/
#ifndef WS_H
#define WS_H

#define MESSAGE_LENGTH 2048
#define MAX_CLIENTS    8

#define WS_KEY_LEN     24
#define WS_MS_LEN      36
#define WS_KEYMS_LEN   (WS_KEY_LEN + WS_MS_LEN)
#define MAGIC_STRING   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define WS_HS_REQ      "sec-websocket-key"

#define WS_HS_ACCLEN   130
#define WS_HS_ACCEPT                   \
"HTTP/1.1 101 Switching Protocols\r\n" \
"Upgrade: websocket\r\n"               \
"Connection: Upgrade\r\n"              \
"Sec-WebSocket-Accept: "               \

#define WS_BAD_REQUEST_STR 		"HTTP/1.1 400 Invalid Request\r\n\r\n"
#define WS_SWITCH_PROTO_STR 	"HTTP/1.1 101 Switching Protocols"
#define WS_TOO_BUSY_STR 		"HTTP/1.1 503 Service Unavailable\r\n\r\n"

/* Frame definitions. */
#define WS_FIN    128

/* Frame types. */
#define WS_FR_OP_CONT 0			// Codice frame continuazione
#define WS_FR_OP_TXT  1			// Codice frame tipo testo IMPLEMENTATA
#define WS_FR_OP_BIN  2			// Codice frame tipo binario IMPLEMENTATA
#define WS_FR_OP_END  3			// Codice frame fine
#define WS_FR_OP_CLSE 8			// Codice frame close connection IMPLEMENTATA
#define WS_FR_OP_PING 9			// Codice frame tipo ping IMPLEMENTATA
#define WS_FR_OP_PONG 10		// Codice frame tipo pong

#define WS_FR_OP_UNSUPPORTED 0xF	// Codice frame NON SUPPORTATO

#endif /* WS_H */
