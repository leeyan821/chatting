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
#define MAX_NAME 20

typedef struct Message {
	int user_id;
	char user_name[MAX_NAME];
	char str[MAX_MESSAGE_LEN];
}Message;

void *sendThread();
void *recvThread(void *data);
int isFull();
int isEmpty();
int enqueue(Message item);
Message* dequeue();
int sock_main, sock_client[10];
Message *msgbuff;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int front = -1;
int rear = -1;
int num = 0;

int main()
{
	int count = 0;
	int th_id;
	Message buff;
	pthread_t th_send;

	struct sockaddr_in addr;

	printf("Server Start\n");

	pthread_t th_recv[10];

	msgbuff = (Message *)malloc(sizeof(Message) * MAX_BUFF);

	//SendThread create
	th_id = pthread_create(&th_send, NULL, sendThread, 0);

	if (th_id < 0) {
		printf("SendThread Creation Failed\n");
		exit(1);
	}

	//36007
	addr.sin_family = AF_INET;
	addr.sin_port = htons(36007);
	addr.sin_addr.s_addr = INADDR_ANY;

	//IPv4 TCP
	if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Socket Open Failed\n");
		exit(1);
	}

	// bind
	if (bind(sock_main, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Bind Failed\n");
		exit(2);
	}

	// listen
	if (listen(sock_main, 5) == -1) {
		printf("Listen Failed\n");
		exit(3);
	}

	while (1) {
		// accept and create client thread
		if ((sock_client[count] = accept(sock_main, NULL, NULL)) == -1) {
			printf("Accept Failed\n");
			continue;
		}
		else {
			if (num < 10) {
				int idx = count;
				th_id = pthread_create(&th_recv[count], NULL, recvThread, (void *)&idx);

				if (th_id < 0) {
					printf("Receive Thread #%d Creation Failed\n", count + 1);
					exit(1);
				}
			
				count++;
				num++;
			}
		}
	}
}
void *sendThread() {
	Message *tmp;

	printf("Send Thread Start\n");

	while (1) {
		// dequeue
		if ((tmp = dequeue()) != NULL) {
			//member count
			if(strncmp(tmp->str,"member",6)==0)
			{
				int n = tmp->user_id;
				send(sock_client[n], (char*)tmp, sizeof(Message), 0);
				continue;
			}
			//member send
			if(strncmp(tmp->str,"send ",5)==0)
	                {
        	                char *t;
                	        char *u;
                	        char *m;
				int id;
               		        t = strchr(tmp->str, ' ');
                        	u = t+1;
				id = atoi(u);
                        	t = strchr(u, ' ');
                        	*t = '\0';
                        	m = t+1;
				char re[] = "(Secret Message) ";
				strcat(re, m);
				strcpy(tmp->str, re);
                		send(sock_client[id], (char*)tmp, sizeof(Message), 0);
				continue;
			}
			//warning user
			if(strncmp(tmp->str,"out ",4)==0)
			{
				char *t;
                                char *u;
                                char *m;
                                int id;
                                t = strchr(tmp->str, ' ');
                                u = t+1;
                                id = atoi(u);
                                strcpy(tmp->str, "!warning user");
                                send(sock_client[id], (char*)tmp, sizeof(Message), 0);
				sprintf(tmp->str, "%d User sleep 60 seconds",id);
				for(int i=0;i<10;i++)
				{
					if(i != id) {
						send(sock_client[i], (char*)tmp, sizeof(Message), 0);
					}
				}
			     	continue;
			}
			for (int i = 0; i < 10; i++) {
				if (i != tmp->user_id) {
					send(sock_client[i], (char*)tmp, sizeof(Message), 0);
				}
			}
		}
		usleep(1000);
	}
}

void *recvThread(void *data) {
	Message buff;
	int thread_id = *((int*)data);

	printf("Receive Thread %d Start\n", thread_id);
	
	// enqueue
	memset(&buff, 0, sizeof(Message));
	
	while (recv(sock_client[thread_id], (char*)&buff, sizeof(buff), 0) > 0) {
		buff.user_id = thread_id;
		
		//sleep
		if(strncmp(buff.str, "warning",7)==0)
		{
			printf("%d sleep 30second\n",thread_id);
			strcpy(buff.str, "! sleep 30\n");
			enqueue(buff);
			continue;
		}
		//member count
		if(strncmp(buff.str, "member",6)==0)
                {
                        char re[10];
                        sprintf(re, "%d", num);
                        strcpy(buff.str,"member Num: ");
                        strcat(buff.str, re);
                        enqueue(buff);
			continue;
                }
		//username
		if(strncmp(buff.str, "username ",9)==0)
		{
			char *name;
			char *username;
			name = strchr(buff.str,' ');
			username = name + 1;
			printf("%d User's name: %s \n",thread_id, username);
			continue;
		}
		//enqueue
		if (enqueue(buff) == -1) {
			printf("Messag Buffer Full\n");
		}
		//quit
		if(strncmp(buff.str, "quit",4)==0)
		{
			printf("%d Client Out\n",thread_id);
			num--;
		}
	}
}

int isFull() {
	if ((front == rear + 1) || (front == 0 && rear == MAX_BUFF - 1)) {
		return 1;
	}
	return 0;
}

int isEmpty() {
	if (front == -1) {
		return 1;
	}
	return 0;
}

int enqueue(Message item) {

	if (isFull()) {
		return -1;
	}
	else {
		pthread_mutex_lock(&mutex);
		if (front == -1) {
			front = 0;
		}
		rear = (rear + 1) % MAX_BUFF;
		msgbuff[rear].user_id = item.user_id;
		strcpy(msgbuff[rear].user_name, item.user_name);
		strcpy(msgbuff[rear].str, item.str);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}

Message* dequeue() {
	Message *item;

	if (isEmpty()) {
		return NULL;
	}
	else {
		pthread_mutex_lock(&mutex);
		item = &msgbuff[front];

		if (front == rear) {
			front = -1;
			rear = -1;
		}
		else {
			front = (front + 1) % MAX_BUFF;
		}
		pthread_mutex_unlock(&mutex);
		return item;
	}
}
