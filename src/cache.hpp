#ifndef __CACHE_H__
#define __CACHE_H__

#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>
#include <utility>

#include "myexception.hpp"
#include "request.hpp"
#include "response.hpp"


class cacheNode
{
public:
    cacheNode * prev;
    cacheNode * next;
    std::string URI;
    response httpHeader;  //使用什么方式读进来，是指针还是数据copy
    std::vector<char> content;

    cacheNode(){}
    cacheNode(std::string URI, response httpHeader, std::vector<char> content): URI(URI), httpHeader(httpHeader), content(content), prev(NULL), next(NULL) {}
};

class cache
{
private:
    std::mutex mtx;
    int remain_size;
    std::unordered_map<std::string, cacheNode* > maptable;
    cacheNode * head;
    cacheNode * tail;

    void removeNode(cacheNode * node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void moveToHead(cacheNode * node) {
        node->next = head->next;
        node->prev = head;
        head->next->prev = node;
        head->next = node;
    }

public:
    cache(int capacity);
    bool exist(std::string URI);
    response getHttpHeader(std::string URI);
    std::vector<char> getContent(std::string URI);
    void put(std::string URI, response httpPart, std::vector<char> content);
    void updateHeader(std::string URI, response httpPart);
    std::pair<response, std::vector<char> > getdata(std::string URI);
    int getcacheSize() {
        return maptable.size();
    };
    ~cache();
};

cache::cache(int capacity):remain_size(capacity), head(new cacheNode()), tail(new cacheNode())
{
    head->next = tail;
    tail->prev = head;
}

bool cache::exist(std::string URI) {
    return maptable.find(URI) != maptable.end();
}

response cache::getHttpHeader(std::string URI) {
    std::lock_guard<std::mutex> lck(mtx);
    if(exist(URI)) {
        cacheNode * temp = maptable[URI];
        response re = temp->httpHeader;
        return re;
    } else {
        throw myexception("there is no such item.");  //TODO:how to return a better value other than throw excption
    }
}


std::vector<char> cache::getContent(std::string URI) {
    std::lock_guard<std::mutex> lck(mtx);
    if(exist(URI)) {
        cacheNode * temp = maptable[URI];
        removeNode(temp);
        moveToHead(temp);
        return temp->content;
    } else {
        throw myexception("there is no such item."); //TODO: how to return a better value other than throw exception
    }
}

std::pair<response, std::vector<char> > cache::getdata(std::string URI) {
    std::lock_guard<std::mutex> lck(mtx);
    if(exist(URI)) {
        cacheNode * temp = maptable[URI];
        removeNode(temp);
        moveToHead(temp);
        std::pair<response, std::vector<char> > re;
        re = std::make_pair(temp->httpHeader, temp->content);
        //std::cout << "get cache" << std::endl;
        return re;
    } else throw myexception("there is no such item.");
}

void cache::put(std::string URI, response httpPart, std::vector<char> content) {
    std::lock_guard<std::mutex> lck(mtx);
    //update the cache
    if(exist(URI)) {
        cacheNode * temp = maptable[URI];
        temp->httpHeader = httpPart;
        temp->content = content;
        removeNode(temp);
        moveToHead(temp);
    } else { //add new entry into cache
        cacheNode * needAdd = new cacheNode(URI, httpPart, content);
        maptable[URI] = needAdd;
        moveToHead(needAdd);
        if(remain_size == 0) {
            cacheNode * needRemove = tail->prev;
            removeNode(needRemove);
            maptable.erase(needRemove->URI);
            delete(needRemove);
        } else {
            remain_size--;
        }
        //std::cout << "put success" << std::endl;
    }
}


//this function used to update httpheader
void cache::updateHeader(std::string URI, response httpPart) {
    std::lock_guard<std::mutex> lck(mtx);
    if(exist(URI)) {
        cacheNode * temp = maptable[URI];
        temp->httpHeader = httpPart;
        removeNode(temp);
        moveToHead(temp);
        //std::cout << "update header" << std::endl;
        return;
    } else throw myexception("there is no such item in cache.");
}

cache::~cache()
{
    for(auto it = maptable.begin(); it != maptable.end(); ++it) {
        delete it->second;
    }
    delete head;
    delete tail;
}


#endif