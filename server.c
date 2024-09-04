#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//headers para o socket
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORTA 2000
#define LEN 1024

struct sockassr_in local;
struct sockassr_in remoto;

int main(){
    int sockfd;
    int cliente;
    int len = sizeof(remoto);
    int slen;
    char buffer[LEN];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        perror("socket ");
        exit(1);
    }else{
        printf("Socket Criado com Sucesso\n");
    }

    local.sin_family = AF_INET;
    local.sin_port = htons(PORTA);
    //local.sin_addr.s_addr = inet addr("");
    memset(local.sin_zero, 0x0, 8);

   if (bind(sockfd, (struct sockaddr*)&local, sizeof(local)) == -1){
        perror("bind ");
        exit(1);
    }

    listen(sockfd, 1);

    if(cliente = accept(sockfd, (struct sockaddr*)&remoto, &len) == -1){
        perror("accept ");
        exit(1);
    }

    strcpy(buffer, "Welcome!\n\0");

    if(send(cliente, buffer, LEN, 0)){
        printf("Aguardando resposta do cliente...\n");
        while(1){

            if(slen = recv(cliente, buffer, LEN, 0)>0){
                buffer[slen] = '\0';
                printf("mensagem recebida: %s\n", buffer);
                close(cliente);
                break;
            }
        }
    }


    close(sockfd);
    return 0;



}
