void* gerenciar_cliente(void* arg) {
    int cliente_socket = *((int*)arg);
    char buffer[LEN];
    char nome[50];

    // Receber o nome do cliente
    memset(buffer, 0x0, LEN);
    recv(cliente_socket, buffer, LEN, 0);
    strcpy(nome, buffer);

    printf("Cliente conectado: %s\n", nome);

    // Adicionar o cliente à lista
    pthread_mutex_lock(&mutex);
    if (num_clientes < MAX_CLIENTS) {
        clientes[num_clientes].socket = cliente_socket;
        strcpy(clientes[num_clientes].name, nome);
        num_clientes++;
    } else {
        printf("Número máximo de clientes alcançado.\n");
        close(cliente_socket);
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    pthread_mutex_unlock(&mutex);

    while (1) {
        memset(buffer, 0x0, LEN);
        int bytes = recv(cliente_socket, buffer, LEN, 0);
        if (bytes <= 0) {
            printf("Cliente desconectado: %s\n", nome);
            close(cliente_socket);
            pthread_mutex_lock(&mutex);
            // Remover cliente da lista
            for (int i = 0; i < num_clientes; i++) {
                if (clientes[i].socket == cliente_socket) {
                    for (int j = i; j < num_clientes - 1; j++) {
                        clientes[j] = clientes[j + 1];
                    }
                    num_clientes--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        buffer[bytes] = '\0';
        printf("Mensagem recebida de %s: %s\n", nome, buffer);

        // Enviar mensagem para o destinatário
        char comando[LEN];
        sscanf(buffer, "%s", comando);

        if (strcmp(comando, "/msg") == 0) {
            char destinatario[50];
            char mensagem[LEN];
            sscanf(buffer, "%s %s %[^\n]", comando, destinatario, mensagem);

            pthread_mutex_lock(&mutex);
            int destinatario_socket = -1;
            for (int i = 0; i < num_clientes; i++) {
                if (strcmp(clientes[i].name, destinatario) == 0) {
                    destinatario_socket = clientes[i].socket;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            if (destinatario_socket != -1) {
                // Concatenar o nome do remetente com a mensagem
                char mensagem_completa[LEN];
                snprintf(mensagem_completa, LEN, "%s: %s", nome, mensagem);

                // Enviar mensagem com o nome do remetente ao destinatário
                send(destinatario_socket, mensagem_completa, strlen(mensagem_completa), 0);
            } else {
                char erro[] = "Cliente não encontrado.\n";
                send(cliente_socket, erro, strlen(erro), 0);
            }
        }
    }
}
