prog="../amigadepacker"

echo "Executing a test set."

res="0"

function depack() {
    name="$1"
    infile="$2"
    md5="$3"
    outfile="$4"
    if test "$outfile" = "inplace" ; then
	outfile="$infile"
	$prog "$infile"
    elif test "$outfile" = "stdout" ; then
	outfile="depack.tmp"
	$prog -c "$infile" > "$outfile"
    else
	$prog -o "$outfile" "$infile"
    fi
    result="$(md5sum "$outfile" |cut -d ' ' -f1)"
    if test "$result" = "$md5" ; then
	res="0"
    else
	echo $name error
	res="1"
    fi

    return $res
}

depack "Test 1" "sqsh1" "b3659301b360a83bd737ef80201ddcaf" stdout
newres="$?"
if test "$newres" != "0" ; then
    res="1"
fi

depack "Test 2" "pp20_1" "274e539d6226cc79719841c9671752d2" depack.tmp
newres="$?"
if test "$newres" != "0" ; then
    res="1"
fi

cp "sqsh_random1" "depack.tmp"
depack "Test 3" "depack.tmp" "5f7c2705f9257eb65e38edbc08028375" inplace
newres="$?"
if test "$newres" != "0" ; then
    res="1"
fi

depack "Test 4" "aon.wingsofdeath1.stc" "ab45d5c5427617d6ee6e4260710edaf1" stdout
newres="$?"
if test "$newres" != "0" ; then
    res="1"
fi

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

depack "Test 6" "mod.chip.mmcmp" "a94b233e18dcf26e0c6170c79ef90d16" stdout
newres="$?"
if test "$newres" != "0" ; then
    res="1"
fi

if test "$res" != "0" ; then
    echo "Some errors happened during the test."
else
    echo "All tests were successful."
fi

exit $res
