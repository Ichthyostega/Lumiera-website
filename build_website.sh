#!/bin/bash
DEFAULT_CONF=page
GROUP=$(find -L index.txt -printf "%g")
umask 003

run_menugen=no


# first pass, poor man dependency tracking over all .txt files
if [[ ! "$1" ]]; then
	echo -n "finding dependencies "

	loop=1
	while [[ $loop = 1 ]]; do
		loop=0
		find -L . -name '*.txt' -group "$GROUP" |
			{
				while read file; do
					echo -n "."
					# check for includes
					sed 's/include::\([^[]*\).*/\1/p;d' "$file" | while read prerequisite; do
						if [[ "${prerequisite}" -nt "${file}" ]]; then
							echo -n ":"
							touch "$file"
							loop=1
						fi
					done
					# check for 'sys' commands
					if grep 'sys[23]\?:*\[.*\]' "$file" >/dev/null; then
						echo -n "+"
						touch "$file"
					fi
				done
				if [[ $loop = 1 ]]; then
					false
				fi
			}

		loop=$?
	done
echo
fi


# second pass for every .txt file
echo -n "processing files "
case "$1" in
--all|'')
	find -L . -name '*.txt' -group "$GROUP"
	;;
*)
	for file in "$@"; do
		echo "$file"
	done
	;;
esac |
	{ while read file; do
		echo -n "."
		# when the .txt is newer than an existing .html
		if [[ -w . && "$file" -nt "${file%*.txt}.html" || "$1" ]]; then
			# use the default config file
			conf="${DEFAULT_CONF}.conf"
			# or if there is a .conf file with the same basename as the .txt file use that instead
			if [[ -e "${file%*.txt}.conf" ]]; then
				conf="${file%*.txt}.conf"
			fi
			# run asciidoc over it
	 		echo "asciidocing $file"
			asciidoc --unsafe --backend=xhtml11 \
				--attribute icons --attribute=iconsdir=/images/asciidoc \
				--attribute=badges! --attribute quirks! \
				--conf-file="${conf}" \
				"$file"
			#
			# note we did set	--attribute=revision="$VERS"  --attribute=date="$DATE"
			# IMHO it is better to use the date hard wired in the documents (2/11, Ichthyo)
			echo

			run_menugen=yes
		fi
	done
	if [[ $run_menugen=yes ]]; then
		./menugen.py -p -s -w >menu.html.tmp
		if cmp -s menu.html.tmp menu.html; then
			rm menu.html.tmp
		else
			echo
			echo "regenerate menus"
			mv menu.html.tmp menu.html
		fi
	fi
	}
echo

