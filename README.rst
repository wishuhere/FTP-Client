==========
FTP Client
==========

Follows RFC 959, implements in C++ and Winsock 2.2, test with FileZilla server.

How to run
==========

Use Visual Studio 2013 to open solution.

After build, go to Debug/Release folder and run ::

        ftp_client.exe ftp.example.com

Flow explaination
=================

::

	handle_login
		prompt for username and password
		if fail
			quit program

	loop
		ask for user input
		if input incorrectly
			output help
		else
			change user input -> server command (cmd)

		decide cmd to go where
			single only have connect stream
			dual include data stream (ls, dir, get, put)

		if handle_cmd_single
			send cmd
			receive reply
		else
			if passive
				create data stream
			else
				create listen socket

			if not handle file (ls, dir)
				switch to type ASCII
			else
				switch to type BINARY
				if file to put not exists
					continue to loop

			send cmd
			receive reply

			// active mode require
			if active
				create data stream by accept from listen socket

			// deal with data stream
			get cmd
				open file to save in binary mode
				loop
					receive buf from data stream
					write buf to file
			put cmd
				open file to read in binary mode
				loop
					read buf from open file
					send buf to data stream
			ls, dir cmd
				loop
					receive buf from data stream
					ouput buf
			close data stream
			receive reply 226 (close data stream)