TARGETS=${1-"all"}

CC="gcc -std=gnu99 -g -Wall -D_DEBUG -D_STAND_ALONE "
LINKER=${CC}

CFLAGS=" -I../src -I../include "
LIBS=" -lm -lpthread -ljson-c "


function build()
{
	local target=$1
	[ -z ${target} ] && exit 111
	target=${target/.[ch]/}		# remove extension name

	echo "==== target=${target}"
	
	case "$target" in
		annotation-list)
			echo -e "\e[32m" "build: ${CC} ${CFLAGS} -D_TEST_ANNOTATION_LIST -o ${target} ${target}.c ${LIBS} ..."
			${CC} ${CFLAGS} -D_TEST_ANNOTATION_LIST -o ${target} ${target}.c ${LIBS}
			local rc=$?
			echo -e " --> ret=${rc}" "\e[39m"
			;;
		*)
			return 1
			;;
	esac
	return 0
}

#######################################
## main
#######################################
case ${TARGETS} in
	all|ALL)
		;;
	*)
		for f in ${TARGETS[@]}
		do
			build $f
		done
		;;
esac;

