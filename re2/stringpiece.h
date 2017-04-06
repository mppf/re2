// Copyright 2001-2010 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef RE2_STRINGPIECE_H_
#define RE2_STRINGPIECE_H_

// A string-like object that points to a sized piece of memory.
//
// Functions or methods may use const StringPiece& parameters to accept either
// a "const char*" or a "string" value that will be implicitly converted to
// a StringPiece.  The implicit conversion means that it is often appropriate
// to include this .h file in other files rather than forward-declaring
// StringPiece as would be appropriate for most other Google classes.
//
// Systematic usage of StringPiece is encouraged as it will reduce unnecessary
// conversions from "const char*" to "string" and back again.
//
//
// Arghh!  I wish C++ literals were "string".

#include <string.h>
#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <string>


namespace re2 {

class StringPiece {
 private:
  const char*   ptr_;
  int           length_;

 public:
  // This for supporting other StringPiece-like things
  // (e.g. match on a FILE*)
  typedef const char* ptr_type;
  typedef const char* ptr_rd_type;
  static bool can_discard(long diff) { return false; }
  static int discard_check_period() { return 0; }
  void discard(bool match, ptr_type match_start, ptr_type match_end, ptr_type min_cap) const { }
  static const char* null_ptr() { return NULL; }

  // We provide non-explicit singleton constructors so users can pass
  // in a "const char*" or a "string" wherever a "StringPiece" is
  // expected.
  StringPiece() : ptr_(NULL), length_(0) { }
  StringPiece(const char* str)
    : ptr_(str), length_((str == NULL) ? 0 : static_cast<int>(strlen(str))) { }
  StringPiece(const std::string& str)
    : ptr_(str.data()), length_(static_cast<int>(str.size())) { }
  StringPiece(const char* offset, int len) : ptr_(offset), length_(len) { }

  // data() may return a pointer to a buffer with embedded NULs, and the
  // returned buffer may or may not be null terminated.  Therefore it is
  // typically a mistake to pass data() to a routine that expects a NUL
  // terminated string.
  const char* data() const { return ptr_; }
  int size() const { return length_; }
  int length() const { return length_; }
  bool empty() const { return length_ == 0; }

  void clear() { ptr_ = NULL; length_ = 0; }
  void set(const char* data, int len) { ptr_ = data; length_ = len; }
  void set_ptr_end(const char* data, const char* end) {
    set(data, static_cast<int>(end - data));
  }
  void set(const char* str) {
    ptr_ = str;
    if (str != NULL)
      length_ = static_cast<int>(strlen(str));
    else
      length_ = 0;
  }
  void set(const void* data, int len) {
    ptr_ = reinterpret_cast<const char*>(data);
    length_ = len;
  }

  char operator[](int i) const { return ptr_[i]; }

  void remove_prefix(int n) {
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(int n) {
    length_ -= n;
  }

  int compare(const StringPiece& x) const {
    int r = memcmp(ptr_, x.ptr_, std::min(length_, x.length_));
    if (r == 0) {
      if (length_ < x.length_) r = -1;
      else if (length_ > x.length_) r = +1;
    }
    return r;
  }

  std::string as_string() const {
    return std::string(data(), size());
  }
  // We also define ToString() here, since many other string-like
  // interfaces name the routine that converts to a C++ string
  // "ToString", and it's confusing to have the method that does that
  // for a StringPiece be called "as_string()".  We also leave the
  // "as_string()" method defined here for existing code.
  std::string ToString() const {
    return std::string(data(), size());
  }

  void CopyToString(std::string* target) const;
  void AppendToString(std::string* target) const;

  // Does "this" start with "x"
  bool starts_with(const StringPiece& x) const {
    return ((length_ >= x.length_) &&
            (memcmp(ptr_, x.ptr_, x.length_) == 0));
  }

  // Does "this" end with "x"
  bool ends_with(const StringPiece& x) const {
    return ((length_ >= x.length_) &&
            (memcmp(ptr_ + (length_-x.length_), x.ptr_, x.length_) == 0));
  }

  // Find the position where c occurs (ie memchr)
  // uses begin, end pointers
  const char* find_c(const char* s, const char* end, int c) const
  {
    return reinterpret_cast<const char*>(memchr(s, c, end - s));
  }

  // standard STL container boilerplate
  typedef char value_type;
  typedef const char* pointer;
  typedef const char& reference;
  typedef const char& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  static const size_type npos;
  typedef const char* const_iterator;
  typedef const char* iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  iterator begin() const { return ptr_; }
  // Also define begin_reading
  iterator begin_reading() const { return ptr_; }
  iterator end() const { return ptr_ + length_; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(ptr_ + length_);
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(ptr_);
  }
  // STLS says return size_type, but Google says return int
  int max_size() const { return length_; }
  int capacity() const { return length_; }

  size_type copy(char* buf, size_type n, size_type pos = 0) const;

  bool contains(StringPiece s) const;

  size_type find(const StringPiece& s, size_type pos = 0) const;
  size_type find(char c, size_type pos = 0) const;
  size_type rfind(const StringPiece& s, size_type pos = npos) const;
  size_type rfind(char c, size_type pos = npos) const;

  StringPiece substr(size_type pos, size_type n = npos) const;

  static bool _equal(const StringPiece&, const StringPiece&);
};

inline bool operator==(const StringPiece& x, const StringPiece& y) {
  return StringPiece::_equal(x, y);
}

inline bool operator!=(const StringPiece& x, const StringPiece& y) {
  return !(x == y);
}

inline bool operator<(const StringPiece& x, const StringPiece& y) {
  const int r = memcmp(x.data(), y.data(),
                       std::min(x.size(), y.size()));
  return ((r < 0) || ((r == 0) && (x.size() < y.size())));
}

inline bool operator>(const StringPiece& x, const StringPiece& y) {
  return y < x;
}

inline bool operator<=(const StringPiece& x, const StringPiece& y) {
  return !(x > y);
}

inline bool operator>=(const StringPiece& x, const StringPiece& y) {
  return !(x < y);
}

/*inline const char* re2_memrchr(const char* s, int c, size_t n)
{
  return reinterpret_cast<const char*>(memrchr(s, c, n));
}
*/

}  // namespace re2

// allow StringPiece to be logged
extern std::ostream& operator<<(std::ostream& o, const re2::StringPiece& piece);

#include "file_strings.h"

#endif  // RE2_STRINGPIECE_H_
