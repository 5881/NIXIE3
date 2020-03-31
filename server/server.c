#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define PORT 7890 // Порт, к которому будут подключаться пользователи
// Дамп байтов памяти в шестнадцатеричном виде и с разделителями
void dump(char *data_buffer, const unsigned int length) {
	unsigned char byte;
	unsigned int i, j;
	for(i=0; i < length; i++) {
	byte = data_buffer[i];
	printf("%02x ", data_buffer[i]); 		// Вывести в шестнадцатеричном виде.
		if(((i%16)==15) || (i==length-1)) {
			for(j=0; j < 15-(i%16); j++) printf(" ");
			printf("| ");
			for(j=(i-(i%16)); j <= i; j++) { 	// Показать отображаемые символы.
				byte = data_buffer[j];
				if((byte > 31) && (byte < 127)) printf("%c", byte);
				// Вне диапазона
				// отображаемых символов
				
					else printf(".");
			}
			printf("\n"); // Конец строки дампа (в строке 16 байт)
		} // Конец if
	} // Конец for
}

void fatal(char *str){
	printf("FATAL ERROR: %s", str);
	exit(666);
	
}




int main(void) {
	FILE * cmd;
	char cmd_resp[80];
	int n;
	
	
	int sockfd, new_sockfd; // Слушать на sock_fd, новое соединение на new_fd
	struct sockaddr_in host_addr, client_addr; // Адресные данные
	socklen_t sin_size;
	int recv_length=1, yes=1;
	char buffer[1024];
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) fatal("in socket");
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
						fatal("setting socket option SO_REUSEADDR");

	host_addr.sin_family = AF_INET;
	// Порядок байтов узла
	host_addr.sin_port = htons(PORT); // Короткое целое, сетевой порядок байтов
	host_addr.sin_addr.s_addr = 0;		// Автоматически заполнить моим IP.
	memset(&(host_addr.sin_zero), '\0', 8); // Обнулить остаток структуры.
	if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1)
						fatal("binding to socket");
	if (listen(sockfd, 5) == -1) fatal("listening on socket");

	while(1) {
		// Цикл accept.
		sin_size = sizeof(struct sockaddr_in);
		new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
		if(new_sockfd == -1) fatal("accepting connection");
		printf("server: got connection from %s port %d\n",
				inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	//send(new_sockfd, "Hello, world!\r\n", 13, 0);
	//send(new_sockfd, "response\r\n", 10, 0);
	recv_length = recv(new_sockfd, &buffer, 1024, 0);
	buffer[recv_length]=0;
	printf("request: %s\r\n", buffer);
	
	cmd=popen("./mega_script.sh", "r");
	//cmd=popen(buffer, "r");
	n=fread(cmd_resp,1,80,cmd);
	cmd_resp[n]=0;
	pclose(cmd);
	printf("> %s\r\n",cmd_resp);
	
		//while(recv_length > 0) {
		//	printf("RECV: %d bytes\n", recv_length);
		//	
		//	dump(buffer, recv_length);
		//	recv_length = recv(new_sockfd, &buffer, 1024, 0);
		//	}
	//send(new_sockfd, "response\n\r", 10, 0);
	send(new_sockfd, cmd_resp, n, 0);
	close(new_sockfd);
	}
	return 0;
	}


