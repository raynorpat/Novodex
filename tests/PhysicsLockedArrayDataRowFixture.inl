	// phys_fn_003479: return the begin pointer only when the 32-bit byte
	// distance has at least one complete word after masking its low bits.
	{
	typedef void* (__thiscall* DataOracle)(void*);
	DataOracle const oracle = reinterpret_cast<DataOracle>(base + 0x85590);
	struct Case { const char* name; unsigned begin; unsigned end; bool hasWord; };
	static const Case cases[] = {
		{ "empty", 0x00001000u, 0x00001000u, false },
		{ "one-byte", 0x00001000u, 0x00001001u, false },
		{ "two-bytes", 0x00001000u, 0x00001002u, false },
		{ "three-bytes", 0x00001000u, 0x00001003u, false },
		{ "one-word", 0x00001000u, 0x00001004u, true },
		{ "two-words", 0x00001000u, 0x00001008u, true },
		{ "wrapped-minus-one", 0x00001000u, 0x00000fffu, true },
		{ "wrapped-minus-four", 0x00001000u, 0x00000ffcu, true },
	};
	unsigned mismatches = 0;
	unsigned oracleStackChecks = 0;
	unsigned candidateStackChecks = 0;
	for(unsigned i = 0; i != sizeof(cases) / sizeof(cases[0]); ++i)
		{
		unsigned char object[0x20] = {};
		memcpy(object + 0x14, &cases[i].begin, sizeof(cases[i].begin));
		memcpy(object + 0x18, &cases[i].end, sizeof(cases[i].end));
		const void* const expected = cases[i].hasWord ?
			reinterpret_cast<void*>(static_cast<size_t>(cases[i].begin)) : 0;
		unsigned oracleStackBefore = 0, oracleStackAfter = 0;
#if defined(_M_IX86)
		__asm mov oracleStackBefore, esp
#endif
		const void* const oracleValue = oracle(object);
#if defined(_M_IX86)
		__asm mov oracleStackAfter, esp
#endif
		unsigned candidateStackBefore = 0, candidateStackAfter = 0;
#if defined(_M_IX86)
		__asm mov candidateStackBefore, esp
#endif
		const void* const candidateValue = nxArrayDataIfNonEmpty(object);
#if defined(_M_IX86)
		__asm mov candidateStackAfter, esp
#endif
		if(oracleStackBefore == oracleStackAfter) ++oracleStackChecks;
		if(candidateStackBefore == candidateStackAfter) ++candidateStackChecks;
		if(oracleValue != expected || candidateValue != expected ||
			oracleStackBefore != oracleStackAfter ||
			candidateStackBefore != candidateStackAfter)
			{
			fprintf(stderr, "locked-array-data case=%s expected=%p oracle=%p candidate=%p stack=%u/%u,%u/%u\n",
				cases[i].name, expected, oracleValue, candidateValue,
				oracleStackBefore, oracleStackAfter,
				candidateStackBefore, candidateStackAfter);
			++mismatches;
			}
		}
	printf("locked-array-data row=003479 cases=%u stack=%u/%u mismatches=%u\n",
			(unsigned)(sizeof(cases) / sizeof(cases[0])),
			oracleStackChecks, candidateStackChecks, mismatches);
	candidateMissing += mismatches;
	}
