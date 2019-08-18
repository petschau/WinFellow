#include "fellow/debug/console/DebugWriter.h"
#include <iomanip>

using namespace std;

DebugWriter &DebugWriter::Char(uint8_t v)
{
  if (v >= 0x20 && v < 127)
    _os << v;
  else
    _os << '.';

  return *this;
}

DebugWriter &DebugWriter::Chars(uint32_t v)
{
  return Char((uint8_t)(v >> 24)).Char((uint8_t)(v >> 16)).Char((uint8_t)(v >> 8)).Char((uint8_t)v);
}

DebugWriter &DebugWriter::Hex8(uint16_t v)
{
  return Hex(v, 2);
}

DebugWriter &DebugWriter::Hex12(uint16_t v)
{
  return Hex(v, 3);
}

DebugWriter &DebugWriter::Hex16(uint16_t v)
{
  return Hex(v, 4);
}

DebugWriter &DebugWriter::Hex20(uint32_t v)
{
  return Hex(v, 5);
}

DebugWriter &DebugWriter::Hex24(uint32_t v)
{
  return Hex(v, 6);
}

DebugWriter &DebugWriter::Hex32(uint32_t v)
{
  return Hex(v, 8);
}

DebugWriter &DebugWriter::Hex32Vector(const vector<uint32_t> &array)
{
  bool addSpace = false;
  for (auto value : array)
  {
    if (addSpace)
      Char(' ');
    else
      addSpace = true;

    Hex32(value);
  }

  return *this;
}

DebugWriter &DebugWriter::NumberHex12(uint16_t v)
{
  Number(v).Char('/').Hex12(v);
  return *this;
}

DebugWriter &DebugWriter::StringLeft(const string &v, size_t maxSize)
{
  _os << left << setw(maxSize) << setfill(' ') << v;
  return *this;
}

DebugWriter &DebugWriter::String(const string &v)
{
  _os << v;
  return *this;
}

DebugWriter &DebugWriter::Endl()
{
  _os << endl;
  return *this;
}

string DebugWriter::GetStreamString()
{
  return _os.str();
}
