/*!
 * \file TraderCTPOpt.cpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#include "TraderCTPOpt.h"
#include "../Includes/WTSError.hpp"
#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/WTSSessionInfo.hpp"
#include "../Includes/WTSTradeDef.hpp"
#include "../Includes/WTSDataDef.hpp"
#include "../Includes/WTSParams.hpp"
#include "../Share/TimeUtils.hpp"
#include "../Includes/IBaseDataMgr.h"
#include "../Share/DLLHelper.hpp"
#include "../Share/decimal.h"
#include "../Share/StrUtil.hpp"

#include <boost/filesystem.hpp>

void inst_hlp(){}

#ifdef _WIN32
#include <wtypes.h>
HMODULE	g_dllModule = NULL;

BOOL APIENTRY DllMain(
	HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_dllModule = (HMODULE)hModule;
		break;
	}
	return TRUE;
}
#else
#include <dlfcn.h>

char PLATFORM_NAME[] = "UNIX";

std::string	g_moduleName;

const std::string& getInstPath()
{
	static std::string moduleName;
	if (moduleName.empty())
	{
		Dl_info dl_info;
		dladdr((void *)inst_hlp, &dl_info);
		moduleName = dl_info.dli_fname;
	}

	return moduleName;
}
#endif

std::string getBinDir()
{
	static std::string _bin_dir;
	if (_bin_dir.empty())
	{


#ifdef _WIN32
		char strPath[MAX_PATH];
		GetModuleFileName(g_dllModule, strPath, MAX_PATH);

		_bin_dir = StrUtil::standardisePath(strPath, false);
#else
		_bin_dir = getInstPath();
#endif
		boost::filesystem::path p(_bin_dir);
		_bin_dir = p.branch_path().string() + "/";
	}

	return _bin_dir;
}

const char* ENTRUST_SECTION = "entrusts";
const char* ORDER_SECTION = "orders";


uint32_t strToTime(const char* strTime)
{
	std::string str;
	const char *pos = strTime;
	while (strlen(pos) > 0)
	{
		if (pos[0] != ':')
		{
			str.append(pos, 1);
		}
		pos++;
	}

	return strtoul(str.c_str(), NULL, 10);
}

extern "C"
{
	EXPORT_FLAG ITraderApi* createTrader()
	{
		TraderCTPOpt *instance = new TraderCTPOpt();
		return instance;
	}

	EXPORT_FLAG void deleteTrader(ITraderApi* &trader)
	{
		if (NULL != trader)
		{
			delete trader;
			trader = NULL;
		}
	}
}

inline int wrapDirectionType(WTSDirectionType dirType, WTSOffsetType offsetType)
{
	if (WDT_LONG == dirType)
		if (offsetType == WOT_OPEN)
			return THOST_FTDC_D_Buy;
		else
			return THOST_FTDC_D_Sell;
	else
		if (offsetType == WOT_OPEN)
			return THOST_FTDC_D_Sell;
		else
			return THOST_FTDC_D_Buy;
}

inline int wrapPosDirType(WTSDirectionType dirType)
{
	if (WDT_LONG == dirType)
		return THOST_FTDC_PD_Long;
	else if (WDT_SHORT == dirType)
		return THOST_FTDC_PD_Short;
	else
		return THOST_FTDC_PD_Net;
}

inline WTSDirectionType wrapPosDirType(TThostFtdcPosiDirectionType dirType)
{
	if (THOST_FTDC_PD_Long == dirType)
		return WDT_LONG;
	else if (THOST_FTDC_PD_Short == dirType)
		return WDT_SHORT;
	else
		return WDT_NET;
}


inline WTSDirectionType wrapDirectionType(TThostFtdcDirectionType dirType, TThostFtdcOffsetFlagType offsetType)
{
	if (THOST_FTDC_D_Buy == dirType)
		if (offsetType == THOST_FTDC_OF_Open)
			return WDT_LONG;
		else
			return WDT_SHORT;
	else
		if (offsetType == THOST_FTDC_OF_Open)
			return WDT_SHORT;
		else
			return WDT_LONG;
}

inline WTSDirectionType wrapPosDirection(TThostFtdcPosiDirectionType dirType)
{
	if (THOST_FTDC_PD_Long == dirType)
		return WDT_LONG;
	else
		return WDT_SHORT;
}

inline int wrapOffsetType(WTSOffsetType offType)
{
	if (WOT_OPEN == offType)
		return THOST_FTDC_OF_Open;
	else if (WOT_CLOSE == offType)
		return THOST_FTDC_OF_Close;
	else if (WOT_CLOSETODAY == offType)
		return THOST_FTDC_OF_CloseToday;
	else if (WOT_CLOSEYESTERDAY == offType)
		return THOST_FTDC_OF_Close;
	else
		return THOST_FTDC_OF_ForceClose;
}

inline WTSOffsetType wrapOffsetType(TThostFtdcOffsetFlagType offType)
{
	if (THOST_FTDC_OF_Open == offType)
		return WOT_OPEN;
	else if (THOST_FTDC_OF_Close == offType)
		return WOT_CLOSE;
	else if (THOST_FTDC_OF_CloseToday == offType)
		return WOT_CLOSETODAY;
	else
		return WOT_FORCECLOSE;
}

inline int wrapPriceType(WTSPriceType priceType, bool isCFFEX /* = false */)
{
	if (WPT_ANYPRICE == priceType)
		return isCFFEX ? THOST_FTDC_OPT_FiveLevelPrice : THOST_FTDC_OPT_AnyPrice;
	else if (WPT_LIMITPRICE == priceType)
		return THOST_FTDC_OPT_LimitPrice;
	else if (WPT_BESTPRICE == priceType)
		return THOST_FTDC_OPT_BestPrice;
	else
		return THOST_FTDC_OPT_LastPrice;
}

inline WTSPriceType wrapPriceType(TThostFtdcOrderPriceTypeType priceType)
{
	if (THOST_FTDC_OPT_AnyPrice == priceType || THOST_FTDC_OPT_FiveLevelPrice == priceType)
		return WPT_ANYPRICE;
	else if (THOST_FTDC_OPT_LimitPrice == priceType)
		return WPT_LIMITPRICE;
	else if (THOST_FTDC_OPT_BestPrice == priceType)
		return WPT_BESTPRICE;
	else
		return WPT_LASTPRICE;
}

inline int wrapTimeCondition(WTSTimeCondition timeCond)
{
	if (WTC_IOC == timeCond)
		return THOST_FTDC_TC_IOC;
	else if (WTC_GFD == timeCond)
		return THOST_FTDC_TC_GFD;
	else
		return THOST_FTDC_TC_GFS;
}

inline WTSTimeCondition wrapTimeCondition(TThostFtdcTimeConditionType timeCond)
{
	if (THOST_FTDC_TC_IOC == timeCond)
		return WTC_IOC;
	else if (THOST_FTDC_TC_GFD == timeCond)
		return WTC_GFD;
	else
		return WTC_GFS;
}

inline WTSOrderState wrapOrderState(TThostFtdcOrderStatusType orderState)
{
	if (orderState != THOST_FTDC_OST_Unknown)
		return (WTSOrderState)orderState;
	else
		return WOS_Submitting;
}

inline int wrapActionFlag(WTSActionFlag actionFlag)
{
	if (WAF_CANCEL == actionFlag)
		return THOST_FTDC_AF_Delete;
	else
		return THOST_FTDC_AF_Modify;
}

TraderCTPOpt::TraderCTPOpt()
	: m_pUserAPI(NULL)
	, m_mapPosition(NULL)
	, m_ayOrders(NULL)
	, m_ayTrades(NULL)
	, m_ayPosDetail(NULL)
	, m_wrapperState(WS_NOTLOGIN)
	, m_uLastQryTime(0)
	, m_iRequestID(0)
	, m_bQuickStart(false)
	, m_optSink(NULL)
	, m_bscSink(NULL)
	, m_bInQuery(false)
	, m_strandIO(NULL)
	, m_lastQryTime(0)
{
}


TraderCTPOpt::~TraderCTPOpt()
{
}

bool TraderCTPOpt::init(WTSParams* params)
{
	m_strFront = params->get("front")->asCString();
	m_strBroker = params->get("broker")->asCString();
	m_strUser = params->get("user")->asCString();
	m_strPass = params->get("pass")->asCString();

	m_strAppID = params->getCString("appid");
	m_strAuthCode = params->getCString("authcode");


	WTSParams* param = params->get("ctpmodule");
	if (param != NULL)
		m_strModule = getBinDir() + param->asCString();
	else
	{
#ifdef _WIN32
		m_strModule = getBinDir() + "soptthosttraderapi_se.dll";
#else
		m_strModule =  getBinDir() + "soptthosttraderapi_se.so";
#endif
	}

	m_hInstCTP = DLLHelper::load_library(m_strModule.c_str());
#ifdef _WIN32
#	ifdef _WIN64
	const char* creatorName = "?CreateFtdcTraderApi@CThostFtdcTraderApi@ctp_sopt@@SAPEAV12@PEBD@Z";
#	else
	const char* creatorName = "?CreateFtdcTraderApi@CThostFtdcTraderApi@ctp_sopt@@SAPAV12@PBD@Z";
#	endif
#else
	const char* creatorName = "_ZN8ctp_sopt19CThostFtdcTraderApi19CreateFtdcTraderApiEPKc";
#endif
	m_funcCreator = (CTPCreator)DLLHelper::get_symbol(m_hInstCTP, creatorName);

	m_bQuickStart = params->getBoolean("quick");

	return true;
}

void TraderCTPOpt::release()
{
	if (m_pUserAPI)
	{
		m_pUserAPI->RegisterSpi(NULL);
		m_pUserAPI->Release();
		m_pUserAPI = NULL;
	}

	if (m_ayOrders)
		m_ayOrders->clear();

	if (m_ayPosDetail)
		m_ayPosDetail->clear();

	if (m_mapPosition)
		m_mapPosition->clear();

	if (m_ayTrades)
		m_ayTrades->clear();
}

void TraderCTPOpt::connect()
{
	std::stringstream ss;
	ss << "./ctpoptdata/flows/" << m_strBroker << "/" << m_strUser << "/";
	boost::filesystem::create_directories(ss.str().c_str());
	m_pUserAPI = m_funcCreator(ss.str().c_str());
	m_pUserAPI->RegisterSpi(this);
	if (m_bQuickStart)
	{
		m_pUserAPI->SubscribePublicTopic(THOST_TERT_QUICK);			// ע�ṫ����
		m_pUserAPI->SubscribePrivateTopic(THOST_TERT_QUICK);		// ע��˽����
	}
	else
	{
		m_pUserAPI->SubscribePublicTopic(THOST_TERT_RESUME);		// ע�ṫ����
		m_pUserAPI->SubscribePrivateTopic(THOST_TERT_RESUME);		// ע��˽����
	}

	m_pUserAPI->RegisterFront((char*)m_strFront.c_str());

	if (m_pUserAPI)
	{
		m_pUserAPI->Init();
	}

	if (m_thrdWorker == NULL)
	{
		m_strandIO = new boost::asio::io_service::strand(m_asyncIO);
		boost::asio::io_service::work work(m_asyncIO);
		m_thrdWorker.reset(new StdThread([this](){
			while (true)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
				m_asyncIO.run_one();
				//m_asyncIO.run();
			}
		}));
	}
}

void TraderCTPOpt::disconnect()
{
	m_asyncIO.post([this](){
		release();
	});

	if (m_thrdWorker)
	{
		m_asyncIO.stop();
		m_thrdWorker->join();
		m_thrdWorker = NULL;

		delete m_strandIO;
		m_strandIO = NULL;
	}
}

bool TraderCTPOpt::makeEntrustID(char* buffer, int length)
{
	if (buffer == NULL || length == 0)
		return false;

	try
	{
		memset(buffer, 0, length);
		uint32_t orderref = m_orderRef.fetch_add(1) + 1;
		sprintf(buffer, "%06u#%010u#%06u", m_frontID, m_sessionID, orderref);
		return true;
	}
	catch (...)
	{

	}

	return false;
}

void TraderCTPOpt::registerSpi(ITraderSpi *listener)
{
	m_bscSink = listener;
	if (m_bscSink)
	{
		m_bdMgr = listener->getBaseDataMgr();

		m_optSink = listener->getOptSpi();
	}
}

uint32_t TraderCTPOpt::genRequestID()
{
	return m_iRequestID.fetch_add(1) + 1;
}

int TraderCTPOpt::login(const char* user, const char* pass, const char* productInfo)
{
	m_strUser = user;
	m_strPass = pass;

	if (m_pUserAPI == NULL)
	{
		return -1;
	}

	m_wrapperState = WS_LOGINING;
	authenticate();

	return 0;
}

int TraderCTPOpt::doLogin()
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_strBroker.c_str());
	strcpy(req.UserID, m_strUser.c_str());
	strcpy(req.Password, m_strPass.c_str());
	strcpy(req.UserProductInfo, m_strProdInfo.c_str());
	int iResult = m_pUserAPI->ReqUserLogin(&req, genRequestID());
	if (iResult != 0)
	{
		m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt]��¼������ʧ��, ������:%d", iResult);
	}

	return 0;
}

int TraderCTPOpt::logout()
{
	if (m_pUserAPI == NULL)
	{
		return -1;
	}

	CThostFtdcUserLogoutField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_strBroker.c_str());
	strcpy(req.UserID, m_strUser.c_str());
	int iResult = m_pUserAPI->ReqUserLogout(&req, genRequestID());
	if (iResult != 0)
	{
		m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt]ע��������ʧ��, ������:%d", iResult);
	}

	return 0;
}

int TraderCTPOpt::orderInsertOpt(WTSEntrust* entrust)
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	if (entrust == NULL)
		return -1;

	if(entrust->getBusinessType() != BT_EXECUTE)
	{
		if(m_bscSink) m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt]��Ȩֻ֧����Ȩҵ������");
		return -1;
	}

	CThostFtdcInputExecOrderField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, m_strBroker.c_str());
	///Ͷ���ߴ���
	strcpy(req.InvestorID, m_strUser.c_str());
	///��Լ����
	strcpy(req.InstrumentID, entrust->getCode());

	strcpy(req.ExchangeID, entrust->getExchg());

	req.ActionType = THOST_FTDC_ACTP_Exec;

	if (strlen(entrust->getUserTag()) == 0)
	{
		///��������
		sprintf(req.ExecOrderRef, "%u", m_orderRef.fetch_add(0));
	}
	else
	{
		uint32_t fid, sid, orderref;
		extractEntrustID(entrust->getEntrustID(), fid, sid, orderref);
		sprintf(req.ExecOrderRef, "%d", orderref);
	}

	if (strlen(entrust->getUserTag()) > 0)
	{
		m_iniHelper.writeString(ENTRUST_SECTION, entrust->getEntrustID(), entrust->getUserTag());
		m_iniHelper.save();
	}

	req.PosiDirection = wrapPosDirType(entrust->getDirection());
	req.OffsetFlag = wrapOffsetType(entrust->getOffsetType());
	req.HedgeFlag = THOST_FTDC_HF_Speculation;
	req.Volume = (int)entrust->getVolume();
	req.CloseFlag = THOST_FTDC_EOCF_AutoClose;

	int iResult = m_pUserAPI->ReqExecOrderInsert(&req, genRequestID());
	if (iResult != 0)
	{
		m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt]������Ȩ����ʧ��, ������:%d", iResult);
	}

	return 0;
}

int TraderCTPOpt::orderActionOpt(WTSEntrustAction* action)
{
	if (m_wrapperState != WS_ALLREADY)
		return -1;

	if (action->getBusinessType() != BT_EXECUTE)
	{
		if (m_bscSink) m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt]��Ȩֻ֧����Ȩҵ������");
		return -1;
	}

	uint32_t frontid, sessionid, orderref;
	if (!extractEntrustID(action->getEntrustID(), frontid, sessionid, orderref))
		return -1;

	CThostFtdcInputExecOrderActionField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, m_strBroker.c_str());
	///Ͷ���ߴ���
	strcpy(req.InvestorID, m_strUser.c_str());
	///��������
	sprintf(req.ExecOrderRef, "%u", orderref);
	///������
	///ǰ�ñ��
	req.FrontID = frontid;
	///�Ự���
	req.SessionID = sessionid;
	///������־
	req.ActionFlag = wrapActionFlag(action->getActionFlag());
	///��Լ����
	strcpy(req.InstrumentID, action->getCode());

	strcpy(req.ExecOrderSysID, action->getOrderID());
	strcpy(req.ExchangeID, action->getExchg());

	int iResult = m_pUserAPI->ReqExecOrderAction(&req, genRequestID());
	if (iResult != 0)
	{
		m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt]��Ȩ����������ʧ��, ������:%d", iResult);
	}

	return 0;
}

int TraderCTPOpt::queryOrdersOpt(WTSBusinessType bType)
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	if (bType != BT_EXECUTE)
	{
		if (m_bscSink) m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt]��Ȩֻ֧����Ȩҵ������");
		return -1;
	}

	StdUniqueLock lock(m_mtxQuery);
	m_queQuery.push([this]() {
		CThostFtdcQryExecOrderField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_strBroker.c_str());
		strcpy(req.InvestorID, m_strUser.c_str());

		m_pUserAPI->ReqQryExecOrder(&req, genRequestID());
	});

	triggerQuery();

	return 0;
}

int TraderCTPOpt::orderInsert(WTSEntrust* entrust)
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, m_strBroker.c_str());
	///Ͷ���ߴ���
	strcpy(req.InvestorID, m_strUser.c_str());
	///��Լ����
	strcpy(req.InstrumentID, entrust->getCode());

	strcpy(req.ExchangeID, entrust->getExchg());

	if (strlen(entrust->getUserTag()) == 0)
	{
		///��������
		sprintf(req.OrderRef, "%u", m_orderRef.fetch_add(0));

		//���ɱ���ί�е���
		//entrust->setEntrustID(generateEntrustID(m_frontID, m_sessionID, m_orderRef++).c_str());	
	}
	else
	{
		uint32_t fid, sid, orderref;
		extractEntrustID(entrust->getEntrustID(), fid, sid, orderref);
		//entrust->setEntrustID(entrust->getUserTag());
		///��������
		sprintf(req.OrderRef, "%d", orderref);
	}

	if (strlen(entrust->getUserTag()) > 0)
	{
		//m_mapEntrustTag[entrust->getEntrustID()] = entrust->getUserTag();
		m_iniHelper.writeString(ENTRUST_SECTION, entrust->getEntrustID(), entrust->getUserTag());
		m_iniHelper.save();
	}

	WTSContractInfo* ct = m_bdMgr->getContract(entrust->getCode(), entrust->getExchg());
	WTSCommodityInfo* commInfo = m_bdMgr->getCommodity(ct);

	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///�����۸�����: �޼�
	req.OrderPriceType = wrapPriceType(entrust->getPriceType(), strcmp(commInfo->getExchg(), "CFFEX") == 0);
	///��������: 
	req.Direction = wrapDirectionType(entrust->getDirection(), entrust->getOffsetType());
	///��Ͽ�ƽ��־: ����
	req.CombOffsetFlag[0] = wrapOffsetType(entrust->getOffsetType());
	///���Ͷ���ױ���־
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///�۸�
	req.LimitPrice = entrust->getPrice();
	///����: 1
	req.VolumeTotalOriginal = (int)entrust->getVolume();
	///��Ч������: ������Ч
	req.TimeCondition = wrapTimeCondition(entrust->getTimeCondition());
	///GTD����
	//	TThostFtdcDateType	GTDDate;
	///�ɽ�������: �κ�����
	req.VolumeCondition = THOST_FTDC_VC_AV;
	///��С�ɽ���: 1
	req.MinVolume = 1;
	///��������: ����
	req.ContingentCondition = THOST_FTDC_CC_Immediately;
	///ֹ���
	//	TThostFtdcPriceType	StopPrice;
	///ǿƽԭ��: ��ǿƽ
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	req.IsAutoSuspend = 0;
	///ҵ��Ԫ
	//	TThostFtdcBusinessUnitType	BusinessUnit;
	///������
	//	TThostFtdcRequestIDType	RequestID;
	///�û�ǿ����־: ��
	req.UserForceClose = 0;

	int iResult = m_pUserAPI->ReqOrderInsert(&req, genRequestID());
	if (iResult != 0)
	{
		m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt]���붩��ʧ��, ������:%d", iResult);
	}

	return 0;
}

int TraderCTPOpt::orderAction(WTSEntrustAction* action)
{
	if (m_wrapperState != WS_ALLREADY)
		return -1;

	uint32_t frontid, sessionid, orderref;
	if (!extractEntrustID(action->getEntrustID(), frontid, sessionid, orderref))
		return -1;

	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, m_strBroker.c_str());
	///Ͷ���ߴ���
	strcpy(req.InvestorID, m_strUser.c_str());
	///��������
	sprintf(req.OrderRef, "%u", orderref);
	///������
	///ǰ�ñ��
	req.FrontID = frontid;
	///�Ự���
	req.SessionID = sessionid;
	///������־
	req.ActionFlag = wrapActionFlag(action->getActionFlag());
	///��Լ����
	strcpy(req.InstrumentID, action->getCode());

	req.LimitPrice = action->getPrice();

	req.VolumeChange = (int32_t)action->getVolume();

	strcpy(req.OrderSysID, action->getOrderID());
	strcpy(req.ExchangeID, action->getExchg());

	int iResult = m_pUserAPI->ReqOrderAction(&req, genRequestID());
	if (iResult != 0)
	{
		m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt]����������ʧ��, ������:%d", iResult);
	}

	return 0;
}

int TraderCTPOpt::queryAccount()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	StdUniqueLock lock(m_mtxQuery);
	m_queQuery.push([this](){
		CThostFtdcQryTradingAccountField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_strBroker.c_str());
		strcpy(req.InvestorID, m_strUser.c_str());
		m_pUserAPI->ReqQryTradingAccount(&req, genRequestID());
	});

	triggerQuery();

	return 0;
}

int TraderCTPOpt::queryPositions()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	StdUniqueLock lock(m_mtxQuery);
	m_queQuery.push([this](){
		CThostFtdcQryInvestorPositionField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_strBroker.c_str());
		strcpy(req.InvestorID, m_strUser.c_str());
		m_pUserAPI->ReqQryInvestorPosition(&req, genRequestID());
	});

	triggerQuery();

	return 0;
}

int TraderCTPOpt::queryOrders()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	StdUniqueLock lock(m_mtxQuery);
	m_queQuery.push([this](){
		CThostFtdcQryOrderField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_strBroker.c_str());
		strcpy(req.InvestorID, m_strUser.c_str());

		m_pUserAPI->ReqQryOrder(&req, genRequestID());
	});

	triggerQuery();

	return 0;
}

int TraderCTPOpt::queryTrades()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	StdUniqueLock lock(m_mtxQuery);
	m_queQuery.push([this](){
		CThostFtdcQryTradeField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_strBroker.c_str());
		strcpy(req.InvestorID, m_strUser.c_str());

		m_pUserAPI->ReqQryTrade(&req, genRequestID());
	});

	triggerQuery();

	return 0;
}

void TraderCTPOpt::OnFrontConnected()
{
	if (m_bscSink)
		m_bscSink->handleEvent(WTE_Connect, 0);
}

void TraderCTPOpt::OnFrontDisconnected(int nReason)
{
	m_wrapperState = WS_NOTLOGIN;
	if (m_bscSink)
		m_bscSink->handleEvent(WTE_Close, nReason);
}

void TraderCTPOpt::OnHeartBeatWarning(int nTimeLapse)
{

}

void TraderCTPOpt::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo))
	{
		doLogin();
	}
	else
	{
		m_bscSink->handleTraderLog(LL_INFO, "[TraderCTPOpt][%s-%s]�ն���֤ʧ��,������Ϣ:%s", m_strBroker.c_str(), m_strUser.c_str(), pRspInfo->ErrorMsg);
		m_wrapperState = WS_LOGINFAILED;

		if (m_bscSink)
			m_bscSink->onLoginResult(false, pRspInfo->ErrorMsg, 0);
	}

}

void TraderCTPOpt::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo))
	{
		m_wrapperState = WS_LOGINED;

		// ����Ự����
		m_frontID = pRspUserLogin->FrontID;
		m_sessionID = pRspUserLogin->SessionID;
		m_orderRef = atoi(pRspUserLogin->MaxOrderRef);
		///��ȡ��ǰ������
		m_lDate = atoi(m_pUserAPI->GetTradingDay());

		m_bscSink->handleTraderLog(LL_INFO, "[TraderCTPOpt][%s-%s]�˻���¼�ɹ���AppID:%s, Sessionid: %u, ��¼ʱ��: %s����",
			m_strBroker.c_str(), m_strUser.c_str(), m_strAppID.c_str(), m_sessionID, pRspUserLogin->LoginTime);

		std::stringstream ss;
		ss << "./ctpdata/local/" << m_strBroker << "/";
		std::string path = StrUtil::standardisePath(ss.str());
		if (!StdFile::exists(path.c_str()))
			boost::filesystem::create_directories(path.c_str());
		ss << m_strUser << ".dat";

		m_iniHelper.load(ss.str().c_str());
		uint32_t lastDate = m_iniHelper.readUInt("marker", "date", 0);
		if(lastDate != m_lDate)
		{
			//�����ղ�ͬ��������ԭ��������
			m_iniHelper.removeSection(ENTRUST_SECTION);
			m_iniHelper.removeSection(ORDER_SECTION);
			m_iniHelper.writeUInt("marker", "date", m_lDate);
			m_iniHelper.save();

			m_bscSink->handleTraderLog(LL_INFO, "[TraderCTPOpt][%s-%s]���������л�[%u -> %u]����ձ������ݻ��桭��", m_strBroker.c_str(), m_strUser.c_str(), lastDate, m_lDate);
		}

		m_bscSink->handleTraderLog(LL_INFO, "[TraderCTPOpt][%s-%s]�˻���¼�ɹ��������գ�%u����", m_strBroker.c_str(), m_strUser.c_str(), m_lDate);

		m_bscSink->handleTraderLog(LL_INFO, "[TraderCTPOpt][%s-%s]���ڲ�ѯ����ȷ����Ϣ����", m_strBroker.c_str(), m_strUser.c_str());
		queryConfirm();
	}
	else
	{
		m_bscSink->handleTraderLog(LL_INFO, "[TraderCTPOpt][%s-%s]�˻���¼ʧ��,������Ϣ:%s", m_strBroker.c_str(), m_strUser.c_str(), pRspInfo->ErrorMsg);
		m_wrapperState = WS_LOGINFAILED;

		if (m_bscSink)
			m_bscSink->onLoginResult(false, pRspInfo->ErrorMsg, 0);
	}
}

void TraderCTPOpt::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	m_wrapperState = WS_NOTLOGIN;
	if (m_bscSink)
		m_bscSink->handleEvent(WTE_Logout, 0);
}

void TraderCTPOpt::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		triggerQuery();
	}

	if (!IsErrorRspInfo(pRspInfo))
	{
		if (pSettlementInfoConfirm != NULL)
		{
			uint32_t uConfirmDate = strtoul(pSettlementInfoConfirm->ConfirmDate, NULL, 10);
			if (uConfirmDate >= m_lDate)
			{
				m_wrapperState = WS_CONFIRMED;

				m_bscSink->handleTraderLog(LL_INFO, "[TraderCTPOpt][%s-%s]�˻����ݳ�ʼ����ɡ���", m_strBroker.c_str(), m_strUser.c_str());
				m_wrapperState = WS_ALLREADY;
				if (m_bscSink)
					m_bscSink->onLoginResult(true, "", m_lDate);
			}
			else
			{
				m_wrapperState = WS_CONFIRM_QRYED;

				m_bscSink->handleTraderLog(LL_INFO, "[TraderCTPOpt][%s-%s]����ȷ�Ͻ���������", m_strBroker.c_str(), m_strUser.c_str());
				confirm();
			}
		}
		else
		{
			m_wrapperState = WS_CONFIRM_QRYED;
			confirm();
		}
	}

}

void TraderCTPOpt::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pSettlementInfoConfirm != NULL)
	{
		if (m_wrapperState == WS_CONFIRM_QRYED)
		{
			m_wrapperState = WS_CONFIRMED;

			m_bscSink->handleTraderLog(LL_INFO, "[TraderCTPOpt][%s-%s]�˻����ݳ�ʼ����ɡ���", m_strBroker.c_str(), m_strUser.c_str());
			m_wrapperState = WS_ALLREADY;
			if (m_bscSink)
				m_bscSink->onLoginResult(true, "", m_lDate);
		}
	}
}

void TraderCTPOpt::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	WTSEntrust* entrust = makeEntrust(pInputOrder);
	if (entrust)
	{
		WTSError *err = makeError(pRspInfo);
		//g_orderMgr.onRspEntrust(entrust, err);
		if (m_bscSink)
			m_bscSink->onRspEntrust(entrust, err);
		entrust->release();
		err->release();
	}
}

void TraderCTPOpt::OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	WTSEntrust* entrust = makeEntrust(pInputExecOrder);
	if (entrust)
	{
		WTSError *err = makeError(pRspInfo);
		//g_orderMgr.onRspEntrust(entrust, err);
		if (m_optSink)
			m_optSink->onRspEntrustOpt(entrust, err);
		entrust->release();
		err->release();
	}
}

void TraderCTPOpt::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (IsErrorRspInfo(pRspInfo))
	{

	}
	else
	{
		WTSError* error = WTSError::create(WEC_EXECCANCEL, pRspInfo->ErrorMsg);
		if (m_bscSink)
			m_bscSink->onTraderError(error);
	}
}

void TraderCTPOpt::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder)
{
	WTSOrderInfo *orderInfo = makeOrderInfo(pExecOrder);
	if (orderInfo)
	{
		if (m_optSink)
			m_optSink->onPushOrderOpt(orderInfo);

		orderInfo->release();
	}
}

void TraderCTPOpt::OnErrRtnExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo)
{
	WTSEntrust* entrust = makeEntrust(pInputExecOrder);
	if (entrust)
	{
		WTSError *err = makeError(pRspInfo);
		//g_orderMgr.onRspEntrust(entrust, err);
		if (m_optSink)
			m_optSink->onRspEntrustOpt(entrust, err);
		entrust->release();
		err->release();
	}
}

void TraderCTPOpt::OnRspQryExecOrder(CThostFtdcExecOrderField *pExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		triggerQuery();
	}

	if (!IsErrorRspInfo(pRspInfo) && pExecOrder)
	{
		if (NULL == m_ayExecOrds)
			m_ayExecOrds = WTSArray::create();

		WTSOrderInfo* orderInfo = makeOrderInfo(pExecOrder);
		if (orderInfo)
		{
			m_ayExecOrds->append(orderInfo, false);
		}
	}

	if (bIsLast)
	{
		if (m_optSink)
			m_optSink->onRspOrdersOpt(m_ayOrders);

		if (m_ayExecOrds)
			m_ayExecOrds->clear();
	}
}

void TraderCTPOpt::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (IsErrorRspInfo(pRspInfo))
	{

	}
	else
	{
		WTSError* error = WTSError::create(WEC_ORDERCANCEL, pRspInfo->ErrorMsg);
		if (m_bscSink)
			m_bscSink->onTraderError(error);
	}
}

void TraderCTPOpt::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		triggerQuery();
	}

	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		WTSAccountInfo* accountInfo = WTSAccountInfo::create();
		accountInfo->setDescription(StrUtil::printf("%s-%s", m_strBroker.c_str(), m_strUser.c_str()).c_str());
		//accountInfo->setUsername(m_strUserName.c_str());
		accountInfo->setPreBalance(pTradingAccount->PreBalance);
		accountInfo->setCloseProfit(pTradingAccount->CloseProfit);
		accountInfo->setDynProfit(pTradingAccount->PositionProfit);
		accountInfo->setMargin(pTradingAccount->CurrMargin);
		accountInfo->setAvailable(pTradingAccount->Available);
		accountInfo->setCommission(pTradingAccount->Commission);
		accountInfo->setFrozenMargin(pTradingAccount->FrozenMargin);
		accountInfo->setFrozenCommission(pTradingAccount->FrozenCommission);
		accountInfo->setDeposit(pTradingAccount->Deposit);
		accountInfo->setWithdraw(pTradingAccount->Withdraw);
		accountInfo->setBalance(accountInfo->getPreBalance() + accountInfo->getCloseProfit() - accountInfo->getCommission() + accountInfo->getDeposit() - accountInfo->getWithdraw());
		accountInfo->setCurrency("CNY");

		WTSArray * ay = WTSArray::create();
		ay->append(accountInfo, false);
		if (m_bscSink)
			m_bscSink->onRspAccount(ay);

		ay->release();
	}
}

void TraderCTPOpt::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		triggerQuery();
	}

	if (!IsErrorRspInfo(pRspInfo) && pInvestorPosition)
	{
		if (NULL == m_mapPosition)
			m_mapPosition = PositionMap::create();

		WTSContractInfo* contract = m_bdMgr->getContract(pInvestorPosition->InstrumentID, pInvestorPosition->ExchangeID);
		WTSCommodityInfo* commInfo = m_bdMgr->getCommodity(contract);
		if (contract)
		{
			std::string key = StrUtil::printf("%s-%d", pInvestorPosition->InstrumentID, pInvestorPosition->PosiDirection);
			WTSPositionItem* pos = (WTSPositionItem*)m_mapPosition->get(key);
			if(pos == NULL)
			{
				pos = WTSPositionItem::create(pInvestorPosition->InstrumentID, commInfo->getCurrency(), commInfo->getExchg());
				m_mapPosition->add(key, pos, false);
			}
			pos->setDirection(wrapPosDirection(pInvestorPosition->PosiDirection));
			if(commInfo->getCoverMode() == CM_CoverToday)
			{
				if (pInvestorPosition->PositionDate == THOST_FTDC_PSD_Today)
					pos->setNewPosition(pInvestorPosition->Position);
				else
					pos->setPrePosition(pInvestorPosition->Position);
			}
			else
			{
				pos->setNewPosition(pInvestorPosition->TodayPosition);
				pos->setPrePosition(pInvestorPosition->Position - pInvestorPosition->TodayPosition);
			}

			pos->setMargin(pos->getMargin() + pInvestorPosition->UseMargin);
			pos->setDynProfit(pos->getDynProfit() + pInvestorPosition->PositionProfit);
			pos->setPositionCost(pos->getPositionCost() + pInvestorPosition->PositionCost);

			if (pos->getTotalPosition() != 0)
			{
				pos->setAvgPrice(pos->getPositionCost() / pos->getTotalPosition() / commInfo->getVolScale());
			}
			else
			{
				pos->setAvgPrice(0);
			}

			if (commInfo->getCategoty() != CC_Combination)
			{
				if (commInfo->getCoverMode() == CM_CoverToday)
				{
					if (pInvestorPosition->PositionDate == THOST_FTDC_PSD_Today)
					{
						int availNew = pInvestorPosition->Position;
						if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
						{
							availNew -= pInvestorPosition->LongFrozen;
						}
						else
						{
							availNew -= pInvestorPosition->ShortFrozen;
						}
						if (availNew < 0)
							availNew = 0;
						pos->setAvailNewPos(availNew);
					}
					else
					{
						int availPre = pInvestorPosition->Position;
						if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
						{
							availPre -= pInvestorPosition->LongFrozen;
						}
						else
						{
							availPre -= pInvestorPosition->ShortFrozen;
						}
						if (availPre < 0)
							availPre = 0;
						pos->setAvailPrePos(availPre);
					}
				}
				else
				{
					int availNew = pInvestorPosition->TodayPosition;
					if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
					{
						availNew -= pInvestorPosition->LongFrozen;
					}
					else
					{
						availNew -= pInvestorPosition->ShortFrozen;
					}
					if (availNew < 0)
						availNew = 0;
					pos->setAvailNewPos(availNew);

					double availPre = pos->getNewPosition() + pos->getPrePosition()
						- pInvestorPosition->LongFrozen - pInvestorPosition->ShortFrozen
						- pos->getAvailNewPos();
					pos->setAvailPrePos(availPre);
				}
			}
			else
			{

			}

			if (decimal::lt(pos->getTotalPosition(), 0.0) && decimal::eq(pos->getMargin(), 0.0))
			{
				//�в�λ�����Ǳ�֤��Ϊ0����˵����������Լ��������Լ�Ŀ��óֲ�ȫ����Ϊ0
				pos->setAvailNewPos(0);
				pos->setAvailPrePos(0);
			}
		}
	}

	if (bIsLast)
	{

		WTSArray* ayPos = WTSArray::create();

		if(m_mapPosition && m_mapPosition->size() > 0)
		{
			for (auto it = m_mapPosition->begin(); it != m_mapPosition->end(); it++)
			{
				ayPos->append(it->second, true);
			}
		}

		if (m_bscSink)
			m_bscSink->onRspPosition(ayPos);

		if (m_mapPosition)
		{
			m_mapPosition->release();
			m_mapPosition = NULL;
		}

		ayPos->release();
	}
}

void TraderCTPOpt::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		triggerQuery();
	}

	if (!IsErrorRspInfo(pRspInfo) && pTrade)
	{
		if (NULL == m_ayTrades)
			m_ayTrades = WTSArray::create();

		WTSTradeInfo* trade = makeTradeRecord(pTrade);
		if (trade)
		{
			m_ayTrades->append(trade, false);
		}
	}

	if (bIsLast)
	{
		if (m_bscSink)
			m_bscSink->onRspTrades(m_ayTrades);

		if (NULL != m_ayTrades)
			m_ayTrades->clear();
	}
}

void TraderCTPOpt::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		triggerQuery();
	}

	if (!IsErrorRspInfo(pRspInfo) && pOrder)
	{
		if (NULL == m_ayOrders)
			m_ayOrders = WTSArray::create();

		WTSOrderInfo* orderInfo = makeOrderInfo(pOrder);
		if (orderInfo)
		{
			m_ayOrders->append(orderInfo, false);
		}
	}

	if (bIsLast)
	{
		if (m_bscSink)
			m_bscSink->onRspOrders(m_ayOrders);

		if (m_ayOrders)
			m_ayOrders->clear();
	}
}

void TraderCTPOpt::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int x = 0;
}

void TraderCTPOpt::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	WTSOrderInfo *orderInfo = makeOrderInfo(pOrder);
	if (orderInfo)
	{
		if (m_bscSink)
			m_bscSink->onPushOrder(orderInfo);

		orderInfo->release();
	}

	//ReqQryTradingAccount();
}

void TraderCTPOpt::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	WTSTradeInfo *tRecord = makeTradeRecord(pTrade);
	if (tRecord)
	{
		if (m_bscSink)
			m_bscSink->onPushTrade(tRecord);

		tRecord->release();
	}
}


WTSOrderInfo* TraderCTPOpt::makeOrderInfo(CThostFtdcOrderField* orderField)
{
	WTSContractInfo* contract = m_bdMgr->getContract(orderField->InstrumentID, orderField->ExchangeID);
	if (contract == NULL)
		return NULL;

	WTSOrderInfo* pRet = WTSOrderInfo::create();
	pRet->setPrice(orderField->LimitPrice);
	pRet->setVolume(orderField->VolumeTotalOriginal);
	pRet->setDirection(wrapDirectionType(orderField->Direction, orderField->CombOffsetFlag[0]));
	pRet->setPriceType(wrapPriceType(orderField->OrderPriceType));
	pRet->setTimeCondition(wrapTimeCondition(orderField->TimeCondition));
	pRet->setOffsetType(wrapOffsetType(orderField->CombOffsetFlag[0]));

	pRet->setVolTraded(orderField->VolumeTraded);
	pRet->setVolLeft(orderField->VolumeTotal);

	pRet->setCode(orderField->InstrumentID);
	pRet->setExchange(contract->getExchg());

	pRet->setOrderDate(strtoul(orderField->InsertDate, NULL, 10));
	std::string strTime = orderField->InsertTime;
	StrUtil::replace(strTime, ":", "");
	uint32_t uTime = strtoul(strTime.c_str(), NULL, 10);
	pRet->setOrderTime(TimeUtils::makeTime(pRet->getOrderDate(), strtoul(strTime.c_str(), NULL, 10) * 1000));

	pRet->setOrderState(wrapOrderState(orderField->OrderStatus));
	if (orderField->OrderSubmitStatus >= THOST_FTDC_OSS_InsertRejected)
		pRet->setError(true);		

	pRet->setEntrustID(generateEntrustID(orderField->FrontID, orderField->SessionID, atoi(orderField->OrderRef)).c_str());
	pRet->setOrderID(orderField->OrderSysID);

	pRet->setStateMsg(orderField->StatusMsg);


	std::string usertag = m_iniHelper.readString(ENTRUST_SECTION, pRet->getEntrustID(), "");
	if(usertag.empty())
	{
		pRet->setUserTag(pRet->getEntrustID());
	}
	else
	{
		pRet->setUserTag(usertag.c_str());

		if (strlen(pRet->getOrderID()) > 0)
		{
			m_iniHelper.writeString(ORDER_SECTION, StrUtil::trim(pRet->getOrderID()).c_str(), usertag.c_str());
			m_iniHelper.save();
		}
	}

	return pRet;
}

WTSEntrust* TraderCTPOpt::makeEntrust(CThostFtdcInputOrderField *entrustField)
{
	WTSContractInfo* ct = m_bdMgr->getContract(entrustField->InstrumentID, entrustField->ExchangeID);
	if (ct == NULL)
		return NULL;

	WTSEntrust* pRet = WTSEntrust::create(
		entrustField->InstrumentID,
		entrustField->VolumeTotalOriginal,
		entrustField->LimitPrice,
		ct->getExchg());

	pRet->setDirection(wrapDirectionType(entrustField->Direction, entrustField->CombOffsetFlag[0]));
	pRet->setPriceType(wrapPriceType(entrustField->OrderPriceType));
	pRet->setOffsetType(wrapOffsetType(entrustField->CombOffsetFlag[0]));
	pRet->setTimeCondition(wrapTimeCondition(entrustField->TimeCondition));

	pRet->setEntrustID(generateEntrustID(m_frontID, m_sessionID, atoi(entrustField->OrderRef)).c_str());

	//StringMap::iterator it = m_mapEntrustTag.find(pRet->getEntrustID());
	//if (it != m_mapEntrustTag.end())
	//{
	//	pRet->setUserTag(it->second.c_str());
	//}
	std::string usertag = m_iniHelper.readString(ENTRUST_SECTION, pRet->getEntrustID());
	if (!usertag.empty())
		pRet->setUserTag(usertag.c_str());

	return pRet;
}

WTSOrderInfo* TraderCTPOpt::makeOrderInfo(CThostFtdcExecOrderField* orderField)
{
	WTSContractInfo* contract = m_bdMgr->getContract(orderField->InstrumentID, orderField->ExchangeID);
	if (contract == NULL)
		return NULL;

	WTSOrderInfo* pRet = WTSOrderInfo::create();
	pRet->setPrice(0);
	pRet->setBusinessType(BT_EXECUTE);
	pRet->setVolume(orderField->Volume);
	pRet->setDirection(wrapPosDirType(orderField->PosiDirection));
	pRet->setOffsetType(wrapOffsetType(orderField->OffsetFlag));

	pRet->setCode(orderField->InstrumentID);
	pRet->setExchange(contract->getExchg());

	pRet->setOrderDate(strtoul(orderField->InsertDate, NULL, 10));
	std::string strTime = orderField->InsertTime;
	StrUtil::replace(strTime, ":", "");
	uint32_t uTime = strtoul(strTime.c_str(), NULL, 10);
	pRet->setOrderTime(TimeUtils::makeTime(pRet->getOrderDate(), strtoul(strTime.c_str(), NULL, 10) * 1000));

	pRet->setOrderState(WOS_Nottouched);
	if (orderField->OrderSubmitStatus >= THOST_FTDC_OSS_InsertRejected)
	{
		pRet->setError(true);
		pRet->setOrderState(WOS_Canceled);
	}

	pRet->setEntrustID(generateEntrustID(orderField->FrontID, orderField->SessionID, atoi(orderField->ExecOrderRef)).c_str());
	pRet->setOrderID(orderField->ExecOrderSysID);

	pRet->setStateMsg(orderField->StatusMsg);


	std::string usertag = m_iniHelper.readString(ENTRUST_SECTION, pRet->getEntrustID(), "");
	if (usertag.empty())
	{
		pRet->setUserTag(pRet->getEntrustID());
	}
	else
	{
		pRet->setUserTag(usertag.c_str());

		if (strlen(pRet->getOrderID()) > 0)
		{
			m_iniHelper.writeString(ORDER_SECTION, StrUtil::trim(pRet->getOrderID()).c_str(), usertag.c_str());
			m_iniHelper.save();
		}
	}

	return pRet;
}

WTSEntrust* TraderCTPOpt::makeEntrust(CThostFtdcInputExecOrderField *entrustField)
{
	WTSContractInfo* ct = m_bdMgr->getContract(entrustField->InstrumentID, entrustField->ExchangeID);
	if (ct == NULL)
		return NULL;

	WTSEntrust* pRet = WTSEntrust::create(entrustField->InstrumentID, entrustField->Volume, 0, ct->getExchg(), BT_EXECUTE);

	pRet->setDirection(wrapPosDirType(entrustField->PosiDirection));
	pRet->setOffsetType(wrapOffsetType(entrustField->OffsetFlag));

	pRet->setEntrustID(generateEntrustID(m_frontID, m_sessionID, atoi(entrustField->ExecOrderRef)).c_str());

	std::string usertag = m_iniHelper.readString(ENTRUST_SECTION, pRet->getEntrustID());
	if (!usertag.empty())
		pRet->setUserTag(usertag.c_str());

	return pRet;
}


WTSError* TraderCTPOpt::makeError(CThostFtdcRspInfoField* rspInfo)
{
	WTSError* pRet = WTSError::create((WTSErroCode)rspInfo->ErrorID, rspInfo->ErrorMsg);
	return pRet;
}

WTSTradeInfo* TraderCTPOpt::makeTradeRecord(CThostFtdcTradeField *tradeField)
{
	WTSContractInfo* contract = m_bdMgr->getContract(tradeField->InstrumentID, tradeField->ExchangeID);
	if (contract == NULL)
		return NULL;

	WTSCommodityInfo* commInfo = m_bdMgr->getCommodity(contract);
	WTSSessionInfo* sInfo = m_bdMgr->getSession(commInfo->getSession());

	WTSTradeInfo *pRet = WTSTradeInfo::create(tradeField->InstrumentID, commInfo->getExchg());
	pRet->setVolume(tradeField->Volume);
	pRet->setPrice(tradeField->Price);
	pRet->setTradeID(tradeField->TradeID);

	std::string strTime = tradeField->TradeTime;
	StrUtil::replace(strTime, ":", "");
	uint32_t uTime = strtoul(strTime.c_str(), NULL, 10);
	uint32_t uDate = strtoul(tradeField->TradeDate, NULL, 10);

	//if(uDate == m_pContractMgr->getTradingDate())
	//{
	//	//�����ǰ���ںͽ�����һ�£���ʱ�����21�㣬˵����ҹ�̣�Ҳ����ʵ������Ҫ��������
	//	if (uTime / 10000 >= 21)
	//	{
	//		uDate = m_pMarketMgr->getPrevTDate(commInfo->getExchg(), uDate, 1);
	//	}
	//	else if(uTime <= 3)
	//	{
	//		//�����3�����ڣ���Ҫ�Ȼ�ȡ��һ�������գ��ٻ�ȡ��һ����Ȼ��
	//		//��������Ŀ���ǣ������������ϵ���������Դ�������
	//		uDate = m_pMarketMgr->getPrevTDate(commInfo->getExchg(), uDate, 1);
	//		uDate = TimeUtils::getNextDate(uDate);
	//	}
	//}

	pRet->setTradeDate(uDate);
	pRet->setTradeTime(TimeUtils::makeTime(uDate, uTime * 1000));

	WTSDirectionType dType = wrapDirectionType(tradeField->Direction, tradeField->OffsetFlag);

	pRet->setDirection(dType);
	pRet->setOffsetType(wrapOffsetType(tradeField->OffsetFlag));
	pRet->setRefOrder(tradeField->OrderSysID);
	pRet->setTradeType((WTSTradeType)tradeField->TradeType);

	double amount = commInfo->getVolScale()*tradeField->Volume*pRet->getPrice();
	pRet->setAmount(amount);

	//StringMap::iterator it = m_mapOrderTag.find(pRet->getRefOrder());
	//if (it != m_mapOrderTag.end())
	//{
	//	pRet->setUserTag(it->second.c_str());
	//}
	std::string usertag = m_iniHelper.readString(ORDER_SECTION, StrUtil::trim(pRet->getRefOrder()).c_str());
	if (!usertag.empty())
		pRet->setUserTag(usertag.c_str());

	return pRet;
}

std::string TraderCTPOpt::generateEntrustID(uint32_t frontid, uint32_t sessionid, uint32_t orderRef)
{
	return StrUtil::printf("%06u#%010u#%06u", frontid, sessionid, orderRef);
}

bool TraderCTPOpt::extractEntrustID(const char* entrustid, uint32_t &frontid, uint32_t &sessionid, uint32_t &orderRef)
{
	//Market.FrontID.SessionID.OrderRef
	const StringVector &vecString = StrUtil::split(entrustid, "#");
	if (vecString.size() != 3)
		return false;

	frontid = strtoul(vecString[0].c_str(), NULL, 10);
	sessionid = strtoul(vecString[1].c_str(), NULL, 10);
	orderRef = strtoul(vecString[2].c_str(), NULL, 10);

	return true;
}

bool TraderCTPOpt::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	if (pRspInfo && pRspInfo->ErrorID != 0)
		return true;

	return false;
}

void TraderCTPOpt::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	WTSEntrust* entrust = makeEntrust(pInputOrder);
	if (entrust)
	{
		WTSError *err = makeError(pRspInfo);
		//g_orderMgr.onRspEntrust(entrust, err);
		if (m_bscSink)
			m_bscSink->onRspEntrust(entrust, err);
		entrust->release();
		err->release();
	}
}

bool TraderCTPOpt::isConnected()
{
	return (m_wrapperState == WS_ALLREADY);
}

int TraderCTPOpt::queryConfirm()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_LOGINED)
	{
		return -1;
	}

	StdUniqueLock lock(m_mtxQuery);
	m_queQuery.push([this](){
		CThostFtdcQrySettlementInfoConfirmField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_strBroker.c_str());
		strcpy(req.InvestorID, m_strUser.c_str());

		int iResult = m_pUserAPI->ReqQrySettlementInfoConfirm(&req, genRequestID());
		if (iResult != 0)
		{
			m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt][%s-%s]��ѯ�˻�����ȷ��������ʧ��, ������:%d", m_strBroker.c_str(), m_strUser.c_str(), iResult);
		}
	});

	triggerQuery();

	return 0;
}

int TraderCTPOpt::confirm()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_CONFIRM_QRYED)
	{
		return -1;
	}

	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	CThostFtdcSettlementInfoConfirmField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_strBroker.c_str());
	strcpy(req.InvestorID, m_strUser.c_str());

	sprintf(req.ConfirmDate, "%u", TimeUtils::getCurDate());
	strncpy(req.ConfirmTime, TimeUtils::getLocalTime().c_str(), 8);

	int iResult = m_pUserAPI->ReqSettlementInfoConfirm(&req, genRequestID());
	if (iResult != 0)
	{
		m_bscSink->handleTraderLog(LL_ERROR, "[TraderCTPOpt][%s-%s]ȷ�Ͻ�����Ϣ������ʧ��, ������:%d", m_strBroker.c_str(), m_strUser.c_str(), iResult);
		return -1;
	}

	return 0;
}

int TraderCTPOpt::authenticate()
{
	CThostFtdcReqAuthenticateField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_strBroker.c_str());
	strcpy(req.UserID, m_strUser.c_str());
	//strcpy(req.UserProductInfo, m_strProdInfo.c_str());
	strcpy(req.AuthCode, m_strAuthCode.c_str());
	strcpy(req.AppID, m_strAppID.c_str());
	m_pUserAPI->ReqAuthenticate(&req, genRequestID());

	return 0;
}

void TraderCTPOpt::triggerQuery()
{
	m_strandIO->post([this](){
		if (m_queQuery.empty() || m_bInQuery)
			return;

		uint64_t curTime = TimeUtils::getLocalTimeNow();
		if (curTime - m_lastQryTime < 1000)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			m_strandIO->post([this](){
				triggerQuery();
			});
			return;
		}


		m_bInQuery = true;
		CommonExecuter& handler = m_queQuery.front();
		handler();

		{
			StdUniqueLock lock(m_mtxQuery);
			m_queQuery.pop();
		}

		m_lastQryTime = TimeUtils::getLocalTimeNow();
	});
}