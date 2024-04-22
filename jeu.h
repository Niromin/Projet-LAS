#ifndef _JEU_H_
#define _JEU_H_

#define SERVER_PORT 9502
#define SERVER_IP "127.0.0.1" /* localhost */
#define MAX_PSEUDO 256

#define INSCRIPTION_REQUEST 10
#define INSCRIPTION_OK 11
#define INSCRIPTION_KO 12
#define START_GAME 13
#define CANCEL_GAME 14

/* struct message used between server and client */
typedef struct
{
  char messageText[MAX_PSEUDO];
  int code;
} StructMessage;
#endif
