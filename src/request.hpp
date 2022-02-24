#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <unordered_map>
#include <string>
#include <vector>

#include "myexception.hpp"

class request {  //TODO: add some exception control
 private:
  std::vector<std::string> requestHttp;
  std::unordered_map<std::string, std::string> headers;
  std::string method;
  std::string URI;
  std::string httpversion;
  std::string host;
  std::string port;
  size_t n = 0;

 public:
    std::string firstline;
    request(){}
    request(std::vector<std::string> requestHttp);
    void initMap();
    std::string findhelper(std::string item);
    void fillMap();
    std::string getheader(std::string headerName);
    std::string getMethod();
    std::string getURI();
    std::string gethost();
    std::string getport();
    std::vector<char> getdata();
    void initRequestLine();
    void initHostandPort();
    size_t getDataLen();
    std::vector<char> getvalidationheader();
    ~request();
};

//constructor
request::request(std::vector<std::string> requestHttp):requestHttp(requestHttp) {
    initRequestLine();
    initHostandPort();
    initMap();
    fillMap();

}


void request::initRequestLine(){
    std::string requestLine = requestHttp[0];
    firstline = requestLine.substr(0, requestLine.size()-2);
    std::size_t foundMethod = requestLine.find(" ");
    if(foundMethod != std::string::npos)
        method = requestLine.substr(0, foundMethod);

    std::size_t foundURI = requestLine.substr(foundMethod+1).find(" ");
    if(foundURI != std::string::npos)
        URI = requestLine.substr(foundMethod+1).substr(0, foundURI);

    std::size_t foundHTTP = requestLine.substr(foundMethod+1).substr(foundURI+1).find(" ");
    if(foundHTTP != std::string::npos)
        httpversion = requestLine.substr(foundMethod+1).substr(foundURI+1).substr(0, foundHTTP);

}

void request::initHostandPort(){
    std::string hostandport = findhelper("Host");
    std::size_t div = hostandport.find(":");
    if(div != std::string::npos) {
        host = hostandport.substr(0, div);
        port = hostandport.substr(div+1);
    } else {
        host = hostandport;
        port = "80";
    }

}
/**
 * @brief this function used to initialize map "headers"
 * the init value of these headers is empty string
 * 
 */
void request::initMap() {
    headers["no-cache"] = "false";
    headers["no-store"] = "false";
    headers["max-age"] = "false";
    headers["max-stale"] = "false";
    headers["only-if-cached"] = "false";
    // headers["min-fresh"] = "false";
    headers["Content-Length"] = "false"; 
    headers["If-Modified-Since"] = "false";
    headers["If-None-Match"] = "false";
    headers["Transfer-Encoding"] = "false";
}

/**
 * @brief this function will loop the incoming vector "requestHttp" and find the value of item
 * 
 * @param item the key we want to find
 * @return std::string the value of item like "max-age=1234" -> "1234" or "true" for no-parameter item
 */
std::string request::findhelper(std::string item) {
    for(int i=1; i<requestHttp.size(); ++i){
        std::size_t found = requestHttp[i].find(item);
        if(found != std::string::npos){
            std::string ans = "";
            if(item == "max-age" || item =="max-stale"){
                std::size_t foundequal = requestHttp[i].find("=", found+1);
                std::size_t foundcomma = requestHttp[i].find(",", foundequal+1, 1);
                if(foundcomma == std::string::npos){
                    ans = requestHttp[i].substr(foundequal+1);
                    ans.pop_back();
                    ans.pop_back();
                }
                else{
                    ans = requestHttp[i].substr(foundequal+1, foundcomma-foundequal-1);
                }
                return ans;
            }
            else if(item == "Content-Length"|| item == "Host"|| item == "If-Modified-Since" || item =="If-None-Match" || item == "Transfer-Encoding"){
                std::size_t foundcolon = requestHttp[i].find(":", found+1);
                ans = requestHttp[i].substr(foundcolon+2);
                ans.pop_back();
                ans.pop_back();
                return ans;
            }
            else
                return "true";
        }
    }
    return "false";
}

/**
 * @brief the function will call findhelper to fill the all keys in the map
 * 
 */
void request::fillMap() {
    headers["no-cache"] = findhelper("no-cache");
    headers["no-store"] = findhelper("no-store");
    headers["max-age"] = findhelper("max-age");
    headers["max-stale"] = findhelper("max-stale");
    headers["only-if-cached"] = findhelper("only-if-cached");
    headers["Content-Length"] = findhelper("Content-Length");
    headers["If-Modified-Since"] = findhelper("If-Modified-Since");
    headers["If-None-Match"] = findhelper("If-None-Match");
    headers["Transfer-Encoding"] = findhelper("Transfer-Encoding");
}

/**
 * @brief use the given string "headerName" as a key to find the value in the map "headers"
 * 
 * @param headerName the header that we need to find its value
 * @return std::string the value of the headerName
 */
std::string request::getheader(std::string headerName) {
    return headers[headerName];
}

std::string request::getMethod() {
    return method;
}

std::string request::getURI() {
    return URI;
}

std::string request::gethost() {
    return host;
}

std::string request::getport() {
    return port;
}

std::vector<char> request::getdata() {
    
    std::vector<char> re;
    //unsigned long len = 0;
    for(auto it: requestHttp) {
        std::copy(it.begin(), it.end(), std::back_inserter(re));
    }
    //n = len;
    return re;
}

size_t request::getDataLen() {
    return n;
}

std::vector<char> request::getvalidationheader() {
    std::vector<char> re;
    std::string firstline = requestHttp[0];
    std::copy(firstline.begin(), firstline.end(), std::back_inserter(re));
    std::string host("Host: ");
    std::copy(host.begin(), host.end(), std::back_inserter(re));
    std::string hostinfo = headers["Host"];
    std::copy(hostinfo.begin(), hostinfo.end(), std::back_inserter(re));
    re.push_back('\r');
    re.push_back('\n');
    return re;
}

request::~request() {
}


#endif