#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>


class HttpRequest{
 public:
  int port_num;
  std::string Reqtime;
  std::string hostname;
  std::string method;
  std::string uri;
  std::string start_line;
  std::vector<std::string> header_field;
  std::unordered_map<std::string, std::string> header;
  std::vector<char> trailer;
  std::vector<char> msg_body;
 HttpRequest():port_num(80), Reqtime(), hostname(), method(), start_line(), header_field(), header(), trailer(), msg_body(){}

 HttpRequest(const HttpRequest& rhs):port_num(rhs.port_num), Reqtime(rhs.Reqtime), hostname(rhs.hostname), method(rhs.method), uri(rhs.uri), start_line(rhs.start_line), header_field(rhs.header_field), header(rhs.header), trailer(rhs.trailer), msg_body(rhs.msg_body){}

  HttpRequest& operator=(const HttpRequest& rhs){
    if(this != &rhs){
      port_num = rhs.port_num;
      Reqtime = rhs.Reqtime;
      hostname = rhs.hostname;
      method = rhs.method;
      uri = rhs.uri;
      start_line = rhs.start_line;
      header_field = rhs.header_field;
      header = rhs.header;
      trailer = rhs.trailer;
      msg_body = rhs.msg_body;
    }
    return *this;
  }
  bool validStartline();
  void removeWSpace();
  bool validateHost();
  bool ifAbsurl();
  bool validHeaderfield();
  void reconstruct(std::vector<char>& buffer);
  void removeConnection();
  bool isValidConnect();
  ~HttpRequest(){}
};
