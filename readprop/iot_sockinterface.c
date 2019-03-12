#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "iot_device_obj.h"

#define PORT 22
#define SERVER "127.0.0.1"
#define MAXBUF 1024
#define MAX_EPOLL_EVENTS 64

extern D2C_DATA *head_ptr_d2c_data_list;
extern int sockfd_main;

void send_d2c_data_to_sock_client(int sockfd){

    D2C_DATA d2c_data;
    if(head_ptr_d2c_data_list == NULL){
        perror("d2c data list null\n");
        return;
    }
    d2c_data = *head_ptr_d2c_data_list;
    send(sockfd, d2c_data->d2c_str, strlen(d2c_data->d2c_str), 0);
    head_ptr_d2c_data_list = &(d2c_data->next);
    free(d2c_data);
}

int init_sock_interface(void)
{
	int sock;
	char send_data[1024];
	struct hostent *host;
	struct sockaddr_in server_addr;

	host = gethostbyname("127.0.0.1");

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    perror("Socket");
	    exit(1);
	}

    sockfd_main = sock;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(5000);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_addr.sin_zero),8);

    while(connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
        perror(" Server connection established Connect...retry?(y/n)");
        if(getchar() == 'n')
            exit(1);
    }

	// if (connect(sock, (struct sockaddr *)&server_addr,
	// 	    sizeof(struct sockaddr)) == -1)
	// {
	//     perror(" Server connection established Connect...retry?(y/n)");
	//     exit(1);
	// }

	// while(1) {
	// 	printf("\nSEND (q or Q to quit) : ");
	// 	gets(send_data);

	// 	if (strcmp(send_data , "q") != 0 && strcmp(send_data , "Q") != 0)
    //         send_d2c_data_to_sock_client(sock);
	// 		//send(sock,send_data,strlen(send_data), 0);
	// 	else
	// 		break;
	// }
	// close(sock);
	return 0;
}
