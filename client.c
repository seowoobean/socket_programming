#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#define PORT 8080 
#define LEN 1025
#define MSGLEN 1024

struct message{
	char header;
	char message[MSGLEN];
};

int connectWithServer(void);
void info(char* send_message);
char filename[11];
int fp;
void errorHandler(char error[]);
int sock;
 
int main(int argc, char const *argv[]) 
{ 
	int valread, key;
	char buffer[8] = {0}; 
	struct message sendM;
	struct message recvM;
	memset(sendM.message,'\0',MSGLEN);
	if(connectWithServer()<0) return -1;
	printf("Program execute\n");
	for(int i = 0; i<1; ){
		printf("-------------------------------------------------\nDepartment Menu : 0\nStudent Menu : 1\nEnd Program : 2\nEnter key : ");
		//send menu key
		scanf("%d",&key);
		sprintf(buffer,"%d",key);
		send(sock,buffer,1,0);
		if(key==2) break;
		fp = key;
		if(key==0) strcpy(filename,"Department");
		else strcpy(filename,"Student");
		printf("<%s Menu>\nADD : 1\nManage : 2\nDisplay : 3\nENTER KEY : ",filename);
		scanf("%d",&key);
		switch(key){
			case 1:				//add
				printf("-Enter %s Information-\n",filename);
				//send to server
				sendM.header = 'A';
				info(sendM.message);
				send(sock,&sendM,LEN,0);
				memset(sendM.message,'\0',MSGLEN);
				//read from server
				valread = read(sock, &recvM, LEN);
				if(recvM.header=='E'){
					errorHandler(recvM.message);
					memset(recvM.message,'\0',MSGLEN);
					break;
				}
				printf("Add Comlete\n");
				break;
			case 2:				
				sendM.header = 'M';
				printf("Enter %s ID to manage : ",filename);
				scanf("%d",&key);
				sprintf(sendM.message,"%d",key);
				send(sock,&sendM,LEN,0);		
				memset(sendM.message,'\0',MSGLEN);
				valread = read(sock,&recvM,MSGLEN);
				if(recvM.header=='E'){
					errorHandler(recvM.message);
					memset(recvM.message,'\0',MSGLEN);
					break;
				}
				memset(recvM.message,'\0',MSGLEN);
				printf("<Display information : 1>	<Update : 2>	<Delete : 3>\nEnter the key : ");
				scanf("%d",&key);
				switch(key){
					case 1:
						sendM.header = 'D';
						send(sock,&sendM,LEN,0);
						valread = read(sock,&recvM,LEN);
						printf("\n%s",recvM.message);
						memset(recvM.message,'\0',MSGLEN);
						break;
					case 2:
						sendM.header = 'U';
						info(sendM.message);
						send(sock,&sendM,LEN,0);
						break;
					case 3:
						sendM.header = 'R';
						send(sock,&sendM,LEN,0);	
						printf("Delete Complete\n");
						break;
					default:
						break;
				}
			case 3:			
				sendM.header = 'D';
				send(sock,&sendM,LEN,0);
				valread = read(sock,&recvM,LEN);
				printf("\n%s",recvM.message);
				memset(recvM.message,'\0',MSGLEN);
				break;
			default:
				break;
		}
	}
	printf("Program terminated\n"); 
	return 0; 
} 
int connectWithServer(void){
	struct sockaddr_in serv_addr; 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 
 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	} 
 
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		return -1; 
	} 
	return 0;
}

void info(char* send_message){
	char buffer[20];
	printf("Enter %s ID : ",filename);
	scanf("%s", buffer);
	strcat(send_message, buffer);
	strcat(send_message,",");
	printf("Enter %s Name : ",filename);
	scanf("%s",buffer);
	strcat(send_message,buffer);
	strcat(send_message,",");
	printf("Enter %s Phone_num : ",filename);
	scanf("%s",buffer);
	strcat(send_message,buffer);
	strcat(send_message,",");
	printf("Enter %s Address : ",filename);
	scanf("%s",buffer);
	strcat(send_message,buffer);
	if(fp==1){
		strcat(send_message,",");
		printf("Enter Department ID : ");
		scanf("%s",buffer);
		strcat(send_message,buffer);
	}
	strcat(send_message,";");
}
void errorHandler(char error[]){
	int i = 0  ;
	printf("ERROR : ");
	while(1){
		if(error[i]==';') break;
		printf("%c",error[i]);
		i++;
	}
	printf("\n");
}
