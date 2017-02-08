#include "DemoServant.h"
#include "EngineImp.h"
#include <time.h>

xic::MethodTab::PairType DemoServant::_methodpairs[] = {
#define CMD(X)  { XS_TOSTR(X), XIC_METHOD_CAST(DemoServant, X) },
	CMD_LIST
#undef CMD
};
xic::MethodTab DemoServant::_methodtab(_methodpairs, XS_ARRCOUNT(_methodpairs));

DemoServant::DemoServant(const SettingPtr& setting, const xic::AdapterPtr& adapter)
	: ServantI(&_methodtab)
{
	_engine = adapter->getEngine();

	xref_inc();
	_selfPrx = adapter->addServant("Demo", this);
	adapter->addServant("Tester", this);
	xref_dec_only();
}

DemoServant::~DemoServant()
{
}

XIC_METHOD(DemoServant, time)
{
	xic::VDict args = quest->args();
	time_t t = args.getInt("time", LONG_MIN);
	if (t == LONG_MIN)
	{
		time(&t);
	}

        xic::AnswerWriter aw;
        aw.param("time", t);

        struct tm tm;
        char buf[32];

        gmtime_r(&t, &tm);
        strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", &tm);
        aw.param("utc", buf);

        localtime_r(&t, &tm);
        strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", &tm);
        aw.param("local", buf);

        return aw;
}

XIC_METHOD(DemoServant, echo)
{
	xic::AnswerWriter aw;
	for (xic::VDict::Node node = quest->args().first(); node; ++node)
	{
		aw.param(node.xstrKey(), node.dataValue());
	}
	return aw;
}

XIC_METHOD(DemoServant, wait)
{
	xic::VDict args = quest->args();
	int seconds = args.getInt("seconds", 1);
	xic::AnswerWriter aw;
	_engine->sleep(seconds);
	aw.param("seconds", seconds);
	return aw;
}

XIC_METHOD(DemoServant, rubbish)
{
	xic::VDict args = quest->args();
	int64_t size = args.getInt("size");
	if (size < 0 || size >= xic::xic_message_size)
		throw XERROR_MSG(XError, "size too big");
	xic::AnswerWriter aw;
	aw.paramBlobHead("data", size);
	aw.buffer(size);
	return aw;
}

XIC_METHOD(DemoServant, discard)
{
	return xic::ONEWAY_ANSWER;
}

XIC_METHOD(DemoServant, rmi)
{
	xic::VDict args = quest->args();
	const xstr_t& rmi_proxy = args.wantXstr("proxy");
	const xstr_t& rmi_method = args.wantXstr("method");
	const vbs_dict_t *rmi_args = args.want_dict("args");

	xic::QuestWriter qw(rmi_method);
	qw.ctx("CACHE", 10);
	xstr_t raw = xstr_slice(&rmi_args->_raw, 1, -1);
	qw.raw(raw.data, raw.len);

	xic::ProxyPtr prx = current.adapter->getEngine()->stringToProxy(make_string(rmi_proxy));
	xic::ContextBuilder ctx("CALLER", "Demo");
	prx->setContext(ctx.build());
	prx->emitQuest(qw, xic::Completion::createPassThrough(current.asynchronous()));
	return xic::ASYNC_ANSWER;
}

XIC_METHOD(DemoServant, throwException)
{
	xic::VDict args = quest->args();
	int code = args.getInt("code");
	const xstr_t& message = args.getXstr("message");
	throw XERROR_CODE_MSG(XError, code, make_string(message));
	return xic::AnswerWriter();
}

XIC_METHOD(DemoServant, selfProxy)
{
	return xic::AnswerWriter()("proxy", _selfPrx->str());
}

