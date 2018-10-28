#!/bin/bash
DEFAULT_CONF=page
GROUP=$(find -L index.txt -printf "%g")
PARALLEL=${PARALLEL:-$(nproc || echo 4)}
ASCIIDOC_OPTIONS="--unsafe --backend=xhtml11 --attribute icons --attribute=iconsdir=/images/asciidoc --attribute=badges! --attribute quirks!"

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
    msg -n "finding dependencies: "

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
    msg -n "processing files: "
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
            # run asciidoc over it
            msg -ne "\nasciidocing $file"

            printf "%q " \
                   $ASCIIDOC_OPTIONS \
                   --conf-file="${conf}" \
                   "$file"


            run_menugen=yes
        else
            msg -n "."
        fi
    done >.todo.$$

    xargs -a .todo.$$ -P $PARALLEL -r -n 10 asciidoc
    rm .todo.$$

    if [[ $run_menugen = yes ]]; then
        ./menugen.py -p -s -w >menu.html.tmp
        if cmp -s menu.html.tmp menu.html; then
            rm menu.html.tmp
        else
            msg
            msg "regenerate menus"
            mv menu.html.tmp menu.html
        fi
    fi
    }
echo

