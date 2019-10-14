#include <unistd.h>  
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <cmath>
#include <sstream>
#include <fstream>
#include "../Headers/common_headers.h"
#include "../Headers/user.h"
#include "../Headers/group.h"

typedef unsigned long long ull;
#define BUFF_SIZE (512*1024)
#define LOCALHOST "127.0.0.1"
#define QUEUE_LENGTH 10
vector<user> logged_in_users;
vector<group> groups;

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

int login(vector<string> params, int socket){ // 1:userID , 2: pwd, 3:peer_port
	ifstream f_in("users");
	if(!f_in){
		cerr << "Users file couldn't be opened.\n";
		return 1;
	}
	user new_user;
	new_user.user_ID = params[1];
	new_user.pwd = params[2];
	int client_port;
	sscanf(params[3].c_str(), "%d", &client_port);
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
		temp_user.print();
		if(temp_user.user_ID == new_user.user_ID){
			if(temp_user.pwd == new_user.pwd){
				cout << "Logged in successfully.\n";
				struct sockaddr_in peer_address;
				int addrlen = sizeof(peer_address);
				getpeername(socket, (struct sockaddr*)&peer_address, (socklen_t*)&addrlen);
				temp_user.address = peer_address;
				temp_user.address.sin_port = htons(client_port);
				temp_user.isLoggedIn = true;
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
	ofstream f_out("groups", ios::app);
	ifstream f_in("groups");
	if(!f_in || !f_out){
		cerr << "Error: Groups file couldn't be opened.\n";
		return 1;
	}
	group new_grp;
	new_grp.group_ID = params[1];
	new_grp.owner_user_ID = params[2];
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
	new_grp.members.push_back(params[2]);
	groups.push_back(new_grp);
	f_in.close();
	f_out.close();
	vector<user> usrs = logged_in_users;
	for(uint i = 0; i < usrs.size(); ++i){
		if(usrs[i].user_ID == params[2]){	//if found the owner of group, then add the group to its group list
			logged_in_users[i].groupIDs.push_back(params[1]);
			break;
		}
	}
	return 0;
}

int request_join(user owner, string groupID, string userID){
	struct sockaddr_in address = owner.address;  
	int tracker_fd = setup_socket();
	if (tracker_fd  == 0) 
	{ 
		perror("Socket creation failed"); 
	}
	if (connect(tracker_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{ 
		perror("Connection Failed."); 
	}
	char command[200]= "\0";
	strcpy(command, "join ");
	strcat(command, groupID.c_str());
	strcat(command, " ");
	strcat(command, userID.c_str());
	write(tracker_fd , command , strlen(command));
	close(tracker_fd);
	return 0;
}

void init_groups(){
	ifstream f_in("groups");
	if(!f_in){
		cerr << "Error: Groups file couldn't be opened.\n";
		return;
	}
	group temp_grp;
	while(f_in >> temp_grp){
		temp_grp.members.push_back(temp_grp.owner_user_ID);
		groups.push_back(temp_grp);
	}
	f_in.close();
}

int join_group(vector<string> params){ // 1:groupID , 2: userID
	if(groups.empty()){
		init_groups();
	}
	vector<group> grp = groups;
	for(group g : grp){
		cout << "checking:\n";
		if(g.group_ID == params[1]){
			for(string user : g.members){
				if(user == params[2]){
					//user is aready a member of this group
					cout << "User is already a member of this group.\n";
					return 1;
				}
			}
			vector<user> usrs = logged_in_users;
			user grp_owner;
			for(user u : usrs){
				if(u.user_ID == g.owner_user_ID){
					grp_owner = u;
					cout << "Group join requested.\n";
					request_join(grp_owner, params[1], params[2]);		//Send request to group owner
					return 0;
				}
			}
			cerr << "Can't send join request. Group owner is not online.\n";
			return 2;
		}
	}
	cerr << "No such group exists.";
	return 3;
}

vector<string> find_owned_groups(string userID){
	ifstream f_in("groups");
	vector<string> g_IDs;
	if(!f_in){
		cerr << "Error: Groups file couldn't be opened.\n";
		return g_IDs;
	}
	group temp_grp;
	while(f_in >> temp_grp){
		if(temp_grp.owner_user_ID == userID){
			g_IDs.push_back(temp_grp.group_ID);
		}
	}
	f_in.close();
	return g_IDs;
}

vector<group> find_all_groups(){
	ifstream f_in("groups");
	vector<group> grps;
	if(!f_in){
		cerr << "Error: Groups file couldn't be opened.\n";
		return grps;
	}
	group temp_grp;
	while(f_in >> temp_grp){
		grps.push_back(temp_grp);
	}
	cout << "sending " << grps.size() << " groups\n";
	f_in.close();
	return grps;
}

void promote_member(string grpID, string usrID){
	ifstream f_in("groups");
	ofstream f_out("groups", ios::out);
	if(!f_in || !f_out){
		cerr << "Error: Groups file couldn't be opened.\n";
	}
	group temp_grp;
	while(f_in >> temp_grp){
		if(temp_grp.group_ID == grpID){
			temp_grp.owner_user_ID = usrID;
		}
		if(!(f_out << temp_grp)){
			cerr << "Error writing record in group file.\n";
		}
	}
	cout << "User promoted.\n";
	f_in.close();
	f_out.close();
}

int delete_group_from_file(string groupID){
	ifstream f_in("groups");
	ofstream f_out("groups", ios::out);
	if(!f_in || !f_out){
		cerr << "Error: Groups file couldn't be opened.\n";
		return 5;
	}
	group temp_grp;
	while(f_in >> temp_grp){
		if(temp_grp.group_ID != groupID){
			if(!(f_out << temp_grp)){
				cerr << "Error writing record in group file.\n";
			}
		}
	}
	cout << "Group deleted.\n";
	f_in.close();
	f_out.close();
	return 1;
}

int leave_group(vector<string> params){
	vector<user> usrs = logged_in_users;
	for(uint i = 0; i < usrs.size(); ++i){
		if(usrs[i].user_ID == params[2]){
			vector<string> gIDs = usrs[i].groupIDs;
			for(uint j = 0; j < gIDs.size(); ++j){
				if(gIDs[j] == params[1]){
					logged_in_users[i].groupIDs.erase(gIDs.begin() + j);
				}
			}
		}
	}
	vector<group> grps = groups;
	for(uint i = 0; i < grps.size(); ++i){
		if(grps[i].group_ID == params[1]){	//groupID found
			if(grps[i].owner_user_ID == params[2]){		//owner is leaving
				cout << "Deleting owner\n";
				cout << "group members:\n";
				for(string m : grps[i].members){
					cout << m <<",";
				}
				cout << endl;
				if(grps[i].members.size() > 1){
					string new_owner = grps[i].members[1];
					cout << new_owner << " promoted to group owner\n";
					promote_member(params[1] ,new_owner);
					groups[i].owner_user_ID = new_owner;
					groups[i].members.erase(groups[i].members.begin()); //Delete the previous owner from members list
					cout << "Deleted owner.\n";
					return 0;
				}else{
					groups.erase(groups.begin() + i);
					//Delete group from file
					return delete_group_from_file(params[1]);
				}
			}
			vector<string> mems = grps[i].members;
			for(uint j = 0; j < mems.size(); ++j){
				if(mems[j] == params[2]){
					mems.erase(mems.begin() + j);
					cout << "Member deleted.\n";
					return 2;
				}
			}
			cout << "User is not a part of this group.\n";
			return 3;
		}
	}
	cout << "No such group exists.\n";
	return 4;
}

int accept_request(vector<string> params){
	vector<user> usrs = logged_in_users;
	for(uint i = 0; i < usrs.size(); ++i){
		if(usrs[i].user_ID == params[2]){
			logged_in_users[i].groupIDs.push_back(params[1]);
		}
	}
	vector<group> grps = groups;
	for(uint i = 0; i < grps.size(); ++i){
		if(grps[i].group_ID == params[1]){
			groups[i].members.push_back(params[2]);
			cout << "User accepted to group.\n";
			return 0;
		}
	}
	cout << "Group doesn't exist.\n";
	return 1;
}

int logout(vector<string> params){
	vector<user> usrs = logged_in_users;
	for(uint i = 0; i < usrs.size(); ++i){
		if(usrs[i].user_ID == params[1]){
			logged_in_users[i].isLoggedIn = false;
			cout << params[1] << " logged out.\n";
			return 0;
		}
	}
	cerr << params[1] << " isn't logged in.\n";
	return 1;
}

int execute_command(vector<string> params, int socket){
	string command_name = params[0]/*, response = "generic response"*/;
	if(command_name == "create_user"){
		cout << "creating user:\n";
		return create_user(params);
	}else if(command_name == "login"){
		cout << "login\n";
		return login(params, socket);
	}else if(command_name == "create_group"){
		cout << "create group\n";
		return create_group(params);
	}else if(command_name == "join_group"){
		cout << "join group\n";
		return join_group(params);
	}else if(command_name == "leave_group"){
		cout << "leave group\n";
		return leave_group(params);
	}else if(command_name == "accept_request"){
		cout << "accept request\n";
		return accept_request(params);
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
		return logout(params);
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
		if(params[0] == "find_owned_groups"){
			vector<string> res = find_owned_groups(params[1]);
			for(string g_ID : res){
				int ret = write(socket, g_ID.c_str(), g_ID.size());
				if(!ret){
					cerr << "\nError in writing response.";
				}
			}
		}else if(params[0] == "find_all_groups"){
			vector<group> res = find_all_groups();
			int size = res.size();
			int ret = write(socket, &size, sizeof(size));
			if(!ret){
				cerr << "\nError in writing size.";
			}
			for(group g : res){
				string ans = g.owner_user_ID + "\t" + g.group_ID;
				int ret = write(socket, ans.c_str(), ans.size());
				if(!ret){
					cerr << "\nError in writing response.";
				}
			}
		}else{
			int return_val = 5;
			return_val = execute_command(params, socket);
			int ret = write(socket, &return_val, sizeof(return_val));
			if(!ret){
				cerr << "\nError in writing response.";
			}
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
