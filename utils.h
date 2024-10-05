// utils.h

#ifndef UTILS_H
#define UTILS_H

#include "client.h"

// Funções utilitárias
void enviar_mensagem(int socket, const char* mensagem);
void enviar_mensagem_para_todos(const char* mensagem, Cliente* remetente);
void notificar_conexao(const char* mensagem, Cliente* remetente);
void notificar_desconexao(const char* mensagem, Cliente* remetente);

#endif // UTILS_H
