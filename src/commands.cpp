#include "commands.h"
#include <cstring>

#include <iostream>

void Command::assign(const std::string name, const std::string ser_code) {
	this->ser_code = ser_code;
	ptr = XPLMFindCommand(name.c_str());
}

void Command::perform(std::string command) {

  if (ptr == NULL) return;
	
	if (!command.empty()) {
		if (strncmp(command.c_str(),ser_code.c_str(),ser_code.size()) == 0) {
		XPLMCommandOnce(ptr);
		}
	}


}


