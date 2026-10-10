void nxContainerAddThunk(void* innerThis)
	{
	reinterpret_cast<SdkContainer*>(
		reinterpret_cast<unsigned>(innerThis) + 0x28)->empty();
	}
