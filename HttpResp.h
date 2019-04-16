#include <string>
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>

class HttpResp{
 public:
  int status_code;
  std::string resptime;
  std::string status_line;
  std::unordered_map<std::string, std::string> header_field;
  std::vector<char> trailer;
  std::vector<char> msg_body;
 HttpResp(): status_code(), resptime(), status_line(), header_field(), trailer(), msg_body(){}
 HttpResp(const HttpResp& rhs):status_code(rhs.status_code), resptime(rhs.resptime), status_line(rhs.status_line), header_field(rhs.header_field), trailer(rhs.trailer), msg_body(rhs.msg_body){}
  HttpResp& operator= (const HttpResp& rhs){
    if(this != &rhs){
      status_code = rhs.status_code;
      resptime = rhs.resptime;
      status_line = rhs.status_line;
      header_field = rhs.header_field;
      trailer = rhs.trailer;
      msg_body = rhs.msg_body;
    }
    return *this;
  }
  void reconstructor(std::vector<char> & buffer);
  ~HttpResp(){}
};
