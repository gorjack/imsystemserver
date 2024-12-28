#include "EpollPoller.h"

#ifndef WIN32
#include <string.h>
#include "../base/Platform.h"
#include "../base/AsyncLog.h"
#include "EventLoop.h"
#include "Channel.h"



using namespace net;

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
//static_assert(EXPOLLIN == XPOLLIN, "EXPOLLIN == XPOLLIN");
//static_assert(EXPOLLPRI == XPOLLPRI, "EXPOLLPRI == XPOLLPRI");
//static_assert(EXPOLLOUT == XPOLLOUT, "EXPOLLOUT == XPOLLOUT");
//static_assert(EXPOLLRDHUP == XPOLLRDHUP,"EXPOLLRDHUP == XPOLLRDHUP");
//static_assert(EXPOLLERR == XPOLLERR, "EXPOLLERR == XPOLLERR");
//static_assert(EXPOLLHUP == XPOLLHUP, "EXPOLLHUP == XPOLLHUP");

namespace
{
    const int cnNew = -1;
    const int cnAdded = 1;
    const int cnDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
    :m_nEpollfd(::epoll_create1(EPOLL_CLOEXEC)),
    m_vecEvents(scnInitEventListSize),
    m_pOwnerLoop(loop)
{
    if (m_nEpollfd < 0)
    {
        LOGF("EPollPoller::EPollPoller");
    }
}

EPollPoller::~EPollPoller()
{
    ::close(m_nEpollfd);
}

bool EPollPoller::hasChannel(Channel* channel) const
{
    assertInLoopThread();
    ChannelMap::const_iterator it = m_mapFd2Channel.find(channel->fd());
    return it != m_mapFd2Channel.end() && it->second == channel;
}

void EPollPoller::assertInLoopThread() const
{
    m_pOwnerLoop->assertInLoopThread();
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    int numEvents = ::epoll_wait(m_nEpollfd,
        &*m_vecEvents.begin(),
        static_cast<int>(m_vecEvents.size()),
        timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        //LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);
        if (static_cast<size_t>(numEvents) == m_vecEvents.size())
        {
            m_vecEvents.resize(m_vecEvents.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        //LOG_TRACE << " nothing happended";
    }
    else
    {
        // error happens, log uncommon ones
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOGSYSE("EPollPoller::poll()");
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    //assert(static_cast<size_t>(numEvents) <= m_vecEvents.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(m_vecEvents[i].data.ptr);
        int fd = channel->fd();
        ChannelMap::const_iterator it = m_mapFd2Channel.find(fd);
        if (it == m_mapFd2Channel.end() || it->second != channel)
            return;
        channel->set_revents(m_vecEvents[i].events);
        activeChannels->push_back(channel);
    }
}

bool EPollPoller::updateChannel(Channel* channel)
{
    assertInLoopThread();
    LOGD("fd = %d  events = %d", channel->fd(), channel->events());
    const int index = channel->index();
    if (index == cnNew || index == cnDeleted)
    {
        // a new one, add with XEPOLL_CTL_ADD
        int fd = channel->fd();
        if (index == cnNew)
        {
            //assert(m_mapFd2Channel.find(fd) == m_mapFd2Channel.end())
            if (m_mapFd2Channel.find(fd) != m_mapFd2Channel.end())
            {
                LOGE("fd = %d  must not exist in m_mapFd2Channel", fd);
                return false;
            }


            m_mapFd2Channel[fd] = channel;
        }
        else // index == cnDeleted
        {
            //assert(m_mapFd2Channel.find(fd) != m_mapFd2Channel.end());
            if (m_mapFd2Channel.find(fd) == m_mapFd2Channel.end())
            {
                LOGE("fd = %d  must exist in m_mapFd2Channel", fd);
                return false;
            }

            //assert(m_mapFd2Channel[fd] == channel);
            if (m_mapFd2Channel[fd] != channel)
            {
                LOGE("current channel is not matched current fd, fd = %d", fd);
                return false;
            }
        }
        channel->set_index(cnAdded);

        return update(XEPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with XEPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        //assert(m_mapFd2Channel.find(fd) != m_mapFd2Channel.end());
        //assert(m_mapFd2Channel[fd] == channel);
        //assert(index == cnAdded);
        if (m_mapFd2Channel.find(fd) == m_mapFd2Channel.end() || m_mapFd2Channel[fd] != channel || index != cnAdded)
        {
            LOGE("current channel is not matched current fd, fd = %d, channel = 0x%x", fd, channel);
            return false;
        }

        if (channel->isNoneEvent())
        {
            if (update(XEPOLL_CTL_DEL, channel))
            {
                channel->set_index(cnDeleted);
                return true;
            }
            return false;
        }
        else
        {
            return update(XEPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel)
{
    assertInLoopThread();
    int fd = channel->fd();

    //assert(m_mapFd2Channel.find(fd) != m_mapFd2Channel.end());
    //assert(m_mapFd2Channel[fd] == channel);
    //assert(channel->isNoneEvent());
    if (m_mapFd2Channel.find(fd) == m_mapFd2Channel.end() || m_mapFd2Channel[fd] != channel || !channel->isNoneEvent())
        return;

    int index = channel->index();
    //assert(index == cnAdded || index == cnDeleted);
    if (index != cnAdded && index != cnDeleted)
        return;

    size_t n = m_mapFd2Channel.erase(fd);
    //(void)n;
    //assert(n == 1);
    if (n != 1)
        return;

    if (index == cnAdded)
    {
        update(XEPOLL_CTL_DEL, channel);
    }
    channel->set_index(cnNew);
}

bool EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(m_nEpollfd, operation, fd, &event) < 0)
    {
        if (operation == XEPOLL_CTL_DEL)
        {
            LOGE("epoll_ctl op=%d fd=%d, epollfd=%d, errno=%d, errorInfo: %s", operation, fd, m_nEpollfd, errno, strerror(errno));
        }
        else
        {
            LOGE("epoll_ctl op=%d fd=%d, epollfd=%d, errno=%d, errorInfo: %s", operation, fd, m_nEpollfd, errno, strerror(errno));
        }

        return false;
    }

    return true;
}

#endif
