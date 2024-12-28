#include "Channel.h"
#include <sstream>

#include "../base/Platform.h"
#include "../base/AsyncLog.h"
#include "Poller.h"
#include "EventLoop.h"

using namespace net;

const int Channel::m_sNoneEvent = 0;
const int Channel::m_sReadEvent = XPOLLIN | XPOLLPRI;
const int Channel::m_sWriteEvent = XPOLLOUT;

Channel::Channel(EventLoop* loop, int fd__): m_pLoop(loop),
                                            m_cnFd(fd__),
                                            m_nEvents(0),
                                            m_Revents(0),
                                            m_nIndex(-1),
                                            m_bLogHup(true),
                                            m_bTied(false)/*,
                                            eventHandling_(false),
                                            addedToLoop_(false)
                                            */
{
}

Channel::~Channel()
{
	//assert(!eventHandling_);
	//assert(!addedToLoop_);
	if (m_pLoop->isInLoopThread())
	{
		//assert(!m_pLoop->hasChannel(this));
	}
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
	m_pTie = obj;
	m_bTied = true;
}

bool Channel::enableReading() 
{ 
    m_nEvents |= m_sReadEvent;
    return update();
}

bool Channel::disableReading()
{
    m_nEvents &= ~m_sReadEvent; 
    
    return update();
}

bool Channel::enableWriting() 
{
    m_nEvents |= m_sWriteEvent; 
    
    return update(); 
}

bool Channel::disableWriting()
{ 
    m_nEvents &= ~m_sWriteEvent; 
    return update();
}

bool Channel::disableAll()
{ 
    m_nEvents = m_sNoneEvent; 
    return update(); 
}

bool Channel::update()
{
	//addedToLoop_ = true;
	return m_pLoop->updateChannel(this);
}

void Channel::remove()
{
	if (!isNoneEvent())
        return;
	//addedToLoop_ = false;
	m_pLoop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
	std::shared_ptr<void> guard;
	if (m_bTied)
	{
		guard = m_pTie.lock();
		if (guard)
		{
			handleEventWithGuard(receiveTime);
		}
	}
	else
	{
		handleEventWithGuard(receiveTime);
	}
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
	{
		if (m_bLogHup)  
		{ 
			LOGW("Channel::handle_event() XPOLLHUP");
		}
		if (m_pCloseCallback)
			m_pCloseCallback();
	}

	if (m_Revents & XPOLLNVAL)
	{
		LOGW("Channel::handle_event() XPOLLNVAL");
	}

	if (m_Revents & (XPOLLERR | XPOLLNVAL))
	{
		if (m_pErrorCallback) 
            m_pErrorCallback();
	}
    
	if (m_Revents & (XPOLLIN | XPOLLPRI | XPOLLRDHUP))
	{
        if (m_pReadCallback) 
            m_pReadCallback(receiveTime);
	}

	if (m_Revents & XPOLLOUT)
	{
        if (m_pWriteCallback) 
            m_pWriteCallback();
	}
	//eventHandling_ = false;
}

string Channel::reventsToString() const
{
	std::ostringstream oss;
	oss << m_cnFd << ": ";
	if (m_Revents & XPOLLIN)
		oss << "IN ";
	if (m_Revents & XPOLLPRI)
		oss << "PRI ";
	if (m_Revents & XPOLLOUT)
		oss << "OUT ";
	if (m_Revents & XPOLLHUP)
		oss << "HUP ";
	if (m_Revents & XPOLLRDHUP)
		oss << "RDHUP ";
	if (m_Revents & XPOLLERR)
		oss << "ERR ";
	if (m_Revents & XPOLLNVAL)
		oss << "NVAL ";

	return oss.str().c_str();
}
