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

// Função para excluir uma sala
void excluir_sala(const char* nome_sala) {
    pthread_mutex_lock(&mutex_salas);
    for (int i = 0; i < num_salas; i++) {
        if (strcmp(salas[i].nome, nome_sala) == 0) {
            // Notificar os clientes da exclusão da sala
            char msg[LEN];
            snprintf(msg, LEN, "A sala %s foi excluída.\n", nome_sala);
            enviar_mensagem_sala(nome_sala, msg, NULL); // Enviar para todos na sala

            // Remover a sala
            for (int j = i; j < num_salas - 1; j++) {
                salas[j] = salas[j + 1];
            }
            num_salas--;
            break;
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

            remover_cliente_sala(cliente); // Remover da sala atual, se houver

            pthread_mutex_lock(&mutex_salas);
            // Verificar se a sala existe
            int sala_encontrada = 0;
            for (int i = 0; i < num_salas; i++) {
                if (strcmp(salas[i].nome, nome_sala) == 0) {
                    sala_encontrada = 1;
                    // Adicionar cliente à sala
                    salas[i].clientes[salas[i].num_clientes++] = cliente;
                    strcpy(cliente->sala_atual, nome_sala);
                    snprintf(buffer, LEN, "%s entrou na sala %s.\n", cliente->name, nome_sala);
                    enviar_mensagem_sala(nome_sala, buffer, cliente);
                    break;
                }
            }
            if (!sala_encontrada) {
                enviar_mensagem(cliente->socket, "Sala não encontrada.\n");
            }
            pthread_mutex_unlock(&mutex_salas);
        }
        else if (strcmp(comando, "/leave") == 0) {
            remover_cliente_sala(cliente);
            enviar_mensagem(cliente->socket, "Você saiu da sala atual.\n");
        }
        else if (strcmp(comando, "/excluir") == 0) {
            char nome_sala[50];
            sscanf(buffer, "%s %s", comando, nome_sala);
            excluir_sala(nome_sala);
            enviar_mensagem(cliente->socket, "Comando de exclusão enviado.\n");
        }
        else if (strcmp(comando, "/rename") == 0) {
            char novo_nome[50];
            sscanf(buffer, "%s %s", comando, novo_nome);
            strcpy(cliente->name, novo_nome);
            char msg[LEN];
            snprintf(msg, LEN, "Seu nome foi alterado para %s.\n", novo_nome);
            enviar_mensagem(cliente->socket, msg);
        }
        else {
            // Enviar a mensagem para a sala atual
            char msg[LEN];
            snprintf(msg, LEN, "%s: %s", cliente->name, buffer);
            enviar_mensagem_sala(cliente->sala_atual, msg, cliente);
        }
    }

    return NULL;
}

int main() {
    int servidor_socket, cliente_socket;
    struct sockaddr_in servidor_addr, cliente_addr;
    socklen_t cliente_len = sizeof(cliente_addr);

    // Criar socket do servidor
    servidor_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço do servidor
    servidor_addr.sin_family = AF_INET;
    servidor_addr.sin_addr.s_addr = INADDR_ANY;
    servidor_addr.sin_port = htons(PORTA);

    // Associar o socket a um endereço
    if (bind(servidor_socket, (struct sockaddr*)&servidor_addr, sizeof(servidor_addr)) < 0) {
        perror("bind");
        close(servidor_socket);
        exit(EXIT_FAILURE);
    }

    // Ouvir por conexões
    listen(servidor_socket, 5);
    printf("Servidor ouvindo na porta %d...\n", PORTA);

    while (1) {
        // Aceitar nova conexão
        cliente_socket = accept(servidor_socket, (struct sockaddr*)&cliente_addr, &cliente_len);
        if (cliente_socket < 0) {
            perror("accept");
            continue;
        }

        // Criar novo cliente
        Cliente* novo_cliente = (Cliente*)malloc(sizeof(Cliente));
        novo_cliente->socket = cliente_socket;
        memset(novo_cliente->sala_atual, 0, sizeof(novo_cliente->sala_atual)); // Inicializar a sala atual

        pthread_t tid;
        pthread_create(&tid, NULL, gerenciar_cliente, (void*)novo_cliente);
    }

    // Fechar o socket do servidor
    close(servidor_socket);
    return 0;
}
