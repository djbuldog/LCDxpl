#include "serial.h"

int Serial::open(std::string tty) {
  this->term.open(tty.c_str());
}

Serial::~Serial() {
  this->term.close();
}

void Serial::close() {
  this->term.close();
}

int Serial::write(const std::string msg) {
  this->term << msg;
  this->term.flush();
  //std::cerr << msg.c_str();
}

const std::string Serial::readln() {
  std::string line;
  getline(this->term,line);
  return line;
}
