#include "serial.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <dirent.h>

int Serial::open(const std::string tty) {

  struct termios tio;
  memset(&tio, 0, sizeof(tio));

  tio.c_iflag=0;
  tio.c_oflag=0;
  
  // CS8 - 8n1 (8bit, no parity, 1 stopbit)
  // CREAD - enable receiving characters
  // CLOCAL - local connection, no modem control
  tio.c_cflag=CS8|CREAD|CLOCAL;

  // tio.c_lflag=0;
  // tio.c_cc[VMIN]=1;
  // tio.c_cc[VTIME]=5;
  tio.c_lflag |= ICANON; // Enable Cannonical Mode
   
  if (opened) {
    return 1;
  }
  
  tty_fd = ::open(tty.c_str(), O_RDWR | O_NONBLOCK);
  if (tty_fd == -1) {
    return 2;
  }

  if (cfsetospeed(&tio,B9600) < 0 || cfsetispeed(&tio,B9600) < 0) {
    close();
    return 3;
  }
  
  if (tcsetattr(tty_fd,TCSANOW,&tio) < 0) {
    close();
    return 4;
  }
  
  opened = true;
  return 0; 
}

Serial::~Serial() {
  close();
}

void Serial::close() {
  if (opened) {
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

