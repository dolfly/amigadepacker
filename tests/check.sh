prog="../amigadepacker"

echo "Executing a test set."

res="0"

name="Test 1"
file="sqsh1"
md5="`$prog -c < $file |md5sum |cut -d ' ' -f1`"
if test "$md5" != "b3659301b360a83bd737ef80201ddcaf" ; then
    echo $name error
    res="1"
fi

name="Test 2"
file="pp20_1"
md5="`$prog -c < $file |md5sum |cut -d ' ' -f1`"
if test "$md5" != "274e539d6226cc79719841c9671752d2" ; then
    echo $name error
    res="1"
fi

name="Test 3"
file="sqsh_random1"
outfile="sqsh.tmp"
$prog -o "$outfile" $file
md5="$(md5sum "$outfile" |cut -d ' ' -f1)"
if test "$md5" != "5f7c2705f9257eb65e38edbc08028375" ; then
    echo $name error
    res="1"
fi
rm -f "$outfile"

name="Test 4"
file="aon.wingsofdeath1.stc"
outfile="stc.tmp"
cp "$file" "$outfile"
$prog "$outfile"
md5="$(md5sum "$outfile" |cut -d ' ' -f1)"
if test "$md5" != "ab45d5c5427617d6ee6e4260710edaf1" ; then
    echo $name error
    res="1"
fi
rm -f "$outfile"

name="Test 5"
if test "$1" = "--enc" ; then
    name="Test 1 encrypted"
    file="pp20_enc_1"
    # The correct pp key will be 092510ce (takes less than an hour to break)
    md5="`$prog -c < $file |md5sum |cut -d ' ' -f1`"
    if test "$md5" != "274e539d6226cc79719841c9671752d2" ; then
	echo $name error
	res="1"
    fi
fi

if test "$res" != "0" ; then
    echo "Some errors happened during the test."
else
    echo "All tests were successful."
fi

exit $res
