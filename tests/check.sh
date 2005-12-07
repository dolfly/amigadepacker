prog="../amigadepacker"

name="Test 1"
file="sqsh1"
md5="`$prog -c < $file |md5sum |cut -d ' ' -f1`"
if test "$md5" != "b3659301b360a83bd737ef80201ddcaf" ; then
    echo $name error
    exit -1
fi

exit 0
