#include "SymmetricKey.h"
#include "IntUtils.h"
#include "StreamReader.h"
#include "StreamWriter.h"

NAMESPACE_SYMMETRICKEY

//~~~Public Properties~~~//

const std::vector<byte> SymmetricKey::Info() 
{ 
	return m_info;
}

const std::vector<byte> SymmetricKey::Key()
{
	return m_key;
}

const SymmetricKeySize SymmetricKey::KeySizes() 
{ 
	return m_keySizes; 
}

const std::vector<byte> SymmetricKey::Nonce() 
{ 
	return m_nonce;
}

//~~~Constructors~~~//

SymmetricKey::SymmetricKey()
	:
	m_info(0),
	m_isDestroyed(false),
	m_key(0),
	m_keySizes(0, 0, 0),
	m_nonce(0)
{
}

SymmetricKey::SymmetricKey(const std::vector<byte> &Key)
	:
	m_info(0),
	m_isDestroyed(false),
	m_key(Key),
	m_keySizes(Key.size(), 0, 0),
	m_nonce(0)

{
	if (Key.size() == 0)
		throw CryptoProcessingException("SymmetricKey:Ctor", "The key can not be zero sized!");
}

SymmetricKey::SymmetricKey(const std::vector<byte> &Key, const std::vector<byte> &Nonce)
	:
	m_info(0),
	m_isDestroyed(false),
	m_key(Key),
	m_keySizes(Key.size(), Nonce.size(), 0),
	m_nonce(Nonce)

{
	if (Key.size() == 0 && Nonce.size() == 0)
		throw CryptoProcessingException("SymmetricKey:Ctor", "The key and nonce can not both be be zero sized!");
}

SymmetricKey::SymmetricKey(const std::vector<byte> &Key, const std::vector<byte> &Nonce, const std::vector<byte> &Info)
	:
	m_info(Info),
	m_isDestroyed(false),
	m_key(Key),
	m_keySizes(Key.size(), Nonce.size(), Info.size()),
	m_nonce(Nonce)
{
	if (Key.size() == 0 && Nonce.size() == 0 && Info.size() == 0)
		throw CryptoProcessingException("SymmetricKey:Ctor", "The key, nonce, and info can not all be be zero sized!");
}

SymmetricKey::~SymmetricKey()
{
	Destroy();
}

//~~~Public Functions~~~//

SymmetricKey* SymmetricKey::Clone()
{
	return new SymmetricKey(m_key, m_nonce, m_info);
}

void SymmetricKey::Destroy()
{
	if (!m_isDestroyed)
	{
		if (m_key.size() > 0)
			Utility::IntUtils::ClearVector(m_key);
		if (m_nonce.size() > 0)
			Utility::IntUtils::ClearVector(m_nonce);
		if (m_info.size() > 0)
			Utility::IntUtils::ClearVector(m_info);

		m_isDestroyed = true;
	}
}

SymmetricKey* SymmetricKey::DeSerialize(const MemoryStream &KeyStream)
{
	IO::StreamReader reader(KeyStream);
	short kLen = reader.ReadInt<short>();
	short nLen = reader.ReadInt<short>();
	short iLen = reader.ReadInt<short>();
	std::vector<byte> key;
	std::vector<byte> nonce;
	std::vector<byte> info;

	if (kLen > 0)
		key = reader.ReadBytes(kLen);
	if (nLen > 0)
		nonce = reader.ReadBytes(nLen);
	if (iLen > 0)
		info = reader.ReadBytes(iLen);

	return new SymmetricKey(key, nonce, info);
}

bool SymmetricKey::Equals(ISymmetricKey &Obj)
{
	return (Obj.Key() == Key() && Obj.Nonce() == Nonce() && Obj.Info() == Info());
}

MemoryStream* SymmetricKey::Serialize(SymmetricKey &KeyObj)
{
	size_t kLen = KeyObj.Key().size();
	size_t nLen = KeyObj.Nonce().size();
	size_t iLen = KeyObj.Info().size();
	size_t tLen = 6 + kLen + nLen + iLen;

	IO::StreamWriter writer(tLen);
	writer.Write(static_cast<ushort>(kLen));
	writer.Write(static_cast<ushort>(nLen));
	writer.Write(static_cast<ushort>(iLen));

	if (kLen != 0)
		writer.Write(KeyObj.Key(), 0, kLen);
	if (nLen != 0)
		writer.Write(KeyObj.Nonce(), 0, nLen);
	if (iLen != 0)
		writer.Write(KeyObj.Info(), 0, iLen);

	IO::MemoryStream* strm = writer.GetStream();
	strm->Seek(0, IO::SeekOrigin::Begin);

	return strm;
}

NAMESPACE_SYMMETRICKEYEND