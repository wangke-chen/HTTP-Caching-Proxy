#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#define BACKLOG 10
#define MAXDATASIZE 1024

std::string parser(std::vector<char>& buffer){
  int index = 0;
  for(; index < buffer.size(); index++){
    if(buffer[index] == '\r' && buffer[index + 1] == '\n' && buffer[index + 2] ==
      '\r' && buffer[index + 3] == '\n'){
      //      std::cout<<"one carriage return found "<<index<<std::endl;
      break;
    }
  }
  buffer.resize(index + 4);
  std::cout<<buffer.data()<<std::endl;
  int count = 0;
  int begin = 0;
  int end = 0;
  for(index = 0; index < buffer.size(); ++index){
    if(buffer[index] == '\n'){
      ++count;
      if(count == 1){
        begin = index + 1;
	continue;
      }
      if(count == 2){
	end = index;
	break;
   }
   }
  }
  std::string hostname;
  for(int i = begin + 1; i < end - 1; ++i){
    if(buffer[i] == ':'){
      index = i;
    }
    if(i >= index + 2){
      //      std::cout<<hostname<<std::endl;
      hostname.append(1, buffer[i]);
   }
  }
  //  hostname.append(1, '\0');
  return hostname;
}

int main(int argc, char * argv[]){
  //  char buffer[MAXDATASIZE];
  std::vector<char> buffer(MAXDATASIZE);
  int status, num;
  int sockfd, new_fd;
  struct addrinfo host_info;
  struct addrinfo * host_list;
  const char * port = "12345";
  char * hostname = NULL;
  socklen_t sin_size;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;
  //load up address struct

  if( (status = getaddrinfo(hostname, port, &host_info, &host_list)) != 0){
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }
  // make a socket;
  if( (sockfd = socket(host_list->ai_family, host_list->ai_socktype, host_list->ai_protocol)) == -1){
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  //printf("ringmaster fd: %d\n", sockfd);
  int yes = 1;
  //lose "address already in use" error message;
  status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if(status == -1){
    perror("setsockpot");
    exit(EXIT_FAILURE);
  }
  // bind it to the port passed in
  if(bind(sockfd, host_list->ai_addr, host_list->ai_addrlen) == -1){
    fprintf(stderr, "bind() error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }     
  freeaddrinfo(host_list);
  if(host_list == NULL){
    fprintf(stderr, "server: bind fail\n");
    exit(EXIT_FAILURE);
  }
  if(listen(sockfd, BACKLOG) == -1){
    perror("listen");
    exit(EXIT_FAILURE);
  }
  struct sockaddr_storage their_addr;
  sin_size = sizeof(their_addr);
  new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
  std::string request;
  char client_ip[1024];
  char client_service[20];
  while(1){
    num = recv(new_fd, buffer.data(), buffer.size(), 0);
    buffer[num] = '\0';
    const char * http_port = "80"; 
    std::cout<<buffer.data()<<std::endl;
    std::string hostname = parser(buffer);
    std::cout<<"hostname"<<hostname<<std::endl;
    std::cout<<hostname.c_str()<<std::endl;
    //    std::cout<<buffer.data()<<std::endl;
    /* connect to real webserver */
    int rv;
    int server_fd;
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    if ((rv = getaddrinfo(hostname.c_str(), http_port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((server_fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_fd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

     freeaddrinfo(servinfo); // all done with this structure
     send(server_fd, buffer.data(), buffer.size(), 0);
     recv(server_fd, buffer.data(), buffer.size(), MSG_WAITALL);
     std::cout<<buffer.data()<<std::endl;

  }
  return EXIT_SUCCESS;
}
