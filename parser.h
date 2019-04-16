#include <string>
#include <utility>
#include <vector>
#include "HttpRequest.h"
#include "HttpResp.h" 

class parser{
 public:
  HttpRequest req;
  HttpResp resp;
  parser():req(), resp(){}
  int findHeader(std::vector<char>& buffer);
  int parseRequest(std::vector<char>& buffer);
  int parseResp(std::vector<char>& buffer);
  bool isValidReq();
  bool isCachable();
  ~parser(){}
};
