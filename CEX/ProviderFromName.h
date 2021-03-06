#ifndef _CEX_PROVIDERFROMNAME_H
#define _CEX_PROVIDERFROMNAME_H

#include "CexDomain.h"
#include "CryptoException.h"
#include "IProvider.h"

NAMESPACE_HELPER

using Provider::IProvider;
using Enumeration::Providers;

/// <summary>
/// Get a seed generator instance from it's enumeration name
/// </summary>
class ProviderFromName
{
public:
	/// <summary>
	/// Get a Seed Generator instance with default initialization parameters
	/// </summary>
	/// 
	/// <param name="ProviderType">The entropy providers enumeration name</param>
	/// 
	/// <returns>An initialized entropy provider</returns>
	/// 
	/// <exception cref="Exception::CryptoException">Thrown if the enumeration name is not supported</exception>
	static IProvider* GetInstance(Providers ProviderType);
};

NAMESPACE_HELPEREND
#endif