# T1-Redes


| Autor                          | NUSP      |
| ------------------------------ | --------- |
| Aruan Bretas de Oliveira Filho | 12609731  |
| Leonardo                       | XXXXXXXX  |
| Andre                          | XXXXXXXX  |
| Fellipe Tripovichy             | XXXXXXXX  |


# Chat Privado com Servidor e Cliente em C

Este projeto implementa um sistema de chat privado onde clientes podem se conectar a um servidor e se comunicar com outros clientes conectados. O servidor usa threads para gerenciar múltiplos clientes simultaneamente, e o cliente permite que os usuários enviem mensagens para destinatários específicos.

## Implementação

### Estrutura do Projeto

- **Servidor (`server.c`)**
  - Gerencia múltiplos clientes usando threads.
  - Roteia mensagens entre clientes.
  - Utiliza a biblioteca pthread para suporte a threads.

- **Cliente (`client.c`)**
  - Permite que o usuário envie mensagens para um destinatário específico.
  - Recebe e exibe mensagens do servidor.

### Funcionalidade

- **Servidor**
  - Aceita conexões de múltiplos clientes.
  - Gerencia mensagens enviadas entre clientes.
  - Utiliza um mutex para garantir acesso seguro à lista de clientes.

- **Cliente**
  - Envia mensagens para um cliente específico usando o comando `/msg <destinatario> <mensagem>`.
  - Exibe mensagens recebidas do servidor.

## Compilação e Execução

### Compilação

1. **Servidor**

   Compile execute o servidor com os seguintes comandos:

   ```bash
   gcc -o server server.c -lpthread

   ./server
2. **Cliente**
    Compile e execute o cliente com os seguintes comandos:

   ```bash
   gcc client.c -o client

   ./client

3. **Alternativa Makefile**
    Visando a facilitação na compilação de ambos os arquivos basta:

    ```bash
    make