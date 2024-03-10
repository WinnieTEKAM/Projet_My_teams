#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h> // Ajout pour difftime()

#define LOG_FILE "conversations.log"
#define BUFSIZ 1024 // Taille du tampon
#define MAX_CLIENTS 10 // Nombre maximal de clients

// Structure pour stocker les informations sur chaque client
struct Client {
    int socket;
    char username[256];
    char status[256];
};

// Prototypes des fonctions
int create_server_socket(int port);
void accept_new_connection(int listener_socket, fd_set *all_sockets, int *fd_max);
void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket, int port);
void update_client_status(int socket, char *status);

// Variables globales
time_t server_start_time;
int total_clients = 0;
struct Client clients[MAX_CLIENTS]; // Tableau de clients
int max_clients = MAX_CLIENTS;
int client_dnd_mode[MAX_CLIENTS] = {0};

int main(int argc, char **argv)
{
    printf("                       __  ___     ______\n  _______________     /  |/  /_  _/_  __/__  ____ _____ ___  _____     _______________\n /____/____/____/    / /|_/ / / / // / / _ \\/ __ `/ __ `__ \\/ ___/    /____/____/____/\n/____/____/____/    / /  / / /_/ // / /  __/ /_/ / / / / / (__  )    /____/____/____/ \n                   /_/  /_/\\__, //_/  \\___/\\__,_/_/ /_/ /_/____/                      \n                          /____/\n");

    if (argc != 2) {
        fprintf(stderr, "\n[ERROR] | Utilisation : %s <Port utilisé>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    server_start_time = time(NULL);

    // Création de la socket du serveur avec le port spécifié en argument
    int server_socket = create_server_socket(port);

    if (server_socket == -1)
        exit(EXIT_FAILURE);

    // Écoute du port via la socket
    printf("\n[SERVEUR] Écoute sur le port : %d\n", port);
    int status = listen(server_socket, 10);
    if (status != 0)
    {
        printf("[SERVEUR] Erreur d'écoute du port: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Préparation des ensembles de sockets pour select()
    fd_set all_sockets; // Ensemble de toutes les sockets du serveur
    fd_set read_fds;    // Ensemble temporaire pour select()
    FD_ZERO(&all_sockets);
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &all_sockets); // Ajout de la socket principale à l'ensemble
    int fd_max = server_socket;          // Le descripteur le plus grand est forcément celui de notre seule socket

    printf("[SERVEUR] Adresse IP : 127.0.0.1\n");

    while (1)
    { // Boucle principale
        // Copie l'ensemble des sockets puisque select() modifie l'ensemble surveillé
        read_fds = all_sockets;
        // Timeout de 2 secondes pour select()
        struct timeval timer;
        timer.tv_sec = 2;
        timer.tv_usec = 0;
        // Surveille les sockets prêtes à être lues
        status = select(fd_max + 1, &read_fds, NULL, NULL, &timer);
        if (status == -1)
        {
            printf("[SERVEUR] Select error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if (status == 0)
        {
            // Vérifie si aucun client n'est connecté avant d'afficher "Waiting for new clients..."
            if (total_clients == 0 && FD_ISSET(server_socket, &read_fds)) {
                printf("En attente d'un nouveau client...\n");
            }
            continue;
        }
        // Boucle sur nos sockets
        for (int i = 0; i <= fd_max; i++)
        {
            if (FD_ISSET(i, &read_fds) != 1)
            {
                // Le fd i n'est pas une socket à surveiller
                // on s'arrête là et on continue la boucle
                continue;
            }
            printf("[%d] Ready for I/O operation\n", i);
            // La socket est prête à être lue !
            if (i == server_socket)
                // La socket est notre socket serveur qui écoute le port
                accept_new_connection(server_socket, &all_sockets, &fd_max);
            else
                // La socket est une socket client, on va la lire
                read_data_from_socket(i, &all_sockets, fd_max, server_socket, port);
        }
    }
    return (0);
}

// Renvoie la socket du serveur liée à l'adresse et au port qu'on veut écouter
int create_server_socket(int port)
{
    struct sockaddr_in sa;
    int socket_fd;
    int status;

    // Préparation de l'adresse et du port pour la socket de notre serveur
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;                     // IPv4
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1, localhost
    sa.sin_port = htons(port);

    // Création de la socket
    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        printf("[SERVEUR] Erreur de socket: %s\n", strerror(errno));
        return -1;
    }

    printf("Lancement de MyTeams...\n");

    // Liaison de la socket à l'adresse et au port
    status = bind(socket_fd, (struct sockaddr *)&sa, sizeof sa);

    if (status != 0)
    {
        printf("[ERROR] | Liaison impossible ! %s\n", strerror(errno));
        return -1;
    }
    printf("Binding...OK\n");
    return socket_fd;
}

// Accepte une nouvelle connexion et ajoute la nouvelle socket à l'ensemble des sockets
void accept_new_connection(int server_socket, fd_set *all_sockets, int *fd_max) {
    int client_fd;
    struct sockaddr_in sa;
    char msg_to_send[BUFSIZ];
    char clients_list[BUFSIZ] = "Clients déjà connectés : ";
    int status;
    client_fd = accept(server_socket, NULL, NULL);
    if (client_fd == -1) {
        printf("[ERROR] Autorisation impossible : %s\n", strerror(errno));
        return;
    }

    // Ajoute la socket client à l'ensemble
    FD_SET(client_fd, all_sockets); 
    if (client_fd > *fd_max) *fd_max = client_fd; // Met à jour la plus grande socket

    // Parcourir toutes les sockets pour trouver les clients déjà connectés
    for (int i = 0; i <= *fd_max; ++i) {
        if (i != server_socket && FD_ISSET(i, all_sockets) && i != client_fd) { // Exclure la socket du serveur et la nouvelle connexion
            char temp[32];
            snprintf(temp, sizeof(temp), "%d \n", i); // Ajouter le numéro de socket à la liste
            strcat(clients_list, temp);
        }
    }

    printf("[SERVEUR] Nouvelle connexion acceptée sur le socket client %d.\n", client_fd);
    // Envoyer la liste des clients déjà connectés au nouveau client
    send(client_fd, clients_list, strlen(clients_list), 0);

}

// Lit le message d'une socket et relaie le message à toutes les autres
void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket, int port)
{
    char buffer[BUFSIZ];
    char msg_to_send[BUFSIZ];
    int bytes_read;
    int status;

    FILE *log_file = fopen("conversations.log", "a");
    if (log_file == NULL)
    {
        perror("Error opening log file");
        return;
    }

    memset(&buffer, '\0', sizeof buffer);
    bytes_read = recv(socket, buffer, BUFSIZ, 0);
    if (bytes_read <= 0)
    {
        if (bytes_read == 0)
        {
            printf("[%d] Connexion fermée du socket client.\n", socket);
            FD_CLR(socket, all_sockets); // Enlève la socket de l'ensemble
            close(socket);               // Ferme la socket
            printf("[socket %d] removed\n", socket);
            total_clients--; // Décrémente le nombre total de clients
        }
        else
        {
            printf("[Server] Recv error: %s\n", strerror(errno));
            FD_CLR(socket, all_sockets); // Enlève la socket de l'ensemble
            close(socket);               // Ferme la socket
            printf("[socket %d] Déconnecté\n", socket);
            total_clients--; // Décrémente le nombre total de clients
        }
    }
    else
    {
        printf("[ Socket %d] Message reçu : %s \n", socket, buffer);

        // Écrire le message dans le fichier de log avec la date et l'heure actuelles
        time_t raw_time;
        struct tm *info;
        char time_buffer[80];
        time(&raw_time);
        info = localtime(&raw_time);
        strftime(time_buffer, sizeof(time_buffer), "[%Y-%m-%d %H:%M:%S]", info);
        fprintf(log_file, "%s [%d] %s\n", time_buffer, socket, buffer);
        fflush(log_file);

        if (strncmp(buffer, "/info", 5) == 0)
        {
            int uptime_seconds = difftime(time(NULL), server_start_time);
            int hours = uptime_seconds / 3600;
            int minutes = (uptime_seconds % 3600) / 60;
            int seconds = uptime_seconds % 60;
            snprintf(msg_to_send, BUFSIZ, "IP: 127.0.0.1\nPort: %d\nUptime: %02d:%02d:%02d\nClients: %d/%d\n", port, hours, minutes, seconds, total_clients, max_clients);
            status = send(socket, msg_to_send, strlen(msg_to_send), 0);
            if (status == -1)
            {
                printf("[Server] Send error: %s\n", strerror(errno));
            }
        }
        else if (strncmp(buffer, "/pause", 6) == 0)
        {
            if (client_dnd_mode[socket] == 0)
            {
                client_dnd_mode[socket] = 1;
                send(socket, "Mode Ne Pas Déranger activé.\n", 30, 0);
            }
            else
            {
                client_dnd_mode[socket] = 0;
                send(socket, "Mode Ne Pas Déranger désactivé.\n", 31, 0);
            }
        }
        else if (strncmp(buffer, "/status", 7) == 0)
        {
            char *status = buffer + 8; // Récupère le statut à partir de la commande /status
            update_client_status(socket, status); // Met à jour le statut du client
            
            // Ajout de la variable i pour parcourir les clients
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].socket == socket)
                {
                    snprintf(msg_to_send, BUFSIZ, "# %s changed its status:\n%s\n", clients[i].username, status);
                    send(socket, msg_to_send, strlen(msg_to_send), 0);
                    break;
                }
            }
        }
        else
        {
            // Si ce n'est pas une commande, relayer le message à tous les autres clients
            for (int i = 0; i <= fd_max; i++)
            {
                if (FD_ISSET(i, all_sockets) && i != server_socket && i != socket)
                {
                    status = send(i, buffer, bytes_read, 0);
                    if (status == -1)
                    {
                        printf("[Server] Send error to client %d\n", i);
                    }
                }
            }
        }
    }

    // Fermer le fichier de log
    fclose(log_file);
}

// Met à jour le statut du client
void update_client_status(int socket, char *status)
{
    // Parcours du tableau de clients pour mettre à jour le statut du client correspondant
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket == socket)
        {
            strncpy(clients[i].status, status, sizeof(clients[i].status));
            break;
        }
    }
}