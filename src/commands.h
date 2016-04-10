#include <string>
#include "XPLMUtilities.h"

class Command {

	protected:
		XPLMCommandRef ptr;
		std::string ser_code;

	public:
		Command(): ptr(NULL) {}
		Command(const std::string name, const std::string ser_code): ptr() { assign(name, ser_code); }
		void assign(const std::string name, const std::string ser_code);
		bool isInit() { return (ptr!=NULL); }
		void perform(std::string command);

};
