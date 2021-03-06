#include "HCR.h"
#include "IntUtils.h"
#include "ProviderFromName.h"

NAMESPACE_PRNG

const std::string HCR::CLASS_NAME("HCR");

//~~~Properties~~~//

const Prngs HCR::Enumeral()
{
	return Prngs::HCR;
}

const std::string HCR::Name()
{
	return CLASS_NAME + "-" + m_rngGenerator->Name();
}

//~~~Constructor~~~//

HCR::HCR(Digests DigestEngine, Providers SeedEngine, size_t BufferSize)
	:
	m_bufferIndex(0),
	m_bufferSize(BufferSize),
	m_digestType(DigestEngine),
	m_isDestroyed(false),
	m_pvdType(SeedEngine),
	m_rngBuffer(BufferSize)
{
	if (BufferSize < BUFFER_MIN)
		throw CryptoRandomException("HCR:Ctor", "BufferSize must be at least 64 bytes!");

	Reset();
}

HCR::HCR(std::vector<byte> Seed, Digests DigestEngine, size_t BufferSize)
	:
	m_bufferIndex(0),
	m_bufferSize(BufferSize),
	m_digestType(DigestEngine),
	m_isDestroyed(false),
	m_rngBuffer(BufferSize)
{
	if (Seed.size() == 0)
		throw CryptoRandomException("HCR:Ctor", "Seed can not be null!");
	if (GetMinimumSeedSize(DigestEngine) > Seed.size())
		throw CryptoRandomException("HCR:Ctor", "The state seed is too small! must be at least digest block size + 8 bytes");
	if (BufferSize < BUFFER_MIN)
		throw CryptoRandomException("HCR:Ctor", "BufferSize must be at least 128 bytes!");

	m_pvdType = Providers::CSP;
	Reset();
}

HCR::~HCR()
{
	Destroy();
}

//~~~Public Functions~~~//

void HCR::Destroy()
{
	if (!m_isDestroyed)
	{
		m_bufferIndex = 0;
		m_bufferSize = 0;

		Utility::IntUtils::ClearVector(m_rngBuffer);
		Utility::IntUtils::ClearVector(m_rndSeed);

		if (m_rngGenerator != 0)
			delete m_rngGenerator;

		m_isDestroyed = true;
	}
}

void HCR::Fill(std::vector<ushort> &Output, size_t Offset, size_t Elements)
{
	CEXASSERT(Output.size() - Offset <= Elements, "the output array is too short");

	size_t bufLen = Elements * sizeof(ushort);
	std::vector<byte> buf(bufLen);
	GetBytes(buf);
	Utility::MemUtils::Copy(buf, 0, Output, Offset, bufLen);
}

void HCR::Fill(std::vector<uint> &Output, size_t Offset, size_t Elements)
{
	CEXASSERT(Output.size() - Offset <= Elements, "the output array is too short");

	size_t bufLen = Elements * sizeof(uint);
	std::vector<byte> buf(bufLen);
	GetBytes(buf);
	Utility::MemUtils::Copy(buf, 0, Output, Offset, bufLen);
}

void HCR::Fill(std::vector<ulong> &Output, size_t Offset, size_t Elements)
{
	CEXASSERT(Output.size() - Offset <= Elements, "the output array is too short");

	size_t bufLen = Elements * sizeof(ulong);
	std::vector<byte> buf(bufLen);
	GetBytes(buf);
	Utility::MemUtils::Copy(buf, 0, Output, Offset, bufLen);
}

std::vector<byte> HCR::GetBytes(size_t Size)
{
	std::vector<byte> data(Size);
	GetBytes(data);
	return data;
}

void HCR::GetBytes(std::vector<byte> &Output)
{
	if (Output.size() == 0)
		throw CryptoRandomException("BCR:GetBytes", "Buffer size must be at least 1 byte!");

	if (m_rngBuffer.size() - m_bufferIndex < Output.size())
	{
		size_t bufSize = m_rngBuffer.size() - m_bufferIndex;
		// copy remaining bytes
		if (bufSize != 0)
			Utility::MemUtils::Copy<byte>(m_rngBuffer, m_bufferIndex, Output, 0, bufSize);

		size_t rmd = Output.size() - bufSize;

		while (rmd > 0)
		{
			// fill buffer
			m_rngGenerator->Generate(m_rngBuffer);

			if (rmd > m_rngBuffer.size())
			{
				Utility::MemUtils::Copy<byte>(m_rngBuffer, 0, Output, bufSize, m_rngBuffer.size());
				bufSize += m_rngBuffer.size();
				rmd -= m_rngBuffer.size();
			}
			else
			{
				Utility::MemUtils::Copy<byte>(m_rngBuffer, 0, Output, bufSize, rmd);
				m_bufferIndex = rmd;
				rmd = 0;
			}
		}
	}
	else
	{
		Utility::MemUtils::Copy<byte>(m_rngBuffer, m_bufferIndex, Output, 0, Output.size());
		m_bufferIndex += Output.size();
	}
}

ushort HCR::NextUShort()
{
	return Utility::IntUtils::LeBytesTo16(GetBytes(2), 0);
}

ushort HCR::NextUShort(ushort Maximum)
{
	CEXASSERT(Maximum != 0, "maximum can not be zero");

	std::vector<byte> rand;
	uint num(0);

	do
	{
		rand = GetByteRange(Maximum);
		num = Utility::IntUtils::LeBytesTo16(rand, 0);
	} 
	while (num > Maximum);

	return num;
}

ushort HCR::NextUShort(ushort Maximum, ushort Minimum)
{
	CEXASSERT(Maximum != 0, "maximum can not be zero");
	CEXASSERT(Maximum > Minimum, "minimum can not be more than maximum");

	uint num = 0;
	while ((num = NextUShort(Maximum)) < Minimum) {}
	return num;
}

uint HCR::Next()
{
	return Utility::IntUtils::LeBytesTo32(GetBytes(4), 0);
}

uint HCR::Next(uint Maximum)
{
	CEXASSERT(Maximum != 0, "maximum can not be zero");

	std::vector<byte> rand;
	uint num(0);

	do
	{
		rand = GetByteRange(Maximum);
		num = Utility::IntUtils::LeBytesTo32(rand, 0);
	} 
	while (num > Maximum);

	return num;
}

uint HCR::Next(uint Maximum, uint Minimum)
{
	CEXASSERT(Maximum != 0, "maximum can not be zero");
	CEXASSERT(Maximum > Minimum, "minimum can not be more than maximum");

	uint num = 0;
	while ((num = Next(Maximum)) < Minimum) {}
	return num;
}

ulong HCR::NextULong()
{
	return Utility::IntUtils::LeBytesTo64(GetBytes(8), 0);
}

ulong HCR::NextULong(ulong Maximum)
{
	CEXASSERT(Maximum != 0, "maximum can not be zero");

	std::vector<byte> rand;
	ulong num(0);

	do
	{
		rand = GetByteRange(Maximum);
		num = Utility::IntUtils::LeBytesTo64(rand, 0);
	} 
	while (num > Maximum);

	return num;
}

ulong HCR::NextULong(ulong Maximum, ulong Minimum)
{
	CEXASSERT(Maximum != 0, "maximum can not be zero");
	CEXASSERT(Maximum > Minimum, "minimum can not be more than maximum");

	ulong num = 0;
	while ((num = NextULong(Maximum)) < Minimum) {}
	return num;
}

void HCR::Reset()
{
	m_rngGenerator = new Drbg::HCG(m_digestType);

	if (m_rndSeed.size() != 0)
	{
		m_rngGenerator->Initialize(m_rndSeed);
	}
	else
	{
		Provider::IProvider* seedGen = Helper::ProviderFromName::GetInstance(m_pvdType == Providers::None ? Providers::CSP : m_pvdType);
		std::vector<byte> seed(m_rngGenerator->LegalKeySizes()[1].KeySize());
		seedGen->GetBytes(seed);
		delete seedGen;
		m_rngGenerator->Initialize(seed);
	}

	m_rngGenerator->Generate(m_rngBuffer);
	m_bufferIndex = 0;
}

//~~~Private Functions~~~//

std::vector<byte> HCR::GetBits(std::vector<byte> &Data, ulong Maximum)
{
	ulong val = 0;
	Utility::MemUtils::Copy<byte, ulong>(Data, 0, val, Data.size());
	ulong bits = Data.size() * 8;

	while (val > Maximum && bits != 0)
	{
		val >>= 1;
		bits--;
	}
	std::vector<byte> ret(sizeof(ulong));
	Utility::MemUtils::Copy<ulong, byte>(val, ret, 0, sizeof(ulong));

	return ret;
}

std::vector<byte> HCR::GetByteRange(ulong Maximum)
{
	std::vector<byte> data;

	if (Maximum < 256)
		data = GetBytes(1);
	else if (Maximum < 65536)
		data = GetBytes(2);
	else if (Maximum < 16777216)
		data = GetBytes(3);
	else if (Maximum < 4294967296)
		data = GetBytes(4);
	else if (Maximum < 1099511627776)
		data = GetBytes(5);
	else if (Maximum < 281474976710656)
		data = GetBytes(6);
	else if (Maximum < 72057594037927936)
		data = GetBytes(7);
	else
		data = GetBytes(8);

	return GetBits(data, Maximum);
}

uint HCR::GetMinimumSeedSize(Digests RngEngine)
{
	int ctrLen = 8;

	switch (RngEngine)
	{
	case Digests::Blake256:
		return ctrLen + 32;
	case Digests::Blake512:
		return ctrLen + 64;
	case Digests::Keccak256:
		return ctrLen + 136;
	case Digests::Keccak512:
		return ctrLen + 72;
	case Digests::SHA256:
		return ctrLen + 64;
	case Digests::SHA512:
		return ctrLen + 128;
	case Digests::Skein1024:
		return ctrLen + 128;
	case Digests::Skein256:
		return ctrLen + 32;
	case Digests::Skein512:
		return ctrLen + 64;
	default:
		return ctrLen + 128;
	}
}

NAMESPACE_PRNGEND
