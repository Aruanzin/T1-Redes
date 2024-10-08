# T1-Redes


| Autor                          | NUSP      |
| ------------------------------ | --------- |
| Aruan Bretas de Oliveira Filho | 12609731  |
| Leonardo Rodrogues de Sousa    | 10716380  |
| Andre Luiz de Souza            | 5631500   |
| Fellipe Tripovichy             | 9850332   |
| Roberto Severo Utagawa         | 12690712  |


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
   gcc client.c -o client -lpthread

   ./client

3. **Alternativa Makefile**
    Visando a facilitação na compilação de ambos os arquivos basta:

    ```bash
    make
    ```

## Informações Relevantes

  * O projeto foi realizado para rodar em um linux, sistemas windows precisariam de bibliotecas diferentes e até externas
  * A conexão como os testes foram feitos apenas em uma máquina, o endereço da conexão foi meu localhost
  * Porta utilizada 2000 (verificar disponibilidade dela)
  * Ai está algumas funcionalidades:

    * "/online": lista quem está conectado no servidor
    * "/msg destinatario mensagem": envia mensagem no privado para alguém
    * "/create nome": cria sala com nome desejado
    * "/join nome": entra na sala que deseja se existir
    * "/sala nome mensagem": manda mensagem na sala que deseja
    * "/leave nome": sai de sala desejavada
    * "/list": lista salas existentes 
