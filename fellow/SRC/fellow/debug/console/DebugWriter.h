#pragma once

#include <stdint.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

class DebugWriter
{
private:
  std::ostringstream _os;

public:
  DebugWriter &Char(uint8_t v);
  DebugWriter &Chars(uint32_t v);
  DebugWriter &Hex8(uint16_t v);
  DebugWriter &Hex12(uint16_t v);
  DebugWriter &Hex16(uint16_t v);
  DebugWriter &Hex20(uint32_t v);
  DebugWriter &Hex24(uint32_t v);
  DebugWriter &Hex32(uint32_t v);
  DebugWriter &Hex32Vector(const std::vector<uint32_t> &array);
  DebugWriter &StringLeft(const std::string &v, size_t maxSize);
  DebugWriter &String(const std::string &v);
  DebugWriter &Endl();

  DebugWriter &NumberHex12(uint16_t v);

  std::string GetStreamString();

  template <typename T> DebugWriter &Number(T v)
  {
    _os << std::dec << v;
    return *this;
  }

  template <typename T> DebugWriter &NumberBit(T v, int bitNumber)
  {
    _os << std::dec << ((v >> bitNumber) & 1);
    return *this;
  }

  template <typename T> DebugWriter &NumberBits(T v, int bitNumber, int bitCount)
  {
    T mask = (~((T)0)) >> ((sizeof(T) * CHAR_BIT) - bitCount);
    _os << std::dec << ((v >> bitNumber) & 1);
    return *this;
  }

  template <typename T> DebugWriter &Hex(T v, unsigned int valueDigits)
  {
    _os << std::right << std::hex << std::setw(valueDigits) << std::setfill('0') << v;
    return *this;
  }

  template <class T> size_t GetMaxLength(const T &i, const std::string &(*get)(const T &item))
  {
    return get(i).length();
  }

  template <class T> size_t GetMaxLength(const std::vector<T> &container, const std::string &(*get)(const T &item))
  {
    size_t maxLength = 0;

    for (const auto &i : container)
    {
      size_t length = GetMaxLength(i, get);
      if (length > maxLength)
      {
        maxLength = length;
      }
    }

    return maxLength;
  }
};
