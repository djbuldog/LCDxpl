#include <string>
#include <vector>

class Serial {

	int tty_fd;
	bool opened;

	public:
		Serial(): opened(false) {}
		~Serial();
		int open(const std::string tty);
		void close();
		int write(const std::string msg);
		const std::string readln();
		std::vector<std::string> getDevList();
};
