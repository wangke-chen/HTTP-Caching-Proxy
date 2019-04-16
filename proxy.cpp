#include "proxy.h"
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <algorithm>
#include <sstream>
#include <string.h>
#include <ctime>
#include <mutex>
#include <fstream>
#include <pthread.h>
#define BACKLOG 100

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int establish_socket(std::vector<std::pair<int, std::string>>& connection_info){
  //  char self_ip[INET6_ADDRSTRLEN];
  int status, num;
  struct addrinfo host_info;
  struct addrinfo * host_list;
  const char * port = "12345";
  const char * hostname = NULL;
  int sockfd;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_INET;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  if((status = getaddrinfo(hostname, port, &host_info, &host_list)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    //    exit(EXIT_FAILURE);
    return -1;
  }
  if((sockfd = socket(host_list->ai_family, host_list->ai_socktype, host_list->ai_protocol)) == -1){
    //fprintf(stderr, "socket error: %s\n", strerror(errno));
    //    exit(EXIT_FAILURE);
    freeaddrinfo(host_list);
    return -1;
  }
  int yes = 1;
  status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if(status == -1){
    perror("setsockopt");
    freeaddrinfo(host_list);
    return -1;
  }
  if(bind(sockfd, host_list->ai_addr, host_list->ai_addrlen) == -1){
    fprintf(stderr, "bind(): %s\n", strerror(errno));
    freeaddrinfo(host_list);
    return -1;
  }
  //    inet_ntop(host_info.ai_family, (struct sockaddr_in*)host_info.ai_addr, self_ip, sizeof(self_ip));
    //  std::cout<<"self ip "<<self_ip<<std::endl;
  freeaddrinfo(host_list);
  if(host_list == NULL){
    fprintf(stderr, "server: bind fail\n");
    return -1;
  }
  if(listen(sockfd, BACKLOG) == -1){
    perror("listen");
    return -1;
  }
  std::string ip;
  connection_info.push_back(std::make_pair(sockfd, ip));
  return 0;
}

int proxy::connect_server(){
  std::string port = std::to_string(myparser.req.port_num);
  std::cout<<"port"<<port<<std::endl;
  int rv;
  struct addrinfo hints, *servinfo, *p;
  //  int server_fd;
  //  std::string server_ip;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;
  std::string hostname = myparser.req.hostname;
  //  std::cout<<"server host "<<hostname<<std::endl;
  if ((rv = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	//	resp400code();
	//	close(connection_info[1].first);
	return -1;
    }
  void* addr;
  char ipstr[INET6_ADDRSTRLEN];
  //  extern pthread_rwlock_t rwlock;
  //  pthread_rwlock_rdlock(&rwlock);
 for(p = servinfo; p != NULL; p = p->ai_next) {
   if ((server_fd = socket(p->ai_family, p->ai_socktype,
			   p->ai_protocol)) == -1) {
     // perror("client: socket");
     continue;
   }  
   if(connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
     close(server_fd);
     //perror("client: connect");
     continue;
   }
   
   break;
 }
 if(p->ai_family == AF_INET){
   struct sockaddr_in * ipv4 = (struct sockaddr_in *)p->ai_addr;
   addr = &(ipv4->sin_addr);
   char* ip = inet_ntoa(ipv4->sin_addr);
   std::cout<<"ip "<<ip<<std::endl;
   //     server_ip.assign(ip, ip + strlen(ip));
   server_ip = std::string(ip);
   std::cout<<"**********"<<server_ip<<std::endl;
 }else{
   struct sockaddr_in6 * ipv4 = (struct sockaddr_in6 *)p->ai_addr;
   addr = &(ipv4->sin6_addr);
   inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
   std::cout<<"ipstr"<<ipstr<<std::endl;
   //   server_ip.assign(ipstr, ipstr + strlen(ipstr));
   server_ip = std::string(ipstr);
   std::cout<<"**********"<<server_ip<<std::endl;
 }
 freeaddrinfo(servinfo); // all done with this structure
 std::cout<<"connected with server "<<std::endl;
 return 0;
}

int accept_connection(int fd, std::string ** browser_ip){
  //thread_id = id;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  sin_size = sizeof(their_addr);
  char s[INET6_ADDRSTRLEN];
  int browser_fd = accept(fd, (struct sockaddr*)&their_addr, &sin_size);
   inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr* )&their_addr), s, sizeof(s));
   *browser_ip = new std::string(s);
   std::cout<<**browser_ip<<std::endl;
   // std::string ip;
   //connection_info.push_back(std::make_pair(browser_fd, ip));
   std::cout<<"accepted connection "<<std::endl;
   return browser_fd;
}

void become_daemon(){
  if(daemon(0,0) == -1){
    fprintf(stderr, "daemon: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
   mode_t mask = 0644;
   umask(mask);
  //mask = 0644;
  // umask(mask);
  pid_t pid = fork();
  if(pid == -1){
    fprintf(stderr, "fork: %s\n", strerror(errno));
    exit(0);
  }
  if(pid != 0){
    exit(0); //kill parent
  }  
}

extern pthread_mutex_t loglock;
//extern std::ofstream outfile;

void proxy::lock(){
  extern pthread_mutex_t lock;
  extern int i;
  pthread_mutex_lock(&lock);
  thread_id = i;
  i++;
  pthread_mutex_unlock(&lock);
}
int proxy::recv_from_browser(int fd, std::string **ip){
  browser_fd = fd;
  int num = 0;
  buffer.clear();
  std::vector<char> temp;
  temp.swap(buffer);
  while(1){
    buffer.resize(buffer.size()+1);
   int byte =  recv(browser_fd, buffer.data()+buffer.size()-1, 1, MSG_WAITALL);
   if(byte == 0 || byte == -1) return -1;
   num = num + byte;
    if(*(buffer.end()-1) == '\n' && *(buffer.end()-2)=='\r' && *(buffer.end()-3)=='\n' && *(buffer.end()-4) == '\r'){
      break;
    }
  }
  buffer[num] = '\0';
  if(myparser.parseRequest(buffer) == -1){
    resp400code();
    //    close(browser_fd);
    return -1;
  }
  time_t rawtime;
  struct tm * ptm;
  time(&rawtime);
  ptm = gmtime(&rawtime);
  std::string reqtime = asctime(ptm);
  myparser.req.Reqtime = reqtime;
  //  server_ip.assign(ip,ip+)
  std::cout<<"------------------"<<server_ip<<std::endl;
  std::string output = std::to_string(thread_id) + ": "+myparser.req.start_line+" from "+**ip+" @ "+myparser.req.Reqtime + "\n";

  pthread_mutex_lock(&loglock);
  outfile<<output;
  pthread_mutex_unlock(&loglock);
  
    if(myparser.isValidReq() == false){
      //  resp400code();
      //close(browser_fd);
      //return -1;
    }
  if(myparser.req.header.find("content-length") != myparser.req.header.end()){
    std::string length = myparser.req.header["content-length"];
    std::size_t found = length.find_first_not_of(' ');
    length.erase(0,found);
    //length.erase(length.end()-2, length.end());
    std::stringstream ss;
    int l;
    ss<<length;
    ss>>l;
    int index = myparser.findHeader(buffer);
    //l = l + num - index - 4;
    buffer.resize(index+4+l);
    int byte = recv(browser_fd, buffer.data()+num, l, MSG_WAITALL);
    if(byte == 0 || byte == -1) return -1;
    myparser.req.msg_body.assign(buffer.begin()+num, buffer.end());
    //recvall(connection_info[2].first, l, num);
  }
  else if(myparser.req.header.find("transfer-encoding") != myparser.req.header.end()){
    std::string & encoding = myparser.req.header["transfer-encoding"];
    size_t found1 = encoding.find("chunked");
    size_t found2 = encoding.find(", chunked");
    size_t found3 = encoding.find("chunked, ");
    if(found1 != std::string::npos){
      myparser.req.msg_body = chunk_decode(browser_fd);
    }
    
    if(found1 != std::string::npos){
      if(encoding == " chunked"){
	myparser.req.header.erase("transfer-encoding:");
      }
      else if(found2 != std::string::npos && found3 == std::string::npos){
	encoding.erase(found2, 9);
      }
      else if(found3 != std::string::npos){
	encoding.erase(found3, 9);
      }
    }
    //myparser.req.reconstructor(buffer);
    /* if(myparser.req.header.find("trailer") != myparser.req.header.end()){
      while(1){
	myparser.req.trailer.resize(myparser.req.trailer.size()+1);
	recv(connection_info[1].first, myparser.req.trailer.data()+myparser.req.trailer.size()-1, 1, MSG_WAITALL);
	if(*(myparser.req.trailer.end()-1) == '\n' && *(myparser.req.trailer.end()-2)=='\r' && *(myparser.req.trailer.end()-3)=='\n' && *(myparser.req.trailer.end()-4) == '\r'){
	  break;
	}  
      }
      for(int i = myparser.req.status_line.size()-1; i>=0; i--){
	myparser.req.trailer.insert(myparser.req.trailer.begin(), myparser.req.start_line[i]);
      }
      myparser.parseReq(myparser.req.trailer);
      myparser.req.header.erase("trailer");
      }*/
  }
  myparser.req.reconstruct(buffer);
  //  std::cout<<"reconstructed buffer "<<buffer.data()<<std::endl;
  
  return 0;
}

int proxy::send_to_server(){
  int num = send(server_fd, buffer.data(), buffer.size(), 0);
  std::string output = std::to_string(thread_id)+": Requesting "+myparser.req.start_line+" from "+ myparser.req.hostname+"\n";
  pthread_mutex_lock(&loglock);
  outfile<<output;
  pthread_mutex_unlock(&loglock);
  if(num == 0 || num ==1){
    return -1;
  }
  return 0;
}

int proxy::recv_from_server(){
  //  std::vector<char> buffer(2048);
  buffer.clear();
   //   buffer.resize(buffer.size(), '\0');
  std::vector<char> temp;
  temp.swap(buffer);
  //  buffer.erase(buffer.begin(),buffer.end());
  //  buffer.clear();
  std::cout<<"receiving from server..."<<std::endl;
  int num = 0;
  while(1){
    buffer.resize(buffer.size()+1);

    int byte = recv(server_fd, buffer.data()+buffer.size()-1, 1, MSG_WAITALL);
    if(byte == 0 || byte == -1) return -1;
    num = num + byte;
    if(*(buffer.end()-1) == '\n' && *(buffer.end()-2)=='\r' && *(buffer.end()-3)=='\n' && *(buffer.end()-4) == '\r'){
      break;
    }  
  }
      myparser.parseResp(buffer);
      std::string output;
      output = std::to_string(thread_id)+": Received "+myparser.resp.status_line+" from "+myparser.req.uri+"\n";
       pthread_mutex_lock(&loglock);
      outfile<<output;
      pthread_mutex_unlock(&loglock);

  if(myparser.parseResp(buffer) == -1){
    resp502code();
    close(server_fd);
    return -1;
  }
  time_t rawtime;
  struct tm* ptm;
  time(&rawtime);
  ptm = gmtime(&rawtime);
  std::string Resptime = asctime(ptm);
  myparser.resp.resptime = Resptime;
  if(myparser.resp.header_field.find("content-length:") != myparser.resp.header_field.end()){
    std::string length = myparser.resp.header_field["content-length:"];
    std::size_t found = length.find_first_not_of(' ');
    length.erase(0,found);
    //length.erase(length.end()-2, length.end());
    std::stringstream ss;
    int l;
    ss<<length;
    ss>>l;
    int index = myparser.findHeader(buffer);
    //l = l + num - index - 4;
     buffer.resize(index+4+l);
    int byte = recv(server_fd, buffer.data()+num, l, MSG_WAITALL);
    if(byte == 0 || byte == -1) return -1;
    myparser.resp.msg_body.assign(buffer.begin()+num, buffer.end());
  }
  else if(myparser.resp.header_field.find("transfer-encoding:") != myparser.resp.header_field.end()){
    std::string & encoding = myparser.resp.header_field["transfer-encoding:"];
    size_t found1 = encoding.find("chunked");
    size_t found2 = encoding.find(", chunked");
    size_t found3 = encoding.find("chunked, ");
    if(found1 != std::string::npos){
      myparser.resp.msg_body = chunk_decode(server_fd);
    }

    if(found1 != std::string::npos){
      if(encoding == " chunked"){
	myparser.resp.header_field.erase("transfer-encoding:");
      }
      else if(found2 != std::string::npos && found3 == std::string::npos){
	encoding.erase(found2, 9);
      }
      else if(found3 != std::string::npos){
	encoding.erase(found3, 9);
      }
    }
    myparser.resp.reconstructor(buffer);
    //    std::cout<<"-------------------- reponse-----------------------"<<std::endl;
    //std::cout<<buffer.data()<<std::endl;
    /* if(myparser.resp.header_field.find("trailer:") != myparser.resp.header_field.end()){
      while(1){
	myparser.resp.trailer.resize(myparser.resp.trailer.size()+1);
	recv(connection_info[2].first, myparser.resp.trailer.data()+myparser.resp.trailer.size()-1, 1, MSG_WAITALL);
	if(*(myparser.resp.trailer.end()-1) == '\n' && *(myparser.resp.trailer.end()-2)=='\r' && *(myparser.resp.trailer.end()-3)=='\n' && *(myparser.resp.trailer.end()-4) == '\r'){
	  break;
	}  
      }
      for(int i = myparser.resp.status_line.size()-1; i>=0; i--){
	myparser.resp.trailer.insert(myparser.resp.trailer.begin(), myparser.resp.status_line[i]);
      }
      myparser.parseResp(myparser.resp.trailer);
      myparser.resp.header_field.erase("trailer:");
      }*/
  }
  // std::cout<<"-------------------- reponse-----------------------"<<std::endl;
  //std::cout<<buffer.data()<<std::endl;
  return 0;

}


int proxy::send_to_browser(){
  //  std::cout<<"buffer "<<buffer.data()<<std::endl;
    int num = send(browser_fd, buffer.data(), buffer.size(), 0);
  //  sendall(connection_info[1].first, )
    if(num == 0 || num == 1){
      return -1;
    }
    return 0;
}

//first receive header->parse to get content length->loop to receive all
void proxy::recvall(int fd, int length, int rec){
  int total = 0;
  int num = 0;
  while(total < length){
    if(total > buffer.size()/2){
      int orignal = buffer.size();
      buffer.resize(orignal * 2);
    }
    num = recv(server_fd, buffer.data()+num+rec, length-total, 0);
    if(num <= 0){
      std::cerr<<"recv error "<<std::endl;
      break;
    }
    total += num;
  }
}
void proxy::sendall(int fd, int size){
  char* ptr = buffer.data();
  int sent = 0;
  int total = 0;
  while(total < size){
    sent = send(fd, ptr, size - sent, MSG_WAITALL);
    total += sent;
  }
}
std::vector<char> proxy::chunk_decode(int fd){
  while(1){
    buffer.resize(buffer.size()+1);
    int byte = recv(fd, buffer.data()+buffer.size()-1, 1, MSG_WAITALL);
    if(*(buffer.end()-1) == '\n' && *(buffer.end()-2)=='\r'){
      break;
    }
  }
  
  int index = myparser.findHeader(buffer);
  std::vector<char> decoded_body;
  index = index + 4;
  int i = index;
  int count = 0;
  int length = 0;
  int chunk_size = 0;
  
  for(i = index; i < buffer.size(); i++){
    if(buffer[i] == '\r' && buffer[i+1] == '\n' ){
      if(count == 0){
	std::string l;
	l.assign(buffer.data()+index, i - index);
	l.insert(0,"0x");
	sscanf(l.c_str(),"%x", &chunk_size);
	length+=chunk_size;
	count = (count + 1) %2;
	index = i+2;
	if(chunk_size == 0){
	  break;
	}	
      }
      else if((i - index) == chunk_size){
	std::vector<char>::iterator head;
	if(decoded_body.empty()){
	  head = decoded_body.begin();
	}
	else{
	  head = decoded_body.end();
	}
	decoded_body.insert(head, buffer.data()+index, buffer.data()+i);
	count = (count + 1) %2;
	index = i+2;
      }
    }
    
  }
  
  //std::cout<<decoded_body.data()<<std::endl;
  if(i == buffer.size()){
    while(1){
      if(count == 0){
	while(1){
	  buffer.resize(buffer.size()+1);
	  recv(fd, buffer.data()+i, 1, MSG_WAITALL);
	  if(buffer[i] == '\n' && buffer[i-1]=='\r'){
	    break;
	  }
	  i++;
	}
	std::string l;
	l.assign(buffer.data()+index, i - index -1);
	l.insert(0,"0x");
	sscanf(l.c_str(),"%x", &chunk_size);
	length+=chunk_size;
	index = i + 1;
      }
      else{
	buffer.resize(buffer.size()+chunk_size+2);
	i = buffer.size();
	recv(fd, buffer.data()+index, chunk_size+2, MSG_WAITALL);
	decoded_body.insert(decoded_body.end(), buffer.data()+ index, buffer.data()+i-2);
	index = i;
      }
      count = (count + 1)%2;
      if(chunk_size == 0){
	buffer.resize(buffer.size()+2);
	recv(fd, buffer.data()+buffer.size()-2, 2, MSG_WAITALL);
	std::string content_length;
	content_length.append(" ");	  
	content_length += std::to_string(length);
	myparser.resp.header_field["content-length:"] = content_length;
	break;
      }
    }
  }
  else{
    std::cout<<"length="<<length<<std::endl;
    std::string content_length;
    content_length.append(" ");	  
    content_length += std::to_string(length);
    //content_length += "\r\n";
    myparser.resp.header_field["content-length:"] = content_length;
  }
  return decoded_body;
}

void proxy::resp400code(){
  std::string httpver = myparser.req.start_line;
  httpver = myparser.req.start_line.substr(myparser.req.start_line.find_first_of(' ')+1);
  httpver = httpver.substr(httpver.find_first_of(' ')+1);
  myparser.resp.status_line = httpver + " 400 Bad Request\r\n\r\n";
  std::cout<<thread_id<<": Responding "<<myparser.resp.status_line<<std::endl;
  myparser.resp.status_line = httpver + " 400 Bad Request";
}

void proxy::resp502code(){
  std::string httpver = myparser.req.start_line;
  httpver = myparser.req.start_line.substr(myparser.req.start_line.find_first_of(' ')+1);
  httpver = httpver.substr(httpver.find_first_of(' ')+1);
  myparser.resp.status_line = httpver + " 502 Bad Gateway";
}

std::string proxy::dateFormat(std::string date){
  //  std::string date = myparser.resp.header_field["Date"];
  if(date.find(',') == std::string::npos){
    return date;
  }else{
    std::string dayname = date.substr(0, date.find(','));
    date = date.substr(date.find(',') + 2);
    std::vector<std::string> dd;
    while(date.find(' ') != std::string::npos){
      int pos = date.find(' ');
      dd.push_back(date.substr(0, pos));
      date = date.substr(pos + 1);
    }
    std::stringstream ss;
    int day;
    ss<<dd[0];
    ss>>day;
    if(day > 10){
      date = dayname + " " + dd[1] + " " + dd[0] + " " + dd[3] + " " + dd[2];
    }else{
      date = dayname + " " + dd[1] + "  " + std::to_string(day) + " " + dd[3] + " " + dd[2];
    }
  }
  return date;
}

void proxy::establishTunnel(){
  if(myparser.req.method == "CONNECT"){
    if(myparser.req.isValidConnect()){
      connect_server();
      std::string httpver;
      int pos = myparser.req.start_line.find_last_of(' ');
      for(int i = pos+1; i < myparser.req.start_line.size(); i++){
	if(myparser.req.start_line[i] == '\r') break;
	httpver.append(1, myparser.req.start_line[i]);
      }
      std::string data = httpver + " 200 OK\r\n\r\n";
      std::cout<<data<<std::endl;
      /*  buffer.resize(data.size());
	  for(int i = 0; i < data.size(); i++){
	  buffer[i] = data[i];
	  }
	  std::cout<<buffer.data()<<std::endl;
	  
	  buffer.resize(2048);*/
      //  send_to_server();
      int byte = send(browser_fd, data.c_str(), data.size(), 0);
      fd_set master;
      fd_set read_fds;
      struct timeval tv;
      tv.tv_sec = 2;
      tv.tv_usec = 500000;
      FD_ZERO(&master);
      FD_ZERO(&read_fds);
      FD_SET(server_fd, &master);
      FD_SET(browser_fd, &master);
      int fdmax = std::max(browser_fd, server_fd);
      while(1){
	read_fds = master;
	if(select((fdmax + 1), &read_fds, NULL, NULL, NULL) == -1){
	  break;
	}
	if(FD_ISSET(browser_fd, &read_fds)){
	  buffer.resize(10000,'\0');
	  int num = recv(browser_fd, buffer.data(), buffer.size(), 0);
	  if(num == 0 || num == -1) break;
	  num = send(server_fd, buffer.data(), num, MSG_WAITALL);
	   if(num == 0 || num == -1) break;
	}
	if(FD_ISSET(server_fd, &read_fds)){
	  buffer.resize(10000, '\0');
	  int num = recv(server_fd, buffer.data(), buffer.size(), 0);
	  if(num == 0 || num == -1) break;
	  num =  send(browser_fd, buffer.data(), num, MSG_WAITALL);
	  if(num == 0 || num == -1) break;
	}
      }
      //      close(browser_fd);
      //      close(server_fd);
      //      extern pthread_mutex_lock loglock;
      std::string output = std::to_string(thread_id)+": Tunnel closed\n";
      pthread_mutex_lock(&loglock);
      outfile<<output;
      pthread_mutex_unlock(&loglock);
    }
  }
}

void proxy::getReq(){
  if(myparser.req.method == "GET"){
    connect_server();
    send_to_server();
    recv_from_server();
    send_to_browser();
  }
}

int proxy::postReq(){
  if(myparser.req.method == "POST"){
    if(connect_server() == -1) return -1;
    if(send_to_server() == -1) return -1;
    if(recv_from_server() == -1) return -1;
    if(send_to_browser() == -1) return -1; 
  }
  return 0;
}


