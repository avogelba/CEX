#include "GCM.h"
#include "IntUtils.h"
#include "MemUtils.h"
#include "SymmetricKey.h"

NAMESPACE_MODE

const std::string GCM::CLASS_NAME("GCM");

//~~~Properties~~~//


//~~~Properties~~~//

bool &GCM::AutoIncrement()
{
	return m_autoIncrement;
}

const size_t GCM::BlockSize()
{
	return BLOCK_SIZE;
}

const BlockCiphers GCM::CipherType()
{
	return m_cipherType;
}

IBlockCipher* GCM::Engine()
{
	return m_cipherMode.Engine();
}

const CipherModes GCM::Enumeral()
{
	return CipherModes::GCM;
}

const bool GCM::IsEncryption()
{
	return m_isEncryption;
}

const bool GCM::IsInitialized()
{
	return m_isInitialized;
}

const bool GCM::IsParallel()
{
	return m_parallelProfile.IsParallel();
}

const std::vector<SymmetricKeySize> &GCM::LegalKeySizes()
{
	return m_legalKeySizes;
}

const size_t GCM::MaxTagSize()
{
	return BLOCK_SIZE;
}

const size_t GCM::MinTagSize()
{
	return MIN_TAGSIZE;
}

const std::string GCM::Name()
{
	return CLASS_NAME + "-" + m_cipherMode.Engine()->Name();
}

const size_t GCM::ParallelBlockSize()
{
	return m_parallelProfile.ParallelBlockSize();
}

ParallelOptions &GCM::ParallelProfile()
{
	return m_cipherMode.ParallelProfile();
}

bool &GCM::PreserveAD()
{
	return m_aadPreserve;
}

const std::vector<byte> GCM::Tag()
{
	if (!m_isFinalized)
		throw CryptoCipherModeException("GCM:Tag", "The cipher mode has not been finalized!");

	return m_msgTag;
}

//~~~Constructor~~~//

GCM::GCM(BlockCiphers CipherType)
	:
	m_aadData(0),
	m_aadLoaded(false),
	m_aadPreserve(false),
	m_aadSize(0),
	m_autoIncrement(false),
	m_checkSum(BLOCK_SIZE),
	m_cipherMode(CipherType),
	m_cipherType(CipherType),
	m_destroyEngine(true),
	m_gcmHash(0),
	m_gcmNonce(0),
	m_gcmVector(0),
	m_isDestroyed(false),
	m_isEncryption(false),
	m_isFinalized(false),
	m_isInitialized(false),
	m_legalKeySizes(0),
	m_msgSize(0),
	m_msgTag(BLOCK_SIZE),
	m_parallelProfile(BLOCK_SIZE, m_cipherMode.ParallelProfile().IsParallel(), m_cipherMode.ParallelProfile().ParallelBlockSize(),
		m_cipherMode.ParallelProfile().ParallelMaxDegree(), true, m_cipherMode.Engine()->StateCacheSize(), true)
{
	Scope();
}

GCM::GCM(IBlockCipher* Cipher)
	:
	m_aadData(0),
	m_aadLoaded(false),
	m_aadPreserve(false),
	m_aadSize(0),
	m_autoIncrement(false),
	m_checkSum(BLOCK_SIZE),
	m_cipherMode(Cipher != 0 ? Cipher : throw CryptoCipherModeException("GCM:CTor", "The Cipher can not be null!")),
	m_cipherType(Cipher->Enumeral()),
	m_destroyEngine(false),
	m_gcmHash(0),
	m_gcmNonce(0),
	m_gcmVector(0),
	m_isDestroyed(false),
	m_isEncryption(false),
	m_isFinalized(false),
	m_isInitialized(false),
	m_legalKeySizes(0),
	m_msgSize(0),
	m_msgTag(BLOCK_SIZE),
	m_parallelProfile(BLOCK_SIZE, m_cipherMode.ParallelProfile().IsParallel(), m_cipherMode.ParallelProfile().ParallelBlockSize(),
		m_cipherMode.ParallelProfile().ParallelMaxDegree(), true, m_cipherMode.Engine()->StateCacheSize(), true)
{
	Scope();
}

GCM::~GCM()
{
	Destroy();
}

//~~~Public Functions~~~//

void GCM::DecryptBlock(const std::vector<byte> &Input, std::vector<byte> &Output)
{
	Decrypt128(Input, 0, Output, 0);
}

void GCM::DecryptBlock(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset)
{
	Decrypt128(Input, InOffset, Output, OutOffset);
}

void GCM::Destroy()
{
	m_aadLoaded = false;
	m_aadPreserve = false;
	m_aadSize = 0;
	m_autoIncrement = false;
	m_cipherType = BlockCiphers::None;
	m_isDestroyed = true;
	m_isEncryption = false;
	m_isFinalized = false;
	m_isInitialized = false;
	m_msgSize = 0;
	m_parallelProfile.Reset();

	try
	{
		m_gcmHash->Reset();

		Utility::IntUtils::ClearVector(m_aadData);
		Utility::IntUtils::ClearVector(m_gcmNonce);
		Utility::IntUtils::ClearVector(m_gcmVector);
		Utility::IntUtils::ClearVector(m_legalKeySizes);
		Utility::IntUtils::ClearVector(m_msgTag);
		Utility::IntUtils::ClearVector(m_checkSum);

		if (m_destroyEngine)
		{
			m_destroyEngine = false;

			if (m_cipherMode.IsInitialized())
				m_cipherMode.Destroy();
		}
	}
	catch (std::exception& ex)
	{
		throw CryptoCipherModeException("GCM:Destroy", "Could not clear all variables!", std::string(ex.what()));
	}
}

void GCM::EncryptBlock(const std::vector<byte> &Input, std::vector<byte> &Output)
{
	Encrypt128(Input, 0, Output, 0);
}

void GCM::EncryptBlock(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset)
{
	Encrypt128(Input, InOffset, Output, OutOffset);
}

void GCM::Finalize(std::vector<byte> &Output, const size_t Offset, const size_t Length)
{
	if (!m_isInitialized)
		throw CryptoCipherModeException("GCM:Finalize", "The cipher mode has not been initialized!");
	if (Length < MIN_TAGSIZE || Length > BLOCK_SIZE)
		throw CryptoCipherModeException("GCM:Finalize", "The length must be minimum of 12 and maximum of MAC code size!");

	CalculateMac();
	Utility::MemUtils::Copy<byte>(m_msgTag, 0, Output, Offset, Length);
}

void GCM::Initialize(bool Encryption, ISymmetricKey &KeyParams)
{
	Scope();

	if (KeyParams.Nonce().size() < 8)
		throw CryptoSymmetricCipherException("GCM:Initialize", "Requires a nonce of minimum 10 bytes in length!");
	if (IsParallel() && ParallelBlockSize() < m_parallelProfile.ParallelMinimumSize() || ParallelBlockSize() > m_parallelProfile.ParallelMaximumSize())
		throw CryptoSymmetricCipherException("GCM:Initialize", "The parallel block size is out of bounds!");
	if (IsParallel() && ParallelBlockSize() % m_parallelProfile.ParallelMinimumSize() != 0)
		throw CryptoSymmetricCipherException("GCM:Initialize", "The parallel block size must be evenly aligned to the ParallelMinimumSize!");

	if (KeyParams.Key().size() == 0)
	{
		if (KeyParams.Nonce() == m_gcmNonce)
			throw CryptoSymmetricCipherException("GCM:Initialize", "The nonce can not be zeroised or repeating!");
		if (!m_cipherMode.IsInitialized())
			throw CryptoSymmetricCipherException("GCM:Initialize", "First initialization requires a key and nonce!");
	}
	else
	{
		if (!SymmetricKeySize::Contains(LegalKeySizes(), KeyParams.Key().size()))
			throw CryptoSymmetricCipherException("GCM:Initialize", "Invalid key size! Key must be one of the LegalKeySizes() in length.");

		// key the cipher and generate the hash key
		m_cipherMode.Engine()->Initialize(true, KeyParams);
		std::vector<byte> tmpH(BLOCK_SIZE);
		const std::vector<byte> ZEROES(BLOCK_SIZE);
		m_cipherMode.Engine()->Transform(ZEROES, 0, tmpH, 0);

		std::vector<ulong> gKey = //505,807
		{
			Utility::IntUtils::BeBytesTo64(tmpH, 0),
			Utility::IntUtils::BeBytesTo64(tmpH, 8)
		};

		m_gcmHash = new Mac::GHASH(gKey);
		m_gcmKey = KeyParams.Key();
	}

	m_isEncryption = Encryption;
	m_gcmNonce = KeyParams.Nonce();
	m_gcmVector = m_gcmNonce;

	if (m_gcmVector.size() == 12)
	{
		m_gcmVector.resize(16);
		m_gcmVector[15] = 1;
	}
	else
	{
		std::vector<byte> tmpN(BLOCK_SIZE);
		m_gcmHash->ProcessSegment(m_gcmVector, 0, tmpN, m_gcmVector.size());
		m_gcmHash->FinalizeBlock(tmpN, 0, m_gcmVector.size());
		m_gcmVector = tmpN;
	}

	m_cipherMode.Initialize(true, Key::Symmetric::SymmetricKey(m_gcmKey, m_gcmVector));
	std::vector<byte> tmpN(BLOCK_SIZE);
	m_cipherMode.Transform(tmpN, 0, m_gcmVector, 0, BLOCK_SIZE);

	if (m_isFinalized)
	{
		Utility::MemUtils::Clear<byte>(m_msgTag, 0, m_msgTag.size());
		m_isFinalized = false;
	}

	m_isInitialized = true;
}

void GCM::ParallelMaxDegree(size_t Degree)
{
	if (Degree == 0)
		throw CryptoCipherModeException("GCM:ParallelMaxDegree", "Parallel degree can not be zero!");
	if (Degree % 2 != 0)
		throw CryptoCipherModeException("GCM:ParallelMaxDegree", "Parallel degree must be an even number!");
	if (Degree > m_parallelProfile.ProcessorCount())
		throw CryptoCipherModeException("GCM:ParallelMaxDegree", "Parallel degree can not exceed processor count!");

	m_parallelProfile.SetMaxDegree(Degree);
}

void GCM::SetAssociatedData(const std::vector<byte> &Input, const size_t Offset, const size_t Length)
{
	if (!m_isInitialized)
		throw CryptoSymmetricCipherException("GCM:SetAssociatedData", "The cipher has not been initialized!");
	if (m_aadLoaded)
		throw CryptoSymmetricCipherException("GCM:SetAssociatedData", "The associated data has already been set!");

	m_aadData.resize(Length);
	Utility::MemUtils::Copy<byte>(Input, Offset, m_aadData, 0, Length);
	m_gcmHash->ProcessSegment(Input, Offset, m_checkSum, Length);
	m_aadSize = Length;
	m_aadLoaded = true;
}

void GCM::Transform(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset, const size_t Length)
{
	CEXASSERT(m_isInitialized, "The cipher mode has not been initialized!");
	CEXASSERT(Utility::IntUtils::Min(Input.size() - InOffset, Output.size() - OutOffset) >= Length, "The data arrays are smaller than the the block-size!");

	if (m_isEncryption)
	{
		m_cipherMode.Transform(Input, InOffset, Output, OutOffset, Length);
		m_gcmHash->Update(Output, OutOffset, m_checkSum, Length);
	}
	else
	{
		m_gcmHash->Update(Input, InOffset, m_checkSum, Length);
		m_cipherMode.Transform(Input, InOffset, Output, OutOffset, Length);
	}

	m_msgSize += Length;
}

bool GCM::Verify(const std::vector<byte> &Input, const size_t Offset, const size_t Length)
{
	if (m_isEncryption)
		throw CryptoCipherModeException("GCM:Verify", "The cipher mode has not been initialized for decryption!");
	if (!m_isInitialized && !m_isFinalized)
		throw CryptoCipherModeException("GCM:Verify", "The cipher mode has not been initialized!");
	if (Length < MIN_TAGSIZE || Length > BLOCK_SIZE)
		throw CryptoCipherModeException("GCM:Verify", "The length must be minimum of 12 and maximum of MAC code size!");

	if (!m_isFinalized)
		CalculateMac();

	return Utility::IntUtils::Compare<byte>(m_msgTag, 0, Input, Offset, Length);
}

void GCM::CalculateMac()
{
	m_gcmHash->FinalizeBlock(m_checkSum, m_aadSize, m_msgSize);
	Utility::MemUtils::XorBlock<byte>(m_gcmVector, 0, m_checkSum, 0, BLOCK_SIZE);
	Utility::MemUtils::COPY128<byte, byte>(m_checkSum, 0, m_msgTag, 0);
	Reset();

	if (m_autoIncrement)
	{
		std::vector<byte> tmpN = m_gcmNonce;
		Utility::IntUtils::BeIncrement8(tmpN);
		std::vector<byte> zero(0);
		Initialize(m_isEncryption, Key::Symmetric::SymmetricKey(zero, tmpN));

		if (m_aadPreserve)
			m_gcmHash->ProcessSegment(m_aadData, 0, m_checkSum, m_aadData.size());
	}

	m_isFinalized = true;
}

void GCM::Decrypt128(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset)
{
	CEXASSERT(m_isInitialized, "The cipher mode has not been initialized!");
	CEXASSERT(Utility::IntUtils::Min(Input.size() - InOffset, Output.size() - OutOffset) >= BLOCK_SIZE, "The data arrays are smaller than the the block-size!");

	m_gcmHash->Update(Input, InOffset, m_checkSum, BLOCK_SIZE);
	m_cipherMode.EncryptBlock(Input, InOffset, Output, OutOffset);
	m_msgSize += BLOCK_SIZE;
}

void GCM::Encrypt128(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset)
{
	CEXASSERT(m_isInitialized, "The cipher mode has not been initialized!");
	CEXASSERT(Utility::IntUtils::Min(Input.size() - InOffset, Output.size() - OutOffset) >= BLOCK_SIZE, "The data arrays are smaller than the the block-size!");

	m_cipherMode.EncryptBlock(Input, InOffset, Output, OutOffset);
	m_gcmHash->Update(Input, InOffset, m_checkSum, BLOCK_SIZE);
	m_msgSize += BLOCK_SIZE;
}

void GCM::Reset()
{
	if (!m_aadPreserve)
	{
		if (m_aadSize != 0)
			Utility::MemUtils::Clear<byte>(m_aadData, 0, m_aadData.size());

		m_aadLoaded = false;
		m_aadSize = 0;
	}

	m_gcmHash->Reset();
	m_isInitialized = false;
	Utility::MemUtils::Clear<byte>(m_gcmVector, 0, m_gcmVector.size());
	Utility::MemUtils::Clear<byte>(m_checkSum, 0, m_checkSum.size());
	m_msgSize = 0;
}

void GCM::Scope()
{
	if (m_legalKeySizes.size() == 0)
		m_legalKeySizes = m_cipherMode.LegalKeySizes();

	if (!m_cipherMode.ParallelProfile().IsDefault())
		m_cipherMode.ParallelProfile().Calculate(m_parallelProfile.IsParallel(), m_cipherMode.ParallelProfile().ParallelBlockSize(), m_cipherMode.ParallelProfile().ParallelMaxDegree());
}

NAMESPACE_MODEEND