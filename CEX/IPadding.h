#ifndef _CEX_IPADDING_H
#define _CEX_IPADDING_H

#include "CexDomain.h"
#include "CryptoPaddingException.h"
#include "PaddingModes.h"

NAMESPACE_PADDING

using Enumeration::PaddingModes;
using Exception::CryptoPaddingException;

/// <summary>
/// Padding Mode Interface
/// </summary>
class IPadding
{
public:
	//~~~Constructor~~~//

	/// <summary>
	/// CTor: Instantiate this class
	/// </summary>
	IPadding() {}

	/// <summary>
	/// Destructor
	/// </summary>
	virtual ~IPadding() {}

	//~~~Properties~~~//

	/// <summary>
	/// Get: The padding modes type name
	/// </summary>
	virtual const PaddingModes Enumeral() = 0;

	/// <summary>
	/// Get: The padding modes class name
	/// </summary>
	virtual const std::string Name() = 0;

	//~~~Public Functions~~~//

	/// <summary>
	/// Add padding to input array
	/// </summary>
	///
	/// <param name="Input">Array to modify</param>
	/// <param name="Offset">Offset into array</param>
	///
	/// <returns>Length of padding</returns>
	virtual size_t AddPadding(std::vector<byte> &Input, size_t Offset) = 0;

	/// <summary>
	/// Get the length of padding in an array
	/// </summary>
	///
	/// <param name="Input">Padded array of bytes</param>
	///
	/// <returns>Length of padding</returns>
	virtual size_t GetPaddingLength(const std::vector<byte> &Input) = 0;

	/// <summary>
	/// Get the length of padding in an array
	/// </summary>
	///
	/// <param name="Input">Padded array of bytes</param>
	/// <param name="Offset">Offset into array</param>
	///
	/// <returns>Length of padding</returns>
	virtual size_t GetPaddingLength(const std::vector<byte> &Input, size_t Offset) = 0;
};

NAMESPACE_PADDINGEND
#endif

