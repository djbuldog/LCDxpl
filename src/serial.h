#include <string>
#include <vector>

#if IBM
#include <windows.h>
#endif

class Serial {

#if LIN
	int tty_fd;
#else
	HANDLE com_handle;
#endif

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
