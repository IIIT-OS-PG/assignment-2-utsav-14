#include <unistd.h>  
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/stat.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <cstring> 
#include <iostream>
#include <pthread.h>
#include <cmath>
#include <vector>
#include <sstream>
#include <openssl/sha.h>

typedef unsigned long long ull;
#define BUFF_SIZE (512*1024)
#define LOCALHOST "127.0.0.1"
#define QUEUE_LENGTH 10
#define HASH_LENGTH 20
//#define PACKET_SIZE (2*1024)
const char* downloads_base_directory =  "./Downloads/";
const char* uploads_base_directory =  "./Uploads/";

using namespace std;

typedef struct {
	int* port_no;
}thread_args;

int setup_socket(){
	return socket(AF_INET, SOCK_STREAM, 0);
}

struct sockaddr_in setup_address(int port_no){
	struct sockaddr_in address; 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons(port_no);
	return address;
}

int bind_server(int server_fd, struct sockaddr_in address){
	return bind(server_fd, (struct sockaddr*)&address, sizeof(address));
} 

void set_filepath(char *file_path, char* buffer){
	strcpy(file_path, uploads_base_directory);
	strcat(file_path, buffer);
}

void send_file(char* file_path, char* buffer, int new_socket){
	FILE *f_in = fopen(file_path, "rb");
	if(!f_in)
	{
		perror("File couldn't be opened.");
		exit(EXIT_FAILURE);
	}	
		//Read file and send
	struct stat st;
	if(stat(file_path, &st) != 0)
	{
		perror("Error calculating the file size");
		exit(EXIT_FAILURE);
	}
	ull file_size = st.st_size;
	cout << "File size: " << file_size << endl;
	ull chunks = (ull)ceil((double)file_size/(BUFF_SIZE));
	sprintf(buffer, "%llu", file_size);
	write(new_socket, buffer, strlen(buffer));
	cout << "No. of chunks: " << chunks << endl;
	ull bytes_written = 0, total_written = 0;
	while(total_written < file_size)
	{	
		ull bytes_read = fread(buffer, sizeof(char), BUFF_SIZE, f_in);
		bytes_written = write(new_socket, buffer, bytes_read);
		total_written += bytes_written;
	}
	fclose(f_in);
} 

void* serve_request(void* new_socket){
	char buffer[BUFF_SIZE] = "\0"; 
	int socket = *((int*)new_socket);
	int bytes_read = read(socket , buffer, BUFF_SIZE); 
	if(!bytes_read)
	{
		cerr << "Error in reading.\n";
		exit(EXIT_FAILURE);
	}else
	{
		char* file_path = new char[200];
		set_filepath(file_path, buffer);
		cout << "File requested: " << file_path << endl;
		send_file(file_path, buffer, socket);
	}
	if(close(socket)){
		cerr << "Error while closing the socket.\n";
	}
	cout << "Request served..\n";
	pthread_exit(NULL);
}

void* start_server(void* args){
	// Creating socket file descriptor 
	thread_args* arg = (thread_args*)args;
	int port_no = *arg->port_no;
	free(args);
	int server_fd = setup_socket();
	if (server_fd  == 0) 
	{ 
		perror("Socket creation failed"); 
		exit(EXIT_FAILURE); 
	}
	struct sockaddr_in address = setup_address(port_no);  
	if (bind_server(server_fd, address) < 0) 
	{ 
		perror("Bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	cout << "Server listening...\n";
	if(listen(server_fd, QUEUE_LENGTH) < 0)
	{ 
		perror("Listen failed"); 
		exit(EXIT_FAILURE); 
	} 

	while (true) {
		
		int new_socket, addrlen = sizeof(address);
		if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen))<0) 
		{ 
			perror("Accept failed"); 
			exit(EXIT_FAILURE); 
		}
		cout << "Request received..\n";
		pthread_t service_thread;
		int ret_val_server = pthread_create(&service_thread, NULL, serve_request, (void*)&new_socket);
		if(ret_val_server){
			perror("Error creating server handler thread.");
			exit(EXIT_FAILURE);
		}else{
			cout << "Server handler thread created with ID: " << service_thread << endl;
		}
		pthread_detach(service_thread);
	}
	close(server_fd);
	cout << "Server fd Closed..";
	pthread_exit(NULL);
}

void download_file(int client_fd){
	char file_name[100] = "\0", buffer[BUFF_SIZE] = "\0";
	cin.get();
	cout << "Enter file name to download:\n";
	scanf("%[^\n]", file_name);
	write(client_fd , file_name , strlen(file_name)); 
	char file_path[200] = "\0";
	strcat(file_path, downloads_base_directory);
	strcat(file_path, file_name);
	FILE *f_out = fopen(file_path, "wb");
	if(!f_out)
	{
		perror("Could not open file.");
		exit(EXIT_FAILURE);
	}
	ull bytes_read = read(client_fd, buffer, BUFF_SIZE);
	ull file_size;
	sscanf(buffer, "%llu", &file_size);
	cout << "File size: " << file_size << endl;
	ull total_read = 0;
	while(total_read < file_size)
	{
		bytes_read = read(client_fd, buffer, BUFF_SIZE);
		if(!bytes_read){
			break;
		}
		fwrite(buffer, sizeof(char), bytes_read, f_out);
		total_read += bytes_read;
		cout << "total: " << total_read << endl;
	}
	fclose(f_out);
}

void upload_file(int client_fd){
	cout << "Under construction..\n";
}

void send_command_to_tracker(char* command, struct sockaddr_in address){
	int client_fd = setup_socket();
	if (client_fd  == 0) 
	{ 
		perror("Socket creation failed"); 
		exit(EXIT_FAILURE); 
	}
	if (connect(client_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{ 
		perror("Connection Failed."); 
		exit(EXIT_FAILURE); 
	}
	write(client_fd , command , strlen(command));
	char response[100] = "\0"; 
	int ret = read(client_fd, response, sizeof(response));
	if(!ret){
		cerr << "\nError in reading response.";
	}else{
		cout << "Response from tracker: " << response << endl;
	}
	close(client_fd);
}

void* start_client(void* args){
	int port_no = 8083;
	free(args);
	struct sockaddr_in address;
	address.sin_family = AF_INET; 
	address.sin_port = htons(port_no);  
	if(inet_pton(AF_INET, LOCALHOST, &address.sin_addr)<=0)
	{ 
		perror("Address not supported."); 
		exit(EXIT_FAILURE); 
	} 
	int client_fd;
	while(true){
		cout << "\n\t\t\t\tClient Program\n";
		//cout << "1-Download a file\n2-Upload a file\n3-Exit\nEnter your choice: ";
		cout << "Enter command > ";
		cin.get();
		char comm[200] = "\0";
		scanf("%[^\n]", comm);
		cout << "Command: " << comm << endl;
		string command(comm);
		stringstream tokenizer(comm);
		vector<string> params;
		string param;
		while(getline(tokenizer, param, ' ')){
			params.push_back(param);
		}
		string command_name = params[0];
		if(command_name == "create_user"){
			if(params.size() != 3){
				cerr << "Exactly 2 arguements required: (userid, password)\n";
				continue;
			}
			//cout << "create user\n";
			//cout << "userid: " << params[1] << endl;
			//cout << "password: " << params[2] << endl;
			//unsigned char hash[HASH_LENGTH]="\0";
			//SHA1((const unsigned char*)params[2].c_str(), params[2].size(), hash);
			//cout << "hash generated: " << hash << endl;
			//send_command_to_tracker(comm, address);
		}else if(command_name == "login"){
			cout << "login\n";
			cout << "userid: " << params[1] << endl;
			cout << "password: " << params[2] << endl;
			// unsigned char hash[HASH_LENGTH]="\0";
			// SHA1((const unsigned char*)params[2].c_str(), params[2].size(), hash);
			// char comm2[200] = "\0";
			// strcpy(comm2, command_name.c_str());
			// strcat(comm2, " ");
			// strcat(comm2, params[1].c_str());
			// strcat(comm2, " ");
			// strcat(comm2, (char *)hash);
			// cout << "hash generated: " << hash << endl;
			// cout << "Sending for login: " << comm2 << endl;
			send_command_to_tracker(comm, address);
		}else if(command_name == "create_group"){
			cout << "create group\n";
		}else if(command_name == "join_group"){
			cout << "join group\n";
		}else if(command_name == "leave_group"){
			cout << "leave group\n";
		}else if(command_name == "list_requests"){
			cout << "list requests\n";
		}else if(command_name == "accept_request"){
			cout << "accept request\n";
		}else if(command_name == "list_groups"){
			cout << "list groups\n";
		}else if(command_name == "list_files"){
			cout << "list files\n";
		}else if(command_name == "upload_file"){
			cout << "upload file\n";
		}else if(command_name == "download_file"){
			cout << "downloading..\n";
		}else if(command_name == "logout"){
			cout << "logout\n";
		}else if(command_name == "show_downloads"){
			cout << "show downloads\n";
		}else if(command_name == "stop_share"){
			cout << "stop share\n";
		}else{
			cout << "wrong command\n";
		}
	// 	cin >> op;
	// 	switch(op){
	// 		case 1 : {
	// 				client_fd = setup_socket();
	// 				if (client_fd  == 0) 
	// 				{ 
	// 					perror("Socket creation failed"); 
	// 					exit(EXIT_FAILURE); 
	// 				}
	// 				if (connect(client_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	// 				{ 
	// 					perror("Connection Failed."); 
	// 					exit(EXIT_FAILURE); 
	// 				}
	// 			download_file(client_fd);
	// 			close(client_fd);
	// 			break;
	// 		}
	// 		case 2 : {
	// 			upload_file(client_fd);
	// 			break;
	// 		}
	// 		case 3 : {
	// 			cout << "Client exiting..\n";
	// 			break;
	// 		}
	// 		default : cout << "Wrong option\n";
	// 		break;
	// 	}
	// }while(op!=3);
	}
	close(client_fd);
	pthread_exit(NULL);
}

int main(int argc, char** argv) 
{ 
	if(argc < 2)
	{
		cerr << "Too few arguements";
		exit(EXIT_FAILURE);
	}
	pthread_t server_thread;
	thread_args *server_args = (thread_args*)malloc(sizeof(thread_args));
	int port_no;
	sscanf(argv[1], "%d", &port_no);
	server_args->port_no = &port_no;
	int ret_val_server = pthread_create(&server_thread, NULL, start_server, (void*)server_args);
	if(ret_val_server){
		perror("Error creating server thread.");
		free(server_args);
		return ret_val_server;
	}else{
		cout << "Server thread created with ID: " << server_thread << endl;
	}
	pthread_t client_thread;
	thread_args *client_args = (thread_args*)malloc(sizeof(thread_args));
	client_args->port_no = &port_no;
	int ret_val_client = pthread_create(&client_thread, NULL, start_client, (void*)client_args);
	if(ret_val_client){
		perror("Error creating client thread.");
		free(client_args);
		return ret_val_client;
	}else{
		cout << "Client thread created with ID: " << client_thread << endl;
	}
	void *server_ptr = NULL;
	ret_val_server = pthread_join(server_thread, &server_ptr);
	if(ret_val_server){
		perror("Failed to join server thread.");
		return ret_val_server;
	}else{
		cout << "Server returned value: " << *(int *)server_ptr << endl;
	}
	return 0; 
} 
