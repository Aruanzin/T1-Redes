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

struct sockaddr_in remoto;

int main(){
    int sockfd;
    int len = sizeof(remoto);
    int slen;
    char buffer[LEN];

    printf("sou o cliente!\n");

    // Criar o socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        perror("socket ");
        exit(1);
    }else{
        printf("Socket Criado com Sucesso\n");
    }

    // Configurar o endereço remoto
    remoto.sin_family = AF_INET;
    remoto.sin_port = htons(PORTA);
    remoto.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(remoto.sin_zero, 0x0, 8);


    // Aceitar a conexão do sockfd
  
    if(connect(sockfd, (struct sockaddr*)&remoto, len) == -1){
        perror("connect ");
        exit(1);
    }


        
    while(1){

        // Receber mensagem do sockfd
        slen = recv(sockfd, buffer, LEN, 0);
        if(slen > 0){
            buffer[slen-1] = '\0';
            printf("Mensagem recebida: %s\n", buffer);
        }

        memset(buffer, 0x0, LEN);
        fgets(buffer, LEN, stdin);
        send(sockfd, buffer, LEN, 0);
    }

    // Fechar o socket do servidor
    close(sockfd);
    printf("cliente encerrado");
    return 0;
}
