#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <thread>

#include "socket.hpp"
#include "fifo.hpp"
#include "myexception.hpp"
#include "request.hpp"
#include "timeTool.hpp"
#include "logger.hpp"

/*
int main() {
        //build a listen socket on port 12345
    connSocket server;
    try
    {
        std::string host("www.bing.com");
        std::string port("80");
        server.build_client(host, port);
        std::cout << "build success" << std::endl;
        std::string buf("GET / HTTP/1.1\r\nHost: www.bing.com\r\n\r\n");
        server.writenB(buf.data(), buf.length());
        std::vector<std::string> temp = server.readhttp();
        for(auto i: temp) {
            std::cout << i;
        }
        std::cout << std::endl;
        response re(temp);
        std::cout << re.getheader("private") << " " << re.getStatusNum() << std::endl;
        std::cout << re.getheader("no-store") << std::endl;
        std::cout << re.getheader("Content-Length") << std::endl;
        std::cout << re.getheader("max-age") << std::endl;
        std::cout << re.getheader("Transfer-Encoding") << std::endl;
        //std::cout << re.getheader("") << std::endl;
        std::cout << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return EXIT_SUCCESS;
}
*/

int main() {
    timeTool timetool;
    std::string testhttpTime = "Wed, 23 Feb 2022 02:12:00 GMT";
    std::cout << timetool.getCurTime() << std::endl;

    std::cout << timetool.getExpiresTime(testhttpTime, 120) << std::endl;
    //std::cout << ans << std::endl;

    //logger LOG("output");
    //LOG.log_request("123", "req", "123.24.23.34");
}


/*
void handle_thread(fifo * connPoll);

int main() {

    //this is a connection Poll which holds
    fifo * connPoll = new fifo(40);
    
    std::thread threadPoll[20];
    for(int i = 0; i < 20; i++) {
        threadPoll[i] = std::thread(handle_thread, connPoll);
        threadPoll[i].detach();
    }

    while(1) {
        //build a listen socket on port 12345
        connSocket server;
        try
        {
            server.build_server("12345");
            std::cout << "build success" << std::endl;
            while(1) {
                //produce read Socket to connPoll.
                connPoll->produce(server.acceptConnection());
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    

    return EXIT_SUCCESS;
}

void handle_thread(fifo * connPoll) {
    while(1) {
        connSocket * clientfd = connPoll->consume();
        std::vector<std::string> temp = clientfd->readhttp();
        for(auto i: temp) {
            std::cout << i;
        }
        std::cout << std::endl;
        request re(temp);
        std::cout << re.getMethod() << re.getURI() << re.gethost() << std::endl;
        std::cout << re.getheader("no-store") << std::endl;
        std::cout << re.getheader("Content-Length") << std::endl;
        std::cout << re.getheader("max-age") << std::endl;
        clientfd->closefd();
        delete clientfd;
    }
}
*/