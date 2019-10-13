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