#include "GatewayServer_PCH.h"


NetMsgHandler::NetMsgHandler()
{
	/* common ��Ҫ�õ��ڲ�Э�� */
	/* �ڲ�Э�飬�����ֲ�ͬservertype����ȥ��ͳһ�����飬���¼�������б���ping�ȵȲ��� */ 
#define REGISTER_INTERNAL_MESSAGE(msg_idx,cls,handler) \
	{\
	RegisterMessage(msg_idx, sizeof(cls), boost::bind(&NetMsgHandler::handler, this, _1, _2, _3)); \
}

	REGISTER_INTERNAL_MESSAGE(PRO_SS_RQ_LOGIN,SSRqLogin,recvLoginRequest);
	REGISTER_INTERNAL_MESSAGE(PRO_SS_RT_LOGINED,SSRtLogined,recvLoginReponse);
	REGISTER_INTERNAL_MESSAGE(PRO_SS_RQ_PING_S,SSRqPingToS, recvPingRequest);
	REGISTER_INTERNAL_MESSAGE(PRO_SS_RT_SERVERINFO_LIST, SSServerRegList, recvSrvListNotifty);
	//REGISTER_INTERNAL_MESSAGE(PRO_SS_REQ_CONNECT_INFO,	SSNotifyConnectInfo,NotifyConnectInfo);
	//REGISTER_INTERNAL_MESSAGE(PRO_SS_REQ_INIT_CLIENT,	SSReqInitClient,	QqInitClient);

#undef REGISTER_INTERNAL_MESSAGE

		// ls
#define REGISTER_LS_MESSAGE(msg_idx,cls,handler)\
	{\
	RegisterMessage(msg_idx, sizeof(cls), boost::bind(&ProcLsHandler::handler, ProcLsHandler::Instance(), _1, _2, _3)); \
	}

//	REGISTER_LS_MESSAGE(PRO_L2W_LOADLIST, L2WLoadList, RqLoadList);

#undef REGISTER_LS_MESSAGE

		// ss
#define REGISTER_SS_MESSAGE(msg_idx,cls,handler)\
	{\
	RegisterMessage(msg_idx, sizeof(cls), boost::bind(&ProcSsHandler::handler, ProcSsHandler::Instance(), _1, _2, _3)); \
	}

	REGISTER_SS_MESSAGE(PRO_S2F_SYNC_USER_SCENE, S2FSyncUserScene, NtSyncUserScene);
	REGISTER_SS_MESSAGE(PRO_S2F_SYNC_USER_DATA, S2FSyncUserData, NtSyncUserData);
	REGISTER_SS_MESSAGE(PRO_S2F_BORADCAST_MSG, S2FBoradCastMsg, NtBroadcastMsg);
	
#undef REGISTER_SS_MESSAGE

	// send to player
#define REGISTER_SEND_TO_PLAYER_MESSAGE(msg_idx,cls)\
	{\
	RegisterMessage(msg_idx, sizeof(cls), boost::bind(&NetMsgHandler::sendSToPlayer, this, _1, _2, _3)); \
	}

	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_L2C_ACC_LOGIN, L2CAccLogin);	
	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_W2C_USERLIST, W2CUserList);
	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_W2C_CREATE_RET, W2CCreateRet);
	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_S2C_SCENE_LOAD_INFO, S2CSceneLoadInfo);

	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_S2C_CHAR_MAIN_DATA, S2CCharMainData);
	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_S2C_ITEM_MAIN_DATA, S2CUserPackages);
	

	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_S2C_CHANNEL_JION, S2CChannelJion);
	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_S2C_CHANNEL_LEAVE, S2CChannelLeave);

	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_S2C_SEND_DATA_FINISH, S2CSendDataFinish);

	// 
	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_S2C_RELATION_LIST,S2CRelationList);
	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_S2C_RELATION_UPDATE, S2CRelationUpdate);
	REGISTER_SEND_TO_PLAYER_MESSAGE(PRO_S2C_RELATION_DELETE, S2CRelationDelete);

#undef REGISTER_SEND_TO_PLAYER_MESSAGE

}


NetMsgHandler::~NetMsgHandler()
{ 
}


void NetMsgHandler::OnNetMsgEnter(NetSocket& rSocket)
{
	Zebra::logger->info("���ӳɹ���������:id=%d ip=%s,port=%d", rSocket.SID(),rSocket.GetIp().c_str(),(int32)rSocket.GetPort());
	zSession* pSession = NetService::getMe().getSessionMgr().get(rSocket.SLongID());
	if (pSession)
	{
		if (pSession->serverType == zSession::SERVER_TYPE_CLIENT)
		{
			SSRqLogin send;
			send.serverID = NetService::getMe().getServerID();
			send.serivceID = pSession->serivceid;
			pSession->sendMsg(&send, send.GetPackLength());
			Zebra::logger->info("���͵�¼��Ϣ��:ip=%s port=%d", rSocket.GetIp().c_str(), (int32)rSocket.GetPort());
		}
	}
	else
	{
		ASSERT(0);
	}
}

void NetMsgHandler::OnNetMsg(NetSocket& rSocket, NetMsgSS* pMsg,int32 nSize)
{
	zSession* pSession = NetService::getMe().getSessionMgr().get(rSocket.SLongID());
	if(pSession == NULL)
	{
		Zebra::logger->error("Can not find session");
		rSocket.OnEventColse();
		return ;
	}

	const MsgFunc* pMsgHandlerInfo = GetMsgHandler(pMsg->protocol);
	if(pMsgHandlerInfo == NULL)
	{
		Zebra::logger->error("�Ҳ�����Э��:%d,��С:%d", pMsg->protocol,nSize);
		rSocket.OnEventColse();
		return;
	}

	Zebra::logger->info("�յ�Э��=%d", pMsg->protocol);

	(pMsgHandlerInfo->handlerFun)((zSession*)(pSession),pMsg,nSize);

}

/* ��Server�Ͽ��ص�(��ʱsocket�Ѿ�����) */ 
void NetMsgHandler::OnNetMsgExit(NetSocket& rSocket)
{     
	Zebra::logger->info("���ӶϿ���������:id=%d ip=%s,port=%d", rSocket.SID(), rSocket.GetIp().c_str(), (int32)rSocket.GetPort());
}

void NetMsgHandler::recvLoginRequest(zSession* pSession, const NetMsgSS* pMsg, int32 nSize)
{
	const SSRqLogin* packet = static_cast<const SSRqLogin*>(pMsg);
	const zSerivceCfgMgr::Server* serverCfg = NetService::getMe().getServerCfgMgr().getServer(packet->serverID);
	if (!serverCfg)
	{
		ASSERT(0);
		return;
	}

	pSession->setSessionType(serverCfg->getSessType());
	pSession->serivceid = packet->serivceID;
	pSession->serverid = packet->serverID;
	zServerRegMgr& regMgr = NetService::getMe().getServerRegMgr();
	zServerReg* addReg = regMgr.CreateObj();
	if (addReg)
	{
		SSRtLogined sendLg;
		addReg->id = packet->serverID;
		addReg->sessid = pSession->id;
		if (!regMgr.add(addReg))
		{
			Zebra::logger->error("ע�������ID�ظ�");
			ASSERT(0);
			regMgr.DestroyObj(addReg);
			sendLg.result = SSRtLogined::SUCCESS;
			pSession->sendMsg(&sendLg, sendLg.GetPackLength());
			return;
		}
		else
		{
			sendLg.result = SSRtLogined::SUCCESS;
			pSession->sendMsg(&sendLg, sendLg.GetPackLength());
		}

		if (serverCfg->recvsrvlist)
		{
			struct MyStruct : public execEntry<zServerReg>
			{
				virtual bool exec(zServerReg* entry)
				{
					const zSerivceCfgMgr::Server* serverCfg = NetService::getMe().getServerCfgMgr().getServer(entry->id);
					if (!serverCfg)
					{
						ASSERT(0);
						return true;
					}

					if (serverCfg->broadcastlist)
					{
						outServerID.push_back(entry->id);
					}
					return true;
				}
				std::vector<int32> outServerID;
			};

			MyStruct exec;
			regMgr.execEveryServer(exec);

			SSServerRegList sendList;
			std::vector<int32>::const_iterator it = exec.outServerID.begin();
			for (; it != exec.outServerID.end(); ++it)
			{
				sendList.reglist[sendList.count].id = *it;
				sendList.count++;
			}
			pSession->sendMsg(&sendList, sendList.GetPackLength());

		}

		if (serverCfg->broadcastlist)
		{
			struct MyStruct : public execEntry<zServerReg>
			{
				MyStruct(int32 serverID):addServerID(serverID)
				{

				}

				virtual bool exec(zServerReg* entry)
				{
					const zSerivceCfgMgr::Server* serverCfg = NetService::getMe().getServerCfgMgr().getServer(entry->id);
					if (!serverCfg)
					{
						ASSERT(0);
						return true;
					}

					if (serverCfg->recvsrvlist)
					{
						zSession* session = NetService::getMe().getSessionMgr().get(entry->sessid);
						if (session)
						{
							SSServerRegList sendList;
							sendList.reglist[sendList.count].id = addServerID;
							sendList.count++;
							session->sendMsg(&sendList,sendList.GetPackLength());
						}
					}
					return true;
				}

				int32 addServerID;
			};
			MyStruct exec(addReg->id);
			regMgr.execEveryServer(exec);
		}
	}
	else
	{
		ASSERT(0);
	}

}

void NetMsgHandler::recvLoginReponse(zSession* pSession, const NetMsgSS* pMsg, int32 nSize)
{
	const SSRtLogined* packet = static_cast<const SSRtLogined*>(pMsg);
	if (packet->result == SSRtLogined::SUCCESS)
	{
		Zebra::logger->error("��¼�ɹ�");
	}
	else
	{
		Zebra::logger->error("��¼ʧ��");
		ASSERT(0);
	}
}

void NetMsgHandler::recvPingRequest(zSession* pSession, const NetMsgSS* pMsg, int32 nSize)
{

}

void NetMsgHandler::recvSrvListNotifty(zSession* pSession, const NetMsgSS* pMsg, int32 nSize)
{
	SSServerRegList recevice;
	memcpy(&recevice,pMsg,nSize);
	const SSServerRegList* packet = &recevice;

	// to connect
	for (int32 i = 0; i < packet->count; ++i)
	{
		int32 serverID = packet->reglist[i].id;
		const zSerivceCfgMgr::Server* serverCfg = NetService::getMe().getServerCfgMgr().getServer(serverID);
		if (serverCfg == NULL)
		{
			Zebra::logger->error("�����Ҳ����÷���������ID:%d",serverID);
			continue;
		}

		std::map<int32, zSerivceCfgMgr::Serivce>::const_iterator it = serverCfg->serivces.begin();
		for (; it != serverCfg->serivces.end();++it)
		{
			const zSerivceCfgMgr::Serivce& info = it->second;
			if (stricmp(info.name.c_str(), "server") == 0 && stricmp(info.fun.c_str(), "forss") == 0)
			{
				zSession* session = NetService::getMe().getSessionMgr().connect(info.id, info.ip.c_str(), info.port,
					boost::bind(&NetMsgHandler::OnNetMsgEnter, NetMsgHandler::Instance(), _1),
					boost::bind(&NetMsgHandler::OnNetMsg, NetMsgHandler::Instance(), _1, _2, _3),
					boost::bind(&NetMsgHandler::OnNetMsgExit, NetMsgHandler::Instance(), _1)
				);
				if (!session)
				{
					Zebra::logger->error("Connect Server Fail!");
					ASSERT(0);
					continue;
				}
				session->setSessionType(serverCfg->getSessType());
				session->setServerID(serverID);
			}
		}
	}
}

void NetMsgHandler::sendSToPlayer(zSession* pSession, const NetMsgSS* pMsg, int32 nSize)
{
	zSession* player = NetService::getMe().getSessionMgr().get(pMsg->sessid);
	if (!player)
	{
		ASSERT(player);
		Zebra::logger->error("sessid����ȷ���Ҳ������session");
		return;
	}

	static char arrUnBuffer[65536];
	static char arrEnBuffer[65536];
	int nCHeadSize = sizeof(NetMsgC);
	int nSHeadSize = sizeof(NetMsgSS);
	int nCSize = nSize - (nSHeadSize - nCHeadSize);
	memcpy(&arrUnBuffer[0], pMsg, nSize);
	memcpy(&arrEnBuffer[0], &arrUnBuffer[0], nCHeadSize);
	memcpy(&arrEnBuffer[nCHeadSize], &arrUnBuffer[nSHeadSize], nSize - nSHeadSize);
	zEncrypt::xorCode(nCSize, player->encrypt, arrEnBuffer, nCSize);
	player->socket->ParkMsg((NetMsgSS*)&arrEnBuffer[0], nCSize);
	player->socket->SendMsg();
}


