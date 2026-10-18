	// phys_fn_003597 and phys_fn_003599 return the callback result only when
	// the final argument is 2; otherwise they return callback_result == 0.
	{
	typedef unsigned (__thiscall* Oracle)(void*, unsigned);
	typedef unsigned (__cdecl* Callback)(unsigned, unsigned, unsigned);
	extern unsigned __cdecl nxGlobalCallNResult(void*, Callback, int, unsigned, unsigned);
	struct Row { unsigned rva; unsigned slot; const char* name; };
	static const Row rows[] = {
		{ 0x88140u, 0x126530u, "003597" },
		{ 0x88180u, 0x1264f0u, "003599" },
	};
	static const unsigned callbackResults[] = { 0u, 0x12345678u };
	static const unsigned modes[] = { 0u, 1u, 2u };
	unsigned mismatches = 0;
	unsigned cases = 0;
	for(unsigned row = 0; row != sizeof(rows) / sizeof(rows[0]); ++row)
		{
		NxFnPtrSaved saved = nxBindFnPtr(base, rows[row].slot,
			reinterpret_cast<void*>(&nxN3ResultStub));
		unsigned char nested[0x40] = {};
		unsigned char object[0x100] = {};
		unsigned char candidateObject[0x100] = {};
		const unsigned first = 0x59700000u + row;
		const unsigned second = 0x59900000u + row;
		const unsigned third = 0x59800000u + row;
		memcpy(nested + 0x30, &first, sizeof(first));
		*reinterpret_cast<void**>(object + 0x7c) = nested;
		*reinterpret_cast<void**>(candidateObject + 0x7c) = nested;
		memcpy(object + 0x80, &second, sizeof(second));
		memcpy(candidateObject + 0x80, &second, sizeof(second));
		*reinterpret_cast<void**>(object + 4) = object;
		*reinterpret_cast<void**>(candidateObject + 4) = candidateObject;
		memcpy(object + 8, &third, sizeof(third));
		memcpy(candidateObject + 8, &third, sizeof(third));
		Oracle oracle = reinterpret_cast<Oracle>(base + rows[row].rva);
		unsigned rowMismatches = 0;
		unsigned rowCases = 0;
		unsigned rowOracleStackChecks = 0;
		unsigned rowCandidateStackChecks = 0;
		for(unsigned result = 0; result != sizeof(callbackResults) / sizeof(callbackResults[0]); ++result)
			for(unsigned mode = 0; mode != sizeof(modes) / sizeof(modes[0]); ++mode)
				{
				gN3ResultValue = callbackResults[result];
				gN3Hits = 0; gN3A = 0; gN3B = 0; gN3C = 0;
				unsigned oracleStackBefore = 0, oracleStackAfter = 0;
#if defined(_M_IX86)
				__asm mov oracleStackBefore, esp
#endif
				const unsigned oracleValue = oracle(object, modes[mode]);
#if defined(_M_IX86)
				__asm mov oracleStackAfter, esp
#endif
				const unsigned oracleHits = gN3Hits;
				const unsigned oracleA = gN3A, oracleB = gN3B, oracleC = gN3C;
				gN3Hits = 0; gN3A = 0; gN3B = 0; gN3C = 0;
				unsigned candidateStackBefore = 0, candidateStackAfter = 0;
#if defined(_M_IX86)
				__asm mov candidateStackBefore, esp
#endif
				const unsigned candidateValue = nxGlobalCallNResult(candidateObject,
					&nxN3ResultStub, 1, modes[mode], third);
#if defined(_M_IX86)
				__asm mov candidateStackAfter, esp
#endif
				const unsigned candidateHits = gN3Hits;
				const unsigned candidateA = gN3A, candidateB = gN3B, candidateC = gN3C;
				const bool equal = oracleValue == candidateValue &&
					oracleHits == 1u && candidateHits == oracleHits &&
					oracleA == candidateA && oracleB == candidateB &&
					oracleC == candidateC && oracleA == first &&
					oracleB == second && oracleC == third &&
					oracleStackBefore == oracleStackAfter &&
					candidateStackBefore == candidateStackAfter;
				if(oracleStackBefore == oracleStackAfter)
					++rowOracleStackChecks;
				if(candidateStackBefore == candidateStackAfter)
					++rowCandidateStackChecks;
				if(!equal)
					{
					fprintf(stderr, "global-call-result row=%s result=%08x mode=%u oracle=%08x candidate=%08x args=%08x.%08x.%08x/%08x.%08x.%08x hits=%u/%u\n",
						rows[row].name, callbackResults[result], modes[mode], oracleValue,
						candidateValue, oracleA, oracleB, oracleC, candidateA,
						candidateB, candidateC, oracleHits, candidateHits);
					++mismatches;
					++rowMismatches;
				}
				++cases;
				++rowCases;
				}
		printf("global-call-result row=%s cases=%u stack=%u/%u mismatches=%u\n",
			rows[row].name, rowCases, rowOracleStackChecks,
			rowCandidateStackChecks, rowMismatches);
		nxUnbindFnPtr(base, rows[row].slot, saved);
		}
	printf("global-call-result rows=003597.003599 cases=%u mismatches=%u\n", cases, mismatches);
	candidateMissing += mismatches;
	}
