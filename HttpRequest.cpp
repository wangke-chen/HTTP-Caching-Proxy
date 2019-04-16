#include "HttpRequest.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>


bool HttpRequest::validStartline(){
  int count = 0;
  std::string temp = start_line;
  while(temp.find_first_of(' ') != std::string::npos){
    count++;
    temp = temp.substr(temp.find_first_of(' ') + 1);
  }
  if(count != 2) return false;
  method = start_line.substr(0, start_line.find_first_of(' '));
  uri = start_line.substr(start_line.find_first_of(' ')+1);
  uri = uri.substr(0, uri.find(' '));
  /*  if(ifAbsurl() == false){
    uri = header["Host"]+uri;
  }
  uri = start_line.substr(start_line.find_first_of(' ')+1, start_line.find_last_of(' '));*/
  return true;
}

void HttpRequest::removeWSpace(){
  for(int i = 0; i < header_field.size(); i++){
    int pos = header_field[i].find(':');
    if(header_field[i][pos - 1] == ' '){
      header_field[i].erase(header_field[i].begin()+pos-1);
    }
  }
}

bool HttpRequest::validateHost(){
  int count = 0;
  int hostindex = 0;
  for(int i = 0; i < header_field.size(); i++){
    if(header_field[i].find("Host") != std::string::npos){
      count++;
      if(count == 1){ hostindex = i;}
    }
  }
  if(count > 1 || count < 1){
    return false;  //must respond with 400 Bad Request
  }
  hostname = header_field[hostindex].substr(header_field[hostindex].find(':')+1);
  //if absolute url, replace host with target request host
  if(ifAbsurl()){
    std::string newhost = start_line.substr(start_line.find_first_of(' ')+1);
    newhost = newhost.substr(0, newhost.find_first_of(' '));
    int pos = newhost.find("//");
    newhost = newhost.substr(pos+2);
    if(newhost.find('/') != std::string::npos){
    pos = newhost.find('/');
    newhost = newhost.substr(0, pos);
    }
    hostname = newhost;
    std::cout<<"new host"<<newhost<<std::endl;
    pos = header_field[hostindex].find(':');
    header_field[hostindex].replace(pos+2, header_field[hostindex].size()-1, newhost); //what if no OWS, what if trailing OWS
  }else{
    uri = header["host"] + uri;
  }
  return true;
}

bool HttpRequest::ifAbsurl(){
  if(validStartline()){
    std::string url = start_line.substr(start_line.find_first_of(' ') + 1);
    url = url.substr(0, url.find_first_of(' '));
    //BEGIN_REF https://stackoverflow.com/questions/10687099/how-to-test-if-a-url-string-is-absolute-or-relative
    if(url.find("//") == 0) return true;
    if(url.find("://") == std::string::npos) return false;
    if(url.find('.') == std::string::npos) return false;
    if(url.find('/') == std::string::npos) return false;
    if(url.find(':') > url.find('/')) return false;
    if(url.find("://") < url.find('.')) return true;
    return false;
    //END_REF
  }
  return false;
}

bool HttpRequest::validHeaderfield(){
  for(int i = 0; i < header_field.size(); i++){
    if(header_field[i].find_first_of(":") != header_field[i].find_last_of(":")){
    if(header_field[i].find("cookie") == std::string::npos){
      return false;
      }
    }
  }
  return true;
}

void HttpRequest::reconstruct(std::vector<char>& buffer){
     int i = 0;
     start_line.append("\r\n");
     for(; i < start_line.size(); i++){
       buffer[i] = start_line[i];
     }
     std::unordered_map<std::string, std::string>::iterator it = header.begin();
     for(; it != header.end(); ++it){
       for(int j = 0; j < it->first.size(); j++){
	 buffer[i] = it->first[j];
	 ++i;
       }
       buffer[i] = ':';
       buffer[i+1] = ' ';
       i =i+2;
       for(int j = 0; j < it->second.size(); j++){
         buffer[i] = it->second[j];
	 ++i;
       }
       buffer[i] = '\r';
       buffer[i+1] = '\n';
       i = i + 2;
     }
     buffer[i] = '\r';
     buffer[i+1] = '\n';
     i = i+2;
     for(int j = 0; j < msg_body.size(); j++){
       buffer[i] = msg_body[j];
       ++i;
     }
}

//removing all Connection header field??
void HttpRequest::removeConnection(){
  for(int i = 0; i < header_field.size(); ++i){
    if(header_field[i].find("connection") != std::string::npos){
      header_field.erase(header_field.begin() + i);
    }
  }
}

bool HttpRequest::isValidConnect(){
    std::string start = start_line.substr(start_line.find(' ')+1);
    start = start.substr(0, start.find(' '));
    if(start.find(':') != std::string::npos){
      hostname = start.substr(0, start.find(':'));
      std::stringstream ss;
      ss<<start.substr(start.find(':')+1);
      ss>>port_num;
      //port_num = std::stoi(start.substr(start.find(':')+1), nullptr, 10);
      return true;
    }else{
      return false; //400 bad request
    }
}

