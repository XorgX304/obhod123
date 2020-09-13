#ifndef SAVELOAD_H
#define SAVELOAD_H

#define users_file_name "/obhod123/users.json"
#define servers_file_name "/obhod123/servers.json"
#define static_file_name "/obhod123/static.json"   // needed for web site confirmations


void saveUsersJson();
void saveServersJson();

void loadUsersJson();
void loadServersJson();
void loadStaticRequests();

#endif // SAVELOAD_H
