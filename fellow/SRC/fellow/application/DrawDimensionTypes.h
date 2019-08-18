#pragma once

#include <sstream>

template <typename T> struct Rect
{
public:
  T Left;
  T Top;
  T Right;
  T Bottom;

  T GetWidth() const
  {
    return Right - Left;
  }

  T GetHeight() const
  {
    return Bottom - Top;
  }

  std::string ToString()
  {
    std::ostringstream oss;
    oss << "(" << Left << ", " << Top << "), (" << Right << ", " << Bottom << ")";
    return oss.str();  
  }

  bool operator==(const Rect<T> &other)
  {
    return other.Left == Left && other.Top == Top && other.Right == Right && other.Bottom == Bottom;
  }

  Rect(const T left, const T top, const T right, const T bottom) noexcept : Left(left), Top(top), Right(right), Bottom(bottom)
  {
  }

  Rect() noexcept : Left(0), Top(0), Right(0), Bottom(0)
  {
  }
};

typedef Rect<unsigned int> RectPixels;
typedef Rect<unsigned int> RectShresi;
typedef Rect<float> RectFloat;

template <typename T> struct Dimensions
{
  T Width;
  T Height;

  std::string ToString()
  {
    std::ostringstream oss;
    oss << "(" << Width << ", " << Height << ")";
    return oss.str();
  }

  bool operator==(const Dimensions<T> &other)
  {
    return other.Width == Width && other.Height == Height;
  }

  Dimensions<T>() noexcept : Width(0), Height(0)
  {
  }

  Dimensions<T>(T width, T height) noexcept : Width(width), Height(height)
  {
  }
};

typedef Dimensions<unsigned int> DimensionsUInt;
typedef Dimensions<float> DimensionsFloat;
