#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#define MAX_BUFF 100
#define MAX_MESSAGE_LEN 256
#define MAX_USER 20

typedef struct Message {
	int user_id;
	char user_name[MAX_USER];
	char str[MAX_MESSAGE_LEN];
}Message;

void *sendThreadClient();
void register_username(int s);

int sock_main, sock_client[10];
Message *msgbuff;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char username[MAX_USER];

int main()
{
	int mode;
	int count = 0;
	int th_id;
	Message buff;
	pthread_t th_send;

	struct sockaddr_in addr;

	// 127.0.0.1 36007
	addr.sin_family = AF_INET;
	addr.sin_port = htons(36007);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// IPv4 TCP
	if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Socket Open Failed\n");
		exit(1);
	}

	// Connect
	if (connect(sock_main, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Connect Failed\n");
		exit(4);
	}

	//username enter
        printf("Enter UserName: ");
        fgets(username, sizeof username, stdin);
        username[strlen(username) - 1] = '\0';
	register_username(sock_main);

	// Client Send Thread
	th_id = pthread_create(&th_send, NULL, sendThreadClient, 0);

	if (th_id < 0) {
		printf("Send Thread Creation Failed\n");
		exit(1);
	}

	printf("Chat Start\n");
	while (1) {

		// Get user Chatting 
		memset(&buff, 0, sizeof(buff));

		if (recv(sock_main, (char*)&buff, sizeof(buff), 0) > 0) {
			
			//warning user
			if(strncmp(buff.str,"!warning user",9)==0)
			{
				printf("===== You can't chatting =====\n");
				sleep(60);
				printf("End 60 seconds\n");
				continue;
			}
			//printf
			printf("[ %d | User: %s ] : %s\n", buff.user_id,buff.user_name, buff.str);
			//quit
			if(strncmp(buff.str,"quit",4)==0)
			{
				printf("===== %d User Out Chatting =====\n",buff.user_id);
			}
		}
		else {
			printf("Disconnected\n");
			exit(5);
		}
	}
	return 0;
}

void *sendThreadClient() {
	Message tmp;
	int count;

	while (1) {
		// send message
		memset(&tmp, 0, sizeof(tmp));
		
		fgets(tmp.str, MAX_MESSAGE_LEN, stdin);
		tmp.str[strlen(tmp.str) - 1] = '\0';
		tmp.user_id = -1;
		sprintf(tmp.user_name, "%s", username);

		count = send(sock_main, (char *)&tmp, sizeof(Message), 0);

		//quit
		if(strncmp(tmp.str, "quit", 4) == 0) {
                        printf("Chatting End\n");
                        exit(0);
                }
		//sleep
		if(strncmp(tmp.str, "warning",7)==0){
			printf("30 second sleep\n");
                        sleep(30);
                        printf("30 End\n");
			continue;
                }
	}
}

void register_username(int sockfd)
{
	Message tmp;
        int count;
	char name[MAX_USER];

	sprintf(tmp.str, "username %s", username);
        tmp.user_id = -1;

        count = send(sock_main, (char *)&tmp, sizeof(Message), 0);
}
