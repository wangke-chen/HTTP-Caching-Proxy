#include <ctime>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <pthread.h>
#include "cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
extern pthread_mutex_t loglock;
//extern std::ofstream outfile;
int cache::getLifetime(std::string uri){
  int lifetime;
  if(info[uri]->second.header_field.find("cache-control:") != info[uri]->second.header_field.end()){
    if(info[uri]->second.header_field["cache-control:"].find("s-maxage") != std::string::npos){
    std::string smax = info[uri]->second.header_field["cache-control:"];
    int pos = info[uri]->second.header_field["cache-control:"].find("s-maxage");
    smax = smax.substr(pos+9);
    pos = smax.find_first_not_of("1234567890");
    smax = smax.substr(0, pos);
    std::stringstream ss;
    ss<<smax;
    ss>>lifetime;
    //    lifetime = std::stoi(smax, nullptr, 10);
    return lifetime;
    }
    if(info[uri]->second.header_field["cache-control:"].find("max-age") != std::string::npos){
    std::string max = info[uri]->second.header_field["cache-control:"];
    int pos = max.find("max-age");
    max = max.substr(pos+8);
    pos = max.find_first_not_of("1234567890");
    max = max.substr(0, pos);
    std::stringstream ss;
    ss<<max;
    ss>>lifetime;
    //    lifetime = std::stoi(max, nullptr, 10);
    return lifetime;
    }
  }
  if(info[uri]->second.header_field.find("expires:") != info[uri]->second.header_field.end()){
    std::string expire = info[uri]->second.header_field["expires:"];
    if(info[uri]->second.header_field.find("date:") != info[uri]->second.header_field.end()){
    std::string date = info[uri]->second.header_field["date:"];
    
    struct tm exp;
    memset(&exp, 0, sizeof(struct tm));
    strptime(expire.c_str(), "%W, %d %m %Y %T %Z", &exp);
    struct tm datetime;
    memset(&datetime, 0, sizeof(struct tm));
    strptime(date.c_str(), "%W, %d %m %Y %T %Z", &datetime);
  //    expire = proxy::dateFormat(expire);
  // expire = proxy::dateFormat(date);
  // lifetime = DateMinus(expire, date);
    time_t exp_t = mktime(&exp);
    time_t date_t = mktime(&datetime);
    lifetime = difftime(exp_t, date_t);
    return lifetime;
  }
  }
  return lifetime;
}

bool cache::isFresh(proxy& proxy1){
  int life_time = getLifetime(proxy1.myparser.req.uri);
  std::cout<<"Life time get"<<std::endl;
  int current_age = getCurrentAge(proxy1);
  std::cout<<"current age get"<<std::endl;
  bool result = false;
  if(life_time > current_age) result = true;
  std::cout<<result<<std::endl;
  return result;
}

int cache::getCurrentAge(proxy& proxy1){
  time_t rawtime;
  struct tm * ptm;
  time(&rawtime);
  ptm = gmtime(&rawtime);
  std::string nowtime = asctime(ptm);
  std::string resptime = info[proxy1.myparser.req.uri]->second.resptime;
  struct tm restime;
  memset(&restime, 0, sizeof(struct tm));
  strptime(resptime.c_str(), "%W, %d %m %Y %T %Z", &restime);
  std::string date_value = info[proxy1.myparser.req.uri]->second.header_field["date:"];
  struct tm date;
  memset(&date, 0, sizeof(struct tm));
  strptime(date_value.c_str(), "%W, %d %m %Y %T %Z", &date);
  time_t rtime = mktime(&restime);
  time_t d = mktime(&date);
  double apparent_age = std::max(0.0, difftime(rtime, d));
  std::string requesttime = proxy1.myparser.req.Reqtime;
  struct tm reqtime;
  memset(&reqtime, 0, sizeof(struct tm));
  strptime(requesttime.c_str(), "%W %m %d %T %Y", &reqtime);
  time_t rqtime = mktime(&reqtime);
  double response_delay = difftime(rtime, rqtime);
  int age;
  if(info[proxy1.myparser.req.uri]->second.header_field.find("age:") != info[proxy1.myparser.req.uri]->second.header_field.end()){
  std::string Age = info[proxy1.myparser.req.uri]->second.header_field["age:"];
  std::stringstream ss;
  ss<<Age;
  ss>>age;
  }  //  double corrected_age_value = std::stoi(info[proxy1.myparser.req.uri]->second.header_field["Age"]) + response_delay;
  double corrected_age_value = age + response_delay;
  double corrected_intial_age = std::max(apparent_age, corrected_age_value);
  double resident_time = difftime(rawtime, rtime);
  //  double resident_time = DateMinus(nowtime, resptime);
  double current_age = corrected_intial_age + resident_time;
  return current_age;
}

bool cache::isIncache(std::string uri){
  if(info.find(uri) == info.end()){
    return false;
  }
  return true;
}

int cache::handleResp(HttpResp& resp){
  if(resp.status_code == 304){
    return 0; 
  }
  if(!resp.msg_body.empty()){
      return 1;
  }
   if(resp.status_code > 500){
     return 1;
  }
}

void cache::save_resp(std::string uri, proxy& proxy1){
  HttpResp copy = proxy1.myparser.resp;
  std::unordered_map<std::string, std::list<std::pair<std::string, HttpResp> >::iterator>::iterator iter = info.find(uri);
  if(iter == info.end()){
  if(storage.size() == capacity){
    std::string lru = storage.back().first;
    std::string output = "(no id): evited "+lru+" from cache\n";
      pthread_mutex_lock(&loglock);
    proxy1.outfile<<output;
    pthread_mutex_unlock(&loglock);
    info.erase(lru);
  storage.pop_back();
  storage.push_front(std::make_pair(uri, copy));
  info[uri] = storage.begin();
  }else{
    storage.push_front(std::make_pair(uri, copy));
    info[uri] = storage.begin();
  }
  }
  else{
    std::list<std::pair<std::string, HttpResp> >::iterator it =    iter->second;
    it->second = proxy1.myparser.resp;
    storage.splice(storage.begin(), storage, it);
    info[uri] = storage.begin();
  }
}

HttpResp cache::getResp(HttpRequest& req){
  std::unordered_map<std::string, std::list<std::pair<std::string, HttpResp> >::iterator>::iterator iter = info.find(req.uri);
  std::list<std::pair<std::string, HttpResp> >::iterator it = iter->second;
  storage.splice(storage.begin(), storage, it);
  info[req.uri] = storage.begin();
  return info[req.uri]->second;
}

void cache::requireValid(HttpRequest& req){
  HttpResp resp = info[req.uri]->second;
  if(resp.header_field.find("etag:") != resp.header_field.end()){
    if(req.header.find("if-none-match") == req.header.end()){
      req.header["if-none-match"] = resp.header_field["etag:"];
    }else{
      req.header["if-none-match"].push_back(',');
      req.header["if-none-match"].append(resp.header_field["etag:"]);
    }
  }
  else{
    if(resp.header_field.find("last-modified:") != resp.header_field.end()){
      if(req.header.find("if-modified-since") == req.header.end()){
	req.header["if-modified-since"] = resp.header_field["last-modified:"];
      }else{
        req.header["if-none-match"].push_back(',');
	req.header["if-none-match"].append(resp.header_field["last-modified:"]);
      }
    }
  }
}

bool cache::compareValidator(HttpRequest& req){
  HttpResp resp = info[req.uri]->second;
  if(req.header.find("if-none-match") != req.header.end()){
    std::string etag = resp.header_field["etag:"];
    std::string validator = req.header["if-none-match"];
    if(etag.size() != validator.size()) return false;
    for(int i = 0; i < validator.size(); i++){
      if(etag[i] != validator[i]){
	return false;
      }
    }
    return true;
  }
  if(req.header.find("if-not-modified") != req.header.end()){
    std::string last = resp.header_field["last-modified:"];
    //    last = proxy::dateFormat(last);
    struct tm lastm;
  memset(&lastm, 0, sizeof(struct tm));
  strptime(last.c_str(), "%W, %d %m %Y %T %Z", &lastm);
  time_t last_t = mktime(&lastm);
    std::string validator = req.header["if-not-modified"];
    //    validator = proxy::dateFormat(last);
    struct tm valid;
  memset(&valid, 0, sizeof(struct tm));
  strptime(validator.c_str(), "%W, %d %m %Y %T %Z", &valid);
  time_t valid_t = mktime(&valid);
  double diff = difftime(valid_t, last_t);
  //    double diff = DateMinus(validator, last);
    if(diff > 0) return true;
    else return false;
  }
  return false;
}

void cache::afterCached(proxy& proxy1){
  if(proxy1.myparser.resp.status_code == 200){
    if(proxy1.myparser.resp.header_field.find("cache-control:") != proxy1.myparser.resp.header_field.end()){
    std::string reason = proxy1.myparser.resp.header_field["cache-control:"];
    if(reason.find("private") != std::string::npos){
      std::string output = std::to_string(proxy1.thread_id) + "not cachable because private\n";
      pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
      return;
    }
    if(reason.find("no-store") != std::string::npos){
      std::string output = std::to_string(proxy1.thread_id) + "not cachable because no-store\n";
       pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
      return;
    }
    if(reason.find("must-revalidation") != std::string::npos || reason.find("proxy-revalidation") != std::string::npos){
      std::string output = std::to_string(proxy1.thread_id) + "cached, but require re-validation\n";
       pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
      return;
    }
    }
  if(proxy1.myparser.resp.header_field.find("expires:") != proxy1.myparser.resp.header_field.end()){
    std::string output = std::to_string(proxy1.thread_id) + "cached expired at "+proxy1.myparser.resp.header_field["expires:"]+"\n";
    pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
    return;
  }
   std::string output = std::to_string(proxy1.thread_id) + "cached, but require re-validation\n";
       pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
      return;
  }
}
int cache::HandleRequest(proxy & proxy1){
  extern pthread_mutex_t cachelock;
  std::cout<<proxy1.myparser.req.method<<std::endl;
  //  std::cout<<"in handle request "<<std::endl;
  if(proxy1.myparser.req.method != "GET") return 0;
   pthread_mutex_lock(&cachelock);
  if(!isIncache(proxy1.myparser.req.uri)){
    std::string output = std::to_string(proxy1.thread_id) + " not in cache\n";
     pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
    if(proxy1.connect_server() == -1){
      pthread_mutex_unlock(&cachelock);
      return -1;
    }
    if(proxy1.send_to_server() == -1){
         pthread_mutex_unlock(&cachelock);
	 return -1;
    }
    //if(proxy1.recv_from_server() == -1) return -1;
    if(proxy1.recv_from_server() == -1){
      pthread_mutex_unlock(&cachelock);
      return -1;
    }
    if(proxy1.myparser.req.header.find("cache-control") != proxy1.myparser.req.header.end() && (proxy1.myparser.req.header["cache-control"].find("private") != std::string::npos || proxy1.myparser.req.header["cache-control"].find("no-store") != std::string::npos)){
      if(proxy1.send_to_browser() == -1){
        pthread_mutex_unlock(&cachelock);
	return -1;
      }
      afterCached(proxy1);
    }else{
      save_resp(proxy1.myparser.req.uri, proxy1);
      afterCached(proxy1);
      if(proxy1.send_to_browser() == -1){
         pthread_mutex_unlock(&cachelock);
	 return -1;
      }
    }
    pthread_mutex_unlock(&cachelock);
    return 0;
 }
  if(isFresh(proxy1)){
   if(proxy1.myparser.req.header.find("cache-control") != proxy1.myparser.req.header.end() && proxy1.myparser.req.header["cache-control"].find("no-cache") != std::string::npos){
     requireValid(proxy1.myparser.req);
     std::string output = std::to_string(proxy1.thread_id) + "in cache, require validation \n";
     pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
     proxy1.myparser.req.reconstruct(proxy1.buffer);
     if(proxy1.send_to_server() == -1){
        pthread_mutex_unlock(&cachelock);
	return -1;
     }
     // if(proxy1.recv_from_server() == -1) return -1;
     if(proxy1.recv_from_server() == -1){
       pthread_mutex_unlock(&cachelock);
       return -1;
     }
     save_resp(proxy1.myparser.req.uri, proxy1);
     if(HandleResp(proxy1) == -1){
        pthread_mutex_unlock(&cachelock);
	return -1;
     }
   }
   else{
     if(proxy1.myparser.resp.header_field.find("cache-control:") != proxy1.myparser.resp.header_field.end() && (proxy1.myparser.resp.header_field["cache-control:"].find("must-revalidation") != std::string::npos || proxy1.myparser.resp.header_field["cache-control"].find("proxy-revalidation") != std::string::npos)){
       requireValid(proxy1.myparser.req);
       std::string output = std::to_string(proxy1.thread_id) + "in cache, require validation \n";
       pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
       proxy1.myparser.req.reconstruct(proxy1.buffer);
       if(proxy1.send_to_server() == -1){
           pthread_mutex_unlock(&cachelock);
	   return -1;
       }
         if(proxy1.recv_from_server() == -1){
             pthread_mutex_unlock(&cachelock);
	     return -1;
	   }
       save_resp(proxy1.myparser.req.uri, proxy1);
       if(HandleResp(proxy1) == -1){
           pthread_mutex_unlock(&cachelock);
	   return -1;
       }
     }
     else{
       if(proxy1.myparser.req.header.find("if-none-match") != proxy1.myparser.req.header.end() || proxy1.myparser.req.header.find("if-modified-since") != proxy1.myparser.req.header.end()){
         if(compareValidator(proxy1.myparser.req)){
	   proxy1.myparser.resp = getResp(proxy1.myparser.req);
	   proxy1.myparser.resp.reconstructor(proxy1.buffer);
	   if(proxy1.send_to_browser() == -1){
               pthread_mutex_unlock(&cachelock);
	       return -1;
	   }
	   std::string output = std::to_string(proxy1.thread_id) + "in cache, valid\n";
	   pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
	 }
	 else{
	   requireValid(proxy1.myparser.req);
	   std::string output = std::to_string(proxy1.thread_id) + "in cache, require validation\n";
           pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
	   proxy1.myparser.req.reconstruct(proxy1.buffer);
	   if(proxy1.send_to_server() == -1){
	      pthread_mutex_unlock(&cachelock);
	      return -1;
	   }
	   if(proxy1.recv_from_server() == -1){
             pthread_mutex_unlock(&cachelock);
	     return -1;
	   }
	   save_resp(proxy1.myparser.req.uri, proxy1);
	   if(HandleResp(proxy1) == -1){
              pthread_mutex_unlock(&cachelock);
	      return -1;
	   }
	 }
       }
       else{
	  std::string output =	std::to_string(proxy1.thread_id) + "in cache, require validation\n";
      	   pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
	 proxy1.myparser.resp = getResp(proxy1.myparser.req);
	 proxy1.myparser.resp.reconstructor(proxy1.buffer);
	 if(proxy1.send_to_browser() == -1){
            pthread_mutex_unlock(&cachelock);
	    return -1;
	 }
       }
     }
   }
 }
 else{
   if(proxy1.myparser.req.header.find("cache-control") != proxy1.myparser.req.header.end() && (proxy1.myparser.req.header["cache-control"].find("max-stale") != std::string::npos)){
     if(proxy1.myparser.resp.header_field["cache-control:"].find("must-revalidation") == std::string::npos && proxy1.myparser.resp.header_field["cache-control:"].find("proxy-revalidation") == std::string::npos){
       int pos = proxy1.myparser.req.header["cache-control"].find("max-stale");
       std::string stale = proxy1.myparser.req.header["cache-control"].substr(pos+10);
       if(stale[0] == ' '){
	 proxy1.myparser.resp = getResp(proxy1.myparser.req);
	 proxy1.myparser.resp.reconstructor(proxy1.buffer);
	 if( proxy1.send_to_browser() == -1){
             pthread_mutex_unlock(&cachelock);
	     return -1;
	 }
	  std::string output =	std::to_string(proxy1.thread_id) + "in cache, but expired at " +proxy1.myparser.resp.header_field["expires:"]+"\n";
       pthread_mutex_lock(&loglock);
      proxy1.outfile<<output;
      pthread_mutex_unlock(&loglock);
       }
       else{
	 int pos = stale.find_first_not_of("1234567890");
	 int max_stale;
	 stale = stale.substr(0, pos);
	 std::stringstream ss;
	 ss>>stale;
         ss<<max_stale;
	 int age = getCurrentAge(proxy1);
	 int lifetime = getLifetime(proxy1.myparser.req.uri);
	 if(age - lifetime <= max_stale){
	   proxy1.myparser.resp = getResp(proxy1.myparser.req);
	   proxy1.myparser.resp.reconstructor(proxy1.buffer);
	   if(proxy1.send_to_browser() == -1){
             pthread_mutex_unlock(&cachelock);
	     return -1;
	   }
	     std::string output =  std::to_string(proxy1.thread_id) + "in cache, but expired at" + proxy1.myparser.resp.header_field["expires:"]+"\n";
           pthread_mutex_lock(&loglock);
	   proxy1.outfile<<output;
	   pthread_mutex_unlock(&loglock);

	 }
	 else{
	   requireValid(proxy1.myparser.req);
	   proxy1.myparser.req.reconstruct(proxy1.buffer);
           if(proxy1.send_to_server() == -1){
              pthread_mutex_unlock(&cachelock);
	      return -1;
	   }
           if(proxy1.recv_from_server() == -1){
             pthread_mutex_unlock(&cachelock);
	     return -1;
	   }
           save_resp(proxy1.myparser.req.uri,proxy1);
	   if(HandleResp(proxy1) == -1){
              pthread_mutex_unlock(&cachelock);
	      return -1;
	   }
	     std::string output =  std::to_string(proxy1.thread_id) + "in cache, require validation\n";
           pthread_mutex_lock(&loglock);
            proxy1.outfile<<output;
           pthread_mutex_unlock(&loglock);
	 }
       }
     }
     else{
       requireValid(proxy1.myparser.req);
       proxy1.myparser.req.reconstruct(proxy1.buffer);
       if(proxy1.send_to_server() == -1){
          pthread_mutex_unlock(&cachelock);
	  return -1;
       }
       if(proxy1.recv_from_server() == -1){
         pthread_mutex_unlock(&cachelock);  
	 return -1;
       }
       save_resp(proxy1.myparser.req.uri, proxy1);
       if(HandleResp(proxy1) == -1){
          pthread_mutex_unlock(&cachelock);
	  return -1;
       }
       std::string output =  std::to_string(proxy1.thread_id) + "in cache, require validation\n";
           pthread_mutex_lock(&loglock);
	   proxy1.outfile<<output;
	   pthread_mutex_unlock(&loglock);
     }
   }
   else{
     requireValid(proxy1.myparser.req);
     proxy1.myparser.req.reconstruct(proxy1.buffer);
     if(proxy1.send_to_browser() == -1){
        pthread_mutex_unlock(&cachelock);
	return -1;
     }
     if(proxy1.recv_from_server() == -1){
         pthread_mutex_unlock(&cachelock);
         return -1;
       }
       save_resp(proxy1.myparser.req.uri, proxy1);
       if(HandleResp(proxy1) == -1){
          pthread_mutex_unlock(&cachelock);
          return -1;
       }
     std::string output =  std::to_string(proxy1.thread_id) + "in cache, require validation\n";
           pthread_mutex_lock(&loglock);
	   proxy1.outfile<<output;
	   pthread_mutex_unlock(&loglock);
   }
 }
   pthread_mutex_unlock(&cachelock);
 return 0;
}

int cache::HandleResp(proxy& proxy1){
  if(proxy1.myparser.resp.status_code == 304){
    proxy1.myparser.resp = info[proxy1.myparser.req.uri]->second;
    proxy1.myparser.resp.reconstructor(proxy1.buffer);
    if(proxy1.send_to_browser() == -1){
      return -1;
    }
    return 0;
  }
  if(proxy1.myparser.resp.status_code >= 500 || proxy1.myparser.resp.msg_body.size() > 0){
    if(proxy1.send_to_browser() == -1){
      return -1;
    }
    return 0;
  }
}

