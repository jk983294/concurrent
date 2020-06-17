#ifndef _PYTHON_UTIL_H_
#define _PYTHON_UTIL_H_

#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <cassert>
#include <map>
#include <streambuf>
#include <unordered_map>
#include <vector>

namespace bp = boost::python;

const std::string PY_LIB_SO_PATH{"/opt/anaconda3/lib/libpython3.7m.so"};
const std::string PY_HOME_PATH{"/opt/anaconda3"};

// for auto completion and history
const std::string PY_STARTUP_SCRIPT{
    "import readline\n"
    "import rlcompleter\n"
    "import atexit\n"
    "import os\n"
    "# tab completion\n"
    "readline.parse_and_bind('tab: complete')\n"
    "# history file\n"
    "histfile = os.path.join(os.environ['HOME'], '.pythonhistory')\n"
    "try:\n"
    "    readline.read_history_file(histfile)\n"
    "except IOError:\n"
    "    pass\n"
    "atexit.register(readline.write_history_file, histfile)\n"
    "del os, histfile, readline, rlcompleter\n"};

inline void IndexError() { PyErr_SetString(PyExc_IndexError, "Index out of range"); }

template <class T>
struct std_item {
    typedef typename T::value_type V;
    static V &get(T const &x, int i) {
        if (i < 0) i += x.size();
        if (i >= 0 && i < x.size()) return x[i];
        IndexError();
    }
    static void set(T const &x, int i, V const &v) {
        if (i < 0) i += x.size();
        if (i >= 0 && i < x.size())
            x[i] = v;
        else
            IndexError();
    }
    static void del(T const &x, int i) {
        if (i < 0) i += x.size();
        if (i >= 0 && i < x.size())
            x.erase(i);
        else
            IndexError();
    }
    static void add(T const &x, V const &v) { x.push_back(v); }
    static bool in(T const &x, V const &v) { return find_eq(x.begin, x.end, v) != x.end(); }
};

inline void KeyError() { PyErr_SetString(PyExc_KeyError, "Key not found"); }

template <class T>
struct map_item {
    typedef typename T::key_type K;
    typedef typename T::mapped_type V;
    static V &get(T const &x, K const &i) {
        if (x.find(i) != x.end()) return x[i];
        KeyError();
    }

    static void set(T const &x, K const &i, V const &v) { x[i] = v; }

    static void del(T const &x, K const &i) {
        if (x.find(i) != x.end())
            x.erase(i);
        else
            KeyError();
    }
    static bool in(T const &x, K const &i) { return x.find() != x.end(); }
    static bp::list keys(T const &x) {
        bp::list t;
        for (typename T::const_iterator it = x.begin(); it != x.end(); ++it) t.append(it->first);
        return t;
    }
    static bp::list values(T const &x) {
        bp::list t;
        for (typename T::const_iterator it = x.begin(); it != x.end(); ++it) t.append(it->second);
        return t;
    }
    static bp::list items(T const &x) {
        bp::list t;
        for (typename T::const_iterator it = x.begin(); it != x.end(); ++it)
            t.append(make_tuple(it->first, it->second));
        return t;
    }
};

// vector to python list
template <class T>
bp::list std_vector_to_pylist(const std::vector<T> &v) {
    bp::list l;
    for (const auto &item : v) {
        l.append(item);
    }
    return l;
}

// map to python dict
template <class K, class V>
bp::dict std_map_to_pydict(const std::map<K, V> &m) {
    bp::dict d;
    for (const auto &item : m) {
        d[item.first] = item.second;
    }
    return d;
}

// unordered_map to python dict
template <class K, class V>
bp::dict std_unordered_map_to_pydict(const std::unordered_map<K, V> &m) {
    bp::dict d;
    for (const auto &item : m) {
        d[item.first] = item.second;
    }
    return d;
}

// unordered_map of pair to python dict, that is enough
template <class F, class K, class V>
bp::dict std_unordered_map_pair_to_pydict(const std::unordered_map<F, std::pair<K, V> > &m) {
    bp::dict d;
    for (const auto &item : m) {
        d[item.first] = bp::make_tuple(item.second.first, item.second.second);
    }
    return d;
}

// dooms
template <class T>
bp::list std_vector_vector_vector_to_pylist(const std::vector<std::vector<std::vector<T> > > &v) {
    bp::list l;
    for (const auto &item : v) {
        bp::list l1;
        for (const auto &item1 : item) {
            bp::list l2;
            for (const auto &item2 : item1) {
                l2.append(item2);
            }
            l1.append(l2);
        }
        l.append(l1);
    }
    return l;
}

inline std::string extractException() {
    PyObject *exc, *val, *tb;
    PyErr_Fetch(&exc, &val, &tb);
    PyErr_NormalizeException(&exc, &val, &tb);
    bp::handle<> hexc(exc), hval(bp::allow_null(val)), htb(bp::allow_null(tb));
    if (!hval) {
        return bp::extract<std::string>(bp::str(hexc));
    } else {
        bp::object traceback(bp::import("traceback"));
        bp::object format_exception(traceback.attr("format_exception"));
        bp::object formatted_list(format_exception(hexc, hval, htb));
        bp::object formatted(bp::str("").join(formatted_list));
        return bp::extract<std::string>(formatted);
    }
}

template <typename T>
std::vector<T> to_std_vector(const boost::python::object &iterable) {
    return std::vector<T>(boost::python::stl_input_iterator<T>(iterable), boost::python::stl_input_iterator<T>());
}

template <class T, class R, R(T::*p)>
char const *py_get_char_array(T const &self) {
    return self.*p;
}

template <class T, class R, R(T::*p)>
void py_set_char_array(T &self, const char *val) {
    strcpy(self.*p, val);
}

#endif
