#include "HttpResp.h"

void HttpResp::reconstructor(std::vector<char> & buffer){
  buffer.clear();
  std::vector<char>().swap(buffer);
  int i = 0;
  for(; i < status_line.size(); i++){
    buffer.push_back(status_line[i]);
  }
  std::unordered_map<std::string, std::string>::iterator it=header_field.begin();
  for(; it!=header_field.end(); ++it){
    for(int j = 0; j < it->first.size(); j++){
        buffer.push_back(it->first[j]);
	//    ++i;
    }
    for(int j = 0; j < it->second.size(); j++){
      buffer.push_back(it->second[j]);
      //      ++i;
    }
    buffer.push_back('\r');
    buffer.push_back('\n');
    //    i = i + 2;
  }
  buffer.push_back('\r');
  buffer.push_back('\n');
  //  i=i+2;

  for(int j = 0; j < msg_body.size(); j++){
    buffer.push_back(msg_body[j]);
    ++i;
  }
}
