#include "common_headers.h"
using namespace std;

class group {
	public:
	string group_ID;
	string owner_user_ID;
	vector<string> members;
	//vector<file> files_shared;

	void print(){
		cout << endl << group_ID << " : " << owner_user_ID << endl;
        cout << "Members:\n";
        for(string memberID : members){
            cout << memberID << "\n";
        }
        // cout << "Files:\n";
        // for(file f : members){
        //     cout << f.SHA1 << "\n";
        // }
	}
 
	friend ostream & operator << (ostream &out, const group & obj)
	{
		out << obj.group_ID << "\n" <<obj.owner_user_ID<< "\n";
        //Not storing members and shared files in file
        return out;
    }   
	friend istream & operator >> (istream &in,  group &obj)
	{
		in >> obj.group_ID;
		in >> obj.owner_user_ID;
		return in;
	}
};