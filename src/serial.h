#include <iostream>
#include <fstream>

class Serial {
  //const std::string tty;
  //bool connected;
  std::fstream term;
 
  public:
    //serial();
    int open(std::string tty);
    void close();
    ~Serial();
    int write(const std::string msg);
    const std::string readln();
};
