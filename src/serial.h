#include <iostream>
#include <fstream>

class serial {
  //const std::string tty;
  //bool connected;
  std::fstream term;
 
  public:
    //serial();
    int open(std::string tty);
    ~serial();
    int write(const std::string msg);
    const std::string readln();
};
