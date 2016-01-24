dnl Process this file with autoconf to produce a configure script.
AC_INIT(my_quick_http_app, 1.0, moodboom@gmail.com)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR(src/MyQuickHttpApp.cpp)

# MDM 2015/01/16 We need this subdir thing now
# AM_INIT_AUTOMAKE
AM_INIT_AUTOMAKE([subdir-objects])

# MDM Ubuntu autotools needed this line...
AC_PROG_CC

AC_PROG_CXX
AC_LANG([C++])
AC_LANG_CPLUSPLUS
#AX_CXX_COMPILE_STDCXX_0X
#AX_CXX_COMPILE_STDCXX_11
#AX_CHECK_COMPILE_FLAG([-std=c++11], [CXXFLAGS="$CXXFLAGS -std=c++11"])
CXXFLAGS="$CXXFLAGS -std=c++11"

AC_OUTPUT(Makefile src/Makefile)
