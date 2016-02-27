
#include <unistd.h>
#include <sstream>
#include "serial.h"

using namespace std;

main() {
  serial ser;
  stringstream ss;
  
  ser.open("/dev/ttyACM1");

  sleep(2);
  
  ss << "SQK " << 124 << endl;
  ser.write(ss.str());
  ss.str( std::string() );
  
  sleep(2);
  
  ss << "SQK " << 222 << endl;
  ser.write(ss.str()); 
  ss.str( std::string() );

  sleep(2);
  
  ss << "SQK " << 444 << endl;
  ser.write(ss.str()); 
  ss.str( std::string() );
  
}
