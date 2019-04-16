#include "cache.h"
//#include "proxy.h"
#include <cstdlib>
#include <vector>
#include <cstdio>
#include <thread>
#include <string>
#include <functional>
#include <iostream>
#include <atomic>
#include <chrono>
#include <utility>
#include <pthread.h>
#include <mutex>
#include <unistd.h>
#include <fstream>

#define THREAD_NUM 1000

int i = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
std::vector<std::pair<int, std::string> > connection_info;
cache mycache(50);
pthread_mutex_t cachelock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t loglock = PTHREAD_MUTEX_INITIALIZER;
struct args{
  int fd;
  std::string * ip;
};

void* proxy_execute(void * argss){
  //  int fd = *((int*)arg);
  struct args* arg = (struct args*)argss;
  proxy myproxy;
  std::cout<<*(arg->ip)<<std::endl;
  //  myproxy.accept_connection(i);
  //  std::ofstream outfile("/home/wg72/proxy/proxy.log", std::ios::app);
  myproxy.lock();
  if(myproxy.recv_from_browser(arg->fd, &(arg->ip)) == -1){
    close(myproxy.getBrowser());
    return NULL;
  }
  if(mycache.HandleRequest(myproxy) == -1){
    close(myproxy.getBrowser());
    close(myproxy.getServer());
    return NULL;
  }
  myproxy.establishTunnel();
  if(myproxy.postReq() == -1){
    close(myproxy.getBrowser());
    close(myproxy.getServer());
  return NULL;
  }
  close(myproxy.getBrowser());
  close(myproxy.getServer());
  myproxy.outfile.close();
}

int main(){
  pthread_t threads;
  while(establish_socket(connection_info) != 0){;}
  //    become_daemon();   
  while(1){
    struct args arg;
    std::string * browser_ip;
    arg.fd = accept_connection(connection_info[0].first, &browser_ip);
    arg.ip = browser_ip;
    pthread_create(&threads, NULL, proxy_execute, &arg);
  }
  
  return EXIT_SUCCESS;
}
