#include <stdio.h> // for perror
#include <stdlib.h>
#include <string.h> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket
#include <pthread.h>
#include <sys/time.h>

void * recv_thread(void * arg){
	int fd = *(int *)arg;
	fd_set readfds;
	const static int BUFSIZE = 1024;
        char buf[BUFSIZE];
	
	while(true){
		FD_SET(fd, &readfds);
		if(select(fd+1, &readfds, NULL, NULL, NULL) == -1){
			perror("select");
			close(fd);
			exit(-1);
		}

		if(FD_ISSET(fd, &readfds)){
			ssize_t received = recv(fd, buf, BUFSIZE - 1, 0);
			if(received == 0){
				perror("eof");
				close(fd);
				exit(0);
			}
			
			if (received == -1) {
                        	perror("recv failed");
				close(fd);
                        	exit(-1);
                	}
			buf[received] = '\0';
                	printf("%s\n", buf);
		}

	}
	close(fd);
	return NULL;
}


int main(int argc, char * argv[]) {
	if(argc != 3){
		printf("syntax : echo_client <host> <port>\n");
		return -1;
	}
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket failed");
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &addr.sin_addr);
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	int res = connect(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
	if (res == -1) {
		perror("connect failed");
		return -1;
	}
	printf("connected\n");
	pthread_t tid;

	if(pthread_create(&tid, NULL, recv_thread, (void*)&sockfd)){
		perror("thread create");
		return -1;
	}

	while (true) {
		const static int BUFSIZE = 1024;
		char buf[BUFSIZE];

		fgets(buf, BUFSIZE, stdin);
		buf[strlen(buf)-1] = 0;
		
		if (strcmp(buf, "quit") == 0) break;

		ssize_t sent = send(sockfd, buf, strlen(buf), 0);
		if (sent == 0) {
			perror("send failed");
			break;
		}
	}

	close(sockfd);
}
