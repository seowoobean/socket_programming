#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <errno.h>
#include <pthread.h>
#define PORT 8080 
#define LEN 1025
#define MSGLEN 1024


/*
	<Recv Message>
	Add : Header - A, Message - information distinct by ','
	Manage : Header - M, Message - ID
	Update : Header - U, Message - update offset + ',' + update information
	Display : Header - D, Message - None
	Display : Header - R(remove), Message - None
	<Send Message>
	Success : Header - S, Message - None
	Information : Header - I, Message - None
	Fail : Header - E, Message - Error Message
*/ 

struct Student{
	int id;
	char name[20];
	int phone;
	char address[20];
	int depId;
};
struct department{
	int id;
	char name[20];
	int phone;
	char address[20];
};
struct message{
	char header;
	char message[MSGLEN];
};

int fp;           //which one client select(department or student)
int sf;           //file descripter
char filename[11];	//setting filename
int f_offset; 		//variable using save offset of file

int add(char line[]);
void search(char* send_message,int offser);
void display(char *send_message);
void displayAll(void);
void update(int offset, char line[]);
void delete(int offset);
struct Student fillLine_s(int offset);
struct department fillLine_d(int offset);
int findOffset(int key);
void restoreInFile(int offset,int size, char buf[]);
void saveFileContext(int offset, char *buf2, char *buf3);

void *clientHandle(void *data) 
{ 
	int valread, key, offset, intbuf; 
	char buffer[MSGLEN] = "\0";   //recieve message
	char buffer_key[8]="\0";	//key buffer(client request)
	int new_socket = *(int *)data;
	struct message sendM;
	struct message recvM;
	while(1){
		//recieve menu key	
		valread = read( new_socket , &buffer_key, 1); 
		sscanf(buffer_key,"%d",&key);
		if(key==2) break;
		sprintf(buffer,"Menu key is %d\n",key);
		write(1,buffer,strlen(buffer));
		memset(buffer,'\0',MSGLEN);
		fp = key;
		//set filename
		if(fp==0) strcpy(filename,"Department");
		else strcpy(filename,"Student");
		//read from client
		valread = read(new_socket,&recvM,LEN);
		switch(recvM.header){
			case 'A':
				intbuf = add(recvM.message);
				if(intbuf < 0){
					sendM.header = 'E';
					if(intbuf==-1){
						sprintf(sendM.message,"Already enrolled %s;",filename);
					}
					else if(intbuf == -2){
						sprintf(sendM.message,"Doesn't exist Department ID;");
					}
					send(new_socket,&sendM,LEN,0);
					memset(sendM.message,'\0',MSGLEN);
					memset(recvM.message,'\0',MSGLEN);
					break;
				}
				sendM.header = 'S';
				send(new_socket,&sendM,LEN,0);
				break;
			case 'M':
				sscanf(recvM.message,"%d",&key);
				intbuf = findOffset(key);
				if(intbuf < 0){
					sendM.header = 'E';
					sprintf(sendM.message,"Can't find %s ID %d;",filename,key);
					send(new_socket,&sendM,LEN,0);
					memset(sendM.message,'\0',MSGLEN);
					memset(recvM.message,'\0',MSGLEN);
					break;
				}
				sendM.header = 'S';
				send(new_socket,&sendM,LEN,0);
				valread = read(new_socket,&recvM,LEN);
				switch(recvM.header){
					case 'D':
						sendM.header = 'I';
						search(sendM.message,intbuf);
						send(new_socket,&sendM,LEN,0);
						memset(sendM.message,'\0',MSGLEN);
						break;
					case 'U':
						sendM.header = 'S';
						update(intbuf, recvM.message);
						send(new_socket,&sendM,LEN,0);
						memset(sendM.message,'\0',MSGLEN);
						break;
					case 'R':
						delete(intbuf);
						break;
					default :
						break;
				}
				break;
			case 'D':
				sendM.header = 'I';
				display(sendM.message);
				send(new_socket,&sendM,LEN,0);
				memset(sendM.message,'\0',MSGLEN);
				break;
		}
	}
	printf("Client terminated\n");
	pthread_exit(NULL);
} 

int main(){
	int server_fd, new_socket; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address);
	sf = open("department.dat",O_CREAT,S_IRWXO|S_IRWXU);
	close(sf);
	sf = open("student.dat",O_CREAT,S_IRWXO|S_IRWXU);
	close(sf);
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( PORT ); 
 
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	if (listen(server_fd, 3) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	}
	printf("Server wait client's request...\n");
	while(1){
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
		{ 
			perror("accept"); 
			exit(EXIT_FAILURE); 
		}
		printf("Client connected\n");
		pthread_t thread_id; 
		pthread_create(&thread_id, NULL, clientHandle, (void *)&new_socket); 
	}
	return 0; 
}
int add(char line[]){
	int key;
	int i=0;
	int j=0;
	int k=0;
	char buf[20] = "\0";		//store ID information
	char buf2[20] = "\0";		//store depID information
	//save ID, depID information
	while(1){
		if(line[i] == ';') break;
		if(j==0)
			buf[i]=line[i];
		if(fp==1&&j==4){
			buf2[k]=line[i];
			k++;
		}
		i++;
		if(line[i] == ',') {
			j++;
			i++;
		}
	}
	//check information overlab
	sscanf(buf,"%d",&key);
	memset(buf,'\0',20);
	i = findOffset(key);
	sprintf(buf,"%d",i);
	if(i>=0) return -1;

	//check department exist
	sscanf(buf2,"%d",&key);
	memset(buf2,'\0',20);
	if(fp==1){
		fp = 0;
		if(findOffset(key)<0) return -2;
		fp = 1;
	}
	if(fp == 0){
		sf=open("department.dat",O_WRONLY|O_APPEND);
	}
	else if(fp==1){
		sf=open("student.dat",O_WRONLY|O_APPEND);
	}
	char buffer;
	for(int i = 0;; i++){
		write(sf,&line[i],1);
		if(line[i]==';') break;
	}
	write(sf,"\0",1);
	write(sf,"\n",1);
	close(sf);
	return 1;
}

void delete(int offset){
	char buf2[1500]="\0";
	char buf3[1500]="\0";
	int size = lseek(sf,0,SEEK_END);
	lseek(sf,0,SEEK_SET);
	saveFileContext(offset, buf2, buf3);
	//after delete save information to buffer
	if(fp == 0){
		sf=open("department.dat",O_WRONLY|O_TRUNC);
	}
	else if(fp==1){
		sf=open("student.dat",O_WRONLY|O_TRUNC);
	}
	restoreInFile(offset,0,buf2);
	restoreInFile(offset,size,buf3);
	close(sf);
	return;
}
void search(char* send_message,int offset){
	struct Student std;
	struct department dep;
	if(fp == 1) {
		std = fillLine_s(offset);
		sprintf(send_message,"<Student Information>\nID : %d\nName : %s\nPhone_num : %d\nAddress : %s\nDepartment ID : %d\n",std.id,std.name,std.phone,std.address,std.depId);
	}
	else if(fp == 0) {
		dep = fillLine_d(offset);
		sprintf(send_message,"<Department Information>\nID : %d\nName : %s\nPhone_num : %d\nAddress : %s\n",dep.id,dep.name,dep.phone,dep.address);
	}
}

int findOffset(int key){
	if(fp == 0){
		sf=open("department.dat",O_RDONLY);
	}
	else if(fp==1){
		sf=open("student.dat",O_RDONLY);
	}
	struct Student std;
	struct department dep;
	f_offset = 0;
	int offset_buffer;
	int nread=1;
	char buffer[8];
	if(read(sf,buffer,1)<=0) return -1;
	while(nread){
		offset_buffer = f_offset;
		if(fp == 1) {
			std = fillLine_s(f_offset);
			if(std.id == key) return offset_buffer;
		}
		else if(fp == 0) {
			dep = fillLine_d(f_offset);
			if(dep.id==key) return offset_buffer;
		}
		read(sf,buffer,3);
		nread = read(sf,buffer,1);
		f_offset += 2;
	}
	close(sf);
	return -1;
}

void display(char *send_message){
	if(fp == 0){
		sf=open("department.dat",O_RDONLY);
	}
	else if(fp==1){
		sf=open("student.dat",O_RDONLY);
	}
	int nread=1;
	char buffer;
	char str[200]="\0";
	f_offset = 0;
	int i = 1;
	struct Student std;
	char string[1000] = "\0";
	struct department dep;
	char buf[5];
	int depoff;
	while(nread){
		if(fp == 1) {
			std = fillLine_s(f_offset);
			fp = 0;
			depoff = findOffset(std.depId);
			dep = fillLine_d(depoff);
			sprintf(str,"<Student %d>\nID : %d\nName : %s\nPhone_num : %d\nAddress : %s\nDepartment ID : %d\nDepartment name : %s\n",i,std.id,std.name,std.phone,std.address,std.depId,dep.name);
		}
		else if(fp == 0) {
			dep = fillLine_d(f_offset);
			sprintf(str,"<Department %d>\nID : %d\nName : %s\nPhone_num : %d\nAddress : %s\n",i,dep.id,dep.name,dep.phone,dep.address);
		}
		strcat(send_message,str);
		read(sf,buf,2);
		nread = read(sf,buf,1);
		i++;
		f_offset += 2;
		memset(str,'\0',200);
	}
	close(sf);
}

void update(int offset, char line[]){
	int size = lseek(sf,0,SEEK_END);
	lseek(sf,0,SEEK_SET);
	char buf2[1500]="\0";
	char buf3[1500]="\0";
	saveFileContext(offset, buf2, buf3);
	if(fp == 0){
		sf=open("department.dat",O_WRONLY|O_TRUNC);
	}
	else if(fp==1){
		sf=open("student.dat",O_WRONLY|O_TRUNC);
	}
	restoreInFile(offset,0,buf2);
	add(line);
	restoreInFile(offset,size,buf3);
	close(sf);
}

struct Student fillLine_s(int offset){
	struct Student s;
	lseek(sf,offset,SEEK_SET);
	char buffer[10]="\0";
	char ID_buffer[32]="\0";
	char phone_buffer[32]="\0";
	char name_buffer[20]="\0";
	char address_buffer[200]="\0";
	char dID_buffer[32]="\0";
	int count = 0;
	while(1){
		read(sf,&buffer,1);
		f_offset++;
		if(!strcmp(buffer," "))
			continue;
		else if(!strcmp(buffer,";")){
			break;
		}
		if(!strcmp(buffer,",")){
			count++;
			continue;
		}
		
		if(count == 0) strcat(ID_buffer,buffer);
		else if(count == 1)	strcat(name_buffer,buffer);
		else if(count == 2)	strcat(phone_buffer,buffer);
		else if(count == 3) strcat(address_buffer,buffer);
		else if(count == 4) strcat(dID_buffer,buffer);
	}
	sscanf(ID_buffer,"%d",&s.id);
	strcpy(s.name,name_buffer);
	sscanf(ID_buffer,"%d",&s.phone);
	strcpy(s.address,address_buffer);
	sscanf(dID_buffer,"%d",&s.depId);
	return s;
}

struct department fillLine_d(int offset){
	lseek(sf,offset,SEEK_SET);
	struct department s;
	char buffer[10]="\0";
	char ID_buffer[32]="\0";
	char phone_buffer[32]="\0";
	char name_buffer[20]="\0";
	char address_buffer[200]="\0";
	int count = 0;
	while(1){
		f_offset++;
		read(sf,&buffer,1);
		if(!strcmp(buffer," "))
			continue;
		else if(!strcmp(buffer,";")){
			break;
		}
		if(!strcmp(buffer,",")){
			count++;
			continue;
		}
		
		if(count == 0) strcat(ID_buffer,buffer);
		else if(count == 1) strcat(name_buffer,buffer);
		else if(count == 2) strcat(phone_buffer,buffer);
		else if(count == 3) strcat(address_buffer,buffer);
	}
	sscanf(ID_buffer,"%d",&s.id);
	strcpy(s.name,name_buffer);
	sscanf(phone_buffer,"%d",&s.phone);
	strcpy(s.address,address_buffer);
	return s;
}

void saveFileContext(int offset, char *buf2, char *buf3){
	if(fp == 0){
		sf=open("department.dat",O_RDONLY);
	}
	else if(fp==1){
		sf=open("student.dat",O_RDONLY);
	}
	int size = lseek(sf,0,SEEK_END);
	lseek(sf,0,SEEK_SET);
	char buf1[2]="\0";
	while(1){		//save information to buffer before line to be deleted
		if(lseek(sf,0,SEEK_CUR)==offset){
			break;
		}
		read(sf,&buf1,1);
		if(!strcmp(buf1,";")){
				lseek(sf,2,SEEK_CUR);		
		}
		strcat(buf2,buf1);
	}
	lseek(sf,1,SEEK_CUR);
	while(1){
		read(sf,&buf1,1);
		if(!strcmp(buf1,";")){
			read(sf,&buf1,1);
			break;
		}
	}
	lseek(sf,1,SEEK_CUR);
	while(1){
		read(sf,&buf1,1);
		if(lseek(sf,0,SEEK_CUR)==size)
				break;	
		strcat(buf3,buf1);
		if(!strcmp(buf1,";")){
			lseek(sf,2,SEEK_CUR);			
		}
	}
	close(sf);
}

void restoreInFile(int offset,int size, char buf[]){
	if(offset !=  size){
		int i = 0;
		while(1){
			if(buf[i]=='\0')break;
			if(buf[i]==';'){
				write(sf,&buf[i],1);
				write(sf,"\0",1);
				write(sf,"\n",1);
				i++;
				continue;
			}
			write(sf,&buf[i],1);
			i++;
		}
	}
}

