#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> // inet_addr
#include <stdlib.h>

#define BUFFER_SIZE 1024
#define MAX_MESSAGE_SIZE 1024

void receive_messages(int sockfd);

int main(int argc, char **argv) {

    printf("                  __  ___    ______\n  ____________   /  |/  /_ _/_  __/__ ___ ___ _  ___   ____________\n /___/___/___/  / /|_/ / // // / / -_) _ `/  ' \\(_-<  /___/___/___/\n/___/___/___/  /_/  /_/\\_, //_/  \\__/\_,_/_/_/_/___/  /___/___/___/ \n                      /___/\n"); 
     
    // On vérifie si les arguments sont bien passés
    if (argc != 4) {
        printf("Utilisation : %s <Adresse IP du serveur> <Port du serveur> <Nom d'utilisateur>\n", argv[0]);
        return 1;
    }

    int sock;
    struct sockaddr_in server;
    char *username = argv[3];
    char message[BUFFER_SIZE];
    ssize_t envoye;
    int port = atoi(argv[2]);

    // Création du socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("\n[ERROR] | Création de la socket impossible !");
        return 1;
    }

    // Initialisation des membres de l'objet serveur
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(argv[1]);

    // Connexion au serveur
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("\n[ERROR] | Connexion au serveur impossible !");
        return 1;
    }
    
    printf("\nConnexion au serveur en cours...\n");
    sleep(1);
    printf("Connexion au serveur réussie en tant que : %s\n", username);
    printf("\nBienvenue dans MyTeams !\n");
    
    

    printf("Envoyer un nouveau message : ");
    scanf("%s", message);

    envoye = send(sock, message, strlen(message), 0);
    message[envoye] = '\0';

    if (envoye < 0) {
        perror("[ERROR] | Échec de l'envoi\n");
        return 1;
    }

    // Reception et affichage des messages
    receive_messages(sock);

    close(sock);
    return 0;
}

void receive_messages(int sockfd) {
    char message[MAX_MESSAGE_SIZE];
    int bytes_received;
    ssize_t envoye;

    while (1) {
        bytes_received = recv(sockfd, message, sizeof(message), 0);
        if (bytes_received <= 0) {
            printf("Déconnexion du serveur.\n");
            exit(EXIT_FAILURE);
        }
        message[bytes_received] = '\0';
        printf("%s\n", message);

        printf("Envoyer un nouveau message : ");
        scanf("%s", message);

        envoye = send(sockfd, message, strlen(message), 0);
        message[envoye] = '\0';

        if (envoye < 0) {
            perror("[ERROR] | Échec de l'envoi\n");
            exit(EXIT_FAILURE);
        }
    }
}