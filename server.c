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
#define MAX_CLIENTS 100

typedef struct {
    int socket;
    char name[50];
} Cliente;

Cliente clientes[MAX_CLIENTS];
int num_clientes = 0;

void enviar_para_cliente(int destinatario_socket, const char* mensagem) {
    send(destinatario_socket, mensagem, strlen(mensagem), 0);
}

int encontrar_cliente_por_nome(const char* nome) {
    for (int i = 0; i < num_clientes; i++) {
        if (strcmp(clientes[i].name, nome) == 0) {
            return clientes[i].socket;
        }
    }
    return -1;  // Cliente não encontrado
}

int main(){
    int sockfd, cliente_socket;
    struct sockaddr_in local, remoto;
    int len = sizeof(remoto);
    char buffer[LEN];
    
    // Criar o socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        perror("socket ");
        exit(1);
    }
    printf("Socket Criado com Sucesso\n");

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
    listen(sockfd, MAX_CLIENTS);

    while (1) {
        cliente_socket = accept(sockfd, (struct sockaddr*)&remoto, &len);
        if(cliente_socket == -1){
            perror("accept ");
            continue;
        }

        // Receber o nome do cliente
        memset(buffer, 0x0, LEN);
        recv(cliente_socket, buffer, LEN, 0);
        printf("Cliente conectado: %s\n", buffer);

        // Adicionar o cliente à lista
        if (num_clientes < MAX_CLIENTS) {
            clientes[num_clientes].socket = cliente_socket;
            strcpy(clientes[num_clientes].name, buffer);
            num_clientes++;
        } else {
            printf("Número máximo de clientes alcançado.\n");
            close(cliente_socket);
        }

        // Lidar com mensagens dos clientes
        while (1) {
            memset(buffer, 0x0, LEN);
            int bytes = recv(cliente_socket, buffer, LEN, 0);
            if (bytes <= 0) {
                printf("Cliente desconectado.\n");
                close(cliente_socket);
                break;
            }

            buffer[bytes] = '\0';
            char comando[LEN];
            sscanf(buffer, "%s", comando);

            if (strcmp(comando, "/msg") == 0) {
                char destinatario[50];
                char mensagem[LEN];
                sscanf(buffer, "%s %s %[^\n]", comando, destinatario, mensagem);

                int destinatario_socket = encontrar_cliente_por_nome(destinatario);
                if (destinatario_socket != -1) {
                    enviar_para_cliente(destinatario_socket, mensagem);
                } else {
                    printf("Cliente %s não encontrado.\n", destinatario);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
