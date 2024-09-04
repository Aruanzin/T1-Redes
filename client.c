#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORTA 2000
#define LEN 1024

int main(){
    int sockfd;
    struct sockaddr_in remoto;
    int len = sizeof(remoto);
    char buffer[LEN];
    char nome[50];

    printf("Digite seu nome: ");
    fgets(nome, sizeof(nome), stdin);
    nome[strcspn(nome, "\n")] = '\0';  // Remove o caractere de nova linha

    printf("sou o cliente %s!\n", nome);

    // Criar o socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
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
    if(connect(sockfd, (struct sockaddr*)&remoto, len) == -1){
        perror("connect ");
        exit(1);
    }

    // Enviar nome para o servidor
    send(sockfd, nome, strlen(nome), 0);

    while(1){
        // Enviar mensagem
        printf("Digite um comando (/msg <destinatario> <mensagem>): ");
        fgets(buffer, LEN, stdin);
        send(sockfd, buffer, strlen(buffer), 0);

        // Receber resposta
        memset(buffer, 0x0, LEN);
        int bytes = recv(sockfd, buffer, LEN, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("Resposta do servidor: %s\n", buffer);
        }
    }

    // Fechar o socket
    close(sockfd);
    return 0;
}
