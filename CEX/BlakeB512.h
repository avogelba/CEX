// The GPL version 3 License (GPLv3)
// 
// Copyright (c) 2016 vtdev.com
// This file is part of the CEX Cryptographic library.
// 
// This program is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.If not, see <http://www.gnu.org/licenses/>.
//
// 
// Principal Algorithms:
// An implementation of Blake2, designed by Jean-Philippe Aumasson, Samuel Neves, Zooko Wilcox-O�Hearn, and Christian Winnerlein. 
// Blake2 whitepaper <a href="https://blake2.net/blake2.pdf">BLAKE2: simpler, smaller, fast as MD5</a>.
// 
// Implementation Details:
// An implementation of the Blake2B and Blake2BP digests with a 512 bit digest output size.
// Based on the Blake2 Github projects by Samuel Neves and Christian Winnerlein.
// Blake2: https://github.com/BLAKE2/BLAKE2
//
// Written by John Underhill, June 19, 2016
// Contact: develop@vtdev.com

#ifndef _CEX_BLAKE2B512_H
#define _CEX_BLAKE2B512_H

#include "Blake2Params.h"
#include "IDigest.h"

NAMESPACE_DIGEST

/// <summary>
/// An implementation of the Blake2B and Blake2BP digests with a 512 bit digest output size
/// </summary> 
/// 
/// <example>
/// <description>Example using the ComputeHash method:</description>
/// <para>Use the ComputeHash method for small to medium data sizes</para>
/// <code>
/// BlakeB512 dgt;
/// std:vector&lt;uint8_t&gt; hash(dgt.DigestSize(), 0);
/// // compute a hash
/// dgt.ComputeHash(input, hash);
/// </code>
/// </example>
///
/// <example>
/// <description>Use the BlockUpdate method for large data sizes:</description>
/// <code>
/// BlakeB512 dgt;
/// std:vector&lt;uint8_t&gt; hash(dgt.DigestSize(), 0);
/// int64_t len = (int64_t)input.size();
///
/// // update blocks
/// while (len > dgt.DigestSize())
/// {
///		dgt.BlockUpdate(input, offset, len);
///		offset += dgt.DigestSize();
///		len -= dgt.DigestSize();
/// }
///
/// if (len > 0)
///		dgt.BlockUpdate(input, offset, len);
///
/// dgt.DoFinal(hash, 0);
/// </code>
/// </example>
/// 
/// <remarks>
/// <description>Implementation Notes:</description>
/// <list type="bullet">
/// <item><description>Algorithm is selected through the constructor (2B or 2BP), parallel version is selected through either the Parallel flag, or via the Blake2Params ThreadCount() configuration parameter.</description></item>
/// <item><description>Parallel and sequential algorithms (Blake2B or Blake2BP) produce different digest outputs, this is expected.</description></item>
/// <item><description>Sequential Block size 128 bytes, (1024 bits), but smaller or larger blocks can be processed, for best performance, align message input to a multiple of the internal block size.</description></item>
/// <item><description>Parallel Block input size to the BlockUpdate function should be aligned to a multiple of ParallelMinimumSize() for best performance.</description></item>
/// <item><description>Best performance for parallel mode is to use a large input block size to minimize parallel loop creation cost, block size should be in a range of 32KiB to 25MiB.</description></item>
/// <item><description>The number of threads used in parallel mode can be user defined through the Blake2Params->ThreadCount property to any even number of threads; note that hash output value will change with threadcount.</description></item>
/// <item><description>Digest output size is fixed at 64 bytes, (512 bits).</description></item>
/// <item><description>The <see cref="ComputeHash(uint8_t[])"/> method wraps the <see cref="BlockUpdate(uint8_t[], size_t, size_t)"/> and DoFinal methods</description>/></item>
/// <item><description>The <see cref="DoFinal(uint8_t[], size_t)"/> method resets the internal state.</description></item>
/// <item><description>Optional intrinsics are runtime enabled automatically based on cpu support.</description></item>
/// <item><description>SIMD implementation requires compilation with SSSE3 or higher.</description></item>
/// </list>
/// 
/// <description>Guiding Publications:</description>
/// <list type="number">
/// <item><description>Blake2 <a href="https://blake2.net/">Homepage</a>.</description></item>
/// <item><description>Blake2 on <a href="https://github.com/BLAKE2/BLAKE2">Github</a>.</description></item>
/// <item><description>Blake2 whitepaper <a href="https://blake2.net/blake2.pdf">BLAKE2: simpler, smaller, fast as MD5</a>.</description></item>
/// <item><description>NIST <a href="https://131002.net/blake">SHA3 Proposal Blake</a>.</description></item>
/// <item><description>NIST <a href="http://nvlpubs.nist.gov/nistpubs/ir/2012/NIST.IR.7896.pdf">SHA3: Third-Round Report</a> of the SHA-3 Cryptographic Hash Algorithm Competition.</description></item>
/// <item><description>SHA3 Submission in C: <a href="https://131002.net/blake/blake_ref.c">blake_ref.c</a>.</description></item>
/// </list>
/// </remarks>
class BlakeB512 : public IDigest
{
private:

	static const uint32_t BLOCK_SIZE = 128;
	static const uint32_t CHAIN_SIZE = 8;
	static const uint32_t COUNTER_SIZE = 2;
	static const uint32_t PARALLEL_DEG = 4;
	const uint32_t DEF_LEAFSIZE = 16384;
	const size_t DIGEST_SIZE = 64;
	const uint32_t FLAG_SIZE = 2;
	const uint32_t MAX_PRLBLOCK = 5120000;
	const uint32_t MIN_PRLBLOCK = 512;
	const size_t ROUND_COUNT = 12;
	const uint64_t ULL_MAX = 18446744073709551615;

	struct Blake2bState
	{
		std::vector<uint64_t> H;
		std::vector<uint64_t> T;
		std::vector<uint64_t> F;

		Blake2bState()
			:
			F(2, 0),
			H(8, 0),
			T(2, 0)
		{
		}

		void Reset()
		{
			if (F.size() > 0)
				memset(&F[0], 0, F.size() * sizeof(uint64_t));
			if (H.size() > 0)
				memset(&H[0], 0, H.size() * sizeof(uint64_t));
			if (T.size() > 0)
				memset(&T[0], 0, T.size() * sizeof(uint64_t));
		}
	};

	std::vector<uint64_t> m_cIV;
	bool m_hasSSE;
	bool m_isDestroyed;
	bool m_isParallel;
	uint32_t m_leafSize;
	std::vector<uint8_t> m_msgBuffer;
	size_t m_msgLength;
	size_t m_parallelBlockSize;
	std::vector<Blake2bState> m_State;
	std::vector<uint64_t> m_treeConfig;
	bool m_treeDestroy;
	Blake2Params m_treeParams;
	size_t m_minParallel;

public:

	BlakeB512(const BlakeB512&) = delete;
	BlakeB512& operator=(const BlakeB512&) = delete;
	BlakeB512& operator=(BlakeB512&&) = delete;

	//~~~Properties~~~//

	/// <summary>
	/// Get: The Digests internal blocksize in bytes
	/// </summary>
	virtual size_t BlockSize() { return BLOCK_SIZE; }

	/// <summary>
	/// Get: Size of returned digest in bytes
	/// </summary>
	virtual size_t DigestSize() { return DIGEST_SIZE; }

	/// <summary>
	/// Get: The digests class name
	/// </summary>
	virtual const std::string Name()
	{
		if (m_isParallel)
			return "BlakeBP512";
		else
			return "BlakeBP512";
	}

	/// <summary>
	/// Get: The digests type name
	/// </summary>
	virtual Digests Enumeral() 
	{ 
		if (m_isParallel)
			return Digests::BlakeBP512;
		else
			return Digests::BlakeB512;
	}

	/// <summary>
	/// Get/Set: Parallel block size; set either automatically, or through the constructors Blake2Params parameter. Must be a multiple of <see cref="ParallelMinimumSize"/>.
	/// </summary>
	size_t &ParallelBlockSize() { return m_parallelBlockSize; }

	/// <summary>
	/// Get: Maximum input size with parallel processing
	/// </summary>
	const size_t ParallelMaximumSize() { return MAX_PRLBLOCK; }

	/// <summary>
	/// Get: The smallest parallel block size. Parallel blocks must be a multiple of this size.
	/// </summary>
	const size_t ParallelMinimumSize() { return m_minParallel; }

	//~~~Constructor~~~//

	/// <summary>
	/// Initialize the class as either the 2B or 2BP.
	/// <para>Initialize as either the parallel version Blake2BP, or sequential Blake2B.</para>
	/// </summary>
	/// 
	/// <param name="Parallel">Setting the Parallel flag to true, instantiates the Blake2BP variant.</param>
	explicit BlakeB512(bool Parallel = false)
		:
		m_cIV({ 0x6A09E667F3BCC908UL, 0xBB67AE8584CAA73BUL, 0x3C6EF372FE94F82BUL, 0xA54FF53A5F1D36F1UL, 0x510E527FADE682D1UL, 0x9B05688C2B3E6C1FUL, 0x1F83D9ABFB41BD6BUL, 0x5BE0CD19137E2179UL }),
		m_hasSSE(false),
		m_isDestroyed(false),
		m_isParallel(Parallel),
		m_leafSize(Parallel ? DEF_LEAFSIZE : BLOCK_SIZE),
		m_minParallel(0),
		m_msgBuffer(Parallel ? 2 * PARALLEL_DEG * BLOCK_SIZE : BLOCK_SIZE),
		m_msgLength(0),
		m_State(Parallel ? PARALLEL_DEG : 1),
		m_treeConfig(8),
		m_treeDestroy(true)
	{
		// intrinsics support switch
		Detect();

		if (m_isParallel)
		{
			// sets defaults of depth 2, fanout 4, 4 threads
			m_treeParams = Blake2Params((uint8_t)DIGEST_SIZE, 0, 4, 2, 0, 0, 0, (uint8_t)DIGEST_SIZE, 4);
			// minimum block size
			m_minParallel = PARALLEL_DEG * BLOCK_SIZE;
			// default parallel input block expected is Pn * 16384 bytes
			m_parallelBlockSize = m_leafSize * PARALLEL_DEG;
			// initialize the leaf nodes
			Reset();
		}
		else
		{
			// default depth 1, fanout 1, leaf length unlimited
			m_treeParams = Blake2Params((uint8_t)DIGEST_SIZE, 0, 1, 1, 0, 0, 0, 0, 0);
			Initialize(m_treeParams, m_State[0]);
		}
	}

	/// <summary>
	/// Initialize the class with a Blake2Params structure.
	/// <para>The parameters structure allows for tuning of the internal configuration string,
	/// and changing the number of threads used by the parallel mechanism (ThreadCount).
	/// If the ThreadCount is greater than 1, parallel mode (Blake2BP) is instantiated.
	/// The default threadcount is 4, changing from the default will produce a different output hash code.</para>
	/// </summary>
	/// 
	/// <param name="Params">The Blake2Params structure, containing the tree configuration settings.</param>
	explicit BlakeB512(Blake2Params &Params)
		:
		m_hasSSE(false),
		m_isDestroyed(false),
		m_isParallel(false),
		m_leafSize(BLOCK_SIZE),
		m_minParallel(0),
		m_msgBuffer(Params.ParallelDegree() > 0 ? 2 * Params.ParallelDegree() * BLOCK_SIZE : BLOCK_SIZE),
		m_msgLength(0),
		m_State(Params.ParallelDegree() > 0 ? Params.ParallelDegree() : 1),
		m_treeConfig(CHAIN_SIZE),
		m_treeDestroy(false),
		m_treeParams(Params)
	{
		m_isParallel = m_treeParams.ParallelDegree() > 1;
		m_cIV =
		{
			0x6A09E667F3BCC908UL, 0xBB67AE8584CAA73BUL, 0x3C6EF372FE94F82BUL, 0xA54FF53A5F1D36F1UL,
			0x510E527FADE682D1UL, 0x9B05688C2B3E6C1FUL, 0x1F83D9ABFB41BD6BUL, 0x5BE0CD19137E2179UL
		};

		// intrinsics support switch
		Detect();

		if (m_isParallel)
		{
			if (Params.LeafLength() != 0 && (Params.LeafLength() < BLOCK_SIZE || Params.LeafLength() % BLOCK_SIZE != 0))
				throw CryptoDigestException("BlakeBP512:Ctor", "The LeafLength parameter is invalid! Must be evenly divisible by digest block size.");
			if (Params.ParallelDegree() < 2 || Params.ParallelDegree() % 2 != 0)
				throw CryptoDigestException("BlakeBP512:Ctor", "The ParallelDegree parameter is invalid! Must be an even number greater than 1.");

			m_minParallel = m_treeParams.ParallelDegree() * BLOCK_SIZE;
			m_leafSize = Params.LeafLength() == 0 ? DEF_LEAFSIZE : Params.LeafLength();
			// set parallel block size as Pn * leaf size 
			m_parallelBlockSize = Params.ParallelDegree() * m_leafSize;
			// initialize leafs
			Reset();
		}
		else
		{
			// fixed at defaults for sequential; depth 1, fanout 1, leaf length unlimited
			m_treeParams = Blake2Params((uint8_t)DIGEST_SIZE, 0, 1, 1, 0, 0, 0, 0, 0);
			Initialize(m_treeParams, m_State[0]);
		}
	}

	/// <summary>
	/// Finalize objects
	/// </summary>
	virtual ~BlakeB512()
	{
		Destroy();
	}

	//~~~Public Methods~~~//

	/// <summary>
	/// Update the message buffer
	/// </summary>
	///
	/// <remarks>
	/// <para>For best performance in parallel mode, use block sizes that are evenly divisible by ParallelMinimumSize() to reduce caching.
	/// Block size for parallel mode should be in a range of minimum 32KiB to 25MiB, larger block sizes reduce the impact of parallel loop creation.</para>
	/// </remarks>
	/// 
	/// <param name="Input">The Input message data</param>
	/// <param name="InOffset">The starting offset within the Input array</param>
	/// <param name="Length">The amount of data to process in bytes</param>
	virtual void BlockUpdate(const std::vector<uint8_t> &Input, size_t InOffset, size_t Length);

	/// <summary>
	/// Process the message data and return the Hash value
	/// </summary>
	/// 
	/// <param name="Input">The message input data</param>
	/// <param name="Output">The hash value output array</param>
	virtual void ComputeHash(const std::vector<uint8_t> &Input, std::vector<uint8_t> &Output);

	/// <summary>
	/// Release all resources associated with the object
	/// </summary>
	void Destroy();

	/// <summary>
	/// Perform final processing and return the hash value
	/// </summary>
	/// 
	/// <param name="Output">The Hash output value array</param>
	/// <param name="OutOffset">The starting offset within the Output array</param>
	/// 
	/// <returns>Size of Hash value</returns>
	///
	/// <exception cref="CryptoDigestException">Thrown if the output buffer is too short</exception>
	virtual size_t DoFinal(std::vector<uint8_t> &Output, const size_t OutOffset);

	/// <summary>
	/// Initialize the digest as a counter based DRBG
	/// </summary>
	/// 
	/// <param name="MacKey">The input key parameters; the input Key must be a minimum of 64 bytes, maximum of combined Key, Salt, and Info, must be 128 bytes or less</param>
	/// <param name="Output">The psuedo random output</param>
	/// 
	/// <returns>The number of bytes generated</returns>
	size_t Generate(ISymmetricKey &MacKey, std::vector<uint8_t> &Output);

	/// <summary>
	/// Initialize the digest as a MAC code generator
	/// </summary>
	/// 
	/// <param name="MacKey">The input key parameters. 
	/// <para>The input Key must be a maximum size of 64 bytes, and a minimum size of 32 bytes. 
	/// If either the Salt or Info parameters are used, their size must be 16 bytes.
	/// The maximum combined size of Key, Salt, and Info, must be 128 bytes or less.</para></param>
	virtual void LoadMacKey(ISymmetricKey &MacKey);

	/// <summary>
	/// Reset the internal state to sequential defaults
	/// </summary>
	virtual void Reset();

	/// <summary>
	/// Update the message digest with a single byte
	/// </summary>
	/// 
	/// <param name="Input">Input message byte</param>
	virtual void Update(uint8_t Input);

private:
	void Detect();
	void Increase(Blake2bState &State, uint64_t Length);
	void Increment(std::vector<uint8_t> &Counter);
	void Initialize(Blake2Params &Params, Blake2bState &State);
	void ProcessBlock(const std::vector<uint8_t> &Input, size_t InOffset, Blake2bState &State, size_t Length);
	void ProcessLeaf(const std::vector<uint8_t> &Input, size_t InOffset, Blake2bState &State, uint64_t Length);
};

NAMESPACE_DIGESTEND
#endif