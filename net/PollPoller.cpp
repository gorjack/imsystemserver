#include "PollPoller.h"

#ifndef WIN32

#include "../base/AsyncLog.h"
#include "Channel.h"
#include "EventLoop.h"

#include <errno.h>
#include <poll.h>

using namespace net;

PollPoller::PollPoller(EventLoop* loop) : m_pOwnerLoop(loop)
{
}

PollPoller::~PollPoller()
{
}

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    // XXX m_vecPollfds shouldn't change
    int numEvents = ::poll(&*m_vecPollfds.begin(), m_vecPollfds.size(), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOGD("%d  events happended", numEvents);
        fillActiveChannels(numEvents, activeChannels);
    }
    else if (numEvents == 0)
    {
        LOGD("nothing happended");
    }
    else
    {
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOGSYSE("PollPoller::poll()");
        }
    }
    return now;
}

void PollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for (PollFdList::const_iterator pfd = m_vecPollfds.begin(); pfd != m_vecPollfds.end() && numEvents > 0; ++pfd)
    {
        if (pfd->revents > 0)
        {
            --numEvents;
            ChannelMap::const_iterator ch = m_mapChannels.find(pfd->fd);
            //assert(ch != m_mapChannels.end());
            if (ch == m_mapChannels.end())
                continue;

            Channel * channel = ch->second;
            //assert(channel->fd() == pfd->fd);
            if (channel->fd() != pfd->fd)
                continue;

            channel->set_revents(pfd->revents);
            // pfd->revents = 0;
            activeChannels->push_back(channel);
        }
    }
}

bool PollPoller::updateChannel(Channel * channel)
{
    assertInLoopThread();
    LOGD("fd = %d events = %d", channel->fd(), channel->events());
    if (channel->index() < 0)
    {
        // a new one, add to m_vecPollfds
        //assert(m_mapChannels.find(channel->fd()) == m_mapChannels.end());
        if (m_mapChannels.find(channel->fd()) != m_mapChannels.end())
            return false;

        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        m_vecPollfds.push_back(pfd);
        int idx = static_cast<int>(m_vecPollfds.size()) - 1;
        channel->set_index(idx);
        m_mapChannels[pfd.fd] = channel;
    }
    else
    {
        // update existing one
        //assert(m_mapChannels.find(channel->fd()) != m_mapChannels.end());
        //assert(m_mapChannels[channel->fd()] == channel);
        if (m_mapChannels.find(channel->fd()) == m_mapChannels.end() || m_mapChannels[channel->fd()] != channel)
            return false;

        int idx = channel->index();
        //assert(0 <= idx && idx < static_cast<int>(m_vecPollfds.size()));
        if (0 > idx || idx >= static_cast<int>(m_vecPollfds.size()))
            return false;

        struct pollfd& pfd = m_vecPollfds[idx];
        //TODO: 为什么是 -channel->fd() ？
        //assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNoneEvent())
        {
            // ignore this pollfd
            pfd.fd = -channel->fd() - 1;
        }
    }

    return true;
}

void PollPoller::removeChannel(Channel * channel)
{
    assertInLoopThread();
    LOGD("fd = %d", channel->fd());
    
    //assert(m_mapChannels.find(channel->fd()) != m_mapChannels.end());
    //assert(m_mapChannels[channel->fd()] == channel);
    //assert(channel->isNoneEvent());

    if (m_mapChannels.find(channel->fd()) == m_mapChannels.end() || m_mapChannels[channel->fd()] != channel || !channel->isNoneEvent())
        return;

    int idx = channel->index();
    //assert(0 <= idx && idx < static_cast<int>(m_vecPollfds.size()));
    if (0 > idx && idx >= static_cast<int>(m_vecPollfds.size()))
        return;

    const struct pollfd& pfd = m_vecPollfds[idx]; (void)pfd;
    //TODO: 为什么是 -channel->fd()？
    //assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
    
    size_t n = m_mapChannels.erase(channel->fd());
    //assert(n == 1); (void)n;
    if (n != 1)
        return;

    if (implicit_cast<size_t>(idx) == m_vecPollfds.size() - 1)
    {
        m_vecPollfds.pop_back();
    }
    else
    {
        int channelAtEnd = m_vecPollfds.back().fd;
        iter_swap(m_vecPollfds.begin() + idx, m_vecPollfds.end() - 1);
        if (channelAtEnd < 0)
        {
            channelAtEnd = -channelAtEnd - 1;
        }
        m_mapChannels[channelAtEnd]->set_index(idx);
        m_vecPollfds.pop_back();
    }
}

void PollPoller::assertInLoopThread() const
{
    m_pOwnerLoop->assertInLoopThread();
}

#endif //!WIN32
