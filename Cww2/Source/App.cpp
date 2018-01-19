#include "App.h"


App::App():
	m_scanner3("s2_mp64_ship.exe")
{
}

void App::loop()
{
	if (!m_scanner.init()) return;

	while (true)
	{
		printf("Type command and press enter.\n");

		char buff[1024];
		char* p = fgets(buff, 1024, stdin);
		if (!p) continue;

		// remove \n
		size_t len = strlen(p);
		if (len) p[len-1] = '\0';

		String s = p;
		if (s == "exit") break;
		else if (s == "new")	m_scanner.doNewScan();
		else if (s == "cont")	m_scanner.continueScan();
		else if (str_contains(s, "up"))		handleHighLow(s, true);
		else if (str_contains(s, "down"))	handleHighLow(s, false);
		else if (s == "showp")	m_scanner.showPlayers();
		else if (s == "show")	m_scanner.refreshEntries();
		else if (s == "tomem")	m_scanner.toMemory();
		else if (str_contains(s, "around")) handleAround(s);
		else if (s == "fplayers") m_scanner.findPlayers();
		else if (s == "rplayers") m_scanner.refreshPlayers();
		else if (str_contains(s, "fbones")) handleBones(s);
		else if (str_contains(s, "kbones")) handleBonesAround(s);
		else if (s == "rbones") m_scanner.refreshBones();
		else if (s == "quats") m_scanner.scanQuats();
		else if (s == "fquats") m_scanner.filterQuats();
		else if (s == "kfquats") m_scanner.keepFilteringQuats();
		else if (s == "?")		
		{
			printf("Commands are:\nnew, up, down, show, tomem\n");
			continue;
		}
		else
		{
			printf("Unrecognized command.\n");
			continue;
		}

		printf("Num entries %d, %.3fK, %.3fM\n", m_scanner.getNumEntries(), (float)m_scanner.getNumEntries()/1000, (float)m_scanner.getNumEntries()/(1000000));
	}
}

void App::loop2()
{
	while (true)
	{
		printf("Type command and press enter.\n");

		char buff[1024];
		char* p = fgets(buff, 1024, stdin);
		if (!p) continue;

		// remove \n
		size_t len = strlen(p);
		if (len) p[len-1] = '\0';

		String s = p;
		if (s == "exit") break;
		else if (str_contains(s, "scan"))		handleScan(s); 
		else if (str_contains(s, "filter"))		handleFilter(s);
		else if (str_contains(s, "tomem"))		handleToMemory(s);
		else if (str_contains(s, "show"))		handleShow(s);
		else if (str_contains(s, "num"))		handleNum(s);
		else if (str_contains(s, "around"))		handleShowAround(s);
		else if (str_contains(s, "cptr"))		handleClearPtrList(s);
		else if (s == "?")		
		{
			printf("Commands are:\nscan, tomem, show, num\n");
			continue;
		}
		else
		{
			printf("Unrecognized command.\n");
			continue;
		}
	}
}

void App::handleAround(String s)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.size() < 4)
	{
		printf("Around should have 3 arguments.\n");
		return;
	}
	u32 entryIdx = str_toU32(substr[1]);
	u32 numEntries = m_scanner.getNumEntries();
	u32 lowerEntries  = str_toU32(substr[2]);
	u32 higherEntries = str_toU32(substr[3]);
	AroundType type = AroundType::Entry;
	if (substr.size() > 4)
	{
		type = (AroundType) str_toU32(substr[4]);
	}
	m_scanner.showAround(entryIdx, lowerEntries, higherEntries, type);
}

void App::handleHighLow(String s, bool mustBeHigher)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.empty()) 
	{
		printf("Invalid argument count.\n");
		return;
	}
	float delta = 200.f;
	if (substr.size() > 1) delta = str_toFloat(substr[1]);
	m_scanner.filterHighLow(mustBeHigher, delta);
}

void App::handleBones(String s)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.size() < 2)
	{
		printf("handleBones should have 2 arguments.\n");
		return;
	}
	m_scanner.findBones(str_toU32(substr[1]));
}

void App::handleBonesAround(String s)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.size() < 2)
	{
		printf("Around should have 2 arguments.\n");
		return;
	}
	u32 entryIdx = str_toU32(substr[1]);
	m_scanner.countBonesAround(entryIdx);
}



void App::handleScan(String s)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.size() < 5) { printf("Invalid arg count.\n"); return; }

	u32 alignment   = str_toU32(substr[1]);
	u32 patternSize = str_toU32(substr[2]);
	CompareType ct  = ct_to_str(substr[3]);
	ValueType vt    = vt_to_str(substr[4]);

	CompareValue low, high;
	String strL, strH;
	if (substr.size() > 5) strL = substr[5];
	if (substr.size() > 6) strH = substr[6];
	getLowHigh( vt, low, high, strL, strH );

	printf("--- Doing new scan with align %d, patt_size %d, comp type %s, value type %s ---\n", alignment, patternSize, substr[3].c_str(), substr[4].c_str());
	u32 scanIdx = m_scanner3.newScan( patternSize, alignment, ct, vt, low, high );

	postUpdate(scanIdx);
}


void App::handleFilter(String s)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.size() < 3) { printf("Invalid arg count.\n"); return; }

	u32 scanIdx		= str_toU32(substr[1]);
	Scan* sc = m_scanner3.getScan(scanIdx);
	if (!sc) return;

	CompareType ct  = ct_to_str(substr[2]);

	CompareValue low, high;
	String strL, strH;
	if (substr.size() > 3) strL = substr[3];
	if (substr.size() > 4) strH = substr[4];
	getLowHigh( sc->getValueType(), low, high, strL, strH );

	printf("--- Filter scan %d comp type %s,  ---\n", scanIdx, substr[2].c_str());
	m_scanner3.filterScan( scanIdx, ct, low, high );

	postUpdate(scanIdx);

}

void App::postUpdate(u32 scanIdx)
{
	u64 numEntries = m_scanner3.getNumEntries(scanIdx, 0);
	printf("Num entries for scan idx %d, %lld.\n", scanIdx, numEntries);
	if (numEntries == 0)
	{
		m_scanner3.deleteScan(scanIdx);
	}
	else if (numEntries <= 1000)
	{
		m_scanner3.toMemory(scanIdx, 0);
	}
}

void App::handleToMemory(String s)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.size() < 2) { printf("Invalid arg count.\n"); return; }
	u32 scanIdx = str_toU32(substr[1]);
	m_scanner3.toMemory( scanIdx, false );
}

void App::handleShow(String s)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.size() < 2) { printf("Invalid arg count.\n"); return; }
	u32 scanIdx = str_toU32(substr[1]);
	m_scanner3.showScan(scanIdx);
}

void App::handleNum(String s)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.size() < 2) { printf("Invalid arg count.\n"); return; }
	u32 scanIdx = str_toU32(substr[1]);
	u64 numEntries = m_scanner3.getNumEntries(scanIdx,false);
	printf("Num entries for scan idx %d, %lld.\n", scanIdx, numEntries);
}

void App::handleShowAround(String s)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.size() < 5) { printf("Invalid arg count.\n"); return; }
	u32 scanIdx  = str_toU32(substr[1]);
	u32 entryIdx = str_toU32(substr[2]);
	u32 lower  = str_toU32(substr[3]);
	u32 higher = str_toU32(substr[4]);
	MemoryBlock cache;
	mem_zero(&cache, sizeof(MemoryBlock));
	m_scanner3.showAround(scanIdx, entryIdx, lower, higher, substr.size()>=6, cache);
	mem_free(cache.data);
}

void App::handleClearPtrList(String s)
{
	Array<String> substr;
	str_split(s, " ", substr);
	if (substr.size() < 2) { printf("Invalid arg count.\n"); return; }
	u32 scanIdx  = str_toU32(substr[1]);
	m_scanner3.clearPtrList(scanIdx);
}

void App::getLowHigh(ValueType vt, CompareValue& low, CompareValue& high, const String& strL, const String& strH)
{
	switch (vt)
	{
	case ValueType::String:
		low.ptr = nullptr;
		low.ptr = str_replace(low.ptr, strL.size()+1, strL.c_str());
		break;

	case ValueType::Float:
		low.f  = str_toFloat(strL);
		high.f = str_toFloat(strH);
		break;

	case ValueType::Int:
		low.i  = str_toI32(strL);
		high.i = str_toI32(strH);
		break;

	case ValueType::Uint:
		low.ui  = str_toU32(strL);
		high.ui = str_toU32(strH);
		break;

	case ValueType::Ptr:
		low.ptr = (char*)str_toPtr(strL);
		break;
	}
}
