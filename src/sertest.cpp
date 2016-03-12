
#include <unistd.h>
#include <sstream>
#include <iostream>
#include "serial.h"

using namespace std;

main() {
  Serial ser;
  stringstream ss;
  vector<string> devlist;

	cerr << "Available devices:" << endl;
	devlist = ser.getDevList();
	for(vector<string>::iterator it = devlist.begin(); it != devlist.end(); ++it) {
		cerr << "item: " << *it << endl;
	}

  cerr << "Openning serial" << endl;
  cerr << "ret: " << ser.open("/dev/ttyACM1") << endl;

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
  
  cerr << "Switching to reading infinite loop:" << endl;
  while(1) {
    cout << ser.readln();
    cerr << "wating 1s.." << endl;
    sleep(1);
  }

  
}
