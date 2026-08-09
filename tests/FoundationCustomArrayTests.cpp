#include <stdio.h>
#include <string.h>

#include "CustomArray.h"

static int fail(const char* message)
	{
	fprintf(stderr, "FAIL %s\n", message);
	return 1;
	}

int main()
	{
	CustomArray binary(4);
	binary.Store(NxU32(0));
	if(!binary.PushAddress()) return fail("push address");
	binary.Store(char('A')).Store(NxU16(0x2233)).Store(1.5f);
	binary.PopAddressAndStore(NxU32(0x11223344));
	if(binary.GetOffset()!=11 || binary.GetCellMaxSize()!=8 || binary.GetCellUsedSize()!=7)
		return fail("block growth/offset");
	unsigned char expected[] = { 0x44,0x33,0x22,0x11, 0x41,0x33,0x22, 0x00,0x00,0xc0,0x3f };
	unsigned char collapsed[sizeof(expected)] = {};
	if(binary.Collapse(collapsed)!=collapsed || memcmp(collapsed, expected, sizeof(expected)))
		return fail("collapse/patch bytes");

	CustomArray reader(sizeof(expected), collapsed);
	reader.Reset();
	if(reader.GetDword()!=0x11223344 || reader.GetByte()!=NxU8('A') ||
		reader.GetWord()!=0x2233 || reader.GetFloat()!=1.5f)
		return fail("typed readback");

	CustomArray bits(1);
	bits.StoreBits(0xb, 4).EndBits();
	if(bits.GetOffset()!=1 || *reinterpret_cast<NxU8*>(bits.Collapse())!=0xb0)
		return fail("bit packing");
	bits.Reset();
	if(bits.GetBits(4)!=0xb) return fail("bit readback");

	CustomArray ascii(2);
	ascii.StoreASCII("ab").StoreASCII(NxU32(42));
	const char asciiExpected[] = "ab42";
	if(ascii.GetOffset()!=4 || memcmp(ascii.Collapse(), asciiExpected, 4))
		return fail("ASCII storage");

	FILE* stream = tmpfile();
	if(!stream || !binary.ExportToDisk(stream)) return fail("stream export");
	rewind(stream);
	unsigned char streamed[sizeof(expected)] = {};
	if(fread(streamed, 1, sizeof(streamed), stream)!=sizeof(streamed) ||
		memcmp(streamed, expected, sizeof(expected)) || fgetc(stream)!=EOF)
		return fail("stream bytes");
	fclose(stream);

	CustomArray copied(binary);
	CustomArray assigned(1);
	assigned = binary;
	if(copied.GetOffset()!=binary.GetOffset() || assigned.GetOffset()!=binary.GetOffset() ||
		memcmp(copied.Collapse(), expected, sizeof(expected)) ||
		memcmp(assigned.Collapse(), expected, sizeof(expected)))
		return fail("copy/assignment ownership");
	memset(collapsed, 0, sizeof(collapsed));
	if(memcmp(copied.Collapse(), expected, sizeof(expected))) return fail("independent collapsed data");

	printf("misc exports=0 custom_array block=4,8 offset=11 typed=roundtrip bits=b0 ascii=ab42 stream=exact copy=deep assignment=deep\n");
	return 0;
	}
