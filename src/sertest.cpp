
#include <unistd.h>
#include <sstream>
#include <iostream>
#include "serial.h"

using namespace std;

main() {
	Serial ser;
	stringstream ss;
	vector<string> devlist;

	// testing devlist
	cerr << "Available devices:" << endl;
	devlist = ser.getDevList();
	for(vector<string>::iterator it = devlist.begin(); it != devlist.end(); ++it) {
		cerr << "item: " << *it << endl;
	}

	// testing close() bug
	cerr << "Openning bad serial" << endl;
	cerr << "ret: " << ser.open("/dev/ttyACM1") << endl;

	cerr << "Writing some data.." << endl;
	ss << "SQK" << 9988 << endl;
	ser.write(ss.str());

	cerr << "Closing bad serial" << endl;
	ser.close();

	// testing value change through serial
	cerr << "Openning serial" << endl;
	cerr << "ret: " << ser.open("/dev/ttyACM0") << endl;

	ss << "SQK" << 124 << endl;
	ser.write(ss.str());
	ss.str( std::string() );
	cerr << "SQK124" << endl;

	sleep(2);

	ss << "SQK" << 222 << endl;
	ser.write(ss.str()); 
	ss.str( std::string() );
	cerr << "SQK222" << endl;

	sleep(2);

	ss << "SQK" << 444 << endl;
	ser.write(ss.str()); 
	ss.str( std::string() );
	cerr << "SQK4444" << endl;

	sleep(2);

	// testing reading values from serial
	cerr << "Switching to reading infinite loop:" << endl;
	while(1) {
		cout << ser.readln();
		cerr << "wating 1s.." << endl;
		sleep(1);
	}

}
