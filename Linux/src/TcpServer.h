/*
 * Copyright: JessMA Open Source (ldcsaa@gmail.com)
 *
 * Author	: Bruce Liang
 * Website	: https://github.com/ldcsaa
 * Project	: https://github.com/ldcsaa/HP-Socket
 * Blog		: http://www.cnblogs.com/ldcsaa
 * Wiki		: http://www.oschina.net/p/hp-socket
 * QQ Group	: 44636872, 75375912
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#include "SocketHelper.h"
#include "./common/GeneralHelper.h"
#include "./common/IODispatcher.h"

class CTcpServer : public ITcpServer, private CIOHandler
{
public:
	virtual BOOL Start	(LPCTSTR lpszBindAddress, USHORT usPort);
	virtual BOOL Stop	();
	virtual BOOL Send	(CONNID dwConnID, const BYTE* pBuffer, int iLength, int iOffset = 0);
	virtual BOOL SendSmallFile	(CONNID dwConnID, LPCTSTR lpszFileName, const LPWSABUF pHead = nullptr, const LPWSABUF pTail = nullptr);
	virtual BOOL SendPackets	(CONNID dwConnID, const WSABUF pBuffers[], int iCount)	{return DoSendPackets(dwConnID, pBuffers, iCount);}
	virtual BOOL PauseReceive	(CONNID dwConnID, BOOL bPause = TRUE);
	virtual BOOL Wait			(DWORD dwMilliseconds = INFINITE) {return m_evWait.WaitFor(dwMilliseconds, WAIT_FOR_STOP_PREDICATE);}
	virtual BOOL			HasStarted					()	{return m_enState == SS_STARTED || m_enState == SS_STARTING;}
	virtual EnServiceState	GetState					()	{return m_enState;}
	virtual BOOL			Disconnect					(CONNID dwConnID, BOOL bForce = TRUE);
	virtual BOOL			DisconnectLongConnections	(DWORD dwPeriod, BOOL bForce = TRUE);
	virtual BOOL			DisconnectSilenceConnections(DWORD dwPeriod, BOOL bForce = TRUE);
	virtual BOOL			GetListenAddress			(TCHAR lpszAddress[], int& iAddressLen, USHORT& usPort);
	virtual BOOL			GetLocalAddress				(CONNID dwConnID, TCHAR lpszAddress[], int& iAddressLen, USHORT& usPort);
	virtual BOOL			GetRemoteAddress			(CONNID dwConnID, TCHAR lpszAddress[], int& iAddressLen, USHORT& usPort);
	
	virtual BOOL IsConnected			(CONNID dwConnID);
	virtual BOOL IsPauseReceive			(CONNID dwConnID, BOOL& bPaused);
	virtual BOOL GetPendingDataLength	(CONNID dwConnID, int& iPending);
	virtual DWORD GetConnectionCount	();
	virtual BOOL GetAllConnectionIDs	(CONNID pIDs[], DWORD& dwCount);
	virtual BOOL GetConnectPeriod		(CONNID dwConnID, DWORD& dwPeriod);
	virtual BOOL GetSilencePeriod		(CONNID dwConnID, DWORD& dwPeriod);
	virtual EnSocketError GetLastError	()	{return m_enLastError;}
	virtual LPCTSTR	GetLastErrorDesc	()	{return ::GetSocketErrorDesc(m_enLastError);}

#ifdef _SSL_SUPPORT
	virtual BOOL SetupSSLContext	(int iVerifyMode = SSL_VM_NONE, LPCTSTR lpszPemCertFile = nullptr, LPCTSTR lpszPemKeyFile = nullptr, LPCTSTR lpszKeyPassword = nullptr, LPCTSTR lpszCAPemCertFileOrPath = nullptr, Fn_SNI_ServerNameCallback fnServerNameCallback = nullptr)	{return FALSE;}
	virtual BOOL SetupSSLContextByMemory(int iVerifyMode = SSL_VM_NONE, LPCSTR lpszPemCert = nullptr, LPCSTR lpszPemKey = nullptr, LPCSTR lpszKeyPassword = nullptr, LPCSTR lpszCAPemCert = nullptr, Fn_SNI_ServerNameCallback fnServerNameCallback = nullptr)						{return FALSE;}
	virtual int AddSSLContext		(int iVerifyMode = SSL_VM_NONE, LPCTSTR lpszPemCertFile = nullptr, LPCTSTR lpszPemKeyFile = nullptr, LPCTSTR lpszKeyPassword = nullptr, LPCTSTR lpszCAPemCertFileOrPath = nullptr)																{return FALSE;}
	virtual int AddSSLContextByMemory(int iVerifyMode = SSL_VM_NONE, LPCSTR lpszPemCert = nullptr, LPCSTR lpszPemKey = nullptr, LPCSTR lpszKeyPassword = nullptr, LPCSTR lpszCAPemCert = nullptr)																					{return FALSE;}
	virtual BOOL BindSSLServerName	(LPCTSTR lpszServerName, int iContextIndex)																																																		{return FALSE;}
	virtual void CleanupSSLContext	()						{}

	virtual BOOL StartSSLHandShake	(CONNID dwConnID)		{return FALSE;}
	virtual void SetSSLAutoHandShake(BOOL bAutoHandShake)	{}
	virtual BOOL IsSSLAutoHandShake	()						{return FALSE;}
	virtual void SetSSLCipherList	(LPCTSTR lpszCipherList){}
	virtual LPCTSTR GetSSLCipherList()						{return nullptr;}
	virtual BOOL GetSSLSessionInfo(CONNID dwConnID, EnSSLSessionInfo enInfo, LPVOID* lppInfo)	{return FALSE;}

protected:
	virtual BOOL StartSSLHandShake	(TSocketObj* pSocketObj){return FALSE;}
#endif

private:
	virtual BOOL OnBeforeProcessIo(const TDispContext* pContext, PVOID pv, UINT events)			override;
	virtual VOID OnAfterProcessIo(const TDispContext* pContext, PVOID pv, UINT events, BOOL rs)	override;
	virtual VOID OnCommand(const TDispContext* pContext, TDispCommand* pCmd)					override;
	virtual BOOL OnReadyRead(const TDispContext* pContext, PVOID pv, UINT events)				override;
	virtual BOOL OnReadyWrite(const TDispContext* pContext, PVOID pv, UINT events)				override;
	virtual BOOL OnHungUp(const TDispContext* pContext, PVOID pv, UINT events)					override;
	virtual BOOL OnError(const TDispContext* pContext, PVOID pv, UINT events)					override;
	virtual VOID OnDispatchThreadStart(THR_ID tid)												override;
	virtual VOID OnDispatchThreadEnd(THR_ID tid)												override;

public:
	virtual BOOL IsSecure					() {return FALSE;}

	virtual BOOL SetConnectionExtra(CONNID dwConnID, PVOID pExtra);
	virtual BOOL GetConnectionExtra(CONNID dwConnID, PVOID* ppExtra);

	virtual void SetReuseAddressPolicy		(EnReuseAddressPolicy enReusePolicy)	{ENSURE_HAS_STOPPED(); ASSERT(m_enReusePolicy == enReusePolicy);}
	virtual void SetSendPolicy				(EnSendPolicy enSendPolicy)				{ENSURE_HAS_STOPPED(); ASSERT(m_enSendPolicy == enSendPolicy);}
	virtual void SetOnSendSyncPolicy		(EnOnSendSyncPolicy enOnSendSyncPolicy)	{ENSURE_HAS_STOPPED(); ASSERT(m_enOnSendSyncPolicy == enOnSendSyncPolicy);}
	virtual void SetMaxConnectionCount		(DWORD dwMaxConnectionCount)	{ENSURE_HAS_STOPPED(); m_dwMaxConnectionCount		= dwMaxConnectionCount;}
	virtual void SetWorkerThreadCount		(DWORD dwWorkerThreadCount)		{ENSURE_HAS_STOPPED(); m_dwWorkerThreadCount		= dwWorkerThreadCount;}
	virtual void SetSocketListenQueue		(DWORD dwSocketListenQueue)		{ENSURE_HAS_STOPPED(); m_dwSocketListenQueue		= dwSocketListenQueue;}
	virtual void SetAcceptSocketCount		(DWORD dwAcceptSocketCount)		{ENSURE_HAS_STOPPED(); m_dwAcceptSocketCount		= dwAcceptSocketCount;}
	virtual void SetSocketBufferSize		(DWORD dwSocketBufferSize)		{ENSURE_HAS_STOPPED(); m_dwSocketBufferSize			= dwSocketBufferSize;}
	virtual void SetFreeSocketObjLockTime	(DWORD dwFreeSocketObjLockTime)	{ENSURE_HAS_STOPPED(); m_dwFreeSocketObjLockTime	= dwFreeSocketObjLockTime;}
	virtual void SetFreeSocketObjPool		(DWORD dwFreeSocketObjPool)		{ENSURE_HAS_STOPPED(); m_dwFreeSocketObjPool		= dwFreeSocketObjPool;}
	virtual void SetFreeBufferObjPool		(DWORD dwFreeBufferObjPool)		{ENSURE_HAS_STOPPED(); m_dwFreeBufferObjPool		= dwFreeBufferObjPool;}
	virtual void SetFreeSocketObjHold		(DWORD dwFreeSocketObjHold)		{ENSURE_HAS_STOPPED(); m_dwFreeSocketObjHold		= dwFreeSocketObjHold;}
	virtual void SetFreeBufferObjHold		(DWORD dwFreeBufferObjHold)		{ENSURE_HAS_STOPPED(); m_dwFreeBufferObjHold		= dwFreeBufferObjHold;}
	virtual void SetKeepAliveTime			(DWORD dwKeepAliveTime)			{ENSURE_HAS_STOPPED(); m_dwKeepAliveTime			= dwKeepAliveTime;}
	virtual void SetKeepAliveInterval		(DWORD dwKeepAliveInterval)		{ENSURE_HAS_STOPPED(); m_dwKeepAliveInterval		= dwKeepAliveInterval;}
	virtual void SetMarkSilence				(BOOL bMarkSilence)				{ENSURE_HAS_STOPPED(); m_bMarkSilence				= bMarkSilence;}
	virtual void SetNoDelay					(BOOL bNoDelay)					{ENSURE_HAS_STOPPED(); m_bNoDelay					= bNoDelay;}

	virtual EnReuseAddressPolicy GetReuseAddressPolicy	()	{return m_enReusePolicy;}
	virtual EnSendPolicy GetSendPolicy					()	{return m_enSendPolicy;}
	virtual EnOnSendSyncPolicy GetOnSendSyncPolicy		()	{return m_enOnSendSyncPolicy;}
	virtual DWORD GetMaxConnectionCount		()	{return m_dwMaxConnectionCount;}
	virtual DWORD GetWorkerThreadCount		()	{return m_dwWorkerThreadCount;}
	virtual DWORD GetSocketListenQueue		()	{return m_dwSocketListenQueue;}
	virtual DWORD GetAcceptSocketCount		()	{return m_dwAcceptSocketCount;}
	virtual DWORD GetSocketBufferSize		()	{return m_dwSocketBufferSize;}
	virtual DWORD GetFreeSocketObjLockTime	()	{return m_dwFreeSocketObjLockTime;}
	virtual DWORD GetFreeSocketObjPool		()	{return m_dwFreeSocketObjPool;}
	virtual DWORD GetFreeBufferObjPool		()	{return m_dwFreeBufferObjPool;}
	virtual DWORD GetFreeSocketObjHold		()	{return m_dwFreeSocketObjHold;}
	virtual DWORD GetFreeBufferObjHold		()	{return m_dwFreeBufferObjHold;}
	virtual DWORD GetKeepAliveTime			()	{return m_dwKeepAliveTime;}
	virtual DWORD GetKeepAliveInterval		()	{return m_dwKeepAliveInterval;}
	virtual BOOL  IsMarkSilence				()	{return m_bMarkSilence;}
	virtual BOOL  IsNoDelay					()	{return m_bNoDelay;}

protected:
	virtual EnHandleResult FirePrepareListen(SOCKET soListen)
		{return DoFirePrepareListen(soListen);}
	virtual EnHandleResult FireAccept(TSocketObj* pSocketObj)
		{
			EnHandleResult rs		= DoFireAccept(pSocketObj);
			if(rs != HR_ERROR) rs	= FireHandShake(pSocketObj);
			return rs;
		}
	virtual EnHandleResult FireHandShake(TSocketObj* pSocketObj)
		{return DoFireHandShake(pSocketObj);}
	virtual EnHandleResult FireReceive(TSocketObj* pSocketObj, const BYTE* pData, int iLength)
		{return DoFireReceive(pSocketObj, pData, iLength);}
	virtual EnHandleResult FireReceive(TSocketObj* pSocketObj, int iLength)
		{return DoFireReceive(pSocketObj, iLength);}
	virtual EnHandleResult FireSend(TSocketObj* pSocketObj, const BYTE* pData, int iLength)
		{return DoFireSend(pSocketObj, pData, iLength);}
	virtual EnHandleResult FireClose(TSocketObj* pSocketObj, EnSocketOperation enOperation, int iErrorCode)
		{return DoFireClose(pSocketObj, enOperation, iErrorCode);}
	virtual EnHandleResult FireShutdown()
		{return DoFireShutdown();}

	virtual EnHandleResult DoFirePrepareListen(SOCKET soListen)
		{return m_pListener->OnPrepareListen(this, soListen);}
	virtual EnHandleResult DoFireAccept(TSocketObj* pSocketObj)
		{return m_pListener->OnAccept(this, pSocketObj->connID, pSocketObj->socket);}
	virtual EnHandleResult DoFireHandShake(TSocketObj* pSocketObj)
		{return m_pListener->OnHandShake(this, pSocketObj->connID);}
	virtual EnHandleResult DoFireReceive(TSocketObj* pSocketObj, const BYTE* pData, int iLength)
		{return m_pListener->OnReceive(this, pSocketObj->connID, pData, iLength);}
	virtual EnHandleResult DoFireReceive(TSocketObj* pSocketObj, int iLength)
		{return m_pListener->OnReceive(this, pSocketObj->connID, iLength);}
	virtual EnHandleResult DoFireSend(TSocketObj* pSocketObj, const BYTE* pData, int iLength)
		{return m_pListener->OnSend(this, pSocketObj->connID, pData, iLength);}
	virtual EnHandleResult DoFireClose(TSocketObj* pSocketObj, EnSocketOperation enOperation, int iErrorCode)
		{return m_pListener->OnClose(this, pSocketObj->connID, enOperation, iErrorCode);}
	virtual EnHandleResult DoFireShutdown()
		{return m_pListener->OnShutdown(this);}

	void SetLastError(EnSocketError code, LPCSTR func, int ec);
	virtual BOOL CheckParams();
	virtual void PrepareStart();
	virtual void Reset();

	virtual BOOL BeforeUnpause(TSocketObj* pSocketObj) {return TRUE;}

	virtual void OnWorkerThreadStart(THR_ID tid)	{}
	virtual void OnWorkerThreadEnd(THR_ID tid)		{}

	virtual void ReleaseGCSocketObj(BOOL bForce = FALSE);

	BOOL DoSendPackets(CONNID dwConnID, const WSABUF pBuffers[], int iCount);
	BOOL DoSendPackets(TSocketObj* pSocketObj, const WSABUF pBuffers[], int iCount);
	TSocketObj* FindSocketObj(CONNID dwConnID);

protected:
	BOOL SetConnectionExtra(TSocketObj* pSocketObj, PVOID pExtra);
	BOOL GetConnectionExtra(TSocketObj* pSocketObj, PVOID* ppExtra);
	BOOL SetConnectionReserved(CONNID dwConnID, PVOID pReserved);
	BOOL GetConnectionReserved(CONNID dwConnID, PVOID* ppReserved);
	BOOL SetConnectionReserved(TSocketObj* pSocketObj, PVOID pReserved);
	BOOL GetConnectionReserved(TSocketObj* pSocketObj, PVOID* ppReserved);
	BOOL SetConnectionReserved2(CONNID dwConnID, PVOID pReserved2);
	BOOL GetConnectionReserved2(CONNID dwConnID, PVOID* ppReserved2);
	BOOL SetConnectionReserved2(TSocketObj* pSocketObj, PVOID pReserved2);
	BOOL GetConnectionReserved2(TSocketObj* pSocketObj, PVOID* ppReserved2);

private:
	BOOL CheckStarting();
	BOOL CheckStoping();
	BOOL CreateListenSocket(LPCTSTR lpszBindAddress, USHORT usPort);
	BOOL CreateWorkerThreads();
	BOOL StartAccept();

	void CloseListenSocket();
	void DisconnectClientSocket();
	void WaitForClientSocketClose();
	void ReleaseClientSocket();
	void ReleaseFreeSocket();
	void WaitForWorkerThreadEnd();

	TSocketObj* GetFreeSocketObj(CONNID dwConnID, SOCKET soClient);
	TSocketObj* CreateSocketObj();
	void AddFreeSocketObj	(TSocketObj* pSocketObj, EnSocketCloseFlag enFlag = SCF_NONE, EnSocketOperation enOperation = SO_UNKNOWN, int iErrorCode = 0);
	void DeleteSocketObj	(TSocketObj* pSocketObj);
	BOOL InvalidSocketObj	(TSocketObj* pSocketObj);
	void AddClientSocketObj	(CONNID dwConnID, TSocketObj* pSocketObj, const HP_SOCKADDR& remoteAddr);
	void CloseClientSocketObj(TSocketObj* pSocketObj, EnSocketCloseFlag enFlag = SCF_NONE, EnSocketOperation enOperation = SO_UNKNOWN, int iErrorCode = 0, int iShutdownFlag = SHUT_WR);

private:
	VOID HandleCmdSend		(const TDispContext* pContext, CONNID dwConnID);
	VOID HandleCmdUnpause	(const TDispContext* pContext, CONNID dwConnID);
	VOID HandleCmdDisconnect(const TDispContext* pContext, CONNID dwConnID, BOOL bForce);
	BOOL HandleAccept		(const TDispContext* pContext, UINT events);
	BOOL HandleReceive		(const TDispContext* pContext, TSocketObj* pSocketObj, int flag);
	BOOL HandleSend			(const TDispContext* pContext, TSocketObj* pSocketObj, int flag);
	BOOL HandleClose		(const TDispContext* pContext, TSocketObj* pSocketObj, EnSocketCloseFlag enFlag, UINT events);

	int SendInternal	(TSocketObj* pSocketObj, const WSABUF pBuffers[], int iCount);
	BOOL SendItem		(TSocketObj* pSocketObj, TItem* pItem, BOOL& bBlocked);

public:
	CTcpServer(ITcpServerListener* pListener)
	: m_pListener				(pListener)
	, m_enLastError				(SE_OK)
	, m_enState					(SS_STOPPED)
	, m_fdGCTimer				(INVALID_FD)
	, m_enReusePolicy			(RAP_ADDR_AND_PORT)
	, m_enSendPolicy			(SP_PACK)
	, m_enOnSendSyncPolicy		(OSSP_RECEIVE)
	, m_dwMaxConnectionCount	(DEFAULT_CONNECTION_COUNT)
	, m_dwWorkerThreadCount		(DEFAULT_WORKER_THREAD_COUNT)
	, m_dwSocketListenQueue		(DEFAULT_TCP_SERVER_SOCKET_LISTEN_QUEUE)
	, m_dwAcceptSocketCount		(DEFAULT_WORKER_MAX_EVENT_COUNT)
	, m_dwSocketBufferSize		(DEFAULT_TCP_SOCKET_BUFFER_SIZE)
	, m_dwFreeSocketObjLockTime	(DEFAULT_FREE_SOCKETOBJ_LOCK_TIME)
	, m_dwFreeSocketObjPool		(DEFAULT_FREE_SOCKETOBJ_POOL)
	, m_dwFreeBufferObjPool		(DEFAULT_FREE_BUFFEROBJ_POOL)
	, m_dwFreeSocketObjHold		(DEFAULT_FREE_SOCKETOBJ_HOLD)
	, m_dwFreeBufferObjHold		(DEFAULT_FREE_BUFFEROBJ_HOLD)
	, m_dwKeepAliveTime			(DEFALUT_TCP_KEEPALIVE_TIME)
	, m_dwKeepAliveInterval		(DEFALUT_TCP_KEEPALIVE_INTERVAL)
	, m_bMarkSilence			(TRUE)
	, m_bNoDelay				(FALSE)
	{
		ASSERT(m_pListener);
	}

	virtual ~CTcpServer()
	{
		ENSURE_STOP();
	}

private:
	EnReuseAddressPolicy m_enReusePolicy;
	EnSendPolicy m_enSendPolicy;
	EnOnSendSyncPolicy m_enOnSendSyncPolicy;
	DWORD m_dwMaxConnectionCount;
	DWORD m_dwWorkerThreadCount;
	DWORD m_dwSocketListenQueue;
	DWORD m_dwAcceptSocketCount;
	DWORD m_dwSocketBufferSize;
	DWORD m_dwFreeSocketObjLockTime;
	DWORD m_dwFreeSocketObjPool;
	DWORD m_dwFreeBufferObjPool;
	DWORD m_dwFreeSocketObjHold;
	DWORD m_dwFreeBufferObjHold;
	DWORD m_dwKeepAliveTime;
	DWORD m_dwKeepAliveInterval;
	BOOL  m_bMarkSilence;
	BOOL  m_bNoDelay;

private:
	CSEM				m_evWait;

	ITcpServerListener*	m_pListener;
	ListenSocketsPtr	m_soListens;
	EnServiceState		m_enState;
	EnSocketError		m_enLastError;

	CReceiveBuffersPtr	m_rcBuffers;

	CPrivateHeap		m_phSocket;
	CBufferObjPool		m_bfObjPool;

	CSpinGuard			m_csState;

	FD					m_fdGCTimer;

	TSocketObjPtrPool	m_bfActiveSockets;

	TSocketObjPtrList	m_lsFreeSocket;
	TSocketObjPtrQueue	m_lsGCSocket;

	CIODispatcher		m_ioDispatcher;
};
