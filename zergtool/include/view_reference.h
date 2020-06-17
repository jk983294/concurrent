#ifndef _ZERG_REFERENCE_
#define _ZERG_REFERENCE_

namespace ztool {

template <typename T>
class view_reference {
    T* value_;

public:
    view_reference(T& value) : value_(&value) {}

    view_reference() : value_(nullptr) {}

    T operator->() { return *value_; }

    T* get() { return value_; }

    T& operator*() { return *value_; }

    T* operator&() { return value_; }

    operator T&() { return *value_; }

    view_reference<T> operator=(const T& value) {
        *value_ = value;
        return *this;
    }
};

}  // namespace ztool

#endif
