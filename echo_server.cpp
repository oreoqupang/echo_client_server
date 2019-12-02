#include <stdio.h> // for perror
#include <string.h> // for memset
#include <unistd.h> // for close
#include <pthread.h>
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket
#include <vector>
#include <algorithm>

using namespace std;

int broad_cast_mode = 0;
pthread_mutex_t lock;
vector<int> child_fd;

void * session(void * arg){
	int childfd = *(int *)arg;
	
	while (true) {
		const static int BUFSIZE = 1024;
                char buf[BUFSIZE];

                ssize_t received = recv(childfd, buf, BUFSIZE - 1, 0);
                if(received == 0) {
			perror("eof");
			break;
		}
		
		if(received == -1) {
			perror("recv failed");
			break;
                }
                buf[received] = '\0';
                printf("%s\n", buf);
		
		if(broad_cast_mode){
			int err = 0;
			pthread_mutex_lock(&lock);
			for(vector<int>::iterator it = child_fd.begin(); it != child_fd.end(); it++){
				int now_fd = *it;
				ssize_t sent = send(now_fd, buf, strlen(buf), 0);
                        	if (sent == 0) {
                                	perror("send failed");
                                	err = 1;
					break;
                        	}
			}
			pthread_mutex_unlock(&lock);
			if(err) break;
		}
		else{
			ssize_t sent = send(childfd, buf, strlen(buf), 0);
			if (sent == 0) {
				perror("send failed");
				break;
			}
		}
	}

	pthread_mutex_lock(&lock);
	child_fd.erase(find(child_fd.begin(), child_fd.end(), childfd));
	close(childfd);
	pthread_mutex_unlock(&lock);
	return NULL;
}
 
int main(int argc, char * argv[]) {
	if(argc != 2 && argc != 3){
		printf("syntax : echo_server <port> [-b]\n");
		return -1;
	}

	if(argc == 3){
		if(strcmp(argv[2], "-b")){
			printf("syntax : echo_server <port> [-b]\n");
			return -1;
		}
		broad_cast_mode = 1;
	}

	if (pthread_mutex_init(&lock, NULL) != 0) { 
        	perror("mutex init failed"); 
        	return -1; 
	}	

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket failed");
		return -1;
	}

	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,  &optval , sizeof(int));
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	int res = bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
	if (res == -1) {
		perror("bind failed");
		return -1;
	}

	res = listen(sockfd, 5);
	if (res == -1) {
		perror("listen failed");
		return -1;
	}

	while (true) {
		pthread_t tmp;
		struct sockaddr_in addr;
		socklen_t clientlen = sizeof(sockaddr);
		int childfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&addr), &clientlen);
		if (childfd < 0) {
			perror("ERROR on accept");
			break;
		}
		printf("connected\n");
		
		if(pthread_create(&tmp, NULL, session,(void *)&childfd)){
			perror("thread create");
			return -1;
		}

		pthread_mutex_lock(&lock);
		child_fd.push_back(childfd);
		pthread_mutex_unlock(&lock);
	}

	close(sockfd);
	pthread_mutex_destroy(&lock);
}
