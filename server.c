#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>        // Inclui a função close
#include <sys/socket.h>    // Headers para o socket
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>     // Inclui a função inet_addr

#define PORTA 2000
#define LEN 1024

struct sockaddr_in local;
struct sockaddr_in remoto;

int main(){
    int sockfd;
    int cliente;
    int len = sizeof(remoto);
    int slen;
    char buffer[LEN];

    // Criar o socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        perror("socket ");
        exit(1);
    }else{
        printf("Socket Criado com Sucesso\n");
    }

    // Configurar o endereço local
    local.sin_family = AF_INET;
    local.sin_port = htons(PORTA);
    local.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(local.sin_zero, 0x0, 8);

    // Vincular o socket ao endereço local
    if (bind(sockfd, (struct sockaddr*)&local, sizeof(local)) == -1){
        perror("bind ");
        exit(1);
    }

    // Colocar o socket em modo de escuta
    listen(sockfd, 1);

    // Aceitar a conexão do cliente
    cliente = accept(sockfd, (struct sockaddr*)&remoto, &len);
    if(cliente == -1){
        perror("accept ");
        exit(1);
    }

    // Enviar mensagem de boas-vindas ao cliente
    strcpy(buffer, "Welcome!\n\0");
    if(send(cliente, buffer, LEN, 0)){
        printf("Aguardando resposta do cliente...\n");
        while(1){

            memset(buffer, 0, LEN);
            // Receber mensagem do cliente
            slen = recv(cliente, buffer, LEN, 0);
            if(slen > 0){
                buffer[slen] = '\0';
                printf("Mensagem recebida: %s\n", buffer);
                close(cliente);
                break;
            }
        }
    }

    // Fechar o socket do servidor
    close(sockfd);
    return 0;
}
