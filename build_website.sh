#!/bin/bash
DEFAULT_CONF=page
GROUP=$(find -L index.txt -printf "%g")
PARALLEL=${PARALLEL:-$(nproc || echo 4)}
ASCIIDOC=$(which asciidoc)
ASCIIDOC_OPTIONS=("--unsafe" "--backend=xhtml11" "--attribute icons" "--attribute=iconsdir=/images/asciidoc" "--attribute=badges!" "--attribute quirks!")

# unconditional dependencies
DEPENDS=("footer.htmlf")

# files to ignore
IGNORE="robots.txt robots-trac.txt"


function msg()
{
    echo "$@" >&2
}




umask 003

run_menugen=no

# first pass, poor man dependency tracking over all .txt files
if [[ ! "$1" ]]; then
    msg -en "\nfinding dependencies:\n\n\t"

    find -L . -name '*.txt' -group "$GROUP" |
        while read file; do
            [[ "${IGNORE/*${file#./}*}" ]] || continue
            msg -n "."

            # check for unconditional dependencies
            for dep in "${DEPENDS[@]}"; do
                if [[ "${dep}" -nt "${file}" ]]; then
                    msg -n "d"
                    touch "$file"
                fi
            done

            # check for includes
            sed 's/include::\?\([^[]*\).*/\1/p;d' "$file" | while read prerequisite; do
                if [[ "${prerequisite:1:1}" != '/' ]]; then
                    prerequisite="${file%/*}/$prerequisite"
                fi
                if [[ "${prerequisite}" -nt "${file}" ]]; then
                    msg -n "i"
                    touch "$file"
                fi
            done

            # check for 'sys' commands
            if grep '{sys[23]\?:*\[.*\]}' "$file" >/dev/null; then
                msg -n "s"
                touch "$file"
            fi

            # check for 'eval' commands
            if grep '{eval3\?:*\[.*\]}' "$file" >/dev/null; then
                msg -n "e"
                touch "$file"
            fi

            # check for conf
            conf="${DEFAULT_CONF}.conf"
            if [[ -e "${file%*.txt}.conf" ]]; then
                conf="${file%*.txt}.conf"
            fi
            if [[ "${conf}" -nt "${file}" ]]; then
                msg -n "c"
                touch "$file"
            fi
        done

    msg
fi


# second pass for every .txt file
case "$1" in
--help|-h|-?)
    cat <<EOF
    Website rebuild script
Usage:
        ${0##*/}
           Rebuild only whats necessary

        ${0##*/} [asciidocfiles..]
           Rebuild the given files

        ${0##*/} --all
           Rebuild all pages unconditionally

        ${0##*/} --clean
           Cleans/deletes all files not under git control

        ${0##*/} --help
           This help
EOF
    exit 0
esac

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
    {
    run_menugen=no
    msg -en "\nfinding files:\n\n\t"
    while read file; do
        [[ "${IGNORE/*${file#./}*}" ]] || continue
        # when the .txt is newer than an existing .html
        if [[ -w . && "$file" -nt "${file%*.txt}.html" || "$1" ]]; then
            # use the default config file
            conf="${DEFAULT_CONF}.conf"
            # or if there is a .conf file with the same basename as the .txt file use that instead
            if [[ -e "${file%*.txt}.conf" ]]; then
                conf="${file%*.txt}.conf"
            fi
            msg -ne "\n\t$file "
            printf "%q %q\0" --conf-file="${conf}" "$file"

            run_menugen=yes
        else
            msg -n "."
        fi
    done >.todo.$$

    msg
    msg -e "\n\ngenerating HTML:\n"
    msg "$ASCIIDOC ${ASCIIDOC_OPTIONS[*]} ..."
    msg

    xargs -a .todo.$$ -0 -I '{}' -P "$PARALLEL" -n1 -0 \
       sh -c "echo '{}' | sed 's/[^ ]* \(.*\)/\t\1/' >&2; $ASCIIDOC  ${ASCIIDOC_OPTIONS[*]} {}"
    rm .todo.$$

    if [[ $run_menugen = yes ]]; then
        ./menugen.py -p -s -w >menu.html.tmp
        if cmp -s menu.html.tmp menu.html; then
            rm menu.html.tmp
        else
            msg
            msg "regenerated menus"
            mv menu.html.tmp menu.html
        fi
    fi
    }
echo

