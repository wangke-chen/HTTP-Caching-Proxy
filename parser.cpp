#include "parser.h"
#include <vector>
#include <string>
#include <iostream>
#include <bits/stdc++.h>
#include <sstream>
#include <algorithm>

int parser::findHeader(std::vector<char>& buffer){
  int index = 0;
  for(; index < buffer.size(); ++index){
    if(buffer[index] == '\r' && buffer[index + 1] == '\n' && buffer[index + 2] == '\r' && buffer[index + 3] == '\n'){
      break;
    }
  }
  return index;
}

int parser::parseRequest(std::vector<char>& buffer){
  int size = findHeader(buffer);
  //req.msg_body.assign(buffer.begin()+size+4, buffer.end());
  //  buffer.resize(size + 4);
  std::cout<<buffer.data()<<std::endl;
  int count = 0;
  int index;
  int content_length_num = 0;
  std::string iter;
  for(index = 0; index < size+4; ++index){
    if(buffer[index] == '\r' && buffer[index + 1] == '\n'){
      index = index + 2;
      count++;
      if(count == 1){
        req.start_line = iter;
	iter.clear();
      }else{
        req.header_field.push_back(iter);
        int pos = iter.find(':');
	std::string key = iter.substr(0, pos);
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        if(key == "content-length"){
	  content_length_num++;
	}
        req.header[key] = iter.substr(pos + 2);
	iter.clear();
      }
    }
    if(content_length_num > 1){
      return -1;
    }
    iter.push_back(buffer[index]);
  }
  return 0;
 }

bool parser::isValidReq(){
  if(req.validStartline() == false) return false;
  if(req.validateHost() == false) return false;
  if(req.validHeaderfield() == false) return false;
  return true;
}

int parser::parseResp(std::vector<char>& buffer){
  int index = findHeader(buffer);
  int i = 0;
  for(i = 0; i < index; i++){
    if(buffer[i-2] == '\r' && buffer[i-1] == '\n'){
      break;
    }
  }
  std::string key;
  std::string value;
  char *head = buffer.data();
  resp.status_line.assign(head, i);
  std::stringstream ss;
  ss<<resp.status_line.substr(resp.status_line.find_first_of(' ')+1,resp.status_line.find_last_of(' '));
  ss>>resp.status_code;
  head = head + i;
  int count = 0;
  int content_length_num = 0;
  for(int j = i; j < index + 4; j++){
    if(buffer[j] == ':' && (count%2) ==0){
      key.assign(head, j-i+1 );
      std::transform(key.begin(), key.end(), key.begin(), ::tolower);
      head = head + j - i +1;
      i = j+1;
      count++;
      if(key == "content-length:"){
	content_length_num++;
      }
    }
    if(buffer[j-2] == '\r' && buffer[j-1] == '\n' && j!=i){
      value.assign(head, j-2-i);
      head = head + j - i;
      i = j;
      count++;
      resp.header_field[key] = value;
    }    
  }
  if(content_length_num > 1){
    return -1;
  }
  return 0;
}

bool parser::isCachable(){
  if((resp.header_field["Cache-control"] != "no=store" || req.header["Cache-control"] != "no-store") && resp.header_field["Cache-control"] != "private" && req.header.find("Authorization") == req.header.end()){
    if(resp.header_field.find("Expires") != resp.header_field.end() || resp.header_field.find("max-age") != resp.header_field.end() || resp.header_field.find("s-maxage") != resp.header_field.end() || (resp.status_code == 200 || resp.status_code == 203 || resp.status_code == 204 || resp.status_code == 300 || resp.status_code == 301 || resp.status_code == 404 || resp.status_code == 405 || resp.status_code == 410 || resp.status_code == 414 || resp.status_code == 501)){
      return true;
    }
  }
  return false;
}
