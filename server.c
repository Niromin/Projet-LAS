#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

#include "messages.h"
#include "utils_v1.h"

#define MIN_PLAYERS 2 // Nombre minimum de joueurs requis pour démarrer le jeu
#define BACKLOG 5
#define REGISTRATION_TIME 30 // Temps d'inscription en secondes

typedef struct Player
{
  char pseudo[MAX_PSEUDO];
  int sockfd;
} Player;

volatile sig_atomic_t end = 0;

void endServerHandler(int sig)
{
  end = 1;
}

void terminate(Player *tabPlayers, int nbPlayers)
{
  printf("\nJoueurs inscrits : \n");
  for (int i = 0; i < nbPlayers; i++)
  {
      printf("  - %s inscrit\n", tabPlayers[i].pseudo);
  }
  exit(0);
}

/**
 * PRE:  serverPort: a valid port number
 * POST: on success, binds a socket to 0.0.0.0:serverPort and listens to it ;
 *       on failure, displays error cause and quits the program
 * RES: return socket file descriptor
 */
int initSocketServer(int serverPort)
{
  int sockfd = ssocket();

  /* no socket error */

  sbind(serverPort, sockfd);

  /* no bind error */
  slisten(sockfd, BACKLOG);

  /* no listen error */
  return sockfd;
}

// Gestionnaire de signal pour SIGALRM (temporisation)
void alarmHandler(int sig)
{

}

int main(int argc, char **argv)
{
  StructMessage msg;
  Player tabPlayers[BACKLOG]; // Utilisez une taille de tableau pour gérer plusieurs joueurs
  int nbPlayers = 0;

  sigset_t set;
  ssigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  ssigprocmask(SIG_BLOCK, &set, NULL);

  ssigaction(SIGTERM, endServerHandler);
  ssigaction(SIGINT, endServerHandler);

  int sockfd = initSocketServer(SERVER_PORT);
  printf("Le serveur tourne sur le port : %i \n", SERVER_PORT);

  // setsockopt -> to avoid Address Already in Use
  int option = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));

  ssigprocmask(SIG_UNBLOCK, &set, NULL);

  while (!end)
  {
  // Set a timer for the registration phase
  alarm(REGISTRATION_TIME);
  ssigaction(SIGALRM, alarmHandler);

  printf("Phase d'inscription en cours... Temps restant : %d secondes\n", REGISTRATION_TIME);

  // Accept registration requests during the registration phase
  while (!end)
  {
    // Accept client connections
    int newsockfd = accept(sockfd, NULL, NULL);
    if (end)
    {
      terminate(tabPlayers, nbPlayers);
    }
    checkNeg(newsockfd, "ERROR accept");

    // Read registration request from client
    ssize_t ret = read(newsockfd, &msg, sizeof(msg));
    if (end)
    {
      terminate(tabPlayers, nbPlayers);
    }
    checkNeg(ret, "ERROR READ");

    printf("Inscription demandée par le joueur : %s\n", msg.messageText);

    // Accept the registration request
    if (nbPlayers < BACKLOG)
    {
      msg.code = INSCRIPTION_OK;
      strcpy(tabPlayers[nbPlayers].pseudo, msg.messageText);
      tabPlayers[nbPlayers].sockfd = newsockfd;
      nbPlayers++;
    }
    else
    {
      msg.code = INSCRIPTION_KO;
    }

    // Send registration response to the client
    nwrite(newsockfd, &msg, sizeof(msg));
    printf("Nb Inscriptions : %i\n", nbPlayers);
  }

  // Reset the timer and proceed to the game phase
  ssigaction(SIGALRM, SIG_DFL);
  printf("Fin de la phase d'inscription.\n");

  // Optional: Start the game phase here
  }

  sclose(sockfd);
  return 0;
}