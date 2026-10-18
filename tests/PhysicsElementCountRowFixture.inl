	// phys_fn_003477: signed element count ([this+8]-[this+4]) >> 2.
	{
	typedef int (__thiscall* ElemCountOracle)(void*);
	ElemCountOracle ec = reinterpret_cast<ElemCountOracle>(base + 0x85580);
	unsigned ef = 0;
	unsigned char eb[0x20]; memset(eb,0,sizeof(eb));
	void* const arraySelf = eb;
	memcpy(eb, &arraySelf, sizeof(arraySelf));
	unsigned lo=0x1000, hi=0x1000+12; memcpy(eb+4,&lo,4); memcpy(eb+8,&hi,4);
	if(ec(eb)!=3 || nxLockedElementCount(eb, 0)!=ec(eb))
		{fprintf(stderr,"ec3 oracle=%d candidate=%d\n",ec(eb),nxLockedElementCount(eb,0));++ef;}
	hi=0x1000; memcpy(eb+8,&hi,4);
	if(ec(eb)!=0 || nxLockedElementCount(eb, 0)!=ec(eb))
		{fprintf(stderr,"ec0 oracle=%d candidate=%d\n",ec(eb),nxLockedElementCount(eb,0));++ef;}
	lo=0x1000; hi=0x1000-8; memcpy(eb+4,&lo,4); memcpy(eb+8,&hi,4);
	if(ec(eb)!=-2 || nxLockedElementCount(eb, 0)!=ec(eb))
		{fprintf(stderr,"ecneg oracle=%d candidate=%d\n",ec(eb),nxLockedElementCount(eb,0));++ef;}
	printf("elemcount row=phys_fn_003477 cases=3 mismatches=%u\n", ef);
	}
