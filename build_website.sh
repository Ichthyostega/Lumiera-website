#!/bin/bash
DEFAULT_CONF=layout1

umask 003

# first pass, poor man dependency tracking over all .txt files
find -name '*.txt' |
	while read file; do
		sed 's/include::\([^[]*\).*/\1/p;d' "$file" | while read prerequisite; do
			if [[ "${prerequisite}" -nt "${file}" ]]; then
				touch "$file"
			fi
		done
	done

# second pass for every .txt file
find -name '*.txt' |
	while read file; do
		# when the .txt is newer than an existing .html
		if [[ "$file" -nt "${file%*.txt}.html" ]]; then
			# use the default config file
			conf="${DEFAULT_CONF}.conf"
			# or if there is a .conf file with the same basename as the .txt file use that instead
			if [[ -e "${file%*.txt}.conf" ]]; then
				conf="${file%*.txt}.conf"
			fi
			# run asciidoc over it
	 		echo "asciidocing $file"
			python /usr/bin/asciidoc --unsafe --backend=xhtml11 \
				--attribute icons --attribute \
				iconsdir=./images/icons --attribute=badges \
				--attribute=revision="$VERS"  --attribute=date="$DATE" \
				--conf-file="${conf}" \
				"$file"
		fi


	done

