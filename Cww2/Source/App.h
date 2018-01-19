#pragma once

#include "Null.h"
#include "Scanner2.h"

using namespace Null;



class App
{

public:
	App();
	void loop();
	void loop2();

	Scanner2 m_scanner;
	Scanner3 m_scanner3;

private:
	void handleAround(String s);
	void handleHighLow(String s, bool mustBeHigher);
	void handleBones(String s);
	void handleBonesAround(String s);

	// scanner3
	void handleScan(String s);
	void handleFilter(String s);

	void postUpdate(u32 scanIdx);

	void handleToMemory(String s);
	void handleShow(String s);
	void handleNum(String s);
	void handleShowAround(String s);
	void handleClearPtrList(String s);

	void getLowHigh(ValueType vt, CompareValue& low, CompareValue& high, const String& strL, const String& strH);
};