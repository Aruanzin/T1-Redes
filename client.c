// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORTA 2000
#define LEN 1024

int sockfd;

// Função para receber mensagens do servidor
void* receber_mensagens(void* arg) {
    char buffer[LEN];
    int bytes;

    while (1) {
        memset(buffer, 0x0, LEN);
        bytes = recv(sockfd, buffer, LEN, 0);  // Recebe mensagens do servidor
        if (bytes > 0) {
            buffer[bytes] = '\0';  // Garante que a string seja terminada
            printf("\n%s", buffer); // Exibe a mensagem recebida
            printf("Digite um comando: "); // Prompt para o usuário continuar
            fflush(stdout);
        } else if (bytes == 0) {
            printf("Conexão com o servidor encerrada.\n");
            close(sockfd);
            exit(1);
        } else {
            perror("recv ");
            close(sockfd);
            exit(1);
        }
    }
}

int main() {
    struct sockaddr_in remoto;
    socklen_t len = sizeof(remoto);
    char buffer[LEN];
    char nome[50];
    pthread_t thread_id;

    printf("Digite seu nome: ");
    fgets(nome, sizeof(nome), stdin);
    nome[strcspn(nome, "\n")] = '\0';  // Remove o caractere de nova linha

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
    remoto.sin_addr.s_addr = inet_addr("127.0.0.1"); // Altere para o IP do servidor, se necessário
    memset(remoto.sin_zero, 0x0, 8);

    // Conectar ao servidor
    if (connect(sockfd, (struct sockaddr*)&remoto, len) == -1) {
        perror("connect ");
        close(sockfd);
        exit(1);
    }
    printf("Conectado ao servidor.\n");

    // Enviar nome para o servidor
    send(sockfd, nome, strlen(nome), 0);

    // Criar uma thread para receber mensagens
    if (pthread_create(&thread_id, NULL, receber_mensagens, NULL) != 0) {
        perror("pthread_create ");
        close(sockfd);
        exit(1);
    }

    // Desanexar a thread para não precisar dar join
    pthread_detach(thread_id);

    // Loop para enviar mensagens
    while (1) {
        printf("Digite um comando: ");
        fgets(buffer, LEN, stdin);
        // Remove o caractere de nova linha
        buffer[strcspn(buffer, "\n")] = '\0';
        // Envia o comando para o servidor
        if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
            perror("send");
            break;
        }
    }

    // Fechar o socket (nunca será atingido devido ao loop infinito)
    close(sockfd);
    return 0;
}
