// server.c

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
#define MAX_CLIENTS 100
#define MAX_SALAS 50
#define MAX_CLIENTS_POR_SALA 100

typedef struct {
    int socket;
    char name[50];
    char sala_atual[50]; // Nome da sala atual
} Cliente;

typedef struct {
    char nome[50];
    Cliente* clientes[MAX_CLIENTS_POR_SALA];
    int num_clientes;
} Sala;

// Arrays para armazenar clientes e salas
Cliente* clientes[MAX_CLIENTS];
int num_clientes = 0;

Sala salas[MAX_SALAS];
int num_salas = 0;

// Mutexes para sincronização
pthread_mutex_t mutex_clientes = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_salas = PTHREAD_MUTEX_INITIALIZER;

// Função para enviar mensagens para um cliente específico
void enviar_mensagem(int socket, const char* mensagem) {
    if (send(socket, mensagem, strlen(mensagem), 0) < 0) {
        perror("send");
    }
}

// Função para enviar mensagens para todos os clientes em uma sala
void enviar_mensagem_sala(const char* sala_nome, const char* mensagem, Cliente* remetente) {
    pthread_mutex_lock(&mutex_salas);
    for (int i = 0; i < num_salas; i++) {
        if (strcmp(salas[i].nome, sala_nome) == 0) {
            for (int j = 0; j < salas[i].num_clientes; j++) {
                // Opcional: Não enviar a mensagem para o remetente
                if (salas[i].clientes[j]->socket != remetente->socket) {
                    enviar_mensagem(salas[i].clientes[j]->socket, mensagem);
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(&mutex_salas);
}

// Função para remover um cliente de uma sala
void remover_cliente_sala(Cliente* cliente) {
    pthread_mutex_lock(&mutex_salas);
    if (strlen(cliente->sala_atual) > 0) {
        for (int i = 0; i < num_salas; i++) {
            if (strcmp(salas[i].nome, cliente->sala_atual) == 0) {
                // Encontrar o cliente na sala
                for (int j = 0; j < salas[i].num_clientes; j++) {
                    if (salas[i].clientes[j]->socket == cliente->socket) {
                        // Shift para remover o cliente
                        for (int k = j; k < salas[i].num_clientes - 1; k++) {
                            salas[i].clientes[k] = salas[i].clientes[k + 1];
                        }
                        salas[i].num_clientes--;
                        break;
                    }
                }
                // Notificar a sala sobre a saída
                char msg[LEN];
                snprintf(msg, LEN, "%s saiu da sala.\n", cliente->name);
                enviar_mensagem_sala(salas[i].nome, msg, cliente);
                // Limpar a sala atual do cliente
                strcpy(cliente->sala_atual, "");
                break;
            }
        }
    }
    pthread_mutex_unlock(&mutex_salas);
}

// Função para gerenciar a comunicação com um cliente
void* gerenciar_cliente(void* arg) {
    Cliente* cliente = (Cliente*)arg;
    char buffer[LEN];
    int bytes;

    // Receber o nome do cliente
    memset(buffer, 0x0, LEN);
    bytes = recv(cliente->socket, buffer, LEN, 0);
    if (bytes <= 0) {
        perror("recv");
        close(cliente->socket);
        free(cliente);
        pthread_exit(NULL);
    }
    buffer[bytes] = '\0';
    strcpy(cliente->name, buffer);
    printf("Cliente conectado: %s\n", cliente->name);

    // Adicionar o cliente à lista global
    pthread_mutex_lock(&mutex_clientes);
    if (num_clientes < MAX_CLIENTS) {
        clientes[num_clientes++] = cliente;
    } else {
        printf("Número máximo de clientes alcançado.\n");
        enviar_mensagem(cliente->socket, "Servidor cheio. Tente novamente mais tarde.\n");
        close(cliente->socket);
        free(cliente);
        pthread_mutex_unlock(&mutex_clientes);
        pthread_exit(NULL);
    }
    pthread_mutex_unlock(&mutex_clientes);

    // Notificar todos sobre a nova conexão
    char msg_conexao[LEN];
    snprintf(msg_conexao, LEN, "%s entrou no chat.\n", cliente->name);
    pthread_mutex_lock(&mutex_clientes);
    for (int i = 0; i < num_clientes; i++) {
        if (clientes[i]->socket != cliente->socket) {
            enviar_mensagem(clientes[i]->socket, msg_conexao);
        }
    }
    pthread_mutex_unlock(&mutex_clientes);

    while (1) {
        memset(buffer, 0x0, LEN);
        bytes = recv(cliente->socket, buffer, LEN, 0);
        if (bytes <= 0) {
            printf("Cliente desconectado: %s\n", cliente->name);
            // Remover cliente da lista global
            pthread_mutex_lock(&mutex_clientes);
            int pos = -1;
            for (int i = 0; i < num_clientes; i++) {
                if (clientes[i]->socket == cliente->socket) {
                    pos = i;
                    break;
                }
            }
            if (pos != -1) {
                // Remover da lista de clientes
                for (int i = pos; i < num_clientes - 1; i++) {
                    clientes[i] = clientes[i + 1];
                }
                num_clientes--;
            }
            pthread_mutex_unlock(&mutex_clientes);

            // Remover da sala atual, se estiver em alguma
            remover_cliente_sala(cliente);

            // Notificar todos sobre a desconexão
            snprintf(msg_conexao, LEN, "%s saiu do chat.\n", cliente->name);
            pthread_mutex_lock(&mutex_clientes);
            for (int i = 0; i < num_clientes; i++) {
                enviar_mensagem(clientes[i]->socket, msg_conexao);
            }
            pthread_mutex_unlock(&mutex_clientes);

            close(cliente->socket);
            free(cliente);
            pthread_exit(NULL);
        }

        buffer[bytes] = '\0';
        printf("Mensagem recebida de %s: %s\n", cliente->name, buffer);

        // Processar comandos
        char comando[LEN];
        sscanf(buffer, "%s", comando);

        if (strcmp(comando, "/create") == 0) {
            char nome_sala[50];
            sscanf(buffer, "%s %s", comando, nome_sala);
            if (strlen(nome_sala) == 0) {
                enviar_mensagem(cliente->socket, "Uso correto: /create <nome_da_sala>\n");
                continue;
            }

            pthread_mutex_lock(&mutex_salas);
            // Verificar se a sala já existe
            int existe = 0;
            for (int i = 0; i < num_salas; i++) {
                if (strcmp(salas[i].nome, nome_sala) == 0) {
                    existe = 1;
                    break;
                }
            }
            if (existe) {
                enviar_mensagem(cliente->socket, "Sala já existe.\n");
            } else if (num_salas < MAX_SALAS) {
                strcpy(salas[num_salas].nome, nome_sala);
                salas[num_salas].num_clientes = 0;
                num_salas++;
                enviar_mensagem(cliente->socket, "Sala criada com sucesso.\n");
            } else {
                enviar_mensagem(cliente->socket, "Número máximo de salas alcançado.\n");
            }
            pthread_mutex_unlock(&mutex_salas);
        }
        else if (strcmp(comando, "/join") == 0) {
            char nome_sala[50];
            sscanf(buffer, "%s %s", comando, nome_sala);
            if (strlen(nome_sala) == 0) {
                enviar_mensagem(cliente->socket, "Uso correto: /join <nome_da_sala>\n");
                continue;
            }

            pthread_mutex_lock(&mutex_salas);
            int sala_encontrada = -1;
            for (int i = 0; i < num_salas; i++) {
                if (strcmp(salas[i].nome, nome_sala) == 0) {
                    sala_encontrada = i;
                    break;
                }
            }

            if (sala_encontrada != -1) {
                // Verificar se já está em uma sala
                if (strlen(cliente->sala_atual) > 0) {
                    pthread_mutex_unlock(&mutex_salas);
                    enviar_mensagem(cliente->socket, "Você já está em uma sala. Use /leave para sair.\n");
                    continue;
                }

                if (salas[sala_encontrada].num_clientes < MAX_CLIENTS_POR_SALA) {
                    salas[sala_encontrada].clientes[salas[sala_encontrada].num_clientes++] = cliente;
                    strcpy(cliente->sala_atual, nome_sala);
                    enviar_mensagem(cliente->socket, "Entrou na sala com sucesso.\n");

                    // Notificar os membros da sala
                    char msg_sala[LEN];
                    snprintf(msg_sala, LEN, "%s entrou na sala.\n", cliente->name);
                    enviar_mensagem_sala(nome_sala, msg_sala, cliente);
                } else {
                    enviar_mensagem(cliente->socket, "Sala cheia.\n");
                }
            } else {
                enviar_mensagem(cliente->socket, "Sala não encontrada.\n");
            }
            pthread_mutex_unlock(&mutex_salas);
        }
        else if (strcmp(comando, "/leave") == 0) {
            if (strlen(cliente->sala_atual) == 0) {
                enviar_mensagem(cliente->socket, "Você não está em nenhuma sala.\n");
                continue;
            }
            remover_cliente_sala(cliente);
            enviar_mensagem(cliente->socket, "Saiu da sala com sucesso.\n");
        }
        else if (strcmp(comando, "/list") == 0) {
            pthread_mutex_lock(&mutex_salas);
            if (num_salas == 0) {
                enviar_mensagem(cliente->socket, "Nenhuma sala disponível.\n");
            } else {
                char lista_salas[LEN] = "Salas disponíveis: ";
                for (int i = 0; i < num_salas; i++) {
                    strcat(lista_salas, salas[i].nome);
                    if (i < num_salas - 1) {
                        strcat(lista_salas, ", ");
                    }
                }
                strcat(lista_salas, "\n");
                enviar_mensagem(cliente->socket, lista_salas);
            }
            pthread_mutex_unlock(&mutex_salas);
        }
        else if (strcmp(comando, "/sala") == 0) {
            char nome_sala[50];
            char mensagem[LEN];
            // Extrair nome da sala e a mensagem
            sscanf(buffer, "%s %s %[^\n]", comando, nome_sala, mensagem);
            if (strlen(nome_sala) == 0 || strlen(mensagem) == 0) {
                enviar_mensagem(cliente->socket, "Uso correto: /sala <nome_da_sala> <mensagem>\n");
                continue;
            }

            pthread_mutex_lock(&mutex_salas);
            int sala_encontrada = -1;
            for (int i = 0; i < num_salas; i++) {
                if (strcmp(salas[i].nome, nome_sala) == 0) {
                    sala_encontrada = i;
                    break;
                }
            }

            if (sala_encontrada != -1) {
                // Verificar se o cliente está na sala
                int na_sala = 0;
                for (int j = 0; j < salas[sala_encontrada].num_clientes; j++) {
                    if (salas[sala_encontrada].clientes[j]->socket == cliente->socket) {
                        na_sala = 1;
                        break;
                    }
                }

                if (na_sala) {
                    char mensagem_completa[LEN];
                    snprintf(mensagem_completa, LEN, "[%s][%s]: %s\n", nome_sala, cliente->name, mensagem);
                    enviar_mensagem_sala(nome_sala, mensagem_completa, cliente);
                } else {
                    enviar_mensagem(cliente->socket, "Você não está nessa sala.\n");
                }
            } else {
                enviar_mensagem(cliente->socket, "Sala não encontrada.\n");
            }
            pthread_mutex_unlock(&mutex_salas);
        }
        else if (strcmp(comando, "/online") == 0) {
            pthread_mutex_lock(&mutex_clientes);
            if (num_clientes == 0) {
                enviar_mensagem(cliente->socket, "Nenhum cliente online.\n");
            } else {
                char lista_online[LEN] = "Clientes online: ";
                for (int i = 0; i < num_clientes; i++) {
                    strcat(lista_online, clientes[i]->name);
                    if (i < num_clientes - 1) {
                        strcat(lista_online, ", ");
                    }
                }
                strcat(lista_online, "\n");
                enviar_mensagem(cliente->socket, lista_online);
            }
            pthread_mutex_unlock(&mutex_clientes);
        }
        else if (strcmp(comando, "/msg") == 0) {
            char destinatario[50];
            char mensagem[LEN];
            sscanf(buffer, "%s %s %[^\n]", comando, destinatario, mensagem);
            if (strlen(destinatario) == 0 || strlen(mensagem) == 0) {
                enviar_mensagem(cliente->socket, "Uso correto: /msg <destinatario> <mensagem>\n");
                continue;
            }

            pthread_mutex_lock(&mutex_clientes);
            int destinatario_socket = -1;
            for (int i = 0; i < num_clientes; i++) {
                if (strcmp(clientes[i]->name, destinatario) == 0) {
                    destinatario_socket = clientes[i]->socket;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex_clientes);

            if (destinatario_socket != -1) {
                char mensagem_completa[LEN];
                snprintf(mensagem_completa, LEN, "[Privado][%s]: %s\n", cliente->name, mensagem);
                enviar_mensagem(destinatario_socket, mensagem_completa);
                enviar_mensagem(cliente->socket, "Mensagem enviada.\n");
            } else {
                enviar_mensagem(cliente->socket, "Cliente não encontrado.\n");
            }
        }
        else {
            // Mensagem padrão (para salas, se o cliente estiver em uma)
            if (strlen(cliente->sala_atual) > 0) {
                char mensagem_completa[LEN];
                snprintf(mensagem_completa, LEN, "[%s][%s]: %s\n", cliente->sala_atual, cliente->name, buffer);
                enviar_mensagem_sala(cliente->sala_atual, mensagem_completa, cliente);
            } else {
                // Enviar para todos (broadcast)
                char mensagem_completa[LEN];
                snprintf(mensagem_completa, LEN, "[%s]: %s\n", cliente->name, buffer);
                pthread_mutex_lock(&mutex_clientes);
                for (int i = 0; i < num_clientes; i++) {
                    if (clientes[i]->socket != cliente->socket) {
                        enviar_mensagem(clientes[i]->socket, mensagem_completa);
                    }
                }
                pthread_mutex_unlock(&mutex_clientes);
            }
        }
    }

    return NULL;
}

int main() {
    int sockfd, cliente_socket;
    struct sockaddr_in local, remoto;
    socklen_t len = sizeof(remoto);
    pthread_t thread_id;

    // Criar o socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket ");
        exit(1);
    }
    printf("Socket Criado com Sucesso\n");

    // Configurar o endereço local
    local.sin_family = AF_INET;
    local.sin_port = htons(PORTA);
    local.sin_addr.s_addr = inet_addr("127.0.0.1"); // Para aceitar conexões de qualquer IP, use INADDR_ANY
    memset(local.sin_zero, 0x0, 8);

    // Vincular o socket ao endereço local
    if (bind(sockfd, (struct sockaddr*)&local, sizeof(local)) == -1) {
        perror("bind ");
        close(sockfd);
        exit(1);
    }
    printf("Bind realizado com sucesso na porta %d\n", PORTA);

    // Colocar o socket em modo de escuta
    if (listen(sockfd, MAX_CLIENTS) == -1) {
        perror("listen ");
        close(sockfd);
        exit(1);
    }
    printf("Servidor escutando na porta %d...\n", PORTA);

    while (1) {
        // Aceitar uma nova conexão
        cliente_socket = accept(sockfd, (struct sockaddr*)&remoto, &len);
        if (cliente_socket == -1) {
            perror("accept ");
            continue;
        }

        // Alocar memória para o novo cliente
        Cliente* novo_cliente = (Cliente*)malloc(sizeof(Cliente));
        if (!novo_cliente) {
            perror("malloc ");
            close(cliente_socket);
            continue;
        }
        novo_cliente->socket = cliente_socket;
        strcpy(novo_cliente->name, "");
        strcpy(novo_cliente->sala_atual, "");

        // Criar uma nova thread para gerenciar o cliente
        if (pthread_create(&thread_id, NULL, gerenciar_cliente, (void*)novo_cliente) != 0) {
            perror("pthread_create ");
            close(cliente_socket);
            free(novo_cliente);
            continue;
        }

        // Desanexar a thread para liberar recursos automaticamente quando terminar
        pthread_detach(thread_id);
    }

    close(sockfd);
    return 0;
}
