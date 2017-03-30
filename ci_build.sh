#!/usr/bin/env bash

################################################################################
#  This file is copied from fty-asset/ci_build.sh                              #
################################################################################

set -e

#dpkg -l | grep -i sasl || true
#dpkg -S 'libsasl*.pc' || true
#apt-cache search '.*sasl.*' || true

# Set this to enable verbose profiling
[ -n "${CI_TIME-}" ] || CI_TIME=""
case "$CI_TIME" in
    [Yy][Ee][Ss]|[Oo][Nn]|[Tt][Rr][Uu][Ee])
        CI_TIME="time -p " ;;
    [Nn][Oo]|[Oo][Ff][Ff]|[Ff][Aa][Ll][Ss][Ee])
        CI_TIME="" ;;
esac

# Set this to enable verbose tracing
[ -n "${CI_TRACE-}" ] || CI_TRACE="no"
case "$CI_TRACE" in
    [Nn][Oo]|[Oo][Ff][Ff]|[Ff][Aa][Ll][Ss][Ee])
        set +x ;;
    [Yy][Ee][Ss]|[Oo][Nn]|[Tt][Rr][Uu][Ee])
        set -x ;;
esac

if [ "$BUILD_TYPE" == "default" ] || [ "$BUILD_TYPE" == "default-Werror" ] || [ "$BUILD_TYPE" == "valgrind" ]; then
    LANG=C
    LC_ALL=C
    export LANG LC_ALL

    if [ -d "./tmp" ]; then
        rm -rf ./tmp
    fi
    mkdir -p tmp
    BUILD_PREFIX=$PWD/tmp

    PATH="`echo "$PATH" | sed -e 's,^/usr/lib/ccache/?:,,' -e 's,:/usr/lib/ccache/?:,,' -e 's,:/usr/lib/ccache/?$,,' -e 's,^/usr/lib/ccache/?$,,'2`"
    CCACHE_PATH="$PATH"
    CCACHE_DIR="${HOME}/.ccache"
    PATH="${BUILD_PREFIX}/sbin:${BUILD_PREFIX}/bin:$PATH"
    export CCACHE_PATH CCACHE_DIR PATH
    HAVE_CCACHE=no
    if which ccache && ls -la /usr/lib/ccache ; then
        HAVE_CCACHE=yes
    fi

    if [ "$HAVE_CCACHE" = yes ] && [ -d "$CCACHE_DIR" ]; then
        echo "CCache stats before build:"
        ccache -s || true
    fi
    mkdir -p "${HOME}/.ccache"

    CONFIG_OPTS=()
    COMMON_CFLAGS=""
    EXTRA_CFLAGS=""
    EXTRA_CPPFLAGS=""
    EXTRA_CXXFLAGS=""

    is_gnucc() {
        if [ -n "$1" ] && "$1" --version 2>&1 | grep 'Free Software Foundation' > /dev/null ; then true ; else false ; fi
    }

    COMPILER_FAMILY=""
    if [ -n "$CC" -a -n "$CXX" ]; then
        if is_gnucc "$CC" && is_gnucc "$CXX" ; then
            COMPILER_FAMILY="GCC"
            export CC CXX
        fi
    else
        if is_gnucc "gcc" && is_gnucc "g++" ; then
            # Autoconf would pick this by default
            COMPILER_FAMILY="GCC"
            [ -n "$CC" ] || CC=gcc
            [ -n "$CXX" ] || CXX=g++
            export CC CXX
        elif is_gnucc "cc" && is_gnucc "c++" ; then
            COMPILER_FAMILY="GCC"
            [ -n "$CC" ] || CC=cc
            [ -n "$CXX" ] || CXX=c++
            export CC CXX
        fi
    fi

    if [ -n "$CPP" ] ; then
        [ -x "$CPP" ] && export CPP
    else
        if is_gnucc "cpp" ; then
            CPP=cpp && export CPP
        fi
    fi

    CONFIG_OPT_WERROR="--enable-Werror=no"
    if [ "$BUILD_TYPE" == "default-Werror" ] ; then
        case "${COMPILER_FAMILY}" in
            GCC)
                echo "NOTE: Enabling ${COMPILER_FAMILY} compiler pedantic error-checking flags for BUILD_TYPE='$BUILD_TYPE'" >&2
                CONFIG_OPT_WERROR="--enable-Werror=yes"
                CONFIG_OPTS+=("--enable-Werror=yes")
                ;;
            *)
                echo "WARNING: Current compiler is not GCC, might not enable pedantic error-checking flags for BUILD_TYPE='$BUILD_TYPE'" >&2
                CONFIG_OPT_WERROR="--enable-Werror=auto"
                ;;
        esac
    fi

    CONFIG_OPTS+=("CFLAGS=-I${BUILD_PREFIX}/include")
    CONFIG_OPTS+=("CPPFLAGS=-I${BUILD_PREFIX}/include")
    CONFIG_OPTS+=("CXXFLAGS=-I${BUILD_PREFIX}/include")
    CONFIG_OPTS+=("LDFLAGS=-L${BUILD_PREFIX}/lib")
    CONFIG_OPTS+=("PKG_CONFIG_PATH=${BUILD_PREFIX}/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/lib/arm-linux-gnueabihf/pkgconfig:/usr/lib/pkgconfig:/lib/pkgconfig")
    CONFIG_OPTS+=("--prefix=${BUILD_PREFIX}")
    CONFIG_OPTS+=("--with-docs=no")
    if [ -z "${CI_CONFIG_QUIET-}" ] || [ "${CI_CONFIG_QUIET-}" = yes ] || [ "${CI_CONFIG_QUIET-}" = true ]; then
        CONFIG_OPTS+=("--quiet")
    fi

    CONFIG_OPTS+=("--with-pkgconfigdir=${BUILD_PREFIX}/lib/pkgconfig")
    CONFIG_OPTS+=("--with-systemdtmpfilesdir=${BUILD_PREFIX}/usr/lib/tmpfiles.d")
    CONFIG_OPTS+=("--with-systemdsystempresetdir=${BUILD_PREFIX}/usr/lib/systemd/system-preset")
    CONFIG_OPTS+=("--with-systemdsystemunitdir=${BUILD_PREFIX}/usr/lib/systemd/system")

    # fty-rest itself has no notion of drafts, and from Z ecosystem we want stable APIs
    CONFIG_OPTS+=("--enable-drafts=no")

    if [ "$HAVE_CCACHE" = yes ] && [ "${COMPILER_FAMILY}" = GCC ]; then
        PATH="/usr/lib/ccache:$PATH"
        export PATH
        if [ -n "$CC" ] && [ -x "/usr/lib/ccache/`basename "$CC"`" ]; then
            case "$CC" in
                *ccache*) ;;
                */*) DIR_CC="`dirname "$CC"`" && [ -n "$DIR_CC" ] && DIR_CC="`cd "$DIR_CC" && pwd `" && [ -n "$DIR_CC" ] && [ -d "$DIR_CC" ] || DIR_CC=""
                    [ -z "$CCACHE_PATH" ] && CCACHE_PATH="$DIR_CC" || \
                    if echo "$CCACHE_PATH" | egrep '(^'"$DIR_CC"':.*|^'"$DIR_CC"'$|:'"$DIR_CC"':|:'"$DIR_CC"'$)' ; then
                        CCACHE_PATH="$DIR_CC:$CCACHE_PATH"
                    fi
                    ;;
            esac
            CC="/usr/lib/ccache/`basename "$CC"`"
        else
            : # CC="ccache $CC"
        fi
        if [ -n "$CXX" ] && [ -x "/usr/lib/ccache/`basename "$CXX"`" ]; then
            case "$CXX" in
                *ccache*) ;;
                */*) DIR_CXX="`dirname "$CXX"`" && [ -n "$DIR_CXX" ] && DIR_CXX="`cd "$DIR_CXX" && pwd `" && [ -n "$DIR_CXX" ] && [ -d "$DIR_CXX" ] || DIR_CXX=""
                    [ -z "$CCACHE_PATH" ] && CCACHE_PATH="$DIR_CXX" || \
                    if echo "$CCACHE_PATH" | egrep '(^'"$DIR_CXX"':.*|^'"$DIR_CXX"'$|:'"$DIR_CXX"':|:'"$DIR_CXX"'$)' ; then
                        CCACHE_PATH="$DIR_CXX:$CCACHE_PATH"
                    fi
                    ;;
            esac
            CXX="/usr/lib/ccache/`basename "$CXX"`"
        else
            : # CXX="ccache $CXX"
        fi
        if [ -n "$CPP" ] && [ -x "/usr/lib/ccache/`basename "$CPP"`" ]; then
            case "$CPP" in
                *ccache*) ;;
                */*) DIR_CPP="`dirname "$CPP"`" && [ -n "$DIR_CPP" ] && DIR_CPP="`cd "$DIR_CPP" && pwd `" && [ -n "$DIR_CPP" ] && [ -d "$DIR_CPP" ] || DIR_CPP=""
                    [ -z "$CCACHE_PATH" ] && CCACHE_PATH="$DIR_CPP" || \
                    if echo "$CCACHE_PATH" | egrep '(^'"$DIR_CPP"':.*|^'"$DIR_CPP"'$|:'"$DIR_CPP"':|:'"$DIR_CPP"'$)' ; then
                        CCACHE_PATH="$DIR_CPP:$CCACHE_PATH"
                    fi
                    ;;
            esac
            CPP="/usr/lib/ccache/`basename "$CPP"`"
        else
            : # CPP="ccache $CPP"
        fi

        CONFIG_OPTS+=("CC=${CC}")
        CONFIG_OPTS+=("CXX=${CXX}")
        CONFIG_OPTS+=("CPP=${CPP}")
    fi

    # Clone and build dependencies, if not yet installed to Travis env as DEBs
    # or MacOS packages; other OSes are not currently supported by Travis cloud
    [ -z "$CI_TIME" ] || echo "`date`: Starting build of dependencies (if any)..."

    # Start of recipe for dependency: sodium
    if ! (command -v dpkg-query >/dev/null 2>&1 && dpkg-query --list libsodium-dev >/dev/null 2>&1) || \
           (command -v brew >/dev/null 2>&1 && brew ls --versions sodium >/dev/null 2>&1) \
    ; then
        echo ""
        BASE_PWD=${PWD}
        echo "`date`: INFO: Building prerequisite 'sodium' from Git repository..." >&2
        $CI_TIME git clone --quiet --depth 1 -b stable https://github.com/jedisct1/libsodium sodium
        cd sodium
        CCACHE_BASEDIR=${PWD}
        export CCACHE_BASEDIR
        git --no-pager log --oneline -n1
        if [ -e autogen.sh ]; then
            $CI_TIME ./autogen.sh 2> /dev/null
        fi
        if [ -e buildconf ]; then
            $CI_TIME ./buildconf 2> /dev/null
        fi
        if [ ! -e autogen.sh ] && [ ! -e buildconf ] && [ ! -e ./configure ] && [ -s ./configure.ac ]; then
            $CI_TIME libtoolize --copy --force && \
            $CI_TIME aclocal -I . && \
            $CI_TIME autoheader && \
            $CI_TIME automake --add-missing --copy && \
            $CI_TIME autoconf || \
            $CI_TIME autoreconf -fiv
        fi
        $CI_TIME ./configure "${CONFIG_OPTS[@]}"
        $CI_TIME make -j4
        $CI_TIME make install
        cd "${BASE_PWD}"
    fi

    # Start of recipe for dependency: libzmq
    if ! (command -v dpkg-query >/dev/null 2>&1 && dpkg-query --list libzmq3-dev >/dev/null 2>&1) || \
           (command -v brew >/dev/null 2>&1 && brew ls --versions libzmq >/dev/null 2>&1) \
    ; then
        echo ""
        BASE_PWD=${PWD}
        echo "`date`: INFO: Building prerequisite 'libzmq' from Git repository..." >&2
        $CI_TIME git clone --quiet --depth 1 https://github.com/zeromq/libzmq.git libzmq
        #$CI_TIME git clone --quiet --depth 1 https://github.com/42ity/libzmq libzmq
        cd libzmq
        CCACHE_BASEDIR=${PWD}
        export CCACHE_BASEDIR
        git --no-pager log --oneline -n1
        if [ -e autogen.sh ]; then
            $CI_TIME ./autogen.sh 2> /dev/null
        fi
        if [ -e buildconf ]; then
            $CI_TIME ./buildconf 2> /dev/null
        fi
        if [ ! -e autogen.sh ] && [ ! -e buildconf ] && [ ! -e ./configure ] && [ -s ./configure.ac ]; then
            $CI_TIME libtoolize --copy --force && \
            $CI_TIME aclocal -I . && \
            $CI_TIME autoheader && \
            $CI_TIME automake --add-missing --copy && \
            $CI_TIME autoconf || \
            $CI_TIME autoreconf -fiv
        fi
        $CI_TIME ./configure "${CONFIG_OPTS[@]}"
        $CI_TIME make -j4
        $CI_TIME make install
        cd "${BASE_PWD}"
    fi

    # Start of recipe for dependency: czmq
    if ! (command -v dpkg-query >/dev/null 2>&1 && dpkg-query --list libczmq-dev >/dev/null 2>&1) || \
           (command -v brew >/dev/null 2>&1 && brew ls --versions czmq >/dev/null 2>&1) \
    ; then
        echo ""
        BASE_PWD=${PWD}
        echo "`date`: INFO: Building prerequisite 'czmq' from Git repository..." >&2
        # $CI_TIME git clone --quiet --depth 1 -b v3.0.2 https://github.com/zeromq/czmq.git czmq
        # $CI_TIME git clone --quiet --depth 1 https://github.com/42ity/czmq czmq
        # $CI_TIME git clone --quiet --depth 1 -b v3.0.2 https://github.com/42ity/czmq czmq

### NOTE: Manual edit
case "$CI_CZMQ_VER" in
3)
        $CI_TIME git clone --quiet -b v3.0.2 --depth 1 https://github.com/42ity/czmq.git czmq
;;
4|*)
        $CI_TIME git clone --quiet --depth 1 https://github.com/zeromq/czmq.git czmq
;;
esac

        cd czmq
        CCACHE_BASEDIR=${PWD}
        export CCACHE_BASEDIR
        git --no-pager log --oneline -n1
        if [ -e autogen.sh ]; then
            $CI_TIME ./autogen.sh 2> /dev/null
        fi
        if [ -e buildconf ]; then
            $CI_TIME ./buildconf 2> /dev/null
        fi
        if [ ! -e autogen.sh ] && [ ! -e buildconf ] && [ ! -e ./configure ] && [ -s ./configure.ac ]; then
            $CI_TIME libtoolize --copy --force && \
            $CI_TIME aclocal -I . && \
            $CI_TIME autoheader && \
            $CI_TIME automake --add-missing --copy && \
            $CI_TIME autoconf || \
            $CI_TIME autoreconf -fiv
        fi
        $CI_TIME ./configure "${CONFIG_OPTS[@]}"
        $CI_TIME make -j4
        $CI_TIME make install
        cd "${BASE_PWD}"
    fi

    # Start of recipe for dependency: malamute
    if ! (command -v dpkg-query >/dev/null 2>&1 && dpkg-query --list libmlm-dev >/dev/null 2>&1) || \
           (command -v brew >/dev/null 2>&1 && brew ls --versions malamute >/dev/null 2>&1) \
    ; then
        echo ""
        BASE_PWD=${PWD}
        echo "`date`: INFO: Building prerequisite 'malamute' from Git repository..." >&2
        $CI_TIME git clone --quiet --depth 1 https://github.com/zeromq/malamute.git malamute
        # $CI_TIME git clone --quiet --depth 1 https://github.com/42ity/malamute malamute
        cd malamute
        CCACHE_BASEDIR=${PWD}
        export CCACHE_BASEDIR
        git --no-pager log --oneline -n1
        if [ -e autogen.sh ]; then
            $CI_TIME ./autogen.sh 2> /dev/null
        fi
        if [ -e buildconf ]; then
            $CI_TIME ./buildconf 2> /dev/null
        fi
        if [ ! -e autogen.sh ] && [ ! -e buildconf ] && [ ! -e ./configure ] && [ -s ./configure.ac ]; then
            $CI_TIME libtoolize --copy --force && \
            $CI_TIME aclocal -I . && \
            $CI_TIME autoheader && \
            $CI_TIME automake --add-missing --copy && \
            $CI_TIME autoconf || \
            $CI_TIME autoreconf -fiv
        fi
        $CI_TIME ./configure "${CONFIG_OPTS[@]}"
        $CI_TIME make -j4
        $CI_TIME make install
        cd "${BASE_PWD}"
    fi

    # Start of recipe for dependency: magic
    if ! (command -v dpkg-query >/dev/null 2>&1 && dpkg-query --list libmagic-dev >/dev/null 2>&1) || \
           (command -v brew >/dev/null 2>&1 && brew ls --versions magic >/dev/null 2>&1) \
    ; then
        echo ""
        BASE_PWD=${PWD}
        echo "`date`: INFO: Building prerequisite 'magic' from Git repository..." >&2
        $CI_TIME git clone --quiet --depth 1 https://github.com/42ity/libmagic magic
        cd magic
        CCACHE_BASEDIR=${PWD}
        export CCACHE_BASEDIR
        git --no-pager log --oneline -n1
        if [ -e autogen.sh ]; then
            $CI_TIME ./autogen.sh 2> /dev/null
        fi
        if [ -e buildconf ]; then
            $CI_TIME ./buildconf 2> /dev/null
        fi
        if [ ! -e autogen.sh ] && [ ! -e buildconf ] && [ ! -e ./configure ] && [ -s ./configure.ac ]; then
            $CI_TIME libtoolize --copy --force && \
            $CI_TIME aclocal -I . && \
            $CI_TIME autoheader && \
            $CI_TIME automake --add-missing --copy && \
            $CI_TIME autoconf || \
            $CI_TIME autoreconf -fiv
        fi
        $CI_TIME ./configure "${CONFIG_OPTS[@]}"
        $CI_TIME make -j4
        $CI_TIME make install
        cd "${BASE_PWD}"
    fi

    # Start of recipe for dependency: cidr
    if ! (command -v dpkg-query >/dev/null 2>&1 && dpkg-query --list libcidr0-dev >/dev/null 2>&1) || \
           (command -v brew >/dev/null 2>&1 && brew ls --versions cidr >/dev/null 2>&1) \
    ; then
        echo ""
        BASE_PWD=${PWD}
        echo "`date`: INFO: Building prerequisite 'cidr' from Git repository..." >&2
        $CI_TIME git clone --quiet --depth 1 https://github.com/42ity/libcidr cidr
        cd cidr
        CCACHE_BASEDIR=${PWD}
        export CCACHE_BASEDIR
        git --no-pager log --oneline -n1
        if [ -e autogen.sh ]; then
            $CI_TIME ./autogen.sh 2> /dev/null
        fi
        if [ -e buildconf ]; then
            $CI_TIME ./buildconf 2> /dev/null
        fi
        if [ ! -e autogen.sh ] && [ ! -e buildconf ] && [ ! -e ./configure ] && [ -s ./configure.ac ]; then
            $CI_TIME libtoolize --copy --force && \
            $CI_TIME aclocal -I . && \
            $CI_TIME autoheader && \
            $CI_TIME automake --add-missing --copy && \
            $CI_TIME autoconf || \
            $CI_TIME autoreconf -fiv
        fi
        $CI_TIME ./configure "${CONFIG_OPTS[@]}"
        $CI_TIME make -j4
        $CI_TIME make install
        cd "${BASE_PWD}"
    fi

    # Start of recipe for dependency: cxxtools
    if ! (command -v dpkg-query >/dev/null 2>&1 && dpkg-query --list cxxtools-dev >/dev/null 2>&1) || \
           (command -v brew >/dev/null 2>&1 && brew ls --versions cxxtools >/dev/null 2>&1) \
    ; then
        echo ""
        BASE_PWD=${PWD}
        echo "`date`: INFO: Building prerequisite 'cxxtools' from Git repository..." >&2
        $CI_TIME git clone --quiet --depth 1 -b 42ity https://github.com/42ity/cxxtools cxxtools
        cd cxxtools
        CCACHE_BASEDIR=${PWD}
        export CCACHE_BASEDIR
        git --no-pager log --oneline -n1
        if [ -e autogen.sh ]; then
            $CI_TIME ./autogen.sh 2> /dev/null
        fi
        if [ -e buildconf ]; then
            $CI_TIME ./buildconf 2> /dev/null
        fi
        if [ ! -e autogen.sh ] && [ ! -e buildconf ] && [ ! -e ./configure ] && [ -s ./configure.ac ]; then
            $CI_TIME libtoolize --copy --force && \
            $CI_TIME aclocal -I . && \
            $CI_TIME autoheader && \
            $CI_TIME automake --add-missing --copy && \
            $CI_TIME autoconf || \
            $CI_TIME autoreconf -fiv
        fi
        $CI_TIME ./configure "${CONFIG_OPTS[@]}"
        $CI_TIME make -j4
        $CI_TIME make install
        cd "${BASE_PWD}"
    fi

    # Start of recipe for dependency: tntdb
    if ! (command -v dpkg-query >/dev/null 2>&1 && dpkg-query --list tntdb-dev >/dev/null 2>&1) || \
           (command -v brew >/dev/null 2>&1 && brew ls --versions tntdb >/dev/null 2>&1) \
    ; then
        echo ""
        BASE_PWD=${PWD}
        echo "`date`: INFO: Building prerequisite 'tntdb' from Git repository..." >&2
        $CI_TIME git clone --quiet --depth 1 -b 1.3 https://github.com/42ity/tntdb tntdb
        cd tntdb
        cd ./tntdb
        CCACHE_BASEDIR=${PWD}
        export CCACHE_BASEDIR
        git --no-pager log --oneline -n1
        if [ -e autogen.sh ]; then
            $CI_TIME ./autogen.sh 2> /dev/null
        fi
        if [ -e buildconf ]; then
            $CI_TIME ./buildconf 2> /dev/null
        fi
        if [ ! -e autogen.sh ] && [ ! -e buildconf ] && [ ! -e ./configure ] && [ -s ./configure.ac ]; then
            $CI_TIME libtoolize --copy --force && \
            $CI_TIME aclocal -I . && \
            $CI_TIME autoheader && \
            $CI_TIME automake --add-missing --copy && \
            $CI_TIME autoconf || \
            $CI_TIME autoreconf -fiv
        fi
        $CI_TIME ./configure "${CONFIG_OPTS[@]}"
        $CI_TIME make -j4
        $CI_TIME make install
        cd "${BASE_PWD}"
    fi

    # Start of recipe for dependency: tntnet
    if ! (command -v dpkg-query >/dev/null 2>&1 && dpkg-query --list tntnet-dev >/dev/null 2>&1) || \
           (command -v brew >/dev/null 2>&1 && brew ls --versions tntnet >/dev/null 2>&1) \
    ; then
        echo ""
        BASE_PWD=${PWD}
        echo "`date`: INFO: Building prerequisite 'tntnet' from Git repository..." >&2
        $CI_TIME git clone --quiet --depth 1 -b 2.2 https://github.com/42ity/tntnet tntnet
        cd tntnet
        CCACHE_BASEDIR=${PWD}
        export CCACHE_BASEDIR
        git --no-pager log --oneline -n1
        if [ -e autogen.sh ]; then
            $CI_TIME ./autogen.sh 2> /dev/null
        fi
        if [ -e buildconf ]; then
            $CI_TIME ./buildconf 2> /dev/null
        fi
        if [ ! -e autogen.sh ] && [ ! -e buildconf ] && [ ! -e ./configure ] && [ -s ./configure.ac ]; then
            $CI_TIME libtoolize --copy --force && \
            $CI_TIME aclocal -I . && \
            $CI_TIME autoheader && \
            $CI_TIME automake --add-missing --copy && \
            $CI_TIME autoconf || \
            $CI_TIME autoreconf -fiv
        fi
        $CI_TIME ./configure "${CONFIG_OPTS[@]}"
        $CI_TIME make -j4
        $CI_TIME make install
        cd "${BASE_PWD}"
    fi

    # Start of recipe for dependency: fty-proto
    if ! (command -v dpkg-query >/dev/null 2>&1 && dpkg-query --list libfty_proto-dev >/dev/null 2>&1) || \
           (command -v brew >/dev/null 2>&1 && brew ls --versions fty-proto >/dev/null 2>&1) \
    ; then
        echo ""
        BASE_PWD=${PWD}
        echo "`date`: INFO: Building prerequisite 'fty-proto' from Git repository..." >&2
        $CI_TIME git clone --quiet --depth 1 https://github.com/42ity/fty-proto fty-proto
        cd fty-proto
        CCACHE_BASEDIR=${PWD}
        export CCACHE_BASEDIR
        git --no-pager log --oneline -n1
        if [ -e autogen.sh ]; then
            $CI_TIME ./autogen.sh 2> /dev/null
        fi
        if [ -e buildconf ]; then
            $CI_TIME ./buildconf 2> /dev/null
        fi
        if [ ! -e autogen.sh ] && [ ! -e buildconf ] && [ ! -e ./configure ] && [ -s ./configure.ac ]; then
            $CI_TIME libtoolize --copy --force && \
            $CI_TIME aclocal -I . && \
            $CI_TIME autoheader && \
            $CI_TIME automake --add-missing --copy && \
            $CI_TIME autoconf || \
            $CI_TIME autoreconf -fiv
        fi
        $CI_TIME ./configure "${CONFIG_OPTS[@]}"
        $CI_TIME make -j4
        $CI_TIME make install
        cd "${BASE_PWD}"
    fi

    # Build and check this project; note that zprojects always have an autogen.sh
    echo ""
    echo "=== LIBCIDR.PC"
    cat "${BUILD_PREFIX}"/lib/pkgconfig/libcidr.pc || true
    echo "`date`: INFO: Starting build of currently tested project..."
    CCACHE_BASEDIR=${PWD}
    export CCACHE_BASEDIR
    # Only use --enable-Werror on projects that are expected to have it
    # (and it is not our duty to check prerequisite projects anyway)
    CONFIG_OPTS+=("${CONFIG_OPT_WERROR}")
    $CI_TIME ./autogen.sh 2> /dev/null
    echo "+ ./configure" "${CONFIG_OPTS[@]}"
    $CI_TIME ./configure "${CONFIG_OPTS[@]}"
    if [ "$BUILD_TYPE" == "valgrind" ] ; then
        # Build and check this project
        $CI_TIME make VERBOSE=1 memcheck
        exit $?
    fi
    $CI_TIME make VERBOSE=1 all

    echo "=== Are GitIgnores good after 'make all'? (should have no output below)"
    git status -s || true
    echo "==="

    (
        export DISTCHECK_CONFIGURE_FLAGS="${CONFIG_OPTS[@]}"
        $CI_TIME make VERBOSE=1 DISTCHECK_CONFIGURE_FLAGS="$DISTCHECK_CONFIGURE_FLAGS" distcheck

        echo "=== Are GitIgnores good after 'make distcheck'? (should have no output below)"
        git status -s || true
        echo "==="
    )

    if [ "$HAVE_CCACHE" = yes ]; then
        echo "CCache stats after build:"
        ccache -s
    fi

elif [ "$BUILD_TYPE" == "bindings" ]; then
    pushd "./bindings/${BINDING}" && ./ci_build.sh
else
    pushd "./builds/${BUILD_TYPE}" && REPO_DIR="$(dirs -l +1)" ./ci_build.sh
fi
