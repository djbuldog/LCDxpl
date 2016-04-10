#include "serial.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

#if LIN

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <dirent.h>

int Serial::open(const std::string tty) {

	struct termios tio;

	if (opened) {
		return 1;
	}

	tty_fd = ::open(tty.c_str(), O_RDWR | O_NONBLOCK);
	if (tty_fd == -1) {
		return 2;
	}

	if (isatty(tty_fd)!=1) {
		::close(tty_fd);
		return 3;
	}

	memset(&tio, 0, sizeof(tio));

	// CS8    - character size 8
	// CREAD  - enable receiving
	// CLOCAL - local connection, no modem control
	tio.c_cflag=CS8|CREAD|CLOCAL;
	// Enable Cannonical Mode
	tio.c_lflag=ICANON;

	if (cfsetospeed(&tio,B9600) < 0 || cfsetispeed(&tio,B9600) < 0) {
		::close(tty_fd);
		return 5;
	}

	if (tcsetattr(tty_fd,TCSANOW,&tio) < 0) {
		::close(tty_fd);
		return 6;
	}

	opened = true;
	return 0; 
}

Serial::~Serial() {
	close();
}

void Serial::close() {
	if (opened) {

	/* FIXME:
	 * when open bad tty device.... and send some data
	 * close takes a few second - XPL hungs for that time
	 * flushing in/out doesnt help....
	 * closing in the background (serial thread) could help...
	*/
		//tcflush(tty_fd,TCIOFLUSH);
		::close(tty_fd);
		opened = false;
	}
}

int Serial::write(const std::string msg) {

	if (!opened) {
		return -2;
	}

	return ::write(tty_fd, msg.c_str(), msg.size());

}

const std::string Serial::readln() {

	char buf[256];
	int res;

	if (!opened) {
		return "";
	}

	res = read(tty_fd, buf, 255);
	if (res == -1) res=0;
	buf[res]=0;

	return std::string(buf);

}

std::vector<std::string> Serial::getDevList() {

	DIR *dir;
	struct dirent *ent;
	std::string dirname("/dev/");
	std::vector<std::string> list;

	dir = opendir(dirname.c_str());
	if (dir) {
		while((ent = readdir(dir)) != NULL) {
			if (strncmp(ent->d_name, "ttyACM", 6) == 0) list.push_back(dirname+ent->d_name);
		}
		closedir(dir);
	}

	return list;
}

#else

int Serial::open(const std::string tty) {

	if (opened) {
		return 1;
	}

	//FIXME: NOT IMPLEMENTED

	opened = true;
	return 0; 
}

Serial::~Serial() {
	close();
}

void Serial::close() {
	if (opened) {

		//FIXME: NOT IMPLEMENTED
		opened = false;
	}
}

int Serial::write(const std::string msg) {

	if (!opened) {
		return -2;
	}

	//FIXME: NOT IMPLEMENTED
	//return ::write(tty_fd, msg.c_str(), msg.size());

	return 0;

}

const std::string Serial::readln() {

	char buf[256];
	int res;

	if (!opened) {
		return "";
	}

	//FIXME: NOT IMPLEMENTED
	
	//res = read(tty_fd, buf, 255);
	//if (res == -1) res=0;
	buf[res]=0;

	return std::string(buf);

}

std::vector<std::string> Serial::getDevList() {

	std::vector<std::string> list;

	//FIXME: NOT IMPLEMENTED

	return list;
}

#endif

