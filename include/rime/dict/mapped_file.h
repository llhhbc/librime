//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-06-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_MAPPED_FILE_H_
#define RIME_MAPPED_FILE_H_

#include <stdint.h>
#include <cstring>
#include <boost/utility.hpp>
#include <rime/common.h>

namespace rime {

// basic data structure

// Limitation: cannot point to itself (zero is used to represent NULL pointer)
template <class T = char, class Offset = int32_t>
class OffsetPtr {
 public:
  OffsetPtr() : offset_(0) {}
  OffsetPtr(Offset offset) : offset_(offset) {}
  OffsetPtr(const OffsetPtr<T> &ptr)
      : offset_(to_offset(ptr.get())) {}
  OffsetPtr(const T *ptr)
      : offset_(to_offset(ptr)) {}
  OffsetPtr<T>& operator= (const OffsetPtr<T> &ptr) {
    offset_ = to_offset(ptr.get());
    return *this;
  }
  OffsetPtr<T>& operator= (const T *ptr) {
    offset_ = to_offset(ptr);
    return *this;
  }
  operator bool() const {
    return !!offset_;
  }
  T& operator* () const {
    return *get();
  }
  T& operator[] (size_t index) const {
    return *(get() + index);
  }
  // TODO: define other operations
  T* get() const {
    if (!offset_) return NULL;
    return reinterpret_cast<T*>((char*)&offset_ + offset_);
  }
 private:
  Offset to_offset(const T* ptr) const {
    return ptr ? (char*)ptr - (char*)(&offset_) : 0;
  }
  Offset offset_;
};

struct String {
  OffsetPtr<char> data;
  const char* c_str() const { return data.get(); }
  size_t length() const { return c_str() ? strlen(c_str()) : 0; }
  bool empty() const { return !data || !data[0]; }
};

template <class T, class Size = uint32_t>
struct Array {
  Size size;
  T at[1];
  T* begin() { return &at[0]; }
  T* end() { return &at[0] + size; }
  const T* begin() const { return &at[0]; }
  const T* end() const { return &at[0] + size; }
};

template <class T, class Size = uint32_t>
struct List {
  Size size;
  OffsetPtr<T> at;
  T* begin() { return &at[0]; }
  T* end() { return &at[0] + size; }
  const T* begin() const { return &at[0]; }
  const T* end() const { return &at[0] + size; }
};

// MappedFile class definition

class MappedFileImpl;

class MappedFile : boost::noncopyable {
 protected:
  explicit MappedFile(const std::string &file_name);
  virtual ~MappedFile();

  bool Create(size_t capacity);
  bool OpenReadOnly();
  bool OpenReadWrite();
  bool Flush();
  bool Resize(size_t capacity);
  bool ShrinkToFit();

  template <class T>
  T* Allocate(size_t count = 1);

  template <class T>
  T* Find(size_t offset);

  template <class T>
  Array<T>* CreateArray(size_t array_size);

  String* CreateString(const std::string &str);
  bool CopyString(const std::string &src, String *dest);

  size_t capacity() const;
  char * address() const;

public:
  bool IsOpen() const;
  void Close();
  bool Remove();

  const std::string& file_name() const { return file_name_; }
  size_t file_size() const { return size_; }

 private:
  std::string file_name_;
  size_t size_;
  scoped_ptr<MappedFileImpl> file_;
};

// member function definitions

template <class T>
T* MappedFile::Allocate(size_t count) {
  if (!IsOpen())
    return NULL;
  size_t used_space = size_;
  size_t required_space = sizeof(T) * count;
  size_t file_size = capacity();
  if (used_space + required_space > file_size) {
    // not enough space; grow the file
    size_t new_size = (std::max)(used_space + required_space, file_size * 2);
    if(!Resize(new_size) || !OpenReadWrite())
      return NULL;
    // note that size_ has been reset after the file was closed for resizing
    // now lets restore it to the saved value
    size_ = used_space;
  }
  T *ptr = reinterpret_cast<T*>(address() + used_space);
  std::memset(ptr, 0, required_space);
  size_ += required_space;
  return ptr;
}

template <class T>
T* MappedFile::Find(size_t offset) {
  if (!IsOpen() || offset > size_)
    return NULL;
  return reinterpret_cast<T*>(address() + offset);
}

template <class T>
Array<T>* MappedFile::CreateArray(size_t array_size) {
  size_t num_bytes = sizeof(Array<T>) + sizeof(T) * (array_size - 1);
  Array<T>* ret = reinterpret_cast<Array<T>*>(Allocate<char>(num_bytes));
  if (!ret)
    return NULL;
  ret->size = array_size;
  return ret;
}

}  // namespace rime

#endif  // RIME_MAPPED_FILE_H_
