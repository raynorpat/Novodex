	// phys_fn_003595: return [this+0x10] & argument, with ret 4.
	{
	typedef unsigned (__thiscall* AndOracle)(void*, unsigned);
	AndOracle const oracle = reinterpret_cast<AndOracle>(base + 0x88130);
	unsigned char object[0x30] = {};
	void* const selfField = object;
	memcpy(object, &selfField, sizeof(selfField));
	static const unsigned values[3] = { 0x000000bcu, 0x8000f00fu, 0xffffffffu };
	static const unsigned masks[3] = { 0x00000010u, 0x00ff00ffu, 0x7fffffffu };
	unsigned mismatches = 0;
	for(unsigned i = 0; i != 3; ++i)
		{
		memcpy(object + 0x10, &values[i], sizeof(values[i]));
		const unsigned expected = oracle(object, masks[i]);
		const unsigned actual = nxLockedAndRead(object, 0, 0x10, masks[i]);
		if(expected != actual)
			{
			fprintf(stderr, "andget003595 case=%u oracle=%08x candidate=%08x\n",
				i, expected, actual);
			++mismatches;
			}
		}
	printf("andget row=phys_fn_003595 cases=3 mismatches=%u\n", mismatches);
	}
