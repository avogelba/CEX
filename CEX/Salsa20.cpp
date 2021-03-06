#include "Salsa20.h"
#include "Salsa.h"
#include "MemUtils.h"
#if defined(__AVX2__)
#	include "UInt256.h"
#elif defined(__AVX__)
#	include "UInt128.h"
#endif

NAMESPACE_STREAM

const std::string Salsa20::CLASS_NAME("Salsa");
const std::string Salsa20::SIGMA_INFO("expand 32-byte k");
const std::string Salsa20::TAU_INFO("expand 16-byte k");

//~~~Properties~~~//

const size_t Salsa20::BlockSize()
{
	return BLOCK_SIZE;
}

std::vector<byte> &Salsa20::DistributionCode()
{
	return m_dstCode;
}

const StreamCiphers Salsa20::Enumeral()
{
	return StreamCiphers::Salsa20;
}

const bool Salsa20::IsInitialized()
{
	return m_isInitialized;
}

const bool Salsa20::IsParallel()
{
	return m_parallelProfile.IsParallel();
}

const std::vector<SymmetricKeySize> &Salsa20::LegalKeySizes()
{
	return m_legalKeySizes;
}

const std::vector<size_t> &Salsa20::LegalRounds()
{
	return m_legalRounds;
}

const std::string Salsa20::Name()
{
	return CLASS_NAME + Utility::IntUtils::ToString(m_rndCount);
}

const size_t Salsa20::ParallelBlockSize()
{
	return m_parallelProfile.ParallelBlockSize();
}

ParallelOptions &Salsa20::ParallelProfile()
{
	return m_parallelProfile;
}

const size_t Salsa20::Rounds()
{
	return m_rndCount;
}

//~~~Constructor~~~//

Salsa20::Salsa20(size_t Rounds)
	:
	m_ctrVector(2, 0),
	m_isDestroyed(false),
	m_isInitialized(false),
	m_legalKeySizes(0),
	m_parallelProfile(BLOCK_SIZE, true, STATE_PRECACHED, true),
	m_rndCount(Rounds),
	m_wrkState(14, 0)
{
	if (Rounds == 0 || (Rounds & 1) != 0)
		throw CryptoSymmetricCipherException("Salsa20:Ctor", "Rounds must be a positive even number!");
	if (Rounds < MIN_ROUNDS || Rounds > MAX_ROUNDS)
		throw CryptoSymmetricCipherException("Salsa20:Ctor", "Rounds must be between 8 and 30!");

	Scope();
}

Salsa20::~Salsa20()
{
	Destroy();
}

//~~~Public Functions~~~//

void Salsa20::Destroy()
{
	if (!m_isDestroyed)
	{
		m_isDestroyed = true;
		m_isInitialized = false;
		m_parallelProfile.Reset();
		m_rndCount = 0;
		IntUtils::ClearVector(m_ctrVector);
		IntUtils::ClearVector(m_wrkState);
		IntUtils::ClearVector(m_dstCode);
		IntUtils::ClearVector(m_legalKeySizes);
		IntUtils::ClearVector(m_legalRounds);
	}
}

void Salsa20::Initialize(ISymmetricKey &KeyParams)
{
	// recheck params
	Scope();

	if (KeyParams.Nonce().size() != 8)
		throw CryptoSymmetricCipherException("Salsa20:Initialize", "Requires exactly 8 bytes of Nonce!");
	if (KeyParams.Key().size() != 16 && KeyParams.Key().size() != 32)
		throw CryptoSymmetricCipherException("Salsa20:Initialize", "Key must be 16 or 32 bytes!");
	if (IsParallel() && m_parallelProfile.ParallelBlockSize() < m_parallelProfile.ParallelMinimumSize() || m_parallelProfile.ParallelBlockSize() > m_parallelProfile.ParallelMaximumSize())
		throw CryptoSymmetricCipherException("Salsa20:Initialize", "The parallel block size is out of bounds!");
	if (IsParallel() && m_parallelProfile.ParallelBlockSize() % m_parallelProfile.ParallelMinimumSize() != 0)
		throw CryptoSymmetricCipherException("Salsa20:Initialize", "The parallel block size must be evenly aligned to the ParallelMinimumSize!");

	if (KeyParams.Info().size() != 0)
	{
		// custom code
		m_dstCode = KeyParams.Info();
	}
	else
	{
		if (KeyParams.Key().size() == 32)
			m_dstCode.assign(SIGMA_INFO.begin(), SIGMA_INFO.end());
		else
			m_dstCode.assign(TAU_INFO.begin(), TAU_INFO.end());
	}

	Reset();
	Expand(KeyParams.Key(), KeyParams.Nonce());
	m_isInitialized = true;
}

void Salsa20::ParallelMaxDegree(size_t Degree)
{
	if (Degree == 0)
		throw CryptoSymmetricCipherException("Salsa20::ParallelMaxDegree", "Parallel degree can not be zero!");
	if (Degree % 2 != 0)
		throw CryptoSymmetricCipherException("Salsa20::ParallelMaxDegree", "Parallel degree must be an even number!");
	if (Degree > m_parallelProfile.ProcessorCount())
		throw CryptoSymmetricCipherException("Salsa20::ParallelMaxDegree", "Parallel degree can not exceed processor count!");

	m_parallelProfile.SetMaxDegree(Degree);
}

void Salsa20::Reset()
{
	m_ctrVector[0] = 0;
	m_ctrVector[1] = 0;
}

void Salsa20::TransformBlock(const std::vector<byte> &Input, std::vector<byte> &Output)
{
	Process(Input, 0, Output, 0, BLOCK_SIZE);
}

void Salsa20::TransformBlock(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset)
{
	Process(Input, InOffset, Output, OutOffset, BLOCK_SIZE);
}

void Salsa20::Transform(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset, const size_t Length)
{
	Process(Input, InOffset, Output, OutOffset, Length);
}

//~~~Private Functions~~~//

void Salsa20::Expand(const std::vector<byte> &Key, const std::vector<byte> &Iv)
{
	if (Key.size() == 32)
	{
		m_wrkState[0] = IntUtils::LeBytesTo32(m_dstCode, 0);
		m_wrkState[1] = IntUtils::LeBytesTo32(Key, 0);
		m_wrkState[2] = IntUtils::LeBytesTo32(Key, 4);
		m_wrkState[3] = IntUtils::LeBytesTo32(Key, 8);
		m_wrkState[4] = IntUtils::LeBytesTo32(Key, 12);
		m_wrkState[5] = IntUtils::LeBytesTo32(m_dstCode, 4);
		m_wrkState[6] = IntUtils::LeBytesTo32(Iv, 0);
		m_wrkState[7] = IntUtils::LeBytesTo32(Iv, 4);
		m_wrkState[8] = IntUtils::LeBytesTo32(m_dstCode, 8);
		m_wrkState[9] = IntUtils::LeBytesTo32(Key, 16);
		m_wrkState[10] = IntUtils::LeBytesTo32(Key, 20);
		m_wrkState[11] = IntUtils::LeBytesTo32(Key, 24);
		m_wrkState[12] = IntUtils::LeBytesTo32(Key, 28);
		m_wrkState[13] = IntUtils::LeBytesTo32(m_dstCode, 12);
	}
	else
	{
		m_wrkState[0] = IntUtils::LeBytesTo32(m_dstCode, 0);
		m_wrkState[1] = IntUtils::LeBytesTo32(Key, 0);
		m_wrkState[2] = IntUtils::LeBytesTo32(Key, 4);
		m_wrkState[3] = IntUtils::LeBytesTo32(Key, 8);
		m_wrkState[4] = IntUtils::LeBytesTo32(Key, 12);
		m_wrkState[5] = IntUtils::LeBytesTo32(m_dstCode, 4);
		m_wrkState[6] = IntUtils::LeBytesTo32(Iv, 0);
		m_wrkState[7] = IntUtils::LeBytesTo32(Iv, 4);
		m_wrkState[8] = IntUtils::LeBytesTo32(m_dstCode, 8);
		m_wrkState[9] = IntUtils::LeBytesTo32(Key, 0);
		m_wrkState[10] = IntUtils::LeBytesTo32(Key, 4);
		m_wrkState[11] = IntUtils::LeBytesTo32(Key, 8);
		m_wrkState[12] = IntUtils::LeBytesTo32(Key, 12);
		m_wrkState[13] = IntUtils::LeBytesTo32(m_dstCode, 12);
	}
}

void Salsa20::Generate(std::vector<byte> &Output, const size_t OutOffset, std::vector<uint> &Counter, const size_t Length)
{
	size_t ctr = 0;

#if defined(__AVX2__)
	const size_t AVX2BLK = 8 * BLOCK_SIZE;

	if (Length >= AVX2BLK)
	{
		size_t paln = Length - (Length % AVX2BLK);
		std::vector<uint> ctrBlk(16);

		// process 8 blocks (uses avx if available)
		while (ctr != paln)
		{
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 0, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 8, 4);
			IntUtils::LeIncrement32(Counter);
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 1, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 9, 4);
			IntUtils::LeIncrement32(Counter);
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 2, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 10, 4);
			IntUtils::LeIncrement32(Counter);
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 3, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 11, 4);
			IntUtils::LeIncrement32(Counter);
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 4, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 12, 4);
			IntUtils::LeIncrement32(Counter);
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 5, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 13, 4);
			IntUtils::LeIncrement32(Counter);
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 6, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 14, 4);
			IntUtils::LeIncrement32(Counter);
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 7, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 15, 4);
			IntUtils::LeIncrement32(Counter);
			Salsa::SalsaTransformW<Numeric::UInt256>(Output, OutOffset + ctr, ctrBlk, m_wrkState, m_rndCount);
			ctr += AVX2BLK;
		}
	}
#elif defined(__AVX__)
	const size_t AVXBLK = 4 * BLOCK_SIZE;

	if (Length >= AVXBLK)
	{
		size_t paln = Length - (Length % AVXBLK);
		std::vector<uint> ctrBlk(8);

		// process 4 blocks (uses sse intrinsics if available)
		while (ctr != paln)
		{
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 0, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 4, 4);
			IntUtils::LeIncrement32(Counter);
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 1, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 5, 4);
			IntUtils::LeIncrement32(Counter);
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 2, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 6, 4);
			IntUtils::LeIncrement32(Counter);
			Utility::MemUtils::Copy<uint>(Counter, 0, ctrBlk, 3, 4);
			Utility::MemUtils::Copy<uint>(Counter, 1, ctrBlk, 7, 4);
			IntUtils::LeIncrement32(Counter);
			Salsa::SalsaTransformW<Numeric::UInt128>(Output, OutOffset + ctr, ctrBlk, m_wrkState, m_rndCount);
			ctr += AVXBLK;
		}
	}
#endif

	const size_t ALNSZE = Length - (Length % BLOCK_SIZE);
	while (ctr != ALNSZE)
	{
		Salsa::SalsaTransform512(Output, OutOffset + ctr, Counter, m_wrkState, m_rndCount);
		IntUtils::LeIncrement32(Counter);
		ctr += BLOCK_SIZE;
	}

	if (ctr != Length)
	{
		std::vector<byte> outputBlock(BLOCK_SIZE, 0);
		Salsa::SalsaTransform512(outputBlock, 0, Counter, m_wrkState, m_rndCount);
		const size_t FNLSZE = Length % BLOCK_SIZE;
		Utility::MemUtils::Copy<byte>(outputBlock, 0, Output, OutOffset + (Length - FNLSZE), FNLSZE);
		IntUtils::LeIncrement32(Counter);
	}
}

void Salsa20::Process(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset, const size_t Length)
{
	const size_t PRCSZE = (Length >= Input.size() - InOffset) && Length >= Output.size() - OutOffset ? IntUtils::Min(Input.size() - InOffset, Output.size() - OutOffset) : Length;

	if (!m_parallelProfile.IsParallel() || PRCSZE < m_parallelProfile.ParallelMinimumSize())
	{
		// generate random
		Generate(Output, OutOffset, m_ctrVector, PRCSZE);
		// output is input xor random
		const size_t ALNSZE = PRCSZE - (PRCSZE % BLOCK_SIZE);

		if (ALNSZE != 0)
			Utility::MemUtils::XorBlock<byte>(Input, InOffset, Output, OutOffset, ALNSZE);

		// get the remaining bytes
		if (ALNSZE != PRCSZE)
		{
			for (size_t i = ALNSZE; i < PRCSZE; ++i)
				Output[i + OutOffset] ^= Input[i + InOffset];
		}
	}
	else
	{
		// parallel CTR processing //
		const size_t CNKSZE = (PRCSZE / BLOCK_SIZE / m_parallelProfile.ParallelMaxDegree()) * BLOCK_SIZE;
		const size_t RNDSZE = CNKSZE * m_parallelProfile.ParallelMaxDegree();
		const size_t CTRLEN = (CNKSZE / BLOCK_SIZE);
		std::vector<uint> tmpCtr(m_ctrVector.size());

		Utility::ParallelUtils::ParallelFor(0, m_parallelProfile.ParallelMaxDegree(), [this, &Input, InOffset, &Output, OutOffset, &tmpCtr, CNKSZE, CTRLEN](size_t i)
		{
			// thread level counter
			std::vector<uint> thdCtr(m_ctrVector.size());
			// offset counter by chunk size / block size
			IntUtils::LeIncrease32(m_ctrVector, thdCtr, CTRLEN * i);
			// create random at offset position
			this->Generate(Output, OutOffset + (i * CNKSZE), thdCtr, CNKSZE);
			// xor with input at offset
			Utility::MemUtils::XorBlock<byte>(Input, InOffset + (i * CNKSZE), Output, OutOffset + (i * CNKSZE), CNKSZE);
			// store last counter
			if (i == m_parallelProfile.ParallelMaxDegree() - 1)
				Utility::MemUtils::Copy<uint>(thdCtr, 0, tmpCtr, 0, CTR_SIZE);
		});

		// copy last counter to class variable
		Utility::MemUtils::Copy<uint>(tmpCtr, 0, m_ctrVector, 0, CTR_SIZE);

		// last block processing
		if (RNDSZE < PRCSZE)
		{
			const size_t FNLSZE = PRCSZE % RNDSZE;
			Generate(Output, RNDSZE, m_ctrVector, FNLSZE);

			for (size_t i = 0; i < FNLSZE; ++i)
				Output[i + OutOffset + RNDSZE] ^= (byte)(Input[i + InOffset + RNDSZE]);
		}
	}
}

void Salsa20::Scope()
{
	m_legalKeySizes.resize(2);
	m_legalKeySizes[0] = SymmetricKeySize(16, 8, 0);
	m_legalKeySizes[1] = SymmetricKeySize(32, 8, 0);
	m_legalRounds = { 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 };
}

NAMESPACE_STREAMEND