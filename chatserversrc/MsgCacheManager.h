#pragma once
#include <list>
#include <stdint.h>
#include <string>
#include <mutex>

struct NotifyMsgCache
{
    int32_t     userid;
    std::string notifymsg;  
};

struct ChatMsgCache
{
    int32_t     userid;
    std::string chatmsg;
};

class MsgCacheManager final
{
public:
    MsgCacheManager();
    ~MsgCacheManager();

    MsgCacheManager(const MsgCacheManager& rhs) = delete;
    MsgCacheManager& operator =(const MsgCacheManager& rhs) = delete;

    bool addNotifyMsgCache(int32_t userid, const std::string& cache);
    void getNotifyMsgCache(int32_t userid, std::list<NotifyMsgCache>& cached);

    bool addChatMsgCache(int32_t userid, const std::string& cache);
    void getChatMsgCache(int32_t userid, std::list<ChatMsgCache>& cached);


private:
    std::list<NotifyMsgCache>       m_listNotifyMsgCache;    //Í¨ÖªÀàÏûÏ¢»º´æ£¬±ÈÈç¼ÓºÃÓÑÏûÏ¢
    std::mutex                      m_mtNotifyMsgCache;
    std::list<ChatMsgCache>         m_listChatMsgCache;      //ÁÄÌìÏûÏ¢»º´æ
    std::mutex                      m_mtChatMsgCache;
};
