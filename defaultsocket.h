//
// Created by andre on 02/11/2021.
//

#ifndef NUTCLIENT_DEFAULTSOCKET_H
#define NUTCLIENT_DEFAULTSOCKET_H
#include "nutclient.h"
#include <sstream>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#ifdef WIN32
#include <ws2tcpip.h>
#include <io.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h> /* close */
#  include <netdb.h> /* gethostbyname */
#  include <fcntl.h>
#  define INVALID_SOCKET -1
#  define SOCKET_ERROR -1
#  define closesocket(s) close(s)
   typedef int SOCKET;
   typedef struct sockaddr_in SOCKADDR_IN;
   typedef struct sockaddr SOCKADDR;
   typedef struct in_addr IN_ADDR;
#endif /* WIN32 */


/* Include nut common utility functions or define simple ones if not */
namespace nut {
    namespace internal {

        std::shared_ptr<nut::AbstractSocket> defaultFactory();

    }
}
#endif //NUTCLIENT_DEFAULTSOCKET_H
