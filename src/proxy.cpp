#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <thread>
#include <sys/stat.h>
#include <fcntl.h>

#include "socket.hpp"
#include "fifo.hpp"
#include "myexception.hpp"
#include "routine.hpp"
#include "logger.hpp"
#include "cache.hpp"

void basic_deamon();
void handle_thread(fifo * connPoll, cache * c, logger * LOG);

//std::ofstream logfile("log.txt");
std::ofstream logfile("/var/log/erss/proxy.log");

int main() {
    
    basic_deamon();

    signal(SIGPIPE, SIG_IGN);
    //this is a connection Poll which holds
    fifo * connPoll = new fifo(128);
    cache * c = new cache(512);
    
    logger * LOG = new logger(&logfile);
    
    std::thread threadPoll[128];
    for(int i = 0; i < 128; i++) {
        threadPoll[i] = std::thread(handle_thread, connPoll, c, LOG);
        threadPoll[i].detach();
    }

    connSocket server;
    server.build_server("12345");

    //std::cout << "build success" << std::endl;
    int user_id = 1;
    while(1) {
        try
        {
            //produce read Socket to connPoll.
            connPoll->produce(server.acceptConnection(user_id));
            user_id++;
        }
        catch(const std::exception& e)
        {
            std::string msg("(no-id): ");
            msg.append(e.what());
            LOG->log(msg);
        }
    }
    return EXIT_SUCCESS;
}


void handle_thread(fifo * connPoll, cache * c, logger * LOG) {
    while(1) {
        connSocket * clientfd = connPoll->consume();
        routine service(clientfd, c, LOG);
        try
        {
            service.run();
            signal(SIGPIPE, SIG_IGN);
        }
        catch(const std::exception& e)
        {
            LOG->error(clientfd->user_id, e.what());
        }
        clientfd->closefd();
        delete clientfd;
    }
}

void basic_deamon() {
    pid_t pid = fork();
    if(pid == -1) {
        std::cerr << "fail to fork" << std::endl;
    }
    else if (pid != 0) {
        exit(0);
    }

    if(setsid() == -1) {
        std::cerr << "failed to became a session leader" << std::endl;
    }

    int fd1 = open("/dev/null", O_RDONLY);
    int fd2 = open("/dev/null", O_WRONLY);
    int fd3 = open("/dev/null", O_RDWR);
    dup2(STDIN_FILENO, fd1);
    dup2(STDOUT_FILENO, fd2);
    dup2(STDERR_FILENO, fd3);

    if(chdir("/") == -1) {
        std::cerr << "failed to change working directory" << std::endl;
    }

    umask(0);

    signal(SIGHUP, SIG_IGN);
    pid = fork();
    if(pid == -1) {
        std::cerr << "failde to fork" << std::endl;
    }

    else if (pid != 0) {
        exit(0);
    }
    
}
