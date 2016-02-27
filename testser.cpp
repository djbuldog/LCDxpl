
//#include <sstream>
#include "serial.h"

using namespace std;

main() {
  serial ser;
  
//  stringstream ss;
  
  ser.open("/dev/ttyACM1");
//  ss << "SQK " << 123 << endl;
//  ser.write(ss.str());
  
}
