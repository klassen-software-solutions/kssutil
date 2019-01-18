#!/bin/sh

# Set a program from one of two choices if not already set in an environment variable.
set_executable()
{
    prompt=$1
    varname=$2
    exe1=$3
    exe2=$4

    currentExe=`printenv $varname`

    /bin/echo -n "$prompt"..." "
    exe="NOT FOUND"
    if [ ! "$currentExe" = "" ]; then
        exe=$currentExe
    else
        which $exe1 > /dev/null
        if [ $? -eq 0 ]; then
            exe=$exe1
        else
            which $exe2 > /dev/null
            if [ $? -eq 0 ]; then
                exe=$exe2
            fi
        fi
    fi

    if [ ! "$exe" = "NOT FOUND" ]; then
        echo "$varname := $exe" >> config.defs
    fi
    echo $exe
}

# Determine the target directory and obtain the values from its share/config.site.
add_target_share()
{
    targetDir=$1
    echo "Target directory (prefix)... $targetDir"
    echo "TARGETDIR=$targetDir" >> config.defs
    if [ -f $targetDir/share/config.site ]; then
        echo "-include $targetDir/share/config.site" >> config.defs
    fi
}

# Display the usage message.
display_usage()
{
    echo "Usage: ./configure [OPTION]..."
    echo "  Options:"
    echo "    -h, --help       display this message and exit"
    echo "    --prefix=PREFIX  change the install target to PREFIX (default /usr/local)"
    echo ""
    echo "  You can also configure the system by putting items in a local file called"
    echo "  'config.local'. For example a line 'TARGETDIR=/opt/local' would be equivalent"
    echo "  to calling ./configure --prefix=/opt/local."
    echo ""
}

# Parse the command line options.
set +x
prefix=/usr/local
for arg in "$@"; do
    case $arg in
    -h|--help)
        display_usage
        exit 0
        ;;
    --prefix=*)
        prefix="${arg#*=}"
        ;;
    *)
        display_usage
        exit 1
        ;;
    esac
    shift
done

# Init the local configuration file.
echo "# created at `date`" > config.defs

add_target_share $prefix
set_executable "C compiler" CC clang gcc
set_executable "C++ compiler" CXX clang++ g++
