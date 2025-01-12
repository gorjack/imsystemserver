#pragma once

#include <stdint.h>

enum
{
    PACKAGE_UNCOMPRESSED,
    PACKAGE_COMPRESSED
};

enum msg_type
{
    msg_type_unknown,
    msg_type_heartbeat = 1000,     //心跳包
    msg_type_register,             //注册
    msg_type_login,                //登陆
    msg_type_getofriendlist,       //获取好友列表
    msg_type_finduser,             //查找用户
    msg_type_operatefriend,        //添加、删除等好友操作
    msg_type_userstatuschange,     //用户信息改变通知
    msg_type_updateuserinfo,       //更新用户信息
    msg_type_modifypassword,       //修改登陆密码
    msg_type_creategroup,          //创建群组
    msg_type_getgroupmembers,      //获取群组成员列表
    msg_type_chat   = 1100,        //单聊消息
    msg_type_multichat,            //群发消息
    msg_type_kickuser,             //被踢下线
    msg_type_remotedesktop,        //远程桌面
    msg_type_updateteaminfo,       //更新用户好友分组信息
    msg_type_modifyfriendmarkname, //更新好友备注信息
    msg_type_movefriendtootherteam, //移动好友至
};

//在线类型
enum online_type{
    online_type_offline         = 0,    //离线
    online_type_pc_online       = 1,    //电脑在线
    online_type_pc_invisible    = 2,    //电脑隐身
    online_type_android_cellular= 3,    //android 3G/4G/5G在线
    online_type_android_wifi    = 4,    //android wifi在线
    online_type_ios             = 5,    //ios 在线
    online_type_mac             = 6     //MAC在线
};

#pragma pack(push, 1)
//协议头
struct chat_msg_header
{
    char     compressflag;     //压缩标志，如果为1，则启用压缩，反之不启用压缩
    int32_t  originsize;       //包体压缩前大小
    int32_t  compresssize;     //包体压缩后大小
    char     reserved[16];
};

#pragma pack(pop)

//type为1发出加好友申请 2 收到加好友请求(仅客户端使用) 3应答加好友 4删除好友请求 5应答删除好友
//当type=3时，accept是必须字段，0对方拒绝，1对方接受
enum friend_operation_type
{
    //发送加好友申请
    friend_operation_send_add_apply      = 1,
    //接收到加好友申请(仅客户端使用)
    friend_operation_recv_add_apply,
    //应答加好友申请
    friend_operation_reply_add_apply,
    //删除好友申请
    friend_operation_send_delete_apply,
    //应答删好友申请
    friend_operation_recv_delete_apply
};

enum friend_operation_apply_type
{
    //拒绝加好友
    friend_operation_apply_refuse,
    //接受加好友
    friend_operation_apply_accept
};

enum updateteaminfo_operation_type
{
    //新增分组
    updateteaminfo_operation_add,
    //删除分组
    updateteaminfo_operation_delete,
    //修改分组
    updateteaminfo_operation_modify
};


/**
 *  错误码
 *  0   成功
 *  1   未知失败
 *  2   用户未登录
 *  100 注册失败
 *  101 已注册
 *  102 未注册
 *  103 密码错误
 *  104 更新用户信息失败
 *  105 修改密码失败
 *  106 创建群失败
 *  107 客户端版本太旧，需要升级成新版本
 */
//TODO: 其他的地方改成这个错误码
enum error_code
{
    error_code_ok                   = 0,
    error_code_unknown              = 1,
    error_code_notlogin             = 2,
    error_code_registerfail         = 100,
    error_code_registeralready      = 101,
    error_code_notregister          = 102,
    error_code_invalidpassword      = 103,
    error_code_updateuserinfofail   = 104,
    error_code_modifypasswordfail   = 105,
    error_code_creategroupfail      = 106,
    error_code_toooldversion        = 107,
    error_code_modifymarknamefail   = 108,
    error_code_teamname_exsit       = 109, //·Ö×éÒÑ¾­´æÔÚ
};

