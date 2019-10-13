#include <unistd.h>  
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <cmath>
#include <vector>
#include <sstream>
#include <fstream>
#include "user.h"

typedef unsigned long long ull;
#define BUFF_SIZE (512*1024)
#define LOCALHOST "127.0.0.1"
#define QUEUE_LENGTH 10

typedef struct {
	char* command;
}thread_args;

typedef struct {
	in_port_t port;				 // Port number.  
    struct in_addr ip_addr;		// IP address.
}socket_addr;

typedef struct {
	string SHA1;
	vector<socket_addr> suppliers;
	ull size;
	bool is_shared;
}file;

typedef struct {
	string group_ID;
	string user_ID;
	vector<string> members;
	vector<file> files_shared;
}group;



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

string execute_command(vector<string> params){
	string command_name = params[0], response = "generic response";
	if(command_name == "create_user"){
		cout << "creating user:\n";
		ofstream f_out("users", ios::app);
		ifstream f_in("users");
		if(!f_in || !f_out){
			cerr << "Users file couldn't be opened.\n";
			response = "Error: Users file couldn't be opened.";
			return response;
		}
		user new_user;
		new_user.user_ID = params[1];
		new_user.pwd = params[2];
		new_user.group_ID="-1";
		cout << "new user: ";
		new_user.print();
		user temp_user;
		while(f_in >> temp_user){
			//cout << "read:\n";
			//temp_user.print();
			if(temp_user == new_user){
				cerr << "UserID already exists.\n";
				response = "Error: UserID already exists.";
				f_in.close();
				f_out.close();
				return response;
			}
		}
		if(!(f_out << new_user)){
			cerr << "Error writing record in user file.\n";
			response = "Error writing record in user file.";
			f_in.close();
			f_out.close();
			return response;
		}
		cout << "User created successfully.";
		f_in.close();
		f_out.close();
		response =  "User created successfully.";
		return response;
	}else if(command_name == "login"){
		cout << "login\n";
		ifstream f_in("users");
		if(!f_in){
			cerr << "Users file couldn't be opened.\n";
			response = "Error: Users file couldn't be opened.";
			return response;
		}
		user new_user;
		new_user.user_ID = params[1];
		new_user.pwd = params[2];
		user temp_user;
		while(f_in >> temp_user){
			//cout << "read:\n";
			//temp_user.print();
			if(temp_user == new_user){
				if(temp_user.pwd == new_user.pwd){
					cout << "Logged in successfully.\n";
					response = "Success: Logged in successfully.";
					f_in.close();
					return response;
				}else{
					cerr << "Wrong password.\n";
					response = "Error: Wrong password.";
					f_in.close();
					return response;
				}
			}
		}
		cerr << "User does not exist.\n";
		response = "Error: User does not exist.";
		f_in.close();
		return response;
	}else if(command_name == "create_group"){
		cout << "create group\n";
	}else if(command_name == "join_group"){
		cout << "join group\n";
	}else if(command_name == "leave_group"){
		cout << "leave group\n";
	}/*else if(command_name == "list_requests"){
		cout << "list requests\n";
	}else if(command_name == "accept_request"){
		cout << "accept request\n";
	}*/else if(command_name == "list_groups"){
		cout << "list groups\n";
	}else if(command_name == "list_files"){
		cout << "list files\n";
	}else if(command_name == "upload_file"){
		cout << "upload file\n";
	}else if(command_name == "download_file"){
		cout << "downloading..\n";
	}else if(command_name == "logout"){
		cout << "logout\n";
	}/*else if(command_name == "show_downloads"){
		cout << "show downloads\n";
	}else if(command_name == "stop_share"){
		cout << "stop share\n";
	}*/else{
		cout << "wrong command\n";
	}
	return response;
}

void* serve_request(void* new_socket){
	//do stuff
	char buffer[BUFF_SIZE] = "\0"; 
	int socket = *((int*)new_socket);
	int bytes_read = read(socket , buffer, BUFF_SIZE); 
	if(!bytes_read)
	{
		cerr << "Error in reading.\n";
		exit(EXIT_FAILURE);
	}else
	{
		cout << "Command: " << buffer << endl;
		string command(buffer);
		stringstream tokenizer(command);
		vector<string> params;
		string param;
		while(getline(tokenizer, param, ' ')){
			params.push_back(param);
		}
		//Check login status first
		string return_val = execute_command(params);
		int ret = write(socket, return_val.c_str(), return_val.size());
		if(!ret){
			cerr << "\nError in writing response.";
		}
	}
	if(close(socket)){
		cerr << "Error while closing the socket.\n";
	}
	cout << "Request served..\n";
	pthread_exit(NULL);
}

void start_tracker(int port_no){
	// Creating socket file descriptor 
	int tracker_fd = setup_socket();
	if (tracker_fd  == 0) 
	{ 
		perror("Socket creation failed"); 
		exit(EXIT_FAILURE); 
	}
	struct sockaddr_in address = setup_address(port_no);  
	if (bind_server(tracker_fd, address) < 0) 
	{ 
		perror("Bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	cout << "Tracker listening...\n";
	if(listen(tracker_fd, QUEUE_LENGTH) < 0)
	{ 
		perror("Listen failed"); 
		exit(EXIT_FAILURE); 
	} 
	while (true) {
		
		int new_socket, addrlen = sizeof(address);
		if ((new_socket = accept(tracker_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen))<0) 
		{ 
			perror("Accept failed"); 
			exit(EXIT_FAILURE); 
		}
		cout << "Request received..\n";
		pthread_t service_thread;
		int ret_val_tracker = pthread_create(&service_thread, NULL, serve_request, (void*)&new_socket);
		if(ret_val_tracker){
			perror("Error creating tracker handler thread.");
			exit(EXIT_FAILURE);
		}else{
			cout << "Tracker handler thread created with ID: " << service_thread << endl;
		}
		pthread_detach(service_thread);
	}
	close(tracker_fd);
	cout << "Tracker fd Closed..";
	pthread_exit(NULL);
}


int main(int argc, char** argv) 
{ 
	if(argc < 2)
	{
		cerr << "Too few arguements";
		exit(EXIT_FAILURE);
	}
	int port_no;
	sscanf(argv[1], "%d", &port_no);
	start_tracker(port_no);
	return 0; 
} 
