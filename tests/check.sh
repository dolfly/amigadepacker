prog="../amigadepacker"

echo "Executing a test set."

res="0"

name="Test 1"
file="sqsh1"
md5="`$prog -c < $file |md5sum |cut -d ' ' -f1`"
if test "$md5" != "b3659301b360a83bd737ef80201ddcaf" ; then
    echo $name error
    res="-1"
fi


if test "$res" != "0" ; then
    echo "Some errors happened during the test."
    exit -1
fi

echo "All tests were successful."
exit 0
