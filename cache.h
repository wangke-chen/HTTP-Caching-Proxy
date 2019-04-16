#include <unordered_map>
#include <list>
#include <utility>
#include "proxy.h"

class cache{
 private:
  int capacity;
  std::unordered_map<std::string, std::list<std::pair<std::string, HttpResp> >::iterator> info;
  std::list<std::pair<std::string, HttpResp> > storage;
 public:
 cache():capacity(), info(), storage(){}
 cache(int _capacity):capacity(_capacity), info(), storage(){}
  bool isIncache(std::string uri);
  int handleResp(HttpResp& resp);
  bool isFresh(proxy& proxy1);
  int getLifetime(std::string uri);
  int caculateDateMinus(std::string value, std::string expire);
  int getCurrentAge(proxy& proxy1);
  std::vector<int> parseTime(std::string date);
  //int whichMon(std::string month);
  //  double DateMinus(std::string expire, std::string date);
  void save_resp(std::string uri, proxy& proxy1);
  HttpResp getResp(HttpRequest& req);
  void requireValid(HttpRequest& req);
  bool compareValidator(HttpRequest& req);
  int HandleRequest(proxy& proxy1);
  int  HandleResp(proxy& proxy1);
  void afterCached(proxy& proxy1);
  ~cache(){}
};
