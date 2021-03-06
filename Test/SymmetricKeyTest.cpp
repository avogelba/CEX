#include "SymmetricKeyTest.h"
#include "../CEX/CSP.h"
#include "../CEX/MemoryStream.h"
#include "../CEX/SymmetricKeyGenerator.h"

namespace Test
{
	using namespace Key::Symmetric;
	using namespace IO;

	const std::string SymmetricKeyTest::DESCRIPTION = "SymmetricKey test; checks constructors, access, and serialization.";
	const std::string SymmetricKeyTest::FAILURE = "FAILURE! ";
	const std::string SymmetricKeyTest::SUCCESS = "SUCCESS! All SymmetricKey tests have executed succesfully.";

	SymmetricKeyTest::SymmetricKeyTest()
		:
		m_progressEvent()
	{
	}

	SymmetricKeyTest::~SymmetricKeyTest()
	{
	}

	std::string SymmetricKeyTest::Run()
	{
		try
		{
			CheckInit();
			OnProgress(std::string("SymmetricKeyTest: Passed initialization tests.."));
			CheckAccess();
			OnProgress(std::string("SymmetricKeyTest: Passed output comparison tests.."));
			CompareSerial();
			OnProgress(std::string("SymmetricKeyTest: Passed key serialization tests.."));

			return SUCCESS;
		}
		catch (std::exception const &ex)
		{
			throw TestException(std::string(FAILURE + " : " + ex.what()));
		}
		catch (...)
		{
			throw TestException(std::string(FAILURE + " : Unknown Error"));
		}
	}

	void SymmetricKeyTest::CheckAccess()
	{
		Provider::CSP rnd;
		std::vector<byte> key = rnd.GetBytes(32);
		std::vector<byte> iv = rnd.GetBytes(16);
		std::vector<byte> info = rnd.GetBytes(64);

		// test symmetric key properties
		SymmetricKey symKey(key, iv, info);

		if (symKey.Key() != key)
			throw TestException("CheckAccess: The symmetric key is invalid!");
		if (symKey.Nonce() != iv)
			throw TestException("CheckAccess: The symmetric nonce is invalid!");
		if (symKey.Info() != info)
			throw TestException("CheckAccess: The symmetric info is invalid!");

		// test secure key properties
		SymmetricSecureKey secKey(key, iv, info);

		if (secKey.Key() != key)
			throw TestException("CheckAccess: The secure key is invalid!");
		if (secKey.Nonce() != iv)
			throw TestException("CheckAccess: The secure nonce is invalid!");
		if (secKey.Info() != info)
			throw TestException("CheckAccess: The secure info is invalid!");
	}

	void SymmetricKeyTest::CheckInit()
	{
		Provider::CSP rnd;
		std::vector<byte> key = rnd.GetBytes(32);
		std::vector<byte> iv = rnd.GetBytes(16);
		std::vector<byte> info = rnd.GetBytes(64);

		// test symmetric key constructors
		SymmetricKey symKey1(key, iv, info);
		if (symKey1.Key() != key)
			throw TestException("CheckInit: The symmetric key is invalid!");
		if (symKey1.Nonce() != iv)
			throw TestException("CheckInit: The symmetric nonce is invalid!");
		if (symKey1.Info() != info)
			throw TestException("CheckInit: The symmetric info is invalid!");
		// 2 params
		SymmetricKey symKey2(key, iv);
		if (symKey2.Key() != key)
			throw TestException("CheckInit: The symmetric key is invalid!");
		if (symKey2.Nonce() != iv)
			throw TestException("CheckInit: The symmetric nonce is invalid!");
		// key only
		SymmetricKey symKey3(key);
		if (symKey3.Key() != key)
			throw TestException("CheckInit: The symmetric key is invalid!");

		// test secure key constructors
		SymmetricSecureKey secKey1(key, iv, info);
		if (secKey1.Key() != key)
			throw TestException("CheckInit: The secure key is invalid!");
		if (secKey1.Nonce() != iv)
			throw TestException("CheckInit: The secure nonce is invalid!");
		if (secKey1.Info() != info)
			throw TestException("CheckInit: The secure info is invalid!");
		// 2 params
		SymmetricSecureKey secKey2(key, iv);
		if (secKey2.Key() != key)
			throw TestException("CheckInit: The secure key is invalid!");
		if (secKey2.Nonce() != iv)
			throw TestException("CheckInit: The secure nonce is invalid!");
		// key only
		SymmetricSecureKey secKey3(key);
		if (secKey3.Key() != key)
			throw TestException("CheckInit: The secure key is invalid!");
	}

	void SymmetricKeyTest::CompareSerial()
	{
		SymmetricKeySize keySize(64, 16, 64);
		SymmetricKeyGenerator keyGen;

		// test symmetric key serialization
		SymmetricKey symKey1 = keyGen.GetSymmetricKey(keySize);
		MemoryStream* keyStr = SymmetricKey::Serialize(symKey1);
		SymmetricKey* symKey2 = SymmetricKey::DeSerialize(*keyStr);
		if (!symKey1.Equals(*symKey2))
			throw TestException("CompareSerial: The symmetric key serialization has failed!");

		// test secure key serialization
		SymmetricSecureKey secKey1 = keyGen.GetSecureKey(keySize);
		MemoryStream* secStr = SymmetricSecureKey::Serialize(secKey1);
		SymmetricSecureKey* secKey2 = SymmetricSecureKey::DeSerialize(*secStr);
		if (!secKey1.Equals(*secKey2))
			throw TestException("CompareSerial: The secure key serialization has failed!");
	}

	void SymmetricKeyTest::OnProgress(std::string Data)
	{
		m_progressEvent(Data);
	}
}