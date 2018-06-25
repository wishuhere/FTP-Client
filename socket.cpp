#include "socket.h"


int main(int argc, char *argv[])
{
	// Kiem tra cu phap
	if (argc != 2) {
		printf("Using: FTP_Client IPaddress\n");
		exit(1);
	}
	int status;

	// Khoi tao Winsock phien ban 2.2
	WSADATA var_WSADATA;
	status = WSAStartup(MAKEWORD(2, 2), &var_WSADATA);
	if (status != 0) {
		printf("WSAStartup() failed with error: %d\n", status);
		WSACleanup();
		exit(1);
	}

	// Tao connect_SOCKET ket noi FTP server
	SOCKET connect_SOCKET = create_soc_connect(argv[1], FTP_PORT);
	if (connect_SOCKET == INVALID_SOCKET) {
		printf("create_soc_connect() failed\n");
		WSACleanup();
		exit(1);
	}

	// Get socket info (to get client IP)
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	if (getsockname(connect_SOCKET, (struct sockaddr *)&client_addr,
			&addr_len) < 0) {
		printf("getsockname() error\n");
		WSACleanup();
		exit(1);
	}
	// Get client IP
	char ip_client_str[INET_ADDRSTRLEN];
	if (inet_ntop(client_addr.sin_family, &(client_addr.sin_addr),
		      ip_client_str, sizeof(ip_client_str)) == NULL) {
		printf("inet_ntop() error\n");
		WSACleanup();
		exit(1);
	}

	// Login
	status = handle_login(connect_SOCKET);
	if (status == 0) {
		printf("Login error\n");
		closesocket(connect_SOCKET);
		WSACleanup();
		exit(1);
	}

	// Read commands from user
	char input[BUFLEN];
	FTP_MODE cur_mode = active;
	while (true) {
		printf("ftp> ");
		fgets(input, BUFLEN, stdin);
		input[strcspn(input, "\n")] = 0; // remove trailing '\n'

		// cmd for client
		if (strcmp(input, "active") == 0) {
			if (cur_mode == active)
				printf("Keep active mode\n");
			else {
				printf("Switch to active mode\n");
				cur_mode = active;
			}
			continue;
		} else if (strcmp(input, "passive") == 0) {
			if (cur_mode == passive)
				printf("Keep passive mode\n");
			else {
				printf("Switch to passive mode\n");
				cur_mode = passive;
			}
			continue;
		}
		// change client directory
		else if (strncmp(input, "lcd", 3) == 0) {
			if (_chdir(input + 4) == -1)
				printf("Fail to change client working "
				       "directory\n");
			else
				printf(
				    "Change client working directory to %s\n",
				    input + 4);
			continue;
		}
		// show current client directory
		else if (strcmp(input, "lpwd") == 0) {
			char *buffer = _getcwd(NULL, 0);
			printf("Current client directory %s\n", buffer);
			free(buffer);
			continue;
		} else if (strcmp(input, "help") == 0 ||
			   strcmp(input, "?") == 0) {
			print_help();
			continue;
		}

		// cmd for server
		// multi command (mdelete, mget, mput)
		if (strncmp(input, "mdelete ", 8) == 0) {
			string pre = "delete ";
			char *token = NULL;
			char *next_token = NULL;
			token = strtok_s(input + 8, " ", &next_token);
			while (token != NULL) {
				decide_cmd(connect_SOCKET,
					   (pre + string(token)).c_str(),
					   cur_mode, ip_client_str);
				token = strtok_s(NULL, " ", &next_token);
			}
		} else if (strncmp(input, "mget ", 5) == 0) {
			string pre = "get ";
			char *token = NULL;
			char *next_token = NULL;
			token = strtok_s(input + 5, " ", &next_token);
			while (token != NULL) {
				decide_cmd(connect_SOCKET,
					   (pre + string(token)).c_str(),
					   cur_mode, ip_client_str);
				token = strtok_s(NULL, " ", &next_token);
			}
		} else if (strncmp(input, "mput ", 5) == 0) {
			string pre = "put ";
			char *token = NULL;
			char *next_token = NULL;
			token = strtok_s(input + 5, " ", &next_token);
			while (token != NULL) {
				decide_cmd(connect_SOCKET,
					   (pre + string(token)).c_str(),
					   cur_mode, ip_client_str);
				token = strtok_s(NULL, " ", &next_token);
			}
		} else {
			status = decide_cmd(connect_SOCKET, input, cur_mode,
					    ip_client_str);
		}
		if (status == 1)
			break;
	}

	// Cleanup
	closesocket(connect_SOCKET);
	WSACleanup();
	return 0;
}

int handle_login(SOCKET connect_SOCKET)
{
	char buf[BUFLEN]; // buffer receice from or send to ftp serve
	int status;       // for get Winsock error

	// Check connection
	vector<int> arr_reply_code_connect = {220};
	if (recv_reply(connect_SOCKET, arr_reply_code_connect) == 0)
		return 0;

	// Username
	// USER <SP> <username> <CRLF>
	memset(buf, 0, BUFLEN);
	printf("Username: ");
	char username[BUFLEN - 10];
	fgets(username, BUFLEN - 10, stdin);
	username[strcspn(username, "\n")] = 0; // remove trailing '\n'
	sprintf_s(buf, "USER %s\r\n", username);

	// send username
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		return 0;
	}

	// Check server recieving username
	vector<int> arr_reply_code_user = {230, 331};
	if (recv_reply(connect_SOCKET, arr_reply_code_user) == 0)
		return 0;

	// Password
	// PASS <SP> <password> <CRLF>
	memset(buf, 0, BUFLEN);
	printf("Password: ");
	char password[BUFLEN - 10];
	int temp_i = 0, temp_ch;
	// 13 ENTER
	// 8 BACKSPACE
	while ((temp_ch = _getch()) != 13) {
		if (temp_ch == 8) {
			if (temp_i > 0)
				--temp_i;
		} else
			password[temp_i++] = temp_ch;
	}
	password[temp_i] = '\0';
	printf("\n");
	password[strcspn(password, "\n")] = 0; // remove trailing '\n'
	sprintf_s(buf, "PASS %s\r\n", password);

	// Send password
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		return 0;
	}

	// Check server recieving password
	vector<int> arr_reply_code_pass = {230};
	if (recv_reply(connect_SOCKET, arr_reply_code_pass))
		return 0;

	return 0;
}

FTP_CMD change_cmd(const char *input, CMD_WHERE &go_where)
{
	FTP_CMD cmd;
	cmd.file_name = ""; // default is not handle file
	if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
		go_where = single;
		cmd.str = string("QUIT\r\n");
		cmd.expect_reply = {221};
	} else if (strcmp(input, "pwd") == 0) {
		go_where = single;
		cmd.str = string("PWD\r\n");
	} else if (strncmp(input, "cd ", 3) == 0) {
		go_where = single;
		string temp = "CWD";
		temp += string(input + 2);
		temp += "\r\n";
		cmd.str = temp;
		cmd.expect_reply = {250};
	} else if (strncmp(input, "delete ", 7) == 0) {
		go_where = single;
		string temp = "DELE" + string(input + 6) + string("\r\n");
		cmd.str = temp;
		cmd.expect_reply = {250};
	} else if (strncmp(input, "mkdir ", 6) == 0) {
		go_where = single;
		string temp = "MKD" + string(input + 5) + string("\r\n");
		cmd.str = temp;
		cmd.expect_reply = {257};
	} else if (strncmp(input, "rmdir ", 6) == 0) {
		go_where = single;
		string temp = "RMD" + string(input + 5) + string("\r\n");
		cmd.str = temp;
		cmd.expect_reply = {250};
	} else if (strncmp(input, "ls", 2) == 0) {
		go_where = dual;
		string temp = "NLST";
		if (input[2] != '\0') {
			temp += string(input + 2);
		}
		temp += "\r\n";
		cmd.str = temp;
		cmd.expect_reply = {125, 150};
	} else if (strncmp(input, "dir", 3) == 0) {
		go_where = dual;
		string temp = "LIST";
		if (input[3] != '\0') {
			temp += string(input + 3);
		}
		temp += "\r\n";
		cmd.str = temp;
		cmd.expect_reply = {125, 150};
	} else if (strncmp(input, "get ", 4) == 0) {
		go_where = dual;
		string temp = "RETR" + string(input + 3) + string("\r\n");
		cmd.str = temp;
		cmd.expect_reply = {125, 150};
		cmd.file_name = string(input + 4);
	} else if (strncmp(input, "put ", 4) == 0) {
		go_where = dual;
		string temp = "STOR" + string(input + 3) + string("\r\n");
		cmd.str = temp;
		cmd.expect_reply = {125, 150};
		cmd.file_name = string(input + 4);
	} else {
		cmd.str = "_ERROR_";
	}
	return cmd;
}

int decide_cmd(SOCKET connect_SOCKET, const char *input, FTP_MODE cur_mode,
	       char *ip_client_str)
{
	CMD_WHERE go_where;
	FTP_CMD cmd = change_cmd(input, go_where);
	if (cmd.str == "_ERROR_") {
		printf("Type help or ?\n");
		return 0;
	}
	int status;
	if (go_where == single)
		status = handle_cmd_single(connect_SOCKET, cmd);
	else
		status = handle_cmd_dual(connect_SOCKET, cmd, cur_mode,
					 ip_client_str);
	return status;
}

int handle_cmd_single(SOCKET connect_SOCKET, FTP_CMD cmd)
{
	// send cmd
	int status = send(connect_SOCKET, cmd.str.c_str(), cmd.str.length(), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		return 1;
	}

	// receive reply
	status = recv_reply(connect_SOCKET, cmd.expect_reply);

	// only quit return 1
	if (cmd.str == "QUIT\r\n")
		return 1;

	return status;
}

int handle_cmd_dual(SOCKET connect_SOCKET, FTP_CMD cmd, FTP_MODE cur_mode,
		    char *ip_client_str)
{
	int status;

	/* Check if handle file (get, put) or not
	 * if not, ASCII mode
	 * if handle file, check if file exists in put
	 * and open BINARY mode
	 */
	if (strcmp(cmd.file_name.c_str(), "") == 0) {
		// ASCII mode
		string ascii_str = "TYPE A\r\n";
		status = send(connect_SOCKET, ascii_str.c_str(),
			      ascii_str.length(), 0);
		if (status == SOCKET_ERROR) {
			printf("send() error %d\n", WSAGetLastError());
			return 1;
		}
		vector<int> ascii_expect = {200};
		if (recv_reply(connect_SOCKET, ascii_expect) == 0) {
			return 0;
		}
	} else {
		// Check if file to put exists
		if (strncmp(cmd.str.c_str(), "STOR ", 5) == 0) {
			FILE *f;
			fopen_s(&f, cmd.file_name.c_str(), "rb");
			if (f == NULL) {
				printf("File %s can not be opened\n",
				       cmd.file_name.c_str());
				return 0;
			}
			fclose(f);
		}

		// Binary mode
		string bin_mode = "TYPE I\r\n";
		status = send(connect_SOCKET, bin_mode.c_str(),
			      bin_mode.length(), 0);
		if (status == SOCKET_ERROR) {
			printf("send() error %d\n", WSAGetLastError());
			return 1;
		}
		vector<int> bin_expect = {200};
		if (recv_reply(connect_SOCKET, bin_expect) == 0) {
			return 0;
		}
	}

	// create data stream or listen socket depend on current mode
	SOCKET data_SOCKET = INVALID_SOCKET;
	SOCKET listen_SOCKET = INVALID_SOCKET;
	if (cur_mode == active) {
		listen_SOCKET = active_mode(connect_SOCKET, ip_client_str);
	} else {
		data_SOCKET = passive_mode(connect_SOCKET);
	}
	// can not create data stream or listen socket
	if (data_SOCKET == INVALID_SOCKET && listen_SOCKET == INVALID_SOCKET)
		return 0;

	// send cmd
	status = send(connect_SOCKET, cmd.str.c_str(), cmd.str.length(), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		closesocket(listen_SOCKET);
		closesocket(data_SOCKET);
		return 1;
	}

	// receive reply
	if (recv_reply(connect_SOCKET, cmd.expect_reply) == 0) {
		closesocket(listen_SOCKET);
		closesocket(data_SOCKET);
		return 0;
	}

	// active mode require
	if (cur_mode == active) {
		data_SOCKET = accept(listen_SOCKET, NULL, NULL);
		closesocket(listen_SOCKET);
	}

	// buffer
	char buf[BUFLEN];
	int len_buf;
	memset(buf, 0, BUFLEN);
	// get file
	if (strncmp(cmd.str.c_str(), "RETR ", 5) == 0) {
		FILE *f_save;
		fopen_s(&f_save, cmd.file_name.c_str(), "wb");
		while ((len_buf = recv(data_SOCKET, buf, BUFLEN, 0)) > 0) {
			fwrite(buf, sizeof(char), len_buf, f_save);
		}
		fclose(f_save);
	}
	// put file
	else if (strncmp(cmd.str.c_str(), "STOR ", 5) == 0) {
		FILE *f_read;
		status = fopen_s(&f_read, cmd.file_name.c_str(), "rb");
		while ((len_buf = fread(buf, sizeof(char), BUFLEN, f_read)) >
		       0) {
			status = send(data_SOCKET, buf, len_buf, 0);
			if (status == SOCKET_ERROR) {
				printf("send() error %d\n", WSAGetLastError());
				break;
			}
		}
		fclose(f_read);
	}
	// only print buf from data stream
	else {
		while ((len_buf = recv(data_SOCKET, buf, BUFLEN, 0)) > 0) {
			buf[len_buf] = '\0';
			printf("%s", buf);
		}
	}
	closesocket(data_SOCKET);

	// Check server successfully transfer
	vector<int> arr_expect = {226};
	if (recv_reply(connect_SOCKET, arr_expect) == 0) {
		closesocket(data_SOCKET);
		return 0;
	}

	return 0;
}

SOCKET active_mode(SOCKET connect_SOCKET, char *ip_client_str)
{
	// random port (source: stackoverflow)
	random_device rd;  // obtain a random number from hardware
	mt19937 eng(rd()); // seed the generator
	uniform_int_distribution<> distr(50000, 60000); // define the range
	int ACT_PORT = distr(eng);
	string port_str = to_string(ACT_PORT);

	// Tao listen port
	// Tao addrinfo object
	addrinfo hints, *result, *p_addrinfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve server address and port
	int status =
	    getaddrinfo(ip_client_str, port_str.c_str(), &hints, &result);
	if (status != 0) {
		printf("getaddrinfo() failed with error %d\n", status);
		return INVALID_SOCKET;
	}

	// Loop through all the resuls
	char ip_str[INET_ADDRSTRLEN];
	SOCKET listen_SOCKET = INVALID_SOCKET;
	for (p_addrinfo = result; p_addrinfo != NULL;
	     p_addrinfo = p_addrinfo->ai_next) {
		listen_SOCKET =
		    socket(p_addrinfo->ai_family, p_addrinfo->ai_socktype,
			   p_addrinfo->ai_protocol);
		if (listen_SOCKET == INVALID_SOCKET) {
			continue;
		}
		status = bind(listen_SOCKET, p_addrinfo->ai_addr,
			      (int)p_addrinfo->ai_addrlen);
		if (status == SOCKET_ERROR) {
			closesocket(listen_SOCKET);
			continue;
		}
		if (p_addrinfo->ai_family == AF_INET) {
			struct sockaddr_in *ipv4 =
			    (struct sockaddr_in *)p_addrinfo->ai_addr;
			void *addr = &(ipv4->sin_addr);
			inet_ntop(p_addrinfo->ai_family, addr, ip_str,
				  INET_ADDRSTRLEN);
		}
		break;
	}
	freeaddrinfo(result);

	// Listen on a socket
	status = listen(listen_SOCKET, SOMAXCONN);
	if (status == SOCKET_ERROR) {
		printf("listen() failed with error: %d\n", WSAGetLastError());
		closesocket(listen_SOCKET);
		return INVALID_SOCKET;
	}

	// PORT <SP> <host - port> <CRLF>
	// 127.0.0.1 -> 127,0,0,1
	for (size_t i = 0; i < strlen(ip_str); ++i) {
		if (ip_str[i] == '.')
			ip_str[i] = ',';
	}
	int port_num_1 = ACT_PORT / 256;
	int port_num_2 = ACT_PORT % 256;
	string msg = "PORT " + string(ip_str) + string(",") +
		     to_string(port_num_1) + string(",") +
		     to_string(port_num_2) + string("\r\n");
	char buf[BUFLEN];
	strcpy_s(buf, msg.c_str());

	// send PORT to server
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		closesocket(listen_SOCKET);
		return INVALID_SOCKET;
	}

	// Get reply from server
	vector<int> arr_expect = {200};
	if (recv_reply(connect_SOCKET, arr_expect) == 0)
		return INVALID_SOCKET;

	return listen_SOCKET;
}

SOCKET passive_mode(SOCKET connect_SOCKET)
{
	char buf[BUFLEN];
	int reply_code;
	int status;

	// PASV <CRLF>
	strcpy_s(buf, "PASV\r\n");
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}
	memset(buf, 0, BUFLEN);

	// 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
	status = recv(connect_SOCKET, buf, BUFLEN, 0);
	if (status == SOCKET_ERROR) {
		printf("recv() error %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	} else
		buf[status] = '\0'; // recv return len of buf actual receving

	// check if 227 correctly
	char *p_end = NULL;
	printf("%s", buf);
	reply_code = strtol(buf, &p_end, 10);
	if (reply_code != 227) {
		return INVALID_SOCKET;
	}

	int h1, h2, h3, h4, p1, p2;
	status = sscanf_s(buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).",
			  &h1, &h2, &h3, &h4, &p1, &p2);
	if (status != 6) {
		printf(
		    "Passive mode address scan failure. Shouldn't happen!\n");
		return INVALID_SOCKET;
	}

	// ip number -> string
	string ip_str = to_string(h1) + "." + to_string(h2) + "." +
			to_string(h3) + "." + to_string(h4);
	// port number -> string
	int port_num = p1 * 256 + p2;
	string port_str = to_string(port_num);

	// create data_SOCKET connect to ip and port given
	SOCKET data_SOCKET = create_soc_connect(ip_str.c_str(), port_str.c_str());
	return data_SOCKET;
}

int recv_reply(SOCKET connect_SOCKET, vector<int> arr_expect)
{
	char buf[BUFLEN];
	int len_buf;
	int reply_code;
	if ((len_buf = recv(connect_SOCKET, buf, BUFLEN, 0)) > 0) {
		buf[len_buf] = '\0';
		printf("%s", buf);
		char *p_end = NULL;
		reply_code = strtol(buf, &p_end, 10);
	}
	// not the reply code we expect, but the reply code we deserve
	if (!exists_in_arr(arr_expect, reply_code)) {
		return 0;
	}
	return 0;
}

int exists_in_arr(vector<int> arr, int k)
{
	for (size_t i = 0; i < arr.size(); ++i) {
		if (arr[i] == k)
			return 1;
	}
	return 0;
}

SOCKET create_soc_connect(const char *ip_str, const char *port_str)
{
	// Tao addrinfo object
	addrinfo hints, *result, *p_addrinfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve server address and port
	int status = getaddrinfo(ip_str, port_str, &hints, &result);
	if (status != 0) {
		printf("getaddrinfo() failed with error %d\n", status);
		return INVALID_SOCKET;
	}

	// Loop through all the resuls and connect the first
	SOCKET connect_SOCKET = INVALID_SOCKET;
	for (p_addrinfo = result; p_addrinfo != NULL;
		p_addrinfo = p_addrinfo->ai_next) {
		connect_SOCKET =
			socket(p_addrinfo->ai_family, p_addrinfo->ai_socktype,
			p_addrinfo->ai_protocol);
		// invalid_socket -> use next p_addrinfo
		if (connect_SOCKET == INVALID_SOCKET) {
			continue;
		}
		status = connect(connect_SOCKET, p_addrinfo->ai_addr,
			(int)p_addrinfo->ai_addrlen);
		// error_socket -> close then use next p_addrinfo
		if (status == SOCKET_ERROR) {
			closesocket(connect_SOCKET);
			continue;
		}
		break;
	}

	// done with result
	freeaddrinfo(result);

	return connect_SOCKET;
}

void print_help()
{
	printf("- help/?:     Hien thi tat ca cau lenh tren FTP_CLIENT\n");
	printf("- ls:         Liet ke thu muc tren Server\n");
	printf("- put:        Upload file den Server\n");
	printf("- get:        Dowload file tu Server\n");
	printf("- mput:       Upload nhieu file den Server\n");
	printf("- mget:       Dowload nhieu file tu Server\n");
	printf("- cd:         Thay doi duong dan tren Server\n");
	printf("- lcd:        Thay doi duong dan duoi Client\n");
	printf("- delete:     Xoa mot file tren Server\n");
	printf("- mdelete:    Xoa nhieu file tren Server\n");
	printf("- mkdir:      Tao thu muc tren Server\n");
	printf("- rmdir:      Xoa thu muc rong tren Server\n");
	printf("- pwd:        Hien thi duong dan hien tai tren Server\n");
	printf("- passive:    Chuyen qua trang thai passive\n");
	printf("- active:     Chuyen qua trang thai active\n");
	printf("- exit/quit:  Thoat khoi Server\n");
}