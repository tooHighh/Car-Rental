#include "SHA256.h"
#include <sstream>
#include <iomanip>

SHA256::SHA256() : m_BlockLength(0), m_MessageLength(0)
{
    // Initialise the hash values. These words were obtained by taking the first
    // thirty-two bits of the fractional parts of the square roots of the first
    // eight prime numbers.
    m_H[0] = 0x6a09e667;
    m_H[1] = 0xbb67ae85;
    m_H[2] = 0x3c6ef372;
    m_H[3] = 0xa54ff53a;
    m_H[4] = 0x510e527f;
    m_H[5] = 0x9b05688c;
    m_H[6] = 0x1f83d9ab;
    m_H[7] = 0x5be0cd19;
}

void SHA256::update(const std::string &message)
{
    update(reinterpret_cast<const uint8_t *>(message.c_str()), message.size());
}

void SHA256::update(const uint8_t *data, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {

        m_DataChunk[m_BlockLength++] = data[i];

        if (m_BlockLength == 64)
        {
            // This block is finished so transform / process the block
            transform();

            // Track the total message length
            m_MessageLength += 512;

            // Reset the block length tracker ready for the next block
            m_BlockLength = 0;
        }
    }
}

std::uint8_t *SHA256::digest()
{
    uint8_t *hash = new uint8_t[32];
    pad();
    order(hash);

    // For each byte of the hash array
    /*for (size_t i = 0; i < 32; i++)
    {
      hash[i * 4] = ((m_H[i] >> 0 * 8) & 0xFF);
      hash[i * 4 + 1] = ((m_H[i] >> 1 * 8) & 0xFF);
      hash[i * 4 + 2] = ((m_H[i] >> 2 * 8) & 0xFF);
      hash[i * 4 + 3] = ((m_H[i] >> 3 * 8) & 0xFF);
    }*/

    return hash;
}

std::string SHA256::toString(const uint8_t *digest)
{
    std::stringstream ss;
    ss << std::setfill('0') << std::hex;

    for (uint8_t i = 0; i < 32; i++)
    {
        ss << std::setw(2) << (unsigned int)digest[i];
    }

    return ss.str();
}

void SHA256::pad()
{
    uint64_t i = m_BlockLength;
    uint8_t end = m_BlockLength < 56 ? 56 : 64;

    m_DataChunk[i++] = 0x80; // Append a bit 1
    while (i < end)
    {
        m_DataChunk[i++] = 0x00; // Pad with zeros
    }

    if (m_BlockLength >= 56)
    {
        transform();

        // Memset fills with chunks
        memset(m_DataChunk, 0, 56);
    }

    // Datachunk now has 56 bytes in and I need to append the original message size to the last 8 bytes. This take to 64 bytes and the final message can be transformed

    // m_MessageLength will always be a divisble of 512 i.e. a complete block, or 0
    m_MessageLength += m_BlockLength * 8;

    // Copy message length into the last 8 bytes of m_Datachunk - I couldn't work out how to memcpy from index 63 to 56
    m_DataChunk[63] = m_MessageLength;
    m_DataChunk[62] = m_MessageLength >> 8;
    m_DataChunk[61] = m_MessageLength >> 16;
    m_DataChunk[60] = m_MessageLength >> 24;
    m_DataChunk[59] = m_MessageLength >> 32;
    m_DataChunk[58] = m_MessageLength >> 40;
    m_DataChunk[57] = m_MessageLength >> 48;
    m_DataChunk[56] = m_MessageLength >> 56;

    // Perform a final transformation
    transform();
}

void SHA256::transform()
{
    uint32_t schedule[64], state[8], maj, ch, t1, t2; // Will hold the 64 32-bit words

    // Create the first 16 words from the 512-bit message block and add them to the schedule
    for (size_t i = 0, j = 0; i < 16; i++, j += 4)
    {
        schedule[i] = m_DataChunk[j] << 24 | m_DataChunk[j + 1] << 16 | m_DataChunk[j + 2] << 8 | m_DataChunk[j + 3];
    }

    // Create the additional 48 words by using the functions on the first 16 words
    for (size_t k = 16; k < 64; k++)
    {
        schedule[k] = ssigma1(schedule[k - 2]) + schedule[k - 7] + ssigma0(schedule[k - 15]) + schedule[k - 16];
    }

    // Initialize the eight working variables, a, b, c, d, e, f, g, and h, with the (i-1)st hash value
    for (size_t i = 0; i < 8; i++)
    {
        state[i] = m_H[i];
    }

    // Using t here like the specification
    for (size_t t = 0; t < 64; t++)
    {
        maj = majority(state[0], state[1], state[2]);
        ch = choose(state[4], state[5], state[6]);

        // Create the temp words
        t1 = state[7] + bsigma1(state[4]) + ch + K[t] + schedule[t];
        t2 = bsigma0(state[0]) + maj;

        state[7] = state[6];
        state[6] = state[5];
        state[5] = state[4];
        state[4] = state[3] + t1;
        state[3] = state[2];
        state[2] = state[1];
        state[1] = state[0];
        state[0] = t1 + t2;
    }

    // Take the initial hash values and add on the new compression values
    // If there is another message schedule to come, these new, compressed hash
    // values will be the new initial hash values
    for (uint8_t i = 0; i < 8; i++)
    {
        m_H[i] += state[i];
    }
}

void SHA256::order(uint8_t *hash)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
            hash[i + (j * 4)] = (m_H[j] >> (24 - i * 8)) & 0x000000ff;
        }
    }
}

std::uint32_t SHA256::choose(std::uint32_t x, std::uint32_t y, std::uint32_t z)
{
    return (x & y) ^ (~x & z);
}

std::uint32_t SHA256::majority(std::uint32_t x, std::uint32_t y, std::uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

std::uint32_t SHA256::bsigma0(std::uint32_t x)
{
    return ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22);
}

std::uint32_t SHA256::bsigma1(std::uint32_t x)
{
    return ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25);
}

std::uint32_t SHA256::ssigma0(std::uint32_t x)
{
    return ROTR(x, 7) ^ ROTR(x, 18) ^ (x >> 3);
}

std::uint32_t SHA256::ssigma1(std::uint32_t x)
{
    return ROTR(x, 17) ^ ROTR(x, 19) ^ (x >> 10);
}

std::uint32_t SHA256::ROTR(std::uint32_t x, std::uint32_t n)
{
    return (x >> n) | (x << (32 - n));
}

std::uint32_t SHA256::ROTL(std::uint32_t x, std::uint32_t n)
{
    return (x << n) | (x >> (32 - n));
}
