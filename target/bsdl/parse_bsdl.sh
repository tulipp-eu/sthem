#!/bin/bash

function help {
	printf "$(basename $0) [options] [attributes] <bsdl file>\n"
	printf "  -h|--help    show this help page\n"
	printf "  -q|--quiet   surpress warnings\n"
	printf "  -r|--raw     dump raw bsdl file\n"
	printf "  -b|--bin     print in binary format\n"
	printf "  -d|--dec     print in decimal format\n"
	printf "  -x|--hex     print in hexadecimal format (default)\n\n"
	printf "When no attributes are defined, this script will\n"
	printf "print the IDCODE_REGISTER, the corresponding mask\n"
	printf "and the INSTRUCTION_LENGTH sepperated by single\n"
	printf "spaces\n\n"
	printf "Number conversion is only supported if no attributes\n"
	printf "are passed. INSTRUCTION_LENGTH is never printed in\n"
	printf "binary.\n"
}

function normalizeBsdl {
	tmp1=$(mktemp)
	tmp2=$(mktemp)

	sed 's/\-\-.*$//g' > $tmp1 # remove comments
	sed -i 's/\t/ /g' $tmp1 #tabs to spaces
	sed -i 's/^[ ]*//g' $tmp1 #remove spaces from line beginning
	sed -i '/^$/d' $tmp1 #remove empty lines
	tr '\n' ' ' < $tmp1 > $tmp2 #remove LF
	tr '\r\n' ' ' < $tmp2 > $tmp1 # remove CRLF
	sed -i 's/ \+/ /g' $tmp1 #trim spaces to a single one
	ret=$(sed 's/\"[ ]*\&[ ]*\"//g' $tmp1) #concatenate strings
	rm $tmp1
	rm $tmp2
	echo "$ret"
}

function getValue {
	[ $# -ne 1 ] && return 1
	ret="$(egrep -i -m 1 -o "$1[^;]+");"
	[ $? -ne 0 ] && return 1
	echo "$ret" | egrep -i -m 1 -o "( is | := )[^;]+" | sed 's/^\( is \| := \)//' | tr -d '"' | sed 's/^[ \t]*//;s/[ \t]*$//'
}

format="X"
dump=0
quiet=0

while [ $# -gt 0 ]; do
	case $1 in
	-b|--bin)
	    format="b"
	    shift
	    ;;
    -x|--hex)
	    format="x"
	    shift
	    ;;
	-d|--dec)
	    format="d"
	    shift
	    ;;
	-r|--raw)
		dump=1
		shift
		;;
	-q|--quiet)
		quiet=1
		shift
		;;
	-h|--help|-?)
	    help && exit 0
	    ;;
    *)
    	ARGS+=("$1")
    	shift
    	;;
	esac
done
set -- "${ARGS[@]}"

bsdlFile="$_"

[ $# -eq 0 ] && {
	[ $quiet -eq 0 ] && echo "No BSDL file specified!" >&2 && help
	exit 1
}

[ ! -f "$bsdlFile" ] && {
	[ $quiet -eq 0 ] && echo "$bsdlFile not found!" >&2 && help
	exit 1
}

bsdl=$(cat $bsdlFile | normalizeBsdl)

[ $dump -eq 1 ] && echo "$bsdl" && exit 0

[ $# -eq 1 ] && {
	idcode=$(echo "$bsdl" | getValue "IDCODE_REGISTER")
	[ $? -ne 0 ] && {
		[ $quiet -eq 0 ] && echo "WARNING: 'IDCODE_REGISTER' not found" >&2
	    idcode="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	}
	length=$(echo "$bsdl" | getValue "INSTRUCTION_LENGTH")
	[ $? -ne 0 ] && {
		[ $quiet -eq 0 ] && echo "WARNING: 'INSTRUCTION_LENGTH' not found" >&2
	    length="0"
	}
	idmask=$(printf "%s" "$idcode" | tr '[0-9]' '1' | tr '[Xx]' ' 0')
	idcode=$(printf "%s" "$idcode" | tr '[Xx]' '0');


	case "$format" in
		b|B)
			printf "%s %s %s\n" $idcode $idmask $length
			;;
		d|D)
			printf "%s %s %s\n" $((2#$idcode)) $((2#$idmask)) $length
			;;
		x|X)
			printf "0x%08x 0x%08x 0x%x\n" $((2#$idcode)) $((2#$idmask)) $length
			;;
	esac
	exit 0
}

[ $# -gt 1 ] && {
	ret=0
	while [ $# -gt 1 ]; do
		val=$(echo "$bsdl" | getValue "$1")
		[ $? -ne 0 ] && {
			[ $quiet -eq 0 ] && echo "WARNING: '$1' not found" >&2
			ret=1
		} || {
			printf "%s\n" "$val"
		}
		shift
	done
	exit $ret
}

[ $quiet -eq 0 ] && echo "You should never see that :(" >&2
exit 1
