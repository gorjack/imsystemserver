#pragma once

#include <memory>
#include <functional>

#include "../base/Timestamp.h"

namespace net
{
	class EventLoop;

	///
	/// A selectable I/O channel.
	///
	/// This class doesn't own the file descriptor.
	/// The file descriptor could be a socket,
	/// an eventfd, a timerfd, or a signalfd
	class Channel
	{
	public:
		typedef std::function<void()> EventCallback;
		typedef std::function<void(Timestamp)> ReadEventCallback;

		Channel(EventLoop* loop, int fd);
		~Channel();

		void handleEvent(Timestamp receiveTime);
		void setReadCallback(const ReadEventCallback& cb)
		{
			m_pReadCallback = cb;
		}
		void setWriteCallback(const EventCallback& cb)
		{
			m_pWriteCallback = cb;
		}
		void setCloseCallback(const EventCallback& cb)
		{
			m_pCloseCallback = cb;
		}
		void setErrorCallback(const EventCallback& cb)
		{
			m_pErrorCallback = cb;
		}

		/// Tie this channel to the owner object managed by shared_ptr,
		/// prevent the owner object being destroyed in handleEvent.
		void tie(const std::shared_ptr<void>&);

		int fd() const { return m_cnFd; }
		int events() const { return m_nEvents; }
		void set_revents(int revt) { m_Revents  = revt; }  // used by pollers
        void add_revents(int revt) { m_Revents |= revt; } // used by pollers
		// int revents() const { return revents_; }
		bool isNoneEvent() const { return m_nEvents == m_sNoneEvent; }

        bool enableReading();
        bool disableReading();
        bool enableWriting();
        bool disableWriting();
        bool disableAll();

		bool isWriting() const { return m_nEvents & m_sWriteEvent; }

		// for Poller
		int index() { return m_nIndex; }
		void set_index(int idx) { m_nIndex = idx; }

		// for debug
		string reventsToString() const;

		void doNotLogHup() { m_bLogHup = false; }

		EventLoop* ownerLoop() { return m_pLoop; }
		void remove();

	private:
		bool update();
		void handleEventWithGuard(Timestamp receiveTime);

		static const int            m_sNoneEvent;
		static const int            m_sReadEvent;
		static const int            m_sWriteEvent;

		EventLoop*                  m_pLoop;
		const int                   m_cnFd;
		int                         m_nEvents;
		int                         m_Revents; // it's the received event types of epoll or poll
		int                         m_nIndex; // used by Poller.
		bool                        m_bLogHup;

		std::weak_ptr<void>         m_pTie;           
		bool                        m_bTied;
		//bool                        eventHandling_;
		//bool                        addedToLoop_;
		ReadEventCallback           m_pReadCallback;
		EventCallback               m_pWriteCallback;
		EventCallback               m_pCloseCallback;
		EventCallback               m_pErrorCallback;
	};
}
