#include "common_headers.h"
using namespace std;

class file {
	public:
	string sha_1;
    string file_name;
	bool is_shared;
	vector<string> suppliers_userIDs;
    string size;
    string ownerID;

	void print(){
		cout << endl << file_name << " : " << is_shared << " : " << size << " : " << ownerID << endl;
        if(suppliers_userIDs.size() > 0){
            cout << "Suppliers:\n";
            for(string supplierID : suppliers_userIDs){
                cout << supplierID << "\n";
            }
        }
	}
 
	// friend ostream & operator << (ostream &out, const group & obj)
	// {
	// 	out << obj.group_ID << "\n" <<obj.owner_user_ID<< "\n";
    //     //Not storing members and shared files in file
    //     return out;
    // }   
	// friend istream & operator >> (istream &in,  group &obj)
	// {
	// 	in >> obj.group_ID;
	// 	in >> obj.owner_user_ID;
	// 	return in;
	// }
};
