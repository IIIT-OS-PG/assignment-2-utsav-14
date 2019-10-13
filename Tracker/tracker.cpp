#include <unistd.h>  
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <cmath>
#include <vector>
#include <sstream>
#include <fstream>
#include "../Headers/user.h"
#include "../Headers/group.h"

typedef unsigned long long ull;
#define BUFF_SIZE (512*1024)
#define LOCALHOST "127.0.0.1"
#define QUEUE_LENGTH 10
vector<user> logged_in_users;

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

int login(vector<string> params){
	ifstream f_in("users");
	if(!f_in){
		cerr << "Users file couldn't be opened.\n";
		return 1;
	}
	user new_user;
	new_user.user_ID = params[1];
	new_user.pwd = params[2];
	user temp_user;
	vector<user> logged_in = logged_in_users;
	for(user u : logged_in){
		if(u == new_user) {
			cout << "User already logged in.\n";
			f_in.close();
			return 2;
		}
	}
	while(f_in >> temp_user){
		if(temp_user.user_ID == new_user.user_ID){
			if(temp_user.pwd == new_user.pwd){
				cout << "Logged in successfully.\n";
				logged_in_users.push_back(temp_user);
				f_in.close();
				return 0;
			}else{
				cerr << "Wrong password.\n";
				f_in.close();
				return 3;
			}
		}
	}
	cerr << "User does not exist.\n";
	f_in.close();
	return 4;
}

int create_user(vector<string> params){
	ofstream f_out("users", ios::app);
		ifstream f_in("users");
		if(!f_in || !f_out){
			cerr << "Error: Users file couldn't be opened.\n";
			return 1;
		}
		user new_user;
		new_user.user_ID = params[1];
		new_user.pwd = params[2];
		new_user.group_ID="-1";
		cout << "new user: ";
		new_user.print();
		user temp_user;
		while(f_in >> temp_user){
			if(temp_user.user_ID == new_user.user_ID){
				cerr << "UserID already exists.\n";
				f_in.close();
				f_out.close();
				return 2;
			}
		}
		if(!(f_out << new_user)){
			cerr << "Error writing record in user file.\n";
			f_in.close();
			f_out.close();
			return 3;
		}
		cout << "User created successfully.";
		f_in.close();
		f_out.close();
		return 0;
}

int create_group(vector<string> params){
	vector<user> logged_in = logged_in_users;
	ofstream f_out("groups", ios::app);
	ifstream f_in("groups");
	if(!f_in || !f_out){
		cerr << "Error: Groups file couldn't be opened.\n";
		return 1;
	}
	group new_grp;
	new_grp.group_ID = params[1];
	new_grp.owner_user_ID = params[2];
	cout << "new group: ";
	new_grp.print();
	group temp_grp;
	while(f_in >> temp_grp){
		if(temp_grp.group_ID == new_grp.group_ID){
			cerr << "GroupID already exists.\n";
			f_in.close();
			f_out.close();
			return 2;
		}
	}
	if(!(f_out << new_grp)){
		cerr << "Error writing record in group file.\n";
		f_in.close();
		f_out.close();
		return 3;
	}
	cout << "Group created successfully.";
	f_in.close();
	f_out.close();
	return 0;
}

int execute_command(vector<string> params){
	string command_name = params[0]/*, response = "generic response"*/;
	if(command_name == "create_user"){
		cout << "creating user:\n";
		return create_user(params);
	}else if(command_name == "login"){
		cout << "login\n";
		return login(params);
	}else if(command_name == "create_group"){
		cout << "create group\n";
		return create_group(params);
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
	return 0;
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
		int return_val = 5;
		return_val = execute_command(params);
		cout << "returning: " << return_val << endl;
		int ret = write(socket, &return_val, sizeof(return_val));
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
