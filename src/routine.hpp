#ifndef __ROUTINE_H__
#define __ROUTINE_H__

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <utility>

#include "myexception.hpp"
#include "request.hpp"
#include "response.hpp"
#include "socket.hpp"
#include "cache.hpp"
#include "logger.hpp"

class routine {
 private:
  connSocket * client;
  cache * cacheBase;
  logger * LOG;

 public:
  routine(connSocket * client, cache * cacheBase, logger * LOG);
  void run();
  //get the content from the socket according to the content length
  std::vector<char> * getcontent_Len(unsigned long len, connSocket & fd);
  //get the content from the socket according to the chunked encoding format
  std::vector<char> * getcontent_Chunk(connSocket & fd);
  //get the content using while loop until the server to disconect
  std::vector<char> * getcontent_Loop(connSocket & fd);
  //get the content that request contains, only apply to POST method
  std::vector<char> * getRequestContent(request & client_request, connSocket & client_fd);
  //get the content that response contains
  std::vector<char> * getResponseContent(response & origServer_response, connSocket & origServer);
  //return the cache to the client, if the cache is already stale, then refresh it and update cache database
  std::pair<response, std::vector<char> > * pullCache();
  //send data to server
  void send_server(connSocket & origServer, request & client_request);
  //send data to client
  void send_client(connSocket & fd, response & origServer_response, std::vector<char> * response_content);
  //handle response and determine whether it need to put into the cache
  void handleResponse(std::string URI, request & clientrequest, response & origResponse, std::vector<char> * response_content);
  //check whether need to revalidation
  bool checkValidation(request & clientrequest, response & cacheheader);
  //check if the response is expire or not
  bool checkTime(response & cacheheader);
  //send revalidation info to server
  void revalidate(connSocket & origServer, request & client_request, response & cacheHeader);

  void sendErrorPage();
  ~routine();
};

routine::routine(connSocket * client, cache * cacheBase, logger * LOG): client(client), cacheBase(cacheBase), LOG(LOG){
}

void routine::run() {
    request client_request(client->readhttp());
    LOG->log_request(client->user_id, client_request.firstline, client->user_ip);
    std::string method = client_request.getMethod();
    std::string host = client_request.gethost();
    std::string port = client_request.getport();

    if(method == "GET") {
        std::string URI = client_request.getURI();
        if(client_request.getheader("no-store") == "false" && cacheBase->exist(URI)) {//if there is a cache in the cacheBase
            response cacheHeader = cacheBase->getHttpHeader(URI);
            std::vector<std::string> returnHeader;
            if(checkValidation(client_request, cacheHeader) || checkTime(cacheHeader)) {  //check some header to determine whether need revalidation
                connSocket origServer;
                origServer.build_client(host, port);
                revalidate(origServer, client_request, cacheHeader); //send revalidation httpheader
                returnHeader = origServer.readhttp();
                response validationInfo = response(returnHeader);
                LOG->receive_response(client->user_id, validationInfo.firstline, client_request.gethost());
                if(validationInfo.getStatusNum() == "200") {
                    std::vector<char> * response_content = getResponseContent(validationInfo, origServer);
                    handleResponse(URI, client_request, validationInfo, response_content);
                    delete response_content;
                } else if(validationInfo.getStatusNum() == "304"){ //304 still validation
                    cacheBase->updateHeader(URI, validationInfo); //update header
                } else {
                    std::vector<char> * response_content = getResponseContent(validationInfo, origServer);
                    send_client(*client, validationInfo, response_content);
                    delete response_content;
                }
                origServer.closefd();
            } else {
                LOG->valid_cache(client->user_id);
            }
            std::pair<response, std::vector<char> > cachedata = cacheBase->getdata(URI);
            send_client(*client, cachedata.first, &cachedata.second);
        } else {  //if there is no cache in the cacheBase
            LOG->log_not_in_cache(client->user_id);
            connSocket origServer;
            origServer.build_client(host, port);
            send_server(origServer, client_request);
            LOG->requesting(client->user_id, client_request.firstline, client_request.gethost());
            response orig_response(origServer.readhttp());
            LOG->receive_response(client->user_id, orig_response.firstline, client_request.gethost());
            std::vector<char> * response_content;
            try
            {
                response_content = getResponseContent(orig_response, origServer);
            }
            catch(const std::exception& e)
            {
                origServer.closefd();
                throw e;
            }

            handleResponse(URI, client_request, orig_response, response_content);
            send_client(*client, orig_response, response_content);
            delete response_content;
            origServer.closefd();
        }


    } else if (method == "POST") {
        /**
         * @brief get host name and build a client socket to connect to origin server
         * 
         */
        connSocket origServer;
        std::vector<char> * request_content;
        request_content = getRequestContent(client_request, *client);
        origServer.build_client(host, port);
        send_server(origServer, client_request);
        origServer.writenB(request_content->data(), request_content->size());
        response origServer_response(origServer.readhttp());
        std::vector<char> * response_content;
        try
        {
            response_content = getResponseContent(origServer_response, origServer);
        }
        catch(const std::exception& e)
        {
            delete request_content;
            origServer.closefd();
            throw e;
        }
        
        send_client(*client, origServer_response, response_content);
        delete request_content;
        delete response_content;
        origServer.closefd();

    } else if (method == "CONNECT") {
        connSocket origServer;
        origServer.build_client(host, port);
        // response OK
        try
        {
            client->writenB("HTTP/1.1 200 OK\r\n\r\n", 19);
        }
        catch(const std::exception& e)
        {
            origServer.closefd();
            throw e;
        }

        fd_set readfds;

        int nfds = std::max(origServer.getFD(), client->getFD()) + 1;
        char buf[65536];
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(origServer.getFD(), &readfds);
            FD_SET(client->getFD(), &readfds);

            select(nfds, &readfds, NULL, NULL, NULL);
            int len;
            if(FD_ISSET(client->getFD(), &readfds)) {
                len = recv(client->getFD(), buf, sizeof(buf), 0);
                if(len <= 0) break;
                else if(send(origServer.getFD(), buf, len, MSG_NOSIGNAL) <= 0) break;
            } 
            
            if(FD_ISSET(origServer.getFD(), &readfds)){
                len = recv(origServer.getFD(), buf, sizeof(buf), 0);
                if(len <= 0) break;
                //else if (send(client->getFD(), buf, len, MSG_NOSIGNAL) <= 0) break;
                else if(client->writenB(buf, len) <= 0) break;
            }
        }
        origServer.closefd();
        LOG->close_tunnel(client->user_id);
    } else {
        sendErrorPage();
        LOG->respond_client(client->user_id, "\"HTTP/1.1 400 Bad Request\"");
    }
    
}

bool routine::checkValidation(request & clientrequest, response & cacheHeader) {
    if(cacheHeader.needrevalidation() && clientrequest.needrevalidation()) {
        LOG->require_validation(client->user_id);
        return true;
    } else {
        return false;
    }
}

bool routine::checkTime(response & cacheHeader) {
    if(cacheHeader.isExpires()) {
        LOG->in_cache_expired(client->user_id, cacheHeader.expireTime);
        return true;
    } else {
        return false;
    }
}

void routine::revalidate(connSocket & origServer, request & client_request, response & cacheHeader) {
    if(cacheHeader.hasEnoughValidationInfo()) {
        LOG->requesting(client->user_id, client_request.firstline, client_request.gethost());
        std::vector<char> requestpart = client_request.getvalidationheader();
        origServer.writenB(requestpart.data(), requestpart.size());
        std::vector<char> responsevalidationinfo = cacheHeader.getvalidationInfo();
        origServer.writenB(responsevalidationinfo.data(), responsevalidationinfo.size());
        origServer.writenB("\r\n", 2);
    } else {
        send_server(origServer, client_request);
    }
}

/**
 * @brief to determine whethere need to add the response to the cacheBase
 * 
 * @param origResponse 
 * @param response_content 
 */
void routine::handleResponse(std::string URI, request & clientrequest, response & origResponse, std::vector<char> * response_content) {
    if(clientrequest.needCache() && origResponse.needCache()) {
        if(origResponse.needrevalidation()) {
            LOG->cached_need_revalidate(client->user_id);
        } else {
            origResponse.isExpires();
            LOG->cached_expire(client->user_id, origResponse.expireTime);
        }
        cacheBase->put(URI, origResponse, *response_content);
    } else {
        if(!clientrequest.needCache()) {
            LOG->not_cacheable(client->user_id, "no-store");
        }
        LOG->not_cacheable(client->user_id, origResponse.getReason());
    }
}


void routine::send_server(connSocket & origServer, request & client_request) {
    std::vector<char> t = client_request.getdata();
    origServer.writenB(t.data(), t.size());
    origServer.writenB("\r\n", 2);
}

void routine::send_client(connSocket & fd, response & origServer_response, std::vector<char> * response_content) {
    LOG->respond_client(client->user_id, origServer_response.firstline);
    fd.writenB(origServer_response.getdata().data(), origServer_response.getdata().size());
    fd.writenB("\r\n", 2);
    fd.writenB(response_content->data(), response_content->size());
}

std::vector<char> * routine::getRequestContent(request & client_request, connSocket & client_fd) {
    if(client_request.getheader("Transfer-Encoding") == "chunked") return getcontent_Chunk(client_fd);
    else if(client_request.getheader("Content-Length") != "false") {
        std::string len = client_request.getheader("Content-Length");
        return getcontent_Len(std::stoul(len, nullptr, 10), client_fd);
    } else return getcontent_Loop(client_fd);
}

std::vector<char> * routine::getResponseContent(response & origServer_response, connSocket & origServer) {
    if(origServer_response.getheader("Transfer-Encoding") == "chunked") return getcontent_Chunk(origServer);
    else if(origServer_response.getheader("Content-Length") != "false") {
        std::string len = origServer_response.getheader("Content-Length");
        return getcontent_Len(std::stoul(len, nullptr, 10), origServer);
    } else return getcontent_Loop(origServer);
}

std::vector<char> * routine::getcontent_Len(unsigned long len, connSocket & fd) {
    std::vector<char> * re = new std::vector<char>(len);
    if(fd.readnB(re->data(), len) <= 0) {
        delete re;
        throw myexception("cannot get enough bytes from server");
    }
    return re;
}

std::vector<char> * routine::getcontent_Chunk(connSocket & fd) {
    std::vector<char> * re = new std::vector<char>(0);
    size_t size = 0;
    size_t cur = 0;
    while(1) {
        std::string chunk_len;
        try
        {
            chunk_len = fd.readLine();
        }
        catch(const std::exception& e)
        {
            delete re;
            throw e;
        }
        unsigned long num = std::stoul(chunk_len.substr(0, chunk_len.size()-1), nullptr, 16);
        size += num + 2 + chunk_len.length();
        std::copy(chunk_len.begin(), chunk_len.end(), std::back_inserter(*re));
        cur += chunk_len.length();
        re->resize(size);
        if(fd.readnB(&(re->data()[cur]), num + 2) <= 0) {
            delete re;
            throw myexception("chunked data error.");
        }
        cur += num+2;
        if(num == 0) break;
    }
    return re;
}

std::vector<char> * routine::getcontent_Loop(connSocket & fd) {
    std::vector<char> * re = new std::vector<char>(3000);
    ssize_t n;
    size_t size = 0;
    while((n = fd.readnB(&(re->data()[size]), 3000)) > 0) {
        size += n;
        re->resize(size + 3000);
    }
    re->resize(size);
    return re;
}

void routine::sendErrorPage() {
    client->writenB("HTTP/1.1 400 Bad Request\r\n\r\n", 32);
}

routine::~routine() {
}

#endif