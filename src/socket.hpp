#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

#include "myexception.hpp"

#define BUFSIZE 65536
class connSocket {
private:
  int fd; // file descriptor
  char buf[BUFSIZE];
  int buf_cnt = 0;
  char * buf_ptr = buf;
  
  /**
   * @brief read n bytes in the buf to the socket.
   *
   * @param rp file
   * @param buf store the data that we want to read
   * @param n the number of the data that need to be read
   */
  ssize_t rio_read(char *userbuf, size_t n) {
    size_t cnt;
    while (buf_cnt <= 0) { /* Refill if buf is empty */
      buf_cnt = recv(fd, buf, sizeof(buf), 0);
      if (buf_cnt < 0) {
        return -1;
      } else if (buf_cnt == 0) /* EOF */
        return 0;
      else
        buf_ptr = buf; /* Reset buffer ptr */
    }
    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */ 
    cnt = n;
    if (buf_cnt < n)
      cnt = buf_cnt;
    memcpy(userbuf, buf_ptr, cnt);
    buf_ptr += cnt;
    buf_cnt -= cnt;
    return cnt;
  }

public:
  std::string user_id;
  std::string user_ip;
  // constructor: give the default value -1 to fd.
  connSocket() : fd(-1) {}

  // constructor: used for creating socket that return from the accept()
  // function.
  connSocket(int fd) : fd(fd) {}

  // return the fd that the socket hold.
  int getFD() { return fd; }

  /**
   * @brief The function used for build a server socket for listening incoming
   * connection. first build socket and bind the addrinfo, then listen to the
   * connection. put the return file description of socket to the fd filed.
   *
   * @param port the port the server listen to.
   */
  void build_server(const std::string &port) {
    struct addrinfo hint, *list, *p;
    int status;
    int optval = 1;

    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;

    const char *temp = port.c_str();
    status = getaddrinfo(NULL, temp, &hint, &list);
    if(status < 0) throw myexception("cannot get addrinfo.");

    for (p = list; p; p = p->ai_next) {
      /* Create a socket descriptor */
      if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        continue; /* Socket failed, try the next */
      /* Eliminates "Address already in use" error from bind */
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                 sizeof(int));
      /* Bind the descriptor to the address */
      if (bind(fd, p->ai_addr, p->ai_addrlen) == 0)
        break;   /* Success */
      close(fd); /* Bind failed, try the next */
    }

    /* Clean Up */
    freeaddrinfo(list);
    if (!p) /* No address worked */
      throw myexception("cannot find a worked address in list.");
    /* Make it a listening socket ready to accept connection requests */
    if (listen(fd, 100) < 0) {
      close(fd);
      throw myexception("cannot build a listening socket.");
    }
  }

  /**
   * @brief accept the incoming connection.
   * and return the new connection socket to the caller.
   *
   * @return int the file descriptor that accept() return.
   */
  connSocket *acceptConnection(int user_id) {
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;
    client_connection_fd =
        accept(fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    
    struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
    char * ip = inet_ntoa(addr->sin_addr);

    if (client_connection_fd == -1) {
      throw myexception("cannot accept socket");
    }
    connSocket * re = new connSocket(client_connection_fd);
    re->user_id = std::to_string(user_id);
    re->user_ip = ip;
    return re;
  }

  /**
   * @brief build a client socket which connect to the @hostname and port 80
   * Use socket() and connect() to connect to the origin server.
   *
   * @param hostname of the origin server
   */
  void build_client(std::string &hostname, std::string &port) {
    struct addrinfo hint, *list, *p;
    int status;

    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname.c_str(), port.c_str(), &hint, &list);
    if(status < 0) throw myexception("cannot get addrinfo.");
    
    /* Walk the list for one that we can successfully connect to */
    for (p = list; p; p = p->ai_next) {
      /* Create a socket descriptor */
      if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        continue; /* Socket failed, try the next */
      /* Connect to the server */
      if (connect(fd, p->ai_addr, p->ai_addrlen) != -1)
        break;   /* Success */
      close(fd); /* Connect failed, try another */
    }

    /* Clean up */
    freeaddrinfo(list);
    if (!p) /* All connects failed */
      throw myexception("cannot connect to server.");
  }


  /**
   * @brief read a line from the socket.
   *
   * @return std::string the line that read from the socket.
   */
  std::string readLine() {
    int rc;
    char c;
    //std::string ans;
    std::vector<char> ans;
    while(1) {
      if ((rc = rio_read(&c, 1)) == 1) {
        ans.push_back(c);
        if (c == '\n') break;
      } else if (rc == 0) {
        return std::string(ans.begin(), ans.end());
      } else {
        throw myexception("cannot read line.");
      }
    }
    return std::string(ans.begin(), ans.end());
  }

  /**
   * @brief read n bytes from the socket and store them to the buf
   *
   * @param rp file
   * @param buf the user buffer that store the data read from the file buffer
   * @param n the number of the bytes
   */
  ssize_t readnB(char *userbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *bufp = userbuf;
    while (nleft > 0) {
      if ((nread = rio_read(bufp, nleft)) < 0)
        return -1;
        //throw myexception("Error: cannot read n bytes from the socket."); /* errno set by read() */
      else if (nread == 0)
        break; /* EOF */
      nleft -= nread;
      bufp += nread;
    }
    return n - nleft;
  }

  /**
   * @brief write n bytes in the buf to the socket.
   *
   * @param fd socket that we want to write to
   * @param buf store the data that we want to write
   * @param n the number of the data that need to be written
   */
  ssize_t writenB(const char *userbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    const char *bufp = userbuf;
    while (nleft > 0) {
      if ((nwritten = write(fd, bufp, nleft)) <= 0) {
        if (errno == EPIPE) /* Interrupted by sig handler return */
          return 0;
        else
          return -1;
          //throw myexception("Error: cannot write to the socket.");
      }
      nleft -= nwritten;
      bufp += nwritten;
    }
    return n;
  }

  std::vector<std::string> readhttp() {
    std::vector<std::string> re; //contain the http part without the end string "\r\n"
    
    while(1) {
      std::string line = readLine();
      if(line != "\r\n") re.push_back(line);
      else break;
    }
    return re;
    
  }

  void closefd(){
    if(fd != -1) close(fd);
  }

  //desconstructor
  ~connSocket(){}
};

#endif