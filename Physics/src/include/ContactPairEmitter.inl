// The stream appends 000875 makes through the pair's SdkContainer (+0x38),
// growing it with 004840 first: `count == capacity` before a single word,
// `count + 3 > capacity` before three.
static NX_INLINE void cpmStreamAppend(SdkContainer* stream, NxU32 word)
	{
	if(stream->mCount == stream->mCapacity)
		stream->resize(1);
	stream->mEntries[stream->mCount] = word;
	stream->mCount++;
	}

static NX_INLINE void cpmStreamAppend3(SdkContainer* stream, const void* words)
	{
	if(stream->mCount + 3 > stream->mCapacity)
		stream->resize(3);
	cpmCopyWords(stream->mEntries + stream->mCount, words, 3);
	stream->mCount += 3;
	}

// phys_fn_000875 (0x0001d8e0, 915 B)
// The contact-stream emitter for 32-bit features; the pair is the sink.
// thiscall, `ret 0x24`. As 000873, plus: the pair header's flag word gains 4
// when either shape's +0xde has bit 0x20; with that flag set, a contact whose
// full feature ids do not both fit in 16 bits gets bit 31 in its separation
// word and both ids as two words, otherwise one word fid1 << 16 | fid0.
// Unlike the candidate's 000873, every append grows the stream through 004840
// (SdkContainer::resize) at the listing's eight sites.
__declspec(noinline) void NxActorPair::row000875(const NxU8* object1, const NxU8* object0,
	NxU32 separationBits, const NxReal* point, const NxReal* normal, NxU16 featureId0,
	NxU16 featureId1, NxU32 feature0, NxU32 feature1)
	{
	const NxU8* shape1 = cpmAt<const NxU8*>(object1, 8);
	const NxU8* shape0 = cpmAt<const NxU8*>(object0, 8);
	NxReal negated[3];
	if(cpmAt<const void*>(cpmAt<const NxU8*>(shape1, 4), 8) != at<const void*>(8))
		{
		const NxU8* swapShape = shape1;
		shape1 = shape0;
		shape0 = swapShape;
		const NxU32 swapFeature = feature0;
		feature0 = feature1;
		feature1 = swapFeature;
		const NxU16 swapId = featureId0;
		featureId0 = featureId1;
		featureId1 = swapId;
		negated[0] = (NxReal)(-(double)normal[0]);
		negated[1] = (NxReal)(-(double)normal[1]);
		negated[2] = (NxReal)(-(double)normal[2]);
		normal = negated;
		}

	SdkContainer* stream = reinterpret_cast<SdkContainer*>(mBytes + 0x38);
	if(at<const void*>(0x20) != cpmAt<const void*>(shape1, 0x9c)
		|| at<const void*>(0x24) != cpmAt<const void*>(shape0, 0x9c))
		{
		const NxU32 valid = (featureId0 != 0xffff && featureId1 != 0xffff) ? 1u : 0u;
		const NxU32 wide = ((shape1[0xde] & 0x20) || (shape0[0xde] & 0x20)) ? 4u : 0u;
		const NxU32 flags = wide | valid;
		at<NxU32>(0x34) = flags;
		const NxU32 packed = flags << 16;
		at<const void*>(0x20) = cpmAt<const void*>(shape1, 0x9c);
		at<const void*>(0x24) = cpmAt<const void*>(shape0, 0x9c);
		cpmStreamAppend(stream, cpmAt<NxU32>(shape1, 0x9c));
		cpmStreamAppend(stream, cpmAt<NxU32>(shape0, 0x9c));
		const NxU8* holder = cpmAt<const NxU8*>(cpmAt<const NxU8*>(shape1, 4), 8);
		const NxU32 material = holder
			? cpmAt<NxU32>(holder, 0x240)
			: cpmAt<NxU32>(cpmAt<const NxU8*>(cpmAt<const NxU8*>(shape0, 4), 8), 0x240);
		const NxU32 normalCountIndex = stream->mCount;
		cpmStreamAppend(stream, (material << 24) | packed);
		at<NxU32>(0x18) = normalCountIndex;
		stream->mEntries[at<NxU32>(0x14)]++;
		at<NxU32>(0x30) = 0;
		at<NxU32>(0x2c) = 0;
		at<NxU32>(0x28) = 0;
		}

	const NxU32* normalWords = reinterpret_cast<const NxU32*>(normal);
	if(at<NxU32>(0x28) != normalWords[0] || at<NxU32>(0x2c) != normalWords[1]
		|| at<NxU32>(0x30) != normalWords[2])
		{
		cpmCopyWords(mBytes + 0x28, normalWords, 3);
		cpmStreamAppend3(stream, normalWords);
		const NxU32 pointCountIndex = stream->mCount;
		cpmStreamAppend(stream, 0);
		at<NxU32>(0x1c) = pointCountIndex;
		stream->mEntries[at<NxU32>(0x18)]++;
		}

	const NxU32 wideFeature = (feature0 > 0xffff || feature1 > 0xffff) ? 0x80000000u : 0u;
	at<NxU32>(0x10)++;
	cpmStreamAppend3(stream, point);
	cpmStreamAppend(stream, (separationBits & 0x7fffffff) | wideFeature);
	stream->mEntries[at<NxU32>(0x1c)]++;
	if(at<NxU32>(0x34) & 1)
		cpmStreamAppend(stream, ((NxU32)featureId1 << 16) | (NxU32)featureId0);
	if(at<NxU32>(0x34) & 4)
		{
		if(wideFeature)
			{
			cpmStreamAppend(stream, feature0);
			cpmStreamAppend(stream, feature1);
			}
		else
			cpmStreamAppend(stream, (feature1 << 16) | feature0);
		}
	}

