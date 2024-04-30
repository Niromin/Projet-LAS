#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "jeu.h"
#include "utils_v1.h"

#define MAX_PLAYERS 5
#define MIN_PLAYERS 2
#define MAX_TILES 40
#define BACKLOG 5
#define TIME_INSCRIPTION 10
#define TOTAL_ROUNDS 21
#define BUFFER_SIZE 1024

typedef struct Player
{
	char pseudo[MAX_PSEUDO];
	int sockfd;
	int shot;
	int score;
} Player;

/*** globals variables ***/
Player tabPlayers[MAX_PLAYERS];
volatile sig_atomic_t end_inscriptions = 0;
volatile sig_atomic_t game_started = 0;
volatile sig_atomic_t inscription_started = 0;
int exitMessage = 0;

void endServerHandler(int sig)
{
	inscription_started = 0;
	end_inscriptions = 1;
}

void interruptHandler(int sig)
{
	exitMessage = 1;
    if (!game_started && !inscription_started) {
        printf("Arrêt du serveur.\n");
        exit(0);
    }
    // Ne rien faire si le jeu est en cours
}

void disconnect_players(Player *tabPlayers, int nbPlayers)
{
	for (int i = 0; i < nbPlayers; i++)
	{
		sclose(tabPlayers[i].sockfd);
	}
	printf("PLayers disconnected\n");
	return;
}

void createTiles(int *tiles, int numTiles)
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
        tiles[i] = tileNumbers[i];
    }
}
// on doit pas pouvoir sigint pendant la phase insc et game seulement en IDLE et du cp voila jsp
void sortPlayersByScore(Player *players, int numPlayers)
{
    int j;
    Player temp;

    for (j = 0; j < numPlayers - 1; j++)
	{
		if (players[j].score < players[j + 1].score)
		{
			temp = players[j];
			players[j] = players[j + 1];
			players[j + 1] = temp;
		}
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
	char line[256];
	int tableauLine[1024];
	int indiceTabLine = 0;

	if (argc >= 2){
		FILE *fd2 = fopen(argv[1], "r");
		checkNull(fd2,"SIUUUUU\n");
		i = 0;
		while(fgets(line, sizeof(line), fd2) != NULL ){
			tableauLine[i] = atoi(line);
			i++;
		}
	}


	ssigaction(SIGALRM, endServerHandler); // Arret de la phase d'inscription apres fin alarm
	ssigaction(SIGINT, interruptHandler); // Arret du server avec SIGINT seulement si game_started is false

	sockfd = initSocketServer(SERVER_PORT);
	printf("Le serveur tourne sur le port : %i \n", SERVER_PORT);

	i = 0;
	int nbPLayers = 0;

/***************************INSCRIPTION PART****************************************/
	while(1){

		i = 0;

		while (!end_inscriptions)
		{
			/* client trt */
			newsockfd = accept(sockfd, NULL, NULL); // saccept() exit le programme si accept a été interrompu par l'alarme
			if (newsockfd > 0)						/* no error on accept */
			{
				inscription_started = 1;
				alarm(TIME_INSCRIPTION);
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
							inscription_started = 0;
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
		if (nbPLayers < MIN_PLAYERS)
		{
			if (exitMessage == 1){
				printf("Fermeture du serveur \n");
				sclose(sockfd);
				exit(0);
			}
			printf("PARTIE ANNULEE .. PAS ASSEZ DE JOUEURS\n");
			msg.code = CANCEL_GAME;
			for (i = 0; i < nbPLayers; i++)
			{
				swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
			}
			disconnect_players(tabPlayers, nbPLayers);
			i--;
			nbPLayers = 0;
			end_inscriptions = 0;
			continue;
		}
		else
		{
			game_started = 1;
			printf("PARTIE VA DEMARRER ... \n");
			msg.code = START_GAME;
			for (i = 0; i < nbPLayers; i++){
				swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
			}
		}

	/***************************GAME PART****************************************/

		// GESTION TUILE
		int gameTiles[MAX_TILES];
		createTiles(gameTiles, MAX_TILES);
		int lenghtGameTile = MAX_TILES; // Taille logique de gameTile
		int currentTile;

		printf("Game tiles:\n");
		for (int i = 0; i < MAX_TILES; i++)
		{
			printf("%d ", gameTiles[i]);
		}
		printf("\n");
		int tour = 1;

		if (argc >= 2){

			while(tour != TOTAL_ROUNDS){

				// GAME PART
				currentTile = tableauLine[indiceTabLine];
				indiceTabLine++;
				// Envoi de la tuile à chaque joueur
				for (int i = 0; i < nbPLayers; i++)
				{
					StructMessage msg;
					msg.tile = currentTile;
					swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
				}

				int nbPlayersAlreadyPlayed = 0;

				// init poll
				for (i = 0; i < MAX_PLAYERS; i++)
				{
					fds[i].fd = tabPlayers[i].sockfd;
					fds[i].events = POLLIN;
				}

				while (nbPlayersAlreadyPlayed < nbPLayers)
				{
					
					// poll during 1 second
					ret = poll(fds, MAX_PLAYERS, 1000);
					if (ret == -1) {
						if (errno == EINTR) {
							// Si le poll est interrompu par un signal, continuez la boucle
							continue;
						} else {
							perror("server poll error");
							exit(EXIT_FAILURE);
						}
					}

					if (ret == 0)
						continue;

					// check player something to read
					for (i = 0; i < MAX_PLAYERS; i++)
					{
						if (fds[i].revents & POLLIN)
						{
							ret = sread(tabPlayers[i].sockfd, &msg, sizeof(msg));
							
							if (ret != 0 && msg.code == TILE_DRAW)
							{
								tabPlayers[i].shot = msg.code;
								printf("%s a joué \n", tabPlayers[i].pseudo);
								nbPlayersAlreadyPlayed++;
							}
							// printf("Test 1\n");
						}
						// printf("Test 2\n");
					}
					// printf("Test 3\n");
				}
				// printf("Test 4\n");
				

				tour++; //
				
			}
		}
		else{
			while(tour != TOTAL_ROUNDS)
			{
				printf("Tour n° %d\n",tour);
				// Choix d'une tuile au hasard
				srand(time(NULL)); 
				int randomIndex = rand() % lenghtGameTile; // Choix d'un index aléatoire
				int currentTile = gameTiles[randomIndex]; // Sélection de la tuile aléatoire

				// Supprimer la tuile choisie du tableau gameTiles
				for (int j = randomIndex; j < lenghtGameTile - 1; j++) {
					gameTiles[j] = gameTiles[j + 1];
				}
				gameTiles[lenghtGameTile - 1] = 0; // Remplace la dernière tuile par une tuile vide
				lenghtGameTile--;			

				// Envoi de la tuile à chaque joueur
				for (int i = 0; i < nbPLayers; i++)
				{
					StructMessage msg;
					msg.tile = currentTile;
					swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
				}

				// GAME PART
				int nbPlayersAlreadyPlayed = 0;

				// init poll
				for (i = 0; i < MAX_PLAYERS; i++)
				{
					fds[i].fd = tabPlayers[i].sockfd;
					fds[i].events = POLLIN;
				}

				while (nbPlayersAlreadyPlayed < nbPLayers)
				{
					
					// poll during 1 second
					ret = poll(fds, MAX_PLAYERS, 1000);
					if (ret == -1) {
						if (errno == EINTR) {
							// Si le poll est interrompu par un signal, continuez la boucle
							continue;
						} else {
							perror("server poll error");
							exit(EXIT_FAILURE);
						}
					}

					if (ret == 0)
						continue;

					// check player something to read
					for (i = 0; i < MAX_PLAYERS; i++)
					{
						if (fds[i].revents & POLLIN)
						{
							ret = sread(tabPlayers[i].sockfd, &msg, sizeof(msg));
							
							if (ret != 0 && msg.code == TILE_DRAW)
							{
								tabPlayers[i].shot = msg.code;
								printf("%s a joué \n", tabPlayers[i].pseudo);
								nbPlayersAlreadyPlayed++;
							}
							// printf("Test 1\n");
						}
						// printf("Test 2\n");
					}
					// printf("Test 3\n");
				}
				// printf("Test 4\n");
				

				tour++; // Passage au tour suivant
			}
		}
		

		StructMessage endGameMsg;
		endGameMsg.code = END_GAME;
		for (int i = 0; i < nbPLayers; i++)
		{	
			swrite(tabPlayers[i].sockfd, &endGameMsg, sizeof(endGameMsg));
		}

	/***************************RANKING PART****************************************/
		// Recupérer les score aux clients
		for (int i = 0; i < nbPLayers; i++)
		{
			sread(tabPlayers[i].sockfd, &msg, sizeof(msg));
			if(msg.code == END_GAME_SCORE)
			{
				tabPlayers[i].score = msg.tile;
				printf("Joueur %s a %d de points\n", tabPlayers[i].pseudo, tabPlayers[i].score);
			}
		}

		sortPlayersByScore(tabPlayers, nbPLayers);
		printf("Joueurs triés par score :\n");
		for (int i = 0; i < nbPLayers; i++)
		{
			printf("%d# %s : Score = %d\n", i + 1, tabPlayers[i].pseudo, tabPlayers[i].score);
		}

		printf("GAGNANT : %s\n", tabPlayers[0].pseudo);
		disconnect_players(tabPlayers, nbPLayers);
		endGameMsg.code = 0;
		game_started = 0;
		end_inscriptions = 0;
		nbPLayers = 0;
		continue;
	}
}
