#pragma once
#include "../WtCore/HftStraBaseCtx.h"

USING_NS_OTP;

class ExpHftContext : public HftStraBaseCtx
{
public:
	ExpHftContext(WtHftEngine* engine, const char* name):HftStraBaseCtx(engine, name){}
	virtual ~ExpHftContext(){}

public:
	virtual void on_bar(const char* code, const char* period, uint32_t times, WTSBarStruct* newBar) override;

	virtual void on_channel_lost() override;

	virtual void on_channel_ready() override;

	virtual void on_entrust(uint32_t localid, const char* stdCode, bool bSuccess, const char* message) override;

	virtual void on_init() override;

	virtual void on_order(uint32_t localid, const char* stdCode, bool isBuy, double totalQty, double leftQty, double price, bool isCanceled = false) override;

	virtual void on_tick(const char* code, WTSTickData* newTick) override;

	virtual void on_trade(uint32_t localid, const char* stdCode, bool isBuy, double vol, double price) override;
};

