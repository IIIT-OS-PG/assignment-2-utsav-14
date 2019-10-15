#include "common_headers.h"
#include "group.h"
#include <unordered_map>

using namespace std;

class user {
	public:
	string user_ID;
	string pwd;
	vector<string> groupIDs;	//may be null
	struct sockaddr_in address; //Current ip and port of user
	bool isLoggedIn;
	unordered_map<string, vector<uint>> files_available;

	void print(){
		cout << endl << user_ID << " : " << pwd << endl;
		cout << "group member of:\n";
		for(string grp : groupIDs){
			cout << grp << " ";
		}
	}

	bool operator==(const user & obj)
	{
		return (user_ID == obj.user_ID) && (pwd == obj.pwd);
	}
 

	friend ostream & operator << (ostream &out, const user & obj)
	{
		out << obj.user_ID << "\n" << obj.pwd << endl;
		// for(string gID : obj.groupIDs){
		// 	out << gID << endl;
		// }
		return out;
	}

	friend istream & operator >> (istream &in,  user &obj)
	{
		in >> obj.user_ID;
		in >> obj.pwd;
		//in >> obj.group_ID;
		return in;
	}
};
