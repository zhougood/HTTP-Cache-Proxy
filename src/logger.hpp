#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <fstream>
#include <iostream>
#include <string>
#include <mutex>

#include "myexception.hpp"
#include "timeTool.hpp"

class logger{
private:
    timeTool time_tool;
    std::string filepath;
    std::ofstream * logFile;
    std::mutex mtx;

public:
    logger(std::ofstream * logfile);
    void log(std::string msg);
    void log_request(std::string id, std::string request, std::string ip);
    void log_not_in_cache(std::string id);
    void in_cache_expired(std::string id, std::string expired_time);
    void require_validation(std::string id);
    void valid_cache(std::string id);
    void requesting(std::string id, std::string request, std::string ip);
    void receive_response(std::string id, std::string request, std::string ip);
    void not_cacheable(std::string id, std::string reason);
    void cached_expire(std::string id, std::string expire_time);
    void cached_need_revalidate(std::string id);
    void respond_client(std::string id, std::string response);
    void close_tunnel(std::string id);
    void note(std::string id, std::string message);
    void warning(std::string id, std::string message);
    void error(std::string id, std::string message);

    ~logger(){};
};

logger::logger(std::ofstream * logfile): logFile(logfile) {
}

/**
 * @brief when we give a msg to function, it willl write it to log file
 * 
 * @param msg 
 */
void logger::log(std::string msg) {
    std::lock_guard<std::mutex> lk(mtx);
    *logFile << msg << std::endl;

}

void logger::log_request(std::string id, std::string request, std::string ip){
    std::string time = time_tool.getCurTime();
    time.pop_back();
    std::string temp = id + ": " + "\"" + request + "\" from " + ip + " @ " + time;
    log(temp);
}

void logger::log_not_in_cache(std::string id){
    std::string temp = id + ": " + "not in cache" ;
    log(temp);
}

void logger::in_cache_expired(std::string id, std::string expired_time){
    std::string temp = id + ": " + "in cache, but expired at " + expired_time;
    log(temp);
}

void logger::require_validation(std::string id){
    std::string temp = id + ": " + "in cache, requires validation";
    log(temp);
}

void logger::valid_cache(std::string id){
    std::string temp = id + ": " + "in cache, valid";
    log(temp);
}

void logger::requesting(std::string id, std::string request, std::string ip){
    std::string temp = id + ": " + "Requesting " + "\"" + request + "\" from " + ip;
    log(temp);
}

void logger::receive_response(std::string id, std::string response, std::string ip){
    std::string temp = id + ": " + "Received " + "\"" + response + "\" from " + ip;
    log(temp);
}

void logger::not_cacheable(std::string id, std::string reason){
    std::string temp = id + ": " + "not cacheable because " + reason;
    log(temp);
}

void logger::cached_expire(std::string id, std::string expire_time){
    std::string temp = id + ": " + "cached, expires at " + expire_time;
    log(temp);
}

void logger::cached_need_revalidate(std::string id){
    std::string temp = id + ": " + "cached, but requires re-validation";
    log(temp);
}

void logger::respond_client(std::string id, std::string response){
    std::string temp = id + ": " + "Responding \"" + response + "\"";
    log(temp);
}

void logger::close_tunnel(std::string id){
    std::string temp = id + ": " + "Tunnel closed";
    log(temp);   
}

void logger::note(std::string id, std::string message){
    std::string temp = id + ": NOTE " + message;
    log(temp);   
}

void logger::warning(std::string id, std::string message){
    std::string temp = id + ": WARNING " + message;
    log(temp);   
}

void logger::error(std::string id, std::string message){
    std::string temp = id + ": ERROR " + message;
    log(temp);   
}

#endif