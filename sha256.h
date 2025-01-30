#pragma once
#include <cstdint>
#include <array>
#include <string>
#include <cstring>

/// <summary>
/// The hash functions specified herein are used to compute a message
/// digest for a message or data file that is provided as input.The
/// message or data file should be considered to be a bit string.The
/// length of the message is the number of bits in the message(the empty
/// message has length 0).
///
/// The details can be found here:
/// https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
/// </summary>
class SHA256
{
public:
    SHA256();
    void update(const std::string &message);
    void update(const std::uint8_t *data, std::size_t length);
    std::uint8_t *digest();
    static std::string toString(const uint8_t *digest);

private:
    /// <summary>
    /// The SHA works with 512 bit chunks of data. m_DataChunk is therefore a byte array with a len of 64 (and thus 512 bits)
    /// </summary>
    std::uint8_t m_DataChunk[64];

    /// <summary>
    /// The length of the current "processing" block
    /// </summary>
    std::uint32_t m_BlockLength;

    /// <summary>
    /// While block length will reset each 64 bytes, this message length keeps total track of the processed message
    /// </summary>
    std::uint64_t m_MessageLength;

    /// <summary>
    /// m_H will hold the hash values as the digest is processed
    /// </summary>
    uint32_t m_H[8];

    /// <summary>
    /// SHA-224 and SHA-256 use the same sequence of sixty-four constant 32-bit words,
    /// These words represent the first thirty - two bits of the fractional parts of
    /// the cube roots of the first sixty - four prime numbers.In hex, these constant words are (from left
    /// to right)
    /// </summary>
    static constexpr std::array<uint32_t, 64> K = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

private:
    /*
    SHA-256 uses six logical functions, where each function
    operates on 32-bit words, which are represented as x, y, and z.  The
    result of each function is a new 32-bit word.
    */

    /// <summary>
    /// Pad works on the final block of data. It is called when the digest is requested
    /// </summary>
    void pad();

    /// <summary>
    /// Works on the 512bit block of data, separating it out in 16 32-bit words. It then
    /// create an additional 48 32-bit words using the sha functions. Transform is only called
    /// on appropriately padded blocks
    /// </summary>
    void transform();

    void order(uint8_t *hash);

    /// <summary>
    /// CHOOSE(x, y, z) = (x AND y) XOR ( (NOT x) AND z)
    /// </summary>
    /// <param name="x"></param>
    /// <param name="y"></param>
    /// <param name="z"></param>
    /// <returns></returns>
    std::uint32_t choose(std::uint32_t x, std::uint32_t y, std::uint32_t z);

    /// <summary>
    /// MAJORITY(x, y, z) = (x AND y) XOR (x AND z) XOR (y AND z)
    /// </summary>
    /// <param name="x"></param>
    /// <param name="y"></param>
    /// <param name="z"></param>
    /// <returns></returns>
    std::uint32_t majority(std::uint32_t x, std::uint32_t y, std::uint32_t z);

    /// <summary>
    /// BSIG0(x) = ROTR^2(x) XOR ROTR^13(x) XOR ROTR^22(x)
    /// </summary>
    /// <param name="x"></param>
    /// <returns></returns>
    std::uint32_t bsigma0(std::uint32_t x);

    /// <summary>
    /// BSIG1(x) = ROTR^6(x) XOR ROTR^11(x) XOR ROTR^25(x)
    /// </summary>
    /// <param name="x"></param>
    /// <returns></returns>
    std::uint32_t bsigma1(std::uint32_t x);

    /// <summary>
    /// SSIG0(x) = ROTR^7(x) XOR ROTR^18(x) XOR SHR^3(x)
    /// 1. Rotate right the word by 7
    /// 2. Rotate right the word by 18 and XOR with step 1
    /// 3. Shift the word right by 3 and XOR with step 2
    /// </summary>
    /// <param name="x">32 bit word</param>
    /// <returns></returns>
    std::uint32_t ssigma0(std::uint32_t x);

    /// <summary>
    /// SSIG1(x) = ROTR^17(x) XOR ROTR^19(x) XOR SHR^10(x)
    /// </summary>
    /// <param name="x"></param>
    /// <returns></returns>
    std::uint32_t ssigma1(std::uint32_t x);

    /// <summary>
    /// The rotate right function performs a bitshift right operation - (x>>n) OR (x<<(w-n))
    /// </summary>
    /// <param name="x">A 32 bit word</param>
    /// <param name="n"></param>
    /// <returns></returns>
    std::uint32_t ROTR(std::uint32_t x, std::uint32_t n);

    /// <summary>
    /// The rotate right function performs a bitshift right operation - (x>>n) OR (x<<(w-n))
    /// </summary>
    /// <param name="x"></param>
    /// <param name="n"></param>
    /// <returns></returns>
    std::uint32_t ROTL(std::uint32_t x, std::uint32_t n);
};
