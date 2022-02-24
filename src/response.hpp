#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include <unordered_map>
#include <string>
#include <vector>

#include "myexception.hpp"
#include "timeTool.hpp"

class response {
 private:
  std::vector<std::string> responseHttp;
  std::unordered_map<std::string, std::string> headers;
  std::string statusNum;

 public:
  std::string firstline;
  std::string expireTime;
  response(){}
  response(std::vector<std::string> responseHttp);
  void initMap();
  std::string findhelper(std::string item);
  void fillMap();
  std::string getheader(std::string headerName);
  void initResponseLine();
  std::string getStatusNum();
  std::vector<char> getdata();
  bool needCache();
  bool needrevalidation();
  bool isExpires();
  std::vector<char> getvalidationInfo();
  bool hasEnoughValidationInfo();
  std::string getReason();
  ~response();
};

response::response(std::vector<std::string> responseHttp):responseHttp(responseHttp) {
  initResponseLine();
  initMap();
  fillMap();
}

void response::initMap() {
  headers["public"] = "false";
  headers["private"] = "false";
  headers["no-cache"] = "false";
  headers["no-store"] = "false";
  headers["must-revalidate"] = "false";
  headers["proxy-revalidate"] = "false";
  headers["max-age"] = "false";
  headers["s-maxage"] = "false";
  headers["Expires"] = "false";
  headers["Date"] = "false";
  headers["Last-Modified"] = "false";
  headers["ETag"] = "false";
  headers["Content-Length"] = "false";
  headers["Transfer-Encoding"] = "false";
}

void response::initResponseLine() {
  std::string requestLine = responseHttp[0];
  firstline = requestLine.substr(0, requestLine.size() - 2);
  std::size_t foundHttp = requestLine.find(" ");
  std::size_t foundStatus = requestLine.substr(foundHttp+1).find(" ");
  if (foundStatus != std::string::npos)
    statusNum = requestLine.substr(foundHttp+1).substr(0, foundStatus);
}

std::string response::findhelper(std::string item) {
  for (int i = 1; i < responseHttp.size(); ++i) {
    std::size_t found = responseHttp[i].find(item);
    if (found != std::string::npos) {
      std::string ans = "";
      if (item == "max-age" || item == "s-maxage") {
        std::size_t foundequal = responseHttp[i].find("=", found + 1);
        std::size_t foundcomma = responseHttp[i].find(",", foundequal + 1, 1);
        if (foundcomma == std::string::npos) {
          ans = responseHttp[i].substr(foundequal+1);
          ans.pop_back();
          ans.pop_back();
        } else {
          ans = responseHttp[i].substr(foundequal+1, foundcomma - foundequal - 1);
        }
        return ans;
      } else if (item == "Expires" || item == "Date" ||
                 item == "Last-Modified" || item == "ETag" ||
                 item == "Content-Length" || item == "Transfer-Encoding") {
        std::size_t foundcolon = responseHttp[i].find(":", found + 1);
        ans = responseHttp[i].substr(foundcolon + 2);
        ans.pop_back();
        ans.pop_back();
        return ans;
      } else
        return "true";
    }
  }
  return "false";
}

void response::fillMap() {
  headers["public"] = findhelper("public");
  headers["private"] = findhelper("private");
  headers["no-cache"] = findhelper("no-cache");
  headers["no-store"] = findhelper("no-store");
  headers["must-revalidate"] = findhelper("must-revalidate");
  headers["proxy-revalidate"] = findhelper("proxy-revalidate");

  headers["max-age"] = findhelper("max-age");
  headers["s-maxage"] = findhelper("s-maxage");
  headers["Expires"] = findhelper("Expires");
  headers["Date"] = findhelper("Date");
  headers["Last-Modified"] = findhelper("Last-Modified");
  headers["ETag"] = findhelper("Etag");
  headers["Content-Length"] = findhelper("Content-Length");
  headers["Transfer-Encoding"] = findhelper("Transfer-Encoding");
}

std::string response::getheader(std::string headerName) {
  return headers[headerName];
}

std::string response::getStatusNum() {
  return statusNum;
}

std::vector<char> response::getdata() {
    std::vector<char> re;
    for(auto it: responseHttp) {
        std::copy(it.begin(), it.end(), std::back_inserter(re));
    }
    return re;
}

bool response::needCache() {
  if(statusNum == "200" &&
      headers["private"] == "false" &&
      headers["no-store"] == "false") return true;

  else return false;
}

std::string response::getReason() {
  if(statusNum != "200") return statusNum;
  if(headers["private"] == "true") return std::string("private");
  if(headers["no-store"] == "true") return std::string("no-store");
  return " ";
}

bool response::needrevalidation() {
  if(headers["no-cache"] == "true" || headers["must-revalidate"] == "true") return true;
  if(headers["max-age"] != "false" && std::stoul(headers["max-age"], nullptr, 10) <= 0) return true;
  if(headers["s-maxage"] != "false" && std::stoul(headers["s-maxage"], nullptr, 10) <= 0) return true;
  return false;
}

bool response::isExpires() {
  timeTool time_tool;
  if(headers["Date"] != "false") {
    if(headers["s-maxage"] != "false") {
      unsigned long age = std::stoul(headers["s-maxage"], nullptr, 10);
      expireTime = time_tool.getExpiresTime(headers["Date"], age);
      return time_tool.compareTimewithCur(headers["Date"], age);
    }
    if(headers["max-age"] != "false") {  //if there is max-age
      unsigned long age = std::stoul(headers["max-age"], nullptr, 10);
      expireTime = time_tool.getExpiresTime(headers["Date"], age);
      return time_tool.compareTimewithCur(headers["Date"], age);
    }
  }
  if(headers["Expires"] != "false") {
    if(headers["Expires"] == "0" || headers["Expires"] == "-1") return true;
    expireTime = headers["Expires"];
    return time_tool.compareTimewithCur(headers["Expires"], 0);  //TODO: add some exception if the time has an invalid formate
  }
  return false;  //if no time control than just use the cache
}

bool response::hasEnoughValidationInfo() {
  if(headers["ETag"] != "false" || headers["Last-Modified"] != "false") return true;
  else return false; 
}
std::vector<char> response::getvalidationInfo() {
  std::vector<char> re;
  if(headers["ETag"] != "false") {
    std::string ifnomatch("If-None-Match: ");
    std::copy(ifnomatch.begin(), ifnomatch.end(), std::back_inserter(re));
    std::string etag = headers["ETag"];
    std::copy(etag.begin(), etag.end(), std::back_inserter(re));
    re.push_back('\r');
    re.push_back('\n');
  }
  if(headers["Last-Modified"] != "false") {
    std::string ifnotmodified("If-Not-Modified-Since: ");
    std::copy(ifnotmodified.begin(), ifnotmodified.end(), std::back_inserter(re));
    std::string lastmodified = headers["Last-Modified"];
    std::copy(lastmodified.begin(), lastmodified.end(), std::back_inserter(re));
    re.push_back('\r');
    re.push_back('\n');
  }

  return re;
}

response::~response() {}

#endif