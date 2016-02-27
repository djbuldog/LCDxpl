#include "serial.h"

int serial::open(std::string tty) {
  this->term.open(tty.c_str());
}

serial::~serial() {
  this->term.close();
}

int serial::write(const std::string msg) {
  this->term << msg;
  this->term.flush();
  //std::cerr << msg.c_str();
}

const std::string serial::readln() {
  std::string line;
  getline(this->term,line);
  return line;
}
