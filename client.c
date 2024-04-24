#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "jeu.h"
#include "utils_v1.h"

#define GRID_SIZE 20

// Creation de la grille
typedef struct{
	Tile tiles[GRID_SIZE];
	int count; // Nombre de tuile dans la table
}Grid;

/**
 * PRE: serverIP : a valid IP address
 *      serverPort: a valid port number
 * POST: on success, connects a client socket to serverIP:serverPort ;
 *       on failure, displays error cause and quits the program
 * RES: return socket file descriptor
 */
int initSocketClient(char *serverIP, int serverPort)
{
	int sockfd = ssocket();
	sconnect(serverIP, serverPort, sockfd);
	return sockfd;
}

void initGrid(Grid *grid) {
	for (int i = 0; i < GRID_SIZE; i++) {
        grid->tiles[i].number = 0; // Initialise chaque emplacement avec une tuile vide
    }
}

void placeTile(Grid *grid, Tile tile, int position) {
    // Vérifie si la position est valide et si la grille n'est pas pleine
    if (position >= 0 && position < GRID_SIZE && grid->count < GRID_SIZE) {
        // Vérifie si la position est déjà occupée
        while (grid->tiles[position].number != 0) {
            position++; // Essaie la position suivante
            if (position >= GRID_SIZE) {
                position = 0; // Si on dépasse la taille de la grille, revenir au début
            }
        }
        // Place la tuile à la position libre
        grid->tiles[position] = tile;
        grid->count++; // Incrémente le nombre de tuiles dans la grille
    } else {
        printf("Position invalide ou grille pleine.\n");
    }
}

// Affiche la grille avec les tuiles
void printGrid(Grid *grid) {
    printf("Grille :\n");
    for (int i = 0; i < GRID_SIZE; i++) {
        if (grid->tiles[i].number != 0) {
            printf("[%d] %d\n", i, grid->tiles[i].number);
        } else {
            printf("[%d] 0\n", i);
        }
    }
}

void displayTile(Tile tile)
{
  printf("Tuile tirée : %d\n", tile.number);
}

void sendPlayerTurn(int sockfd){
	StructMessage msg;
    msg.code = TILE_DRAW;
    swrite(sockfd, &msg, sizeof(msg));
	printf("Veuillez attendre quelques instants...\n");
}

int main(int argc, char **argv)
{

	char pseudo[MAX_PSEUDO];
	int sockfd;
	int ret;

	StructMessage msg;
	char c;

	/* retrieve player name */
	printf("Bienvenue dans le programe d'inscription au serveur de jeu\n");
	printf("Pour participer entrez votre nom :\n");
	ret = sread(0, pseudo, MAX_PSEUDO);
	checkNeg(ret, "read client error");
	pseudo[ret - 1] = '\0';
	strcpy(msg.messageText, pseudo);
	msg.code = INSCRIPTION_REQUEST;

	sockfd = initSocketClient(SERVER_IP, SERVER_PORT);

	swrite(sockfd, &msg, sizeof(msg));

	/* wait server response */
	sread(sockfd, &msg, sizeof(msg));

	switch (msg.code)
	{
	case INSCRIPTION_OK:
		printf("Réponse du serveur : Inscription acceptée\n");
		break;
	case INSCRIPTION_KO:
		printf("Réponse du serveur : Inscription refusée\n");
		sclose(sockfd);
		exit(0);
	default:
		printf("Réponse du serveur non prévue %d\n", msg.code);
		break;
	}

	/* wait start of game or cancel */
	sread(sockfd, &msg, sizeof(msg));

	if (msg.code == START_GAME)
	{
		printf("DEBUT JEU\n");

		// Initialisation de la grille
		Grid grid;
		initGrid(&grid);

		while(1)
		{
			sread(sockfd, &msg, sizeof(msg));
			if (msg.code == END_GAME){
				printf("Partie terminée\n");
				break;
			}

			displayTile(msg.tile);
			printGrid(&grid);
			printf("Placer une tuile dans la grille\n");

			int position;
			scanf("%d",&position);
			sendPlayerTurn(sockfd);

			placeTile(&grid, msg.tile, position);
		}
		printGrid(&grid);
	}
	else
	{
		printf("PARTIE ANNULEE\n");
	}

	sclose(sockfd);
	return 0;
}
