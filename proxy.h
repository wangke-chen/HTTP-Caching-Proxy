#include <vector>
#include "parser.h"
#include <unordered_map>
#include <string>
#include <utility>
#include <list>
#include <fstream>
class proxy{
 private:
  int browser_fd;
  int server_fd;
  std::string server_ip;
  int thread_id;
  parser myparser; 
  std::vector<char> buffer;
  //  std::ofstream outfile;
 public:
  std::ofstream outfile;
  //  std::vector<std::pair<int, std::string> > connection_info;
 proxy():browser_fd(), server_fd(), server_ip(), thread_id(), myparser(), buffer(), outfile(){
    // outfile.open("/var/log/erss/proxy.log", std::ofstream::app);
    outfile.open("proxy.log", std::ofstream::app);
    //buffer.resize(2048);
  }
  int connect_server();
  //  void accept_connection(int id);
  //  void become_daemon();
  int recv_from_browser(int fd, std::string** ip);
  int send_to_server();
  int recv_from_server();
  int send_to_browser();
  void recvall(int fd, int length, int rec);
  void sendall(int fd, int size);
  std::vector<char> chunk_decode(int fd);
  //  void resp_chunk_decode(int fd);
  void getReq();
  int postReq();
  static  std::string dateFormat(std::string date);
  void resp400code();
  void resp502code();
  void lock();
  //  std::string dateFormat();
  void establishTunnel();
  void clear_connectioninfo();
  int getBrowser(){
    return browser_fd;
  }
  int getServer(){
    return server_fd;
  }
  ~proxy(){}
  
  friend class cache;
};
 void become_daemon();
int establish_socket(std::vector<std::pair<int, std::string> > & connection_info);
int accept_connection(int fd, std::string** browser_ip);
void* get_in_addr(struct sockaddr *sa);
