#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#include "messages.h"
#include "utils_v1.h"

#define MAX_PLAYERS 5
#define MIN_PLAYERS 2
#define MAX_TILES 40
#define BACKLOG 5
#define TIME_INSCRIPTION 10
#define TOTAL_ROUNDS 20

typedef struct Player
{
	char pseudo[MAX_PSEUDO];
	int sockfd;
	int shot;
} Player;

/*** globals variables ***/
Player tabPlayers[MAX_PLAYERS];
volatile sig_atomic_t end_inscriptions = 0;

void endServerHandler(int sig)
{
	end_inscriptions = 1;
}

void disconnect_players(Player *tabPlayers, int nbPlayers)
{
	for (int i = 0; i < nbPlayers; i++)
		sclose(tabPlayers[i].sockfd);
	return;
}

void createTiles(Tile *tiles, int numTiles)
{
    int tileNumbers[MAX_TILES] = 
    {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
      11, 11, 12, 12, 13, 13, 14, 14, 15, 15,
      16, 16, 17, 17, 18, 18, 19, 19, 20, 21,
      22, 23, 24, 25, 26, 27, 28, 29, 30, -1
    }; // -1 = joker

    for (int i = 0; i < numTiles; i++)
    {
        tiles[i].number = tileNumbers[i];
    }
}



/**
 * PRE:  serverPort: a valid port number
 * POST: on success, binds a socket to 0.0.0.0:serverPort and listens to it ;
 *       on failure, displays error cause and quits the program
 * RES:  return socket file descriptor
 */
int initSocketServer(int port)
{
	int sockfd = ssocket();

	/* no socket error */

	// setsockopt -> to avoid Address Already in Use
	// to do before bind !
	int option = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));

	sbind(port, sockfd);

	/* no bind error */
	slisten(sockfd, BACKLOG);

	/* no listen error */
	return sockfd;
}

int main(int argc, char **argv)
{
	int sockfd, newsockfd, i;
	StructMessage msg;
	int ret;
	struct pollfd fds[MAX_PLAYERS];
	char winnerName[256];

	ssigaction(SIGALRM, endServerHandler);

	sockfd = initSocketServer(SERVER_PORT);
	printf("Le serveur tourne sur le port : %i \n", SERVER_PORT);

	i = 0;
	int nbPLayers = 0;

	// INSCRIPTION PART
	alarm(TIME_INSCRIPTION);

	while (!end_inscriptions)
	{
		/* client trt */
		newsockfd = accept(sockfd, NULL, NULL); // saccept() exit le programme si accept a été interrompu par l'alarme
		if (newsockfd > 0)						/* no error on accept */
		{

			ret = sread(newsockfd, &msg, sizeof(msg));

			if (msg.code == INSCRIPTION_REQUEST)
			{
				printf("Inscription demandée par le joueur : %s\n", msg.messageText);

				strcpy(tabPlayers[i].pseudo, msg.messageText);
				tabPlayers[i].sockfd = newsockfd;
				i++;

				if (nbPLayers < MAX_PLAYERS)
				{
					msg.code = INSCRIPTION_OK;
					nbPLayers++;
					if (nbPLayers == MAX_PLAYERS)
					{
						alarm(0); // cancel alarm
						end_inscriptions = 1;
					}
				}
				else
				{
					msg.code = INSCRIPTION_KO;
				}
				ret = swrite(newsockfd, &msg, sizeof(msg));
				printf("Nb Inscriptions : %i\n", nbPLayers);
			}
		}
	}

	printf("FIN DES INSCRIPTIONS\n");
	if (nbPLayers != MIN_PLAYERS)
	{
		printf("PARTIE ANNULEE .. PAS ASSEZ DE JOUEURS\n");
		msg.code = CANCEL_GAME;
		for (i = 0; i < nbPLayers; i++)
		{
			swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
		}
		disconnect_players(tabPlayers, nbPLayers);
		sclose(sockfd);
		exit(0);
	}
	else
	{
		printf("PARTIE VA DEMARRER ... \n");
		msg.code = START_GAME;
		for (i = 0; i < nbPLayers; i++)
			swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
	}

  // GESTION TUILE
  Tile gameTiles[MAX_TILES];
  createTiles(gameTiles, MAX_TILES);

  printf("Game tiles:\n");
  for (int i = 0; i < MAX_TILES; i++)
  {
    printf("%d ", gameTiles[i].number);
  }
  printf("\n");


	// GAME PART
	int nbPlayersAlreadyPlayed = 0;

	// init poll
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		fds[i].fd = tabPlayers[i].sockfd;
		fds[i].events = POLLIN;
	}

  int tour;
  while(tour != TOTAL_ROUNDS)
  {
    // Choix d'une tuile au hasard
    srand(time(NULL)); // Initialisation de la graine pour la fonction rand
    int randomIndex = rand() % MAX_TILES; // Choix d'un index aléatoire
    Tile currentTile = gameTiles[randomIndex]; // Sélection de la tuile aléatoire

    // Envoi de la tuile à chaque joueur
    for (int i = 0; i < nbPLayers; i++)
    {
      StructMessage msg;
      msg.tile = currentTile;
      swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
    }

    // Attente des réponses des joueurs et mise à jour du jeu
    // Code à implémenter dans les prochaines étapes

    tour++; // Passage au tour suivant
  }
	// loop game
	/*while ()
	{
		
	}
  */

	printf("GAGNANT : %s\n", winnerName);
	disconnect_players(tabPlayers, nbPLayers);
	sclose(sockfd);
	return 0;
}
