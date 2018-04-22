# Customized (manually maintained) bits of configure.ac for fty-rest

###m4_include([m4/ac_prog_try_doxygen.m4])
m4_include([m4/bs_check_saslauthd_mux.m4])

### Allow to revise the packaging strings in platform.h
ifdef([__AC_UNDEFINE],[],[
m4_define([__AC_UNDEFINE],[_AC_DEFINE([#ifdef $1
 #undef $1
#endif])])
])

AC_DEFUN([AX_PROJECT_LOCAL_HOOK__PACKAGING_DETAILS], [
    ### PACKAGING METADATA (for sysinfo, etc.)

    PACKAGE_VENDOR="Eaton"
    AC_ARG_WITH([package-vendor],
        [AS_HELP_STRING([--with-package-vendor=ARG],
            [name of the entity which distributes this build
             of the package (default is '$PACKAGE_VENDOR')])],
        [PACKAGE_VENDOR="${withval}"
         AC_MSG_NOTICE([Using requested PACKAGE_VENDOR='$PACKAGE_VENDOR'])]
        # This may be left empty by explicit request
    )

    # General substitutions for all these packaging variables
    AS_IF([test x"$_FIX_PACKAGE_NAME" = xy],
        [AC_MSG_NOTICE([Overriding PACKAGE_NAME='$PACKAGE_NAME'])
         __AC_UNDEFINE([PACKAGE_NAME])
         AC_DEFINE_UNQUOTED([PACKAGE_NAME],
            ["$PACKAGE_NAME"],
            [Packaging metadata: distro source code name])
    ])
    AS_IF([test x"$_FIX_PACKAGE_VERSION" = xy],
        [AC_MSG_NOTICE([Overriding PACKAGE_VERSION='$PACKAGE_VERSION'])
         __AC_UNDEFINE([PACKAGE_VERSION])
         AC_DEFINE_UNQUOTED([PACKAGE_VERSION],
            ["$PACKAGE_VERSION"],
            [Packaging metadata: distro source code/packaged release version])
    ])
    AS_IF([test x"$_FIX_VERSION" = xy],
        [AC_MSG_NOTICE([Overriding VERSION='$VERSION'])
         __AC_UNDEFINE([VERSION])
         AC_DEFINE_UNQUOTED([VERSION],
            ["$VERSION"],
            [Version number of package])
    ])
    AS_IF([test x"$_FIX_PACKAGE_STRING" = xy],
        [AC_MSG_NOTICE([Overriding PACKAGE_STRING='$PACKAGE_STRING'])
         __AC_UNDEFINE([PACKAGE_STRING])
         AC_DEFINE_UNQUOTED([PACKAGE_STRING],
            ["$PACKAGE_STRING"],
            [Packaging metadata: distro source code name+version])
    ])
    AS_IF([test x"$_FIX_PACKAGE_URL" = xy],
        [AC_MSG_NOTICE([Overriding PACKAGE_URL='$PACKAGE_URL'])
         __AC_UNDEFINE([PACKAGE_URL])
         AC_DEFINE_UNQUOTED([PACKAGE_URL],
            ["$PACKAGE_URL"],
            [Packaging metadata: distro contact (website)])
    ])
    AS_IF([test x"$_FIX_PACKAGE_BUGREPORT" = xy],
        [AC_MSG_NOTICE([Overriding PACKAGE_BUGREPORT='$PACKAGE_BUGREPORT'])
         __AC_UNDEFINE([PACKAGE_BUGREPORT])
         AC_DEFINE_UNQUOTED([PACKAGE_BUGREPORT],
            ["$PACKAGE_BUGREPORT"],
            [Packaging metadata: distro contact (email/tracker)])
    ])
    AC_MSG_NOTICE([Overriding PACKAGE_VENDOR='$PACKAGE_VENDOR'])
    AC_DEFINE_UNQUOTED([PACKAGE_VENDOR],
            ["$PACKAGE_VENDOR"],
            [Packaging metadata: name of the entity which distributes this build of the package])
])


AC_DEFUN([AX_PROJECT_LOCAL_HOOK__GIT_DETAILS], [
    ### GIT METADATA (for sysinfo, etc.)
    # display source/build paths of interest to us
    AC_MSG_NOTICE([Determining source code paths:])
    AC_MSG_NOTICE(AS_HELP_STRING([Value of srcdir:], ['$srcdir']))
    _curdir_abs="`pwd`"
    AC_MSG_NOTICE(AS_HELP_STRING([Value of _curdir_abs:], ['$_curdir_abs']))
    AS_CASE(["x$srcdir"],
        [x/*], [_srcdir_abs="$srcdir"],
        [*],   [_srcdir_abs="$_curdir_abs/$srcdir"
            AS_IF([test -d "$_srcdir_abs"],
              [_srcdir_abs="`cd "$_srcdir_abs" && pwd || echo "$_srcdir_abs"`"])
    ])
    AC_MSG_NOTICE(AS_HELP_STRING([Value of _srcdir_abs:], ['$_srcdir_abs']))

    # This script is part of the sources, so we expect it to be available
    # We call it to prepare a file with Git workspace metadata, if available;
    # alternately a copy of the file may have been git-archive'd to tarball
    # with sources that so are not a Git workspace (repository clone) anymore.
    # This includes the case of "make dist(check)" where a copy of the file
    # is passed at some stage via Makefile's EXTRA_DIST.
    GDETAILSH="$_srcdir_abs/tools/git_details.sh"
    AS_IF([test -s "$GDETAILSH" -a -x "$GDETAILSH" -a -r "$GDETAILSH"],
        [AC_MSG_NOTICE([Found git_details.sh: $GDETAILSH])],
        [AC_MSG_ERROR([cannot find required git_details.sh])])

    # If we have an (older) .git_details file, and a clean out-of-tree build,
    # then prepopulate the build directory with a copy
    AS_IF([test x"$_srcdir_abs" != x"$_curdir_abs" -a \
          -s "$_srcdir_abs/.git_details" -a \
        ! -s "$_curdir_abs/.git_details"],
        [AC_MSG_NOTICE([Pre-populating .git_details in the out-of-tree build area from source workspace...])
         cp -pf "$_srcdir_abs/.git_details" "$_curdir_abs/.git_details"
    ])

    AS_IF([test -s "$_curdir_abs/.git_details.tmp"],
        [rm -f "$_curdir_abs/.git_details.tmp"])

    AS_IF([test -d "$_srcdir_abs/.git"],
        [AC_MSG_NOTICE([Getting current Git workspace attributes...])
         # The script would need a git... can detect one or use the set envvar
         # So let's help it if we can
         AC_PATH_PROGS([GIT], [git])
         AS_IF([test -n "$GIT" -a -x "$GIT"], [export GIT])
         AC_PATH_PROGS([DATE], [gdate date])
         AS_IF([test -n "$DATE" -a -x "$DATE"], [export DATE])
         AS_IF([(cd "$_srcdir_abs" && $GDETAILSH ) > "$_curdir_abs/.git_details.tmp"],
            [AS_IF([test -s "$_curdir_abs/.git_details.tmp"],
             [AS_IF([test -s "$_curdir_abs/.git_details"],
                [AS_IF([diff -bu "$_curdir_abs/.git_details" \
                                 "$_curdir_abs/.git_details.tmp" | \
                        egrep -v '^(\-\-\-|\+\+\+|[ @]|[\+\-]PACKAGE_BUILD_TSTAMP)' \
                        >/dev/null 2>&1],
                    [AC_MSG_NOTICE([Got new workspace metadata to replace the old copy])
                     mv -f "$_curdir_abs/.git_details.tmp" "$_curdir_abs/.git_details"],
                    [AC_MSG_NOTICE([Got new workspace metadata but no significant changes against the old copy])]
                 )],
                [AC_MSG_NOTICE([Got new workspace metadata and had no old stashed away, using freely])
                 mv -f "$_curdir_abs/.git_details.tmp" "$_curdir_abs/.git_details"])
             ], [AC_MSG_NOTICE([No Git metadata was detected (empty output)])
                 rm -f "$_curdir_abs/.git_details.tmp"])
                ], [AC_MSG_NOTICE([No Git metadata was detected (script errored out)])
                    rm -f "$_curdir_abs/.git_details.tmp"])
        ], [AC_MSG_NOTICE([No Git workspace was detected ($_srcdir_abs/.git directory missing)])
    ])

    # From old copy or a new generation, we may have the file with metadata...
    # We no longer cook (most of) this data into config.h because it does get
    # obsolete quickly during development. Rather, the build products that
    # need (test, display) this information should use or convert the file.
    # The Makefile takes care to keep it current as long as possible.
    # Also, for display in the end of execution, try to "source" the values.
    AS_IF([test -s "$_curdir_abs/.git_details"],
        [AC_MSG_NOTICE([Getting Git details into PACKAGE_* macros...])
         source "$_curdir_abs/.git_details" && for V in \
            PACKAGE_GIT_ORIGIN PACKAGE_GIT_BRANCH PACKAGE_GIT_TAGGED \
            PACKAGE_GIT_TSTAMP PACKAGE_GIT_HASH_S PACKAGE_GIT_HASH_L \
            PACKAGE_GIT_STATUS PACKAGE_BUILD_TSTAMP \
            PACKAGE_GIT_TSTAMP_ISO8601 PACKAGE_BUILD_TSTAMP_ISO8601 \
            PACKAGE_BUILD_HOST_UNAME PACKAGE_BUILD_HOST_NAME \
            PACKAGE_BUILD_HOST_OS ; do
                eval $V='$'${V}_ESCAPED || eval $V=""
            done
         AS_IF([test -z "$PACKAGE_GIT_HASH_S"], [source "$_curdir_abs/.git_details"])
         AS_IF([test -n "$PACKAGE_GIT_HASH_S" -a x"$PACKAGE_GIT_HASH_S" != x'""' \
                  -a -n "$PACKAGE_GIT_TSTAMP" -a x"$PACKAGE_GIT_TSTAMP" != x'""'],
            [AC_MSG_NOTICE([Setting PACKAGE_STRING and PACKAGE_VERSION to include Git hash '$PACKAGE_GIT_HASH_S' and commit timestamp '$PACKAGE_GIT_TSTAMP'...])
             PACKAGE_STRING="$PACKAGE_STRING.$PACKAGE_GIT_TSTAMP~$PACKAGE_GIT_HASH_S"
             PACKAGE_VERSION="$PACKAGE_VERSION.$PACKAGE_GIT_TSTAMP~$PACKAGE_GIT_HASH_S"
             _FIX_PACKAGE_STRING=y
             _FIX_PACKAGE_VERSION=y
             ### Rather don't fix the VERSION as it influences the
             ### tarball name in "make distcheck", install paths, etc.
             #VERSION="$VERSION.$PACKAGE_GIT_TSTAMP~$PACKAGE_GIT_HASH_S"
             #_FIX_VERSION=y
         ])
        ], [AC_MSG_NOTICE([No Git details detected ($_curdir_abs/.git_details file missing), they will be missing from REST API and other reports])
    ])
])

AC_DEFUN([AX_PROJECT_LOCAL_HOOK__CI_TESTS], [
    ci_tests=false
    AC_ARG_ENABLE(ci-tests,
	    [AS_HELP_STRING([--enable-ci-tests],
		    [turn on build of CI test programs along with "all"])],
	    [AS_CASE(["x$enableval"],
		    ["xyes"],[ci_tests=true],
		    ["xno"],[ci_tests=false],
		    [*],[AC_MSG_ERROR([bad value ${enableval} for --enable-ci-tests])])])
    AM_CONDITIONAL([ENABLE_CI_TESTS], [test "x${ci_tests}" = xtrue])
])

AC_DEFUN([AX_PROJECT_LOCAL_HOOK], [
    # Note: This is a custom edit over zproject-generated code
    # brought in from legacy implementation to identify the
    # main baseline version of REST API on a deployment.
    AX_PROJECT_LOCAL_HOOK__GIT_DETAILS
    AX_PROJECT_LOCAL_HOOK__PACKAGING_DETAILS
    AX_PROJECT_LOCAL_HOOK__CI_TESTS

    ### Detect programs we need
    # sed is great!
    AC_CHECK_PROGS([SED], [sed])
    # TNTnet preprocesor
    AC_CHECK_PROGS([ECPPC], [ecppc])
    # TNTnet
    AC_CHECK_PROGS([TNTNET], [tntnet])
    AS_IF([test -z "$TNTNET" ],[
        AC_MSG_WARN([TNTnet not found, might fail runtime tests!])
    ])
    AS_IF([test -z "$ECPPC"],[
        AC_MSG_ERROR([ECPPC not found, can not compile servlets!])
    ])

    # sourced from an m4/*.m4 include file during configure script compilation
    gl_VISIBILITY
    BS_CHECK_SASLAUTHD_MUX
    ### // End of customized code from legacy days
])

AC_DEFUN([AX_PROJECT_LOCAL_HOOK_FINAL], [
    # Note: DO NOT refer to complete token name of this routine in the
    # message below, or you would get an infinite loop in m4 autoconf ;)
    AC_MSG_WARN([Running the PROJECT_LOCAL_HOOK_FINAL])
])
