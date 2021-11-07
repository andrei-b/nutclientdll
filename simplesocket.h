//
// Created by andre on 02/11/2021.
//

#ifndef NUTCLIENT_SIMPLESOCKET_H
#define NUTCLIENT_SIMPLESOCKET_H
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

        class SimpleSocket : public AbstractSocket {
        public:
            SimpleSocket();

            ~SimpleSocket();

            void connect(const std::string &host, int port) override;

            void disconnect() override;

            bool isConnected() const override;

            size_t read(void *buf, size_t sz) override;

            size_t write(const void *buf, size_t sz) override;

        private:
            SOCKET _sock;
            struct timeval _tv;
            std::string _buffer; /* Received buffer, string because data should be text only. */
        };

        SimpleSocket::SimpleSocket() :
                _sock(INVALID_SOCKET),
                _tv() {
            _tv.tv_sec = -1;
            _tv.tv_usec = 0;
        }

        SimpleSocket::~SimpleSocket() {
            disconnect();
        }

        void SimpleSocket::connect(const std::string &host, int port) {
            FD_TYPE sock_fd;
            struct addrinfo hints, *res, *ai;
            char sport[NI_MAXSERV];
            int v;
            fd_set wfds;
            int error;
            socklen_t error_size;
            long fd_flags;

            _sock = -1;

            if (host.empty()) {
                throw nut::UnknownHostException();
            }

#ifdef WIN32
            if (!WSAInitialised) {
                WSADATA wsaData;
                auto iResult = WSAStartup(MAKEWORD(2, 0), &wsaData);
                if (iResult == 0) {
                    WSAInitialised = true;
                }
            }
#endif

            snprintf(sport, sizeof(sport), "%hu", static_cast<unsigned short int>(port));

            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            while ((v = getaddrinfo(host.c_str(), sport, &hints, &res)) != 0) {
                switch (v) {
                    case EAI_AGAIN:
                        continue;
                    case EAI_NONAME:
                        throw nut::UnknownHostException();
#ifndef WIN32
                        case EAI_SYSTEM:
            throw nut::SystemException();
#endif
                    case EAI_MEMORY:
                        throw nut::NutException("Out of memory");
                    default:
                        throw nut::NutException("Unknown error");
                }
            }

            for (ai = res; ai != nullptr; ai = ai->ai_next) {

                sock_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

                if (sock_fd < 0) {
                    switch (errno) {
                        case EAFNOSUPPORT:
                        case EINVAL:
                            break;
                        default:
                            throw nut::SystemException();
                    }
                    continue;
                }


#ifndef WIN32
                /* non blocking connect */
        if(hasTimeout()) {
            fd_flags = fcntl(sock_fd, F_GETFL);
            fd_flags |= O_NONBLOCK;
            fcntl(sock_fd, F_SETFL, fd_flags);
        }
#endif

                while ((v = ::connect(sock_fd, ai->ai_addr, ai->ai_addrlen)) < 0) {
                    if (errno == EINPROGRESS) {
                        FD_ZERO(&wfds);
                        FD_SET(sock_fd, &wfds);
                        select(sock_fd + 1, nullptr, &wfds, nullptr, hasTimeout() ? &_tv : nullptr);
                        if (FD_ISSET(sock_fd, &wfds)) {
                            error_size = sizeof(error);
#ifndef WIN32
                            getsockopt(sock_fd, SOL_SOCKET, SO_ERROR,
                            &error, &error_size);
#endif
#ifdef WIN32
                            getsockopt(sock_fd, SOL_SOCKET, SO_ERROR,
                                       (char *) &error, &error_size);
#endif
                            if (error == 0) {
                                /* connect successful */
                                v = 0;
                                break;
                            }
                            errno = error;
                        } else {
                            /* Timeout */
                            v = -1;
                            break;
                        }
                    }

                    switch (errno) {
                        case EAFNOSUPPORT:
                            break;
                        case EINTR:
                        case EAGAIN:
                            continue;
                        default:
//				ups->upserror = UPSCLI_ERR_CONNFAILURE;
//				ups->syserrno = errno;
                            break;
                    }
                    break;
                }

                if (v < 0) {
                    xclose(sock_fd);
                    continue;
                }

#ifndef WIN32
                /* switch back to blocking operation */
        if(hasTimeout()) {
            fd_flags = fcntl(sock_fd, F_GETFL);
            fd_flags &= ~O_NONBLOCK;
            fcntl(sock_fd, F_SETFL, fd_flags);
        }
#endif
                _sock = sock_fd;
//		ups->upserror = 0;
//		ups->syserrno = 0;
                break;
            }

            freeaddrinfo(res);

            if (_sock < 0) {
                throw nut::IOException("Cannot connect to host");
            }


#ifdef OLD
            struct hostent *hostinfo = nullptr;
    SOCKADDR_IN sin = { 0 };
    hostinfo = ::gethostbyname(host.c_str());
    if(hostinfo == nullptr) /* Host doesnt exist */
    {
        throw nut::UnknownHostException();
    }

    // Create socket
    _sock = ::socket(PF_INET, SOCK_STREAM, 0);
    if(_sock == INVALID_SOCKET)
    {
        throw nut::IOException("Cannot create socket");
    }

    // Connect
    sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;
    if(::connect(_sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        _sock = INVALID_SOCKET;
        throw nut::IOException("Cannot connect to host");
    }
#endif // OLD
        }

        void SimpleSocket::disconnect() {
            if (_sock != INVALID_SOCKET) {
                ::closesocket(_sock);
                _sock = INVALID_SOCKET;
            }
            _buffer.clear();
        }

        bool SimpleSocket::isConnected() const {
            return _sock != INVALID_SOCKET;
        }

        size_t SimpleSocket::read(void *buf, size_t sz) {
            if (!isConnected()) {
                throw nut::NotConnectedException();
            }

            if (_tv.tv_sec >= 0) {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(_sock, &fds);
                int ret = select(_sock + 1, &fds, nullptr, nullptr, &_tv);
                if (ret < 1) {
                    throw nut::TimeoutException();
                }
            }

            ssize_t res = xread(_sock, static_cast<char *>(buf), sz);
            if (res == -1) {
                disconnect();
                throw nut::IOException("Error while reading from socket");
            }
            return static_cast<size_t>(res);
        }

        size_t SimpleSocket::write(const void *buf, size_t sz) {
            if (!isConnected()) {
                throw nut::NotConnectedException();
            }

            if (_tv.tv_sec >= 0) {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(_sock, &fds);
                int ret = select(_sock + 1, nullptr, &fds, nullptr, &_tv);
                if (ret < 1) {
                    throw nut::TimeoutException();
                }
            }

            ssize_t res = xwrite(_sock, static_cast<const char *>(buf), sz);
            if (res == -1) {
                disconnect();
                throw nut::IOException("Error while writing on socket");
            }
            return static_cast<size_t>(res);
        }

    }
}
#endif //NUTCLIENT_SIMPLESOCKET_H
