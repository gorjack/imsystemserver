#pragma once

#include <functional>

#include "Channel.h"
#include "Sockets.h"
//#include "EventLoop.h"

namespace net
{
    class EventLoop;
    class InetAddress;

    ///
    /// Acceptor of incoming TCP connections.
    ///
    class Acceptor
    {
    public:
        typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

        Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
        ~Acceptor();

        void setNewConnectionCallback(const NewConnectionCallback& cb)
        {
            m_pNewConnectionCallback = cb;
        }

        bool listenning() const { return m_bListenning; }
        void listen();

    private:
        void handleRead();

    private:
        EventLoop*            m_pLoop;
        Socket                m_objAcceptSocket;
        Channel               m_objAcceptChannel;
        NewConnectionCallback m_pNewConnectionCallback;
        bool                  m_bListenning;

#ifndef WIN32
        int                   m_nIdleFd;
#endif
    };

}
