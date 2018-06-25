#ifndef _SOCKET_INCLUDED
#define _SOCKET_INCLUDED

#include <conio.h>
#include <direct.h>
#include <stdio.h>
#include <iostream>
#include <random>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define BUFLEN 1024
#define FTP_PORT "21"
enum FTP_MODE { active, passive };
enum CMD_WHERE { single, dual };
using namespace std;

struct FTP_CMD {
	string str;
	vector<int> expect_reply;
	string file_name;
};

// Login to socket with username and password
int handle_login(SOCKET connect_SOCKET);

/* Change command (cmd)
* from user input for server to understand
* example ls -> NLST
* go_where:	single only have connect stream
*		dual include data stream
*/
FTP_CMD change_cmd(const char *input, CMD_WHERE &go_where);

/* Decide which input go to
* single stream, dual stream or file stream
*/
int decide_cmd(SOCKET connect_SOCKET, const char *input, FTP_MODE cur_mode,
	char *ip_client_str);

// Handle user request
int handle_cmd_single(SOCKET connect_SOCKET, FTP_CMD cmd);
int handle_cmd_dual(SOCKET connect_SOCKET, FTP_CMD cmd, FTP_MODE cur_mode,
	char *ip_client_str);

/* active mode
* return listen socket
* wait to accept transfer data
*/
SOCKET active_mode(SOCKET connect_SOCKET, char *ip_client_str);

/* passive mode
* return socket to transfer data
*/
SOCKET passive_mode(SOCKET connect_SOCKET);

/* receive reply from server
* return reply code
*/
int recv_reply(SOCKET connect_SOCKET, vector<int> arr_expect);

// check if k exists in arr
int exists_in_arr(vector<int> arr, int k);

// create socket connect 
SOCKET create_soc_connect(const char *ip_str, const char *port_str);

void print_help();

#endif