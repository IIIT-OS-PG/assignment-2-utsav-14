// Constants.h
#include "common_headers.h"

std::string login_code_to_string(int code)
{
   switch (code)
   {
      case 0:   return "Logged in successfully."; 
      case 1:   return "Error: Users file couldn't be opened.";
      case 2:   return "User already logged in.";
      case 3:   return "Wrong password.";
      case 4:   return "User does not exist.";
      default:
         break;
   }
   return "Unknown code";
}

std::string create_user_code_to_string(int code)
{
   switch (code)
   {
      case 0:   return "User created successfully.";
      case 1:   return "Error: Users file couldn't be opened.";
      case 2:   return "Error: UserID already exists.";
      case 3:   return "Error writing record in user file.";
      default:
         break;
   }
   return "Unknown code";
}

std::string create_group_code_to_string(int code)
{
   switch (code)
   {
      case 0:   return "Group created successfully.";
      case 1:   return "Error: Groups file couldn't be opened.";
      case 2:   return "Error: GroupID already exists.";
      case 3:   return "Error writing record in group file.";
      default:
         break;
   }
   return "Unknown code";
}

std::string join_group_code_to_string(int code)
{
   switch (code)
   {
      case 0:   return "Group join requested.";
      case 1:   return "User is already a member of this group.";
      case 2:   return "Error: Can't send join request. Group owner is not online.";
      case 3:   return "Error: No such group exists.";      
      default:
         break;
   }
   return "Unknown code";
}

std::string leave_group_code_to_string(int code)
{
   switch (code)
   {
      case 0:   return "Deleted owner.";
      case 1:   return "Group deleted.";
      case 2:   return "Member deleted.";
      case 3:   return "Error: User is not a part of this group.";      
      case 4:   return "Error: No such group exists.";  
      case 5:   return "Error: Groups file couldn't be opened.";         
      default:
         break;
   }
   return "Unknown code";
}

std::string upload_file_code_to_string(int code)
{
   switch (code)
   {
      case 0:   return "File shared on the group.";
      case 1:   return "File is already shared on this group.";
      case 2:   return "Error: User is not a member of this group.";
      case 3:   return "Error: GroupID not found.";      
      default:
         break;
   }
   return "Unknown code";
}