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
/* Windows/Linux Socket compatibility layer: */
/* Thanks to Benjamin Roux (http://broux.developpez.com/articles/c/sockets/) */
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
/* End of Windows/Linux Socket compatibility layer: */

#include "nutclient.h"


/* Include nut common utility functions or define simple ones if not */
namespace nut {
    namespace intertnal {

#ifdef WIN32
        static bool WSAInitialised = false;
#define ssize_t long
#define FD_TYPE SOCKET

        inline int xread(FD_TYPE socket, char *buf, int size) {
            return recv(socket, buf, size, 0);
        }

        inline int xwrite(FD_TYPE socket, const char *buf, int size) {
            return send(socket, buf, size, 0);
        }

        inline int xclose(FD_TYPE socket) {
            return ::closesocket(socket);
        }

#endif
#ifndef WIN32
#define FD_TYPE int
        inline int xread(FD_TYPE socket, char * buf, int size) {
            return ::read(socket, buf, size);
        }
        inline int xwrite(FD_TYPE socket, const char * buf, int size) {
            return ::write(socket, buf, size);
        }
        inline int xclose(FD_TYPE socket) {
            return ::close(socket);
        }
#endif

        class DefaultSocket : public AbstractSocket {
        public:
            DefaultSocket();

            ~DefaultSocket();

            void connect(const std::string &host, int port) override;

            void disconnect() override;

            bool isConnected() const override;

            size_t read(void *buf, size_t sz) override;

            size_t write(const void *buf, size_t sz) override;

            std::string  read() override;

            void write(const std::string & s) override;

        private:
            SOCKET _sock;
            struct timeval _tv;
            std::string _buffer; /* Received buffer, string because data should be text only. */
        };


    }
}
#endif //NUTCLIENT_DEFAULTSOCKET_H
