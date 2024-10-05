// server.c

#include "server.h"
#include "client.h"
#include "room.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORTA 2000
#define LEN 1024

// Implementação da função gerenciar_cliente
void* gerenciar_cliente(void* arg) {
    Cliente* cliente = (Cliente*)arg;
    char buffer[LEN];
    int bytes;

    // Receber o nome do cliente
    memset(buffer, 0, LEN);
    bytes = recv(cliente->socket, buffer, LEN, 0);
    if (bytes <= 0) {
        perror("recv");
        close(cliente->socket);
        remover_cliente(cliente->socket);
        free(cliente);
        pthread_exit(NULL);
    }
    buffer[bytes] = '\0';
    strncpy(cliente->name, buffer, NAME_LEN);
    printf("Cliente conectado: %s\n", cliente->name);

    // Adicionar cliente à lista global
    adicionar_cliente(cliente);

    // Notificar outros clientes sobre a nova conexão
    char msg_conexao[LEN];
    snprintf(msg_conexao, LEN, "%s entrou no chat.\n", cliente->name);
    notificar_conexao(msg_conexao, cliente);

    while (1) {
        memset(buffer, 0, LEN);
        bytes = recv(cliente->socket, buffer, LEN, 0);
        if (bytes <= 0) {
            printf("Cliente desconectado: %s\n", cliente->name);
            // Notificar outros clientes sobre a desconexão
            snprintf(msg_conexao, LEN, "%s saiu do chat.\n", cliente->name);
            notificar_desconexao(msg_conexao, cliente);

            // Remover cliente das listas
            remover_cliente(cliente->socket);
            sair_sala(cliente);
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
            char nome_sala[SALA_LEN];
            sscanf(buffer, "%s %s", comando, nome_sala);
            if (strlen(nome_sala) == 0) {
                enviar_mensagem(cliente->socket, "Uso correto: /create <nome_da_sala>\n");
                continue;
            }

            int resultado = criar_sala(nome_sala);
            if (resultado == 0) {
                enviar_mensagem(cliente->socket, "Sala criada com sucesso.\n");
            } else if (resultado == -1) {
                enviar_mensagem(cliente->socket, "Sala já existe.\n");
            } else if (resultado == -2) {
                enviar_mensagem(cliente->socket, "Número máximo de salas alcançado.\n");
            }
        }
        else if (strcmp(comando, "/join") == 0) {
            char nome_sala[SALA_LEN];
            sscanf(buffer, "%s %s", comando, nome_sala);
            if (strlen(nome_sala) == 0) {
                enviar_mensagem(cliente->socket, "Uso correto: /join <nome_da_sala>\n");
                continue;
            }

            int resultado = entrar_sala(nome_sala, cliente);
            if (resultado == 0) {
                enviar_mensagem(cliente->socket, "Entrou na sala com sucesso.\n");
            } else if (resultado == -1) {
                enviar_mensagem(cliente->socket, "Sala não encontrada.\n");
            } else if (resultado == -2) {
                enviar_mensagem(cliente->socket, "Sala cheia.\n");
            } else {
                enviar_mensagem(cliente->socket, "Erro ao entrar na sala.\n");
            }
        }
        else if (strcmp(comando, "/leave") == 0) {
            int resultado = sair_sala(cliente);
            if (resultado == 0) {
                enviar_mensagem(cliente->socket, "Saiu da sala com sucesso.\n");
            } else {
                enviar_mensagem(cliente->socket, "Você não está em nenhuma sala.\n");
            }
        }
        else if (strcmp(comando, "/list") == 0) {
            char lista_salas[LEN];
            listar_salas(lista_salas, LEN);
            enviar_mensagem(cliente->socket, lista_salas);
        }
        else if (strcmp(comando, "/sala") == 0) {
            char nome_sala[SALA_LEN];
            char mensagem[LEN];
            sscanf(buffer, "%s %s %[^\n]", comando, nome_sala, mensagem);
            if (strlen(nome_sala) == 0 || strlen(mensagem) == 0) {
                enviar_mensagem(cliente->socket, "Uso correto: /sala <nome_da_sala> <mensagem>\n");
                continue;
            }

            enviar_mensagem_sala(nome_sala, mensagem, cliente);
        }
        else if (strcmp(comando, "/online") == 0) {
            char lista_online[LEN];
            listar_clientes_online(lista_online, LEN);
            enviar_mensagem(cliente->socket, lista_online);
        }
        else if (strcmp(comando, "/msg") == 0) {
            char destinatario[NAME_LEN];
            char mensagem[LEN];
            sscanf(buffer, "%s %s %[^\n]", comando, destinatario, mensagem);
            if (strlen(destinatario) == 0 || strlen(mensagem) == 0) {
                enviar_mensagem(cliente->socket, "Uso correto: /msg <destinatario> <mensagem>\n");
                continue;
            }

            Cliente* dest = buscar_cliente_por_nome(destinatario);
            if (dest) {
                char mensagem_privada[LEN];
                snprintf(mensagem_privada, LEN, "[Privado][%s]: %s\n", cliente->name, mensagem);
                enviar_mensagem(dest->socket, mensagem_privada);
                enviar_mensagem(cliente->socket, "Mensagem enviada.\n");
            } else {
                enviar_mensagem(cliente->socket, "Cliente não encontrado.\n");
            }
        }
        else {
            // Mensagem padrão
            if (strlen(cliente->sala_atual) > 0) {
                char mensagem_sala[LEN];
                snprintf(mensagem_sala, LEN, "[%s][%s]: %s\n", cliente->sala_atual, cliente->name, buffer);
                enviar_mensagem_sala(cliente->sala_atual, mensagem_sala, cliente);
            } else {
                // Broadcast para todos
                char mensagem_broadcast[LEN];
                snprintf(mensagem_broadcast, LEN, "[%s]: %s\n", cliente->name, buffer);
                enviar_mensagem_para_todos(mensagem_broadcast, cliente);
            }
        }
    }

    return NULL;
}
