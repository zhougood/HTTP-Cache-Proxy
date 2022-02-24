#ifndef __TIMETOOL_H__
#define __TIMETOOL_H__

#include <string>
#include <vector>
#include <ctime>
#include <map>
#include <time.h>
#include <iostream>

class timeTool
{
public:
    timeTool(){}
    std::string getCurTime();
    //std::string httpTimeToLogTime(std::string httpTime);
    bool compareTimewithCur(std::string httpTime, unsigned long age);
    struct tm parsehttpTime(std::string httpTime);
    std::string getExpiresTime(std::string Date, unsigned long age);
    ~timeTool(){};
};


// return Format: Wed Feb 13 17:17:11 2013
std::string timeTool::getCurTime() {
    time_t curtime = time(NULL);
    struct tm * temp = gmtime(&curtime);
    std::string re(asctime(temp));
    return re;
}

// Transfer HTTP time to Log Time
/*
std::string timeTool::httpTimeToLogTime(std::string httpTime) {
    std::string ans;
    
    return ans;
}
*/

// Compare Http Time with cur Time
bool timeTool::compareTimewithCur(std::string httpTime, unsigned long age) {
    time_t curTime = time(NULL);
    struct tm * curTimeGMT = gmtime(&curTime);
    time_t curTime_real = std::mktime(curTimeGMT);
    //std::cout << curTime_real << std::endl;
    //Sat, 09 Oct 2010 14:28:02 GMT
    struct tm tm_struct = parsehttpTime(httpTime);
    time_t temp_tm_struct = std::mktime(&tm_struct);
    //std::cout << temp_tm_struct << std::endl;

    double diffTime = difftime(curTime_real, temp_tm_struct + age);
    if(diffTime > 0) return true; // curTime > httpTime
    else return false; 
}


struct tm timeTool::parsehttpTime(std::string httpTime){
    
    std::map<std::string, int> monMap;
    monMap["Jan"] = 1;
    monMap["Feb"] = 2;
    monMap["Mar"] = 3;
    monMap["Apr"] = 4;
    monMap["May"] = 5;
    monMap["Jun"] = 6;
    monMap["Jul"] = 7;
    monMap["Aug"] = 8;
    monMap["Sep"] = 9;
    monMap["Oct"] = 10;
    monMap["Nov"] = 11;
    monMap["Dec"] = 12;

    std::map<std::string, int> dayMap;
    dayMap["Mon"] = 1;
    dayMap["Tue"] = 2;
    dayMap["Wed"] = 3;
    dayMap["Thu"] = 4;
    dayMap["Fri"] = 5;
    dayMap["Sat"] = 6;
    dayMap["Sun"] = 0;

    struct tm tm_struct;
    tm_struct.tm_sec = atoi(httpTime.substr(23,2).c_str());
    tm_struct.tm_min = atoi(httpTime.substr(20,2).c_str());
    tm_struct.tm_hour = atoi(httpTime.substr(17,2).c_str());
    tm_struct.tm_mday = atoi(httpTime.substr(5,2).c_str());
    std::string mon = httpTime.substr(8,3);
    tm_struct.tm_mon = monMap[mon] - 1;
    tm_struct.tm_year = atoi(httpTime.substr(12,4).c_str()) - 1900;
    std::string wday = httpTime.substr(0,3);
    tm_struct.tm_wday = dayMap[wday];
    //tm_struct->tm_yday
    tm_struct.tm_isdst = -1;
    return tm_struct;
}

std::string timeTool::getExpiresTime(std::string Date, unsigned long age) {
    struct tm temp = parsehttpTime(Date);
    time_t origTime = std::mktime(&temp) + age;
    struct tm * newTime = localtime(&origTime);
    std::string re(asctime(newTime));
    re.pop_back();
    return re;
}

#endif