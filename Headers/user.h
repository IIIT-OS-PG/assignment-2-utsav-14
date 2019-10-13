#include "common_headers.h"
using namespace std;

class user {
	public:
	string user_ID;
	string pwd;
	string group_ID;	//may be null

	void print(){
		cout << endl << user_ID << " : " << pwd << " : " << group_ID << endl;
	}

	bool operator==(const user & obj)
	{
		return (user_ID == obj.user_ID) && (pwd == obj.pwd);
	}
 

	friend ostream & operator << (ostream &out, const user & obj)
	{
		out << obj.user_ID << "\n" <<obj.pwd<<"\n"<<obj.group_ID<<endl;
		return out;
	}

	friend istream & operator >> (istream &in,  user &obj)
	{
		in >> obj.user_ID;
		in >> obj.pwd;
		in >> obj.group_ID;
		return in;
	}
};