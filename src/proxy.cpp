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
//void handle_thread(connSocket * clientfd, cache * c);

//std::ofstream logfile("log.txt");
std::ofstream logfile("/var/log/erss/proxy.log");

int main() {
    
    basic_deamon();

    //this is a connection Poll which holds
    fifo * connPoll = new fifo(128);
    cache * c = new cache(512);
    //logger * LOG = new logger(std::string("log.txt"));
    
    logger * LOG = new logger(&logfile);
    
    std::thread threadPoll[128];
    for(int i = 0; i < 128; i++) {
        threadPoll[i] = std::thread(handle_thread, connPoll, c, LOG);
        threadPoll[i].detach();
    }
    
    /*
    connSocket server;
    int user_id = 1;
    server.build_server("12345");
    //std::thread threadPoll[20];
    for(int i = 0; i < 20; i++) {
        std::thread worker(handle_thread, server.acceptConnection(user_id), c);
        user_id++;
        worker.detach();
    }
    delete c;
    */
    connSocket server;
    server.build_server("12345");

    std::cout << "build success" << std::endl;
    int user_id = 1;
    while(1) {
        try
        {
            //produce read Socket to connPoll.
            connPoll->produce(server.acceptConnection(user_id));
            //std::thread worker(handle_thread, server.acceptConnection(user_id), c);
            //worker.detach();
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

/*
void handle_thread(connSocket * clientfd, cache * c) {
    //std::cout << "begin work " << std::this_thread::get_id() << std::endl;
    //connSocket * clientfd = connPoll->consume();
    routine service(clientfd, c);
    try
    {
        service.run();
        signal(SIGPIPE, SIG_IGN);
    }
    catch(const std::exception& e)
    {
        
        std::cerr << e.what() << '\n';
    }
    std::cout << clientfd->getFD() << std::endl;
    clientfd->closefd();
    delete clientfd;
    std::cout << "delete clientfd" << std::endl;
    
}
*/

/*
void deamon() {
    pid_t pid;

    if ((pid == fork()) < 0)
        return (-1);
    else if (pid)
        exit(0); // parent terminates

    if (setsid() < 0) // dissociate
        return (-1);

    dup2(STDOUT_FILENO, open("/dev/null", O_RDONLY));
    dup2(STDIN_FILENO, open("/dev/null", O_RDWR));
    dup2(STDERR_FILENO, open("/dev/null", O_RDWR));

    chdir("/");
    umask(0);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    if ((pid == fork()) < 0)
        return (-1);
    else if (pid)
        exit(0);

}
*/

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
