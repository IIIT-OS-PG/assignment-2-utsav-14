#include <unistd.h>  
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/stat.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <pthread.h>
#include <cmath>
#include <sstream>
#include <unordered_map>
#include <openssl/sha.h>
#include "../Headers/user.h"
#include "../Headers/constants.h"
#include "../Headers/common_headers.h"

typedef unsigned long long ull;
#define BUFF_SIZE (512*1024)
#define QUEUE_LENGTH 10
#define HASH_LENGTH 20
#define MAX_COMMAND_SIZE 50000
//#define PACKET_SIZE (2*1024)
const char* downloads_base_directory =  "./Downloads/";
const char* uploads_base_directory =  "./Uploads/";
unordered_map<string, vector<string>> join_requests;
vector<string> owned_groups;
using namespace std;

typedef struct {
	int* port_no;
	char* tracker_file;
	sockaddr_in address;
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
		string command(buffer);
		stringstream tokenizer(command);
		vector<string> params;
		string param;
		while(getline(tokenizer, param, ' ')){
			params.push_back(param);
		}
		string command_name = params[0];
		if(command_name == "join"){
			auto it = join_requests.find(params[1]);
			if(it == join_requests.end()){
				vector<string> req;
				req.push_back(params[2]);
				join_requests.insert(make_pair(params[1], req));
			}else{
				it->second.push_back(params[2]);
			}
			cout << "Join request added.\n";
		}else{	
			char* file_path = new char[200];
			set_filepath(file_path, buffer);
			cout << "File requested: " << file_path << endl;
			send_file(file_path, buffer, socket);
		}
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
	int opt = 1; 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    { 
        perror("Setsockopt"); 
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

int send_command_to_tracker(char* command, struct sockaddr_in address){
	int client_fd = setup_socket();
	if (client_fd  == 0) 
	{ 
		perror("Socket creation failed"); 
	}
	if (connect(client_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{ 
		perror("Connection Failed."); 
	}
	write(client_fd , command , strlen(command));
	int response = -100; 
	int ret = read(client_fd, &response, sizeof(response));
	if(!ret){
		cerr << "\nError in reading response.";
	}
	close(client_fd);
	return response;
}

vector<string> find_owned_groups(string user_ID, struct sockaddr_in address){
	char command[100] = "\0";
	strcpy(command, "find_owned_groups ");
	strcat(command, user_ID.c_str());
	int client_fd = setup_socket();
	if (client_fd  == 0) 
	{ 
		perror("Socket creation failed"); 
	}
	if (connect(client_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{ 
		perror("Connection Failed."); 
	}
	write(client_fd , command , strlen(command));
	vector<string> response;
	char res[10] = "\0";
	while(read(client_fd, &res, sizeof(res))){
		string owned_ID(res);
		response.push_back(owned_ID);
	}
	close(client_fd);
	return response;
}

vector<string> find_all_groups(struct sockaddr_in address){
	char command[100] = "\0";
	strcpy(command, "find_all_groups");
	int client_fd = setup_socket();
	if (client_fd  == 0) 
	{ 
		perror("Socket creation failed"); 
	}
	if (connect(client_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{ 
		perror("Connection Failed."); 
	}
	write(client_fd , command , strlen(command));
	vector<string> response;
	char res[100] = "\0";
	uint* size = new uint;
	int ret = read(client_fd, size, sizeof(size));
	if(!ret){
		cout << "Error reading the number of groups.\n";
		return response;
	}
	//cout << "size: " << *size << endl;
	while(read(client_fd, &res, sizeof(res))){
		string grp(res);
		response.push_back(grp);
	}
	close(client_fd);
	return response;
}

ull get_file_size(string file_path){
	struct stat st;
	ull file_size = 0LLU;
	if(stat(file_path.c_str(), &st) != 0)
	{
		perror("Error calculating the file size");
		return file_size;
	}
	if(S_ISDIR(st.st_mode)){
		cerr << file_path << " is a directory.\n";
		return file_size;
	}
	file_size = st.st_size;
	return file_size;
}

string get_file_sha(string file_path){
	ull file_size = get_file_size(file_path);
	string sha1;
	FILE *f_in = fopen(file_path.c_str(), "r");
	if(!f_in){
		cerr << "Couldn't open the file.\n";
		return sha1;
	}
	cout << "File opened successfully.\n";
	ull chunks = (ull)ceil((double)file_size/(BUFF_SIZE));
	cout << "File size: " << file_size << " Chunks: " << chunks << endl;
	for(ull i = 0; i < chunks; ++i){
		char chunk_buffer[BUFF_SIZE] = "\0";
		int bytes_read = fread(chunk_buffer, sizeof(char), BUFF_SIZE, f_in);
		if(!bytes_read){
			cerr << "Error while reading file.\n";
			return sha1;
		}
		unsigned char chunk_hash[HASH_LENGTH]="\0";
		SHA1((const unsigned char*)chunk_buffer, bytes_read, chunk_hash);
		//cout << "hash generated for chunk " << i << ": " << chunk_hash << endl;
		string hash_str((char*)chunk_hash);
		sha1 += hash_str;
	}
	fclose(f_in);
	return sha1;
}

void* start_client(void* args){
	
	thread_args* arg = (thread_args*)args;
	char* file = arg->tracker_file;
	int port_no = *arg->port_no;
	free(args);
	ifstream f_in(file);
	if(!f_in){
		cerr << "Error: " << file << " file couldn't be opened.\n";
		pthread_exit(NULL);
	}
	string ip, port;
	getline(f_in, ip);
	getline(f_in, port);
	int server_port;
	sscanf(port.c_str(), "%d", &server_port);
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(server_port);
	if(inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0)
	{ 
		perror("Address not supported."); 
		exit(EXIT_FAILURE); 
	} 
	char userID[20] = "\0";
	while(true){
		cout << "\n\t\t\t\tClient Program\n";
		cout << "Enter command > ";
		cin.get();
		char comm[200] = "\0"; //For small commands ie no SHA1
		scanf("%[^\n]", comm);
		if(strlen(comm) == 0){
			continue;
		}
		string command(comm);
		stringstream tokenizer(command);
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
			int ret = send_command_to_tracker(comm, address);
			if(ret >= 0){
				cout << create_user_code_to_string(ret);
			}
		}else if(command_name == "login"){
			cout << "login\n";
			if(params.size() != 3){
				cerr << "Exactly 2 arguements required: (userid, password)\n";
				continue;
			}
			if(strlen(userID) > 0){
				cerr << "You must logout first.\n";
				continue;
			}
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
			char port[10] = "\0";
			sprintf(port, "%d", port_no);
			strcat(comm, " ");
			strcat(comm, port);
			int ret = send_command_to_tracker(comm, address);
			if(ret >= 0){
				cout << login_code_to_string(ret);
				if(ret==0 || ret == 2){
					strcpy(userID, params[1].c_str());
				}
			}
		}else if(command_name == "create_group"){
			cout << "create group\n";
			if(params.size() != 2){
				cerr << "Exactly 1 arguement required: (groupid)\n";
				continue;
			}
			if(strlen(userID) == 0){
				cerr << "You must login first.\n";
				continue;
			}
			strcat(comm, " ");
			strcat(comm, userID);
			int ret = send_command_to_tracker(comm, address);
			if(ret >= 0){
				cout << create_group_code_to_string(ret);
			}
		}else if(command_name == "join_group"){
			cout << "join group\n";
			if(params.size() != 2){
				cerr << "Exactly 1 arguement required: (groupid)\n";
				continue;
			}
			if(strlen(userID) == 0){
				cerr << "You must login first.\n";
				continue;
			}
			strcat(comm, " ");
			strcat(comm, userID);
			int ret = send_command_to_tracker(comm, address);
			if(ret >= 0){
				cout << join_group_code_to_string(ret);
			}
		}else if(command_name == "leave_group"){
			cout << "leave group\n";
			if(params.size() != 2){
				cerr << "Exactly 1 arguement required: (groupid)\n";
				continue;
			}
			if(strlen(userID) == 0){
				cerr << "You must login first.\n";
				continue;
			}
			strcat(comm, " ");
			strcat(comm, userID);
			int ret = send_command_to_tracker(comm, address);
			if(ret >= 0){
				cout << leave_group_code_to_string(ret);
				if(ret == 1){
					join_requests.erase(params[1]); //Group deleted => remove all join requests for it
				}
			}
		}else if(command_name == "list_requests"){
			cout << "list requests\n";
			if(params.size() != 2){
				cerr << "Exactly 1 arguement required: (groupid)\n";
				continue;
			}
			if(strlen(userID) == 0){
				cerr << "You must login first.\n";
				continue;
			}
			auto it = join_requests.find(params[1]);
			if(it != join_requests.end()){
				bool owned = false;
				owned_groups = find_owned_groups(userID, address);
				for(string owned_gID : owned_groups){
					if(owned_gID == params[1]){ 		//if requested groupID is in owned list
						owned = true;
						break;
					}
				}
				if(owned){
					cout << "USER_ID:\n";
					vector<string> req = it->second;
					for(string userID : req){
						cout << userID << endl;
					}
				}else{
					cout << "Logged in user doesn't own this groupID.\n";
				}
			}else{
				cout << "GroupID not present/ GroupID doesn't have pending requests.\n";
			}
		}else if(command_name == "accept_request"){
			cout << "accept request\n";
			if(params.size() != 3){
				cerr << "Exactly 2 arguements required: (groupid, userid)\n";
				continue;
			}
			if(strlen(userID) == 0){
				cerr << "You must login first.\n";
				continue;
			}
			bool owned = false;
			owned_groups = find_owned_groups(userID, address);
			for(string owned_gID : owned_groups){
				if(owned_gID == params[1]){ 		//if requested groupID is in owned list
					owned = true;
					break;
				}
			}
			if(!owned){
				cout << "User doesn't own this group./ Group doesn't exist.\n";
				continue;
			}
			auto req = join_requests.find(params[1]);
			if(req == join_requests.end()){
				cout << "Group doesn't have pending requests./ Group doesn't exist.\n";
				continue;
			}
			vector<string> reqs = req->second;
			bool req_present = false;
			for(string userid : reqs){
				if(userid == params[2]){
					req_present = true;
					break;
				}
			}
			if(!req_present){
				cout << "UserID not present in pending requests.\n";
				continue;
			}
			int ret = send_command_to_tracker(comm, address);
			if(ret >= 0){
				if(ret == 1){
					cout << "Group doesn't exist.\n";
				}
			}
		}else if(command_name == "list_groups"){
			cout << "list groups\n";
			if(strlen(userID) == 0){
				cerr << "You must login first.\n";
				continue;
			}
			vector<string> all_groups = find_all_groups(address);
			cout << "GroupID\tOwnerID\n";
			for(string g : all_groups){
				cout << g << endl;
			}
		}else if(command_name == "list_files"){
			cout << "list files\n";
		}else if(command_name == "upload_file"){
			cout << "upload file\n";
			if(params.size() != 3){
				cerr << "Exactly 2 arguements required: (filepath, groupid)\n";
				continue;
			}
			if(strlen(userID) == 0){
				cerr << "You must login first.\n";
				continue;
			}
			char big_command[MAX_COMMAND_SIZE] = "\0";
			strcpy(big_command, comm);
			strcat(big_command, " ");
			strcat(big_command, userID);
			string file_sha = get_file_sha(params[1]);
			if(file_sha.size() == 0){
				continue;
			}
			//cout << "File sha: " << file_sha_and_size << endl;
			strcat(big_command, " ");
			strcat(big_command, file_sha.c_str());
			ull file_size = get_file_size(params[1]);
			char size[40] = "\0";
			sprintf(size, "%llu", file_size);
			//cout << "Size string: " << size << endl;
			strcat(big_command, " ");
			strcat(big_command, size);
			int ret = send_command_to_tracker(big_command, address);
			if(ret >= 0){
				cout << upload_file_code_to_string(ret);
			}
		}else if(command_name == "download_file"){
			cout << "downloading..\n";
		}else if(command_name == "logout"){
			cout << "logout\n";
			if(strlen(userID) == 0){
				cerr << "You must login first.\n";
				continue;
			}
			strcat(comm, " ");
			strcat(comm, userID);
			int ret = send_command_to_tracker(comm, address);
			if(ret == 0){
				cout << userID << " logged out.\n";
			}
			strcpy(userID,"\0");

		}else if(command_name == "show_downloads"){
			cout << "show downloads\n";
		}else if(command_name == "stop_share"){
			cout << "stop share\n";
		}else{
			cout << "wrong command\n";
		}
	}
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
	client_args->tracker_file = argv[2];
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
