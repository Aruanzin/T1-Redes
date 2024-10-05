#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORTA 2000
#define LEN 1024

int sockfd;

// Função para receber mensagens do servidor
void* receber_mensagens(void* arg) {
    char buffer[LEN];
    int bytes;
    
    while(1) {
        memset(buffer, 0x0, LEN);
        bytes = recv(sockfd, buffer, LEN, 0);  // Recebe mensagens do servidor
        if (bytes > 0) {
            buffer[bytes] = '\0';  // Garante que a string seja terminada
            printf("\nMensagem recebida: %s\n", buffer);  // Exibe a mensagem recebida
        } else if (bytes == 0) {
            printf("Conexão com o servidor encerrada.\n");
            close(sockfd);
            exit(1);
        } else {
            perror("recv ");
            exit(1);
        }
    }
}

int main() {
    struct sockaddr_in remoto;
    int len = sizeof(remoto);
    char buffer[LEN];
    char nome[50];
    pthread_t thread_id;

    printf("Digite seu nome: ");
    fgets(nome, sizeof(nome), stdin);
    nome[strcspn(nome, "\n")] = '\0';  // Remove o caractere de nova linha

    printf("sou o cliente %s!\n", nome);

    // Criar o socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket ");
        exit(1);
    }
    printf("Socket Criado com Sucesso\n");

    // Configurar o endereço remoto
    remoto.sin_family = AF_INET;
    remoto.sin_port = htons(PORTA);
    remoto.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(remoto.sin_zero, 0x0, 8);

    // Conectar ao servidor
    if (connect(sockfd, (struct sockaddr*)&remoto, len) == -1) {
        perror("connect ");
        exit(1);
    }

    // Enviar nome para o servidor
    send(sockfd, nome, strlen(nome), 0);

    // Criar uma thread para receber mensagens
    if (pthread_create(&thread_id, NULL, receber_mensagens, NULL) != 0) {
        perror("pthread_create ");
        exit(1);
    }

    // Loop para enviar mensagens
    while (1) {
        printf("Digite um comando (/help para ver os comandos disponíveis): ");
        fgets(buffer, LEN, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove o caractere de nova linha

        if (strcmp(buffer, "/help") == 0) {
            printf("Comandos disponíveis:\n");
            printf("/help - Exibe esta mensagem de ajuda\n");
            printf("/online - Lista os clientes online\n");
            printf("/msg <destinatario> <mensagem> - Envia uma mensagem privada\n");
            printf("/create <nome_sala> - Cria uma nova sala de chat\n");
            printf("/join <nome_sala> - Entra em uma sala de chat\n");
            printf("/list - Lista as salas de chat disponíveis\n");
            printf("/leave <nome_sala> - Sai de uma sala de chat\n");
            printf("/sala <nome_sala> <mensagem> - Envia uma mensagem para uma sala de chat\n");
        } else {
            send(sockfd, buffer, strlen(buffer), 0);
        }
    }

    // Fechar o socket (nunca será atingido devido ao loop infinito)
    close(sockfd);
    return 0;
}