#!/bin/bash

show_help() {
    echo "Usage: $0 [OPTION] DIRECTORY"
    echo "Change the case of file names in the specified DIRECTORY."
    echo ""
    echo "Options:"
    echo "  -l, --lower    Convert file names to lowercase."
    echo "  -u, --upper    Convert file names to uppercase."
    echo "  -h, --help     Display this help message."
}

if [ $# -eq 0 ] || [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    show_help
    exit 0
fi

if [ $# -ne 2 ]; then
    echo "Error: Incorrect number of arguments. Use '$0 --help' for usage information."
    exit 1
fi

to_case=$1
dir=$2

case "$1" in
    "-l"|"--lower")
        to_case="-l"
        ;;
    "-u"|"--upper")
        to_case="-u"
        ;;
    *)
        echo "Error: Invalid option '$1'. Use '-l' or '--lower' for lowercase, '-u' or '--upper' for uppercase."
        exit 1
        ;;
esac

if [ ! -d "$dir" ]; then
    echo "Error: Directory '$dir' does not exist."
    exit 1
fi

change_case() {
    local to_case=$1
    local dir=$2
    local new_name

    for file in "$dir"/*; do
        if [ -f "$file" ]; then
            if [ "$to_case" == "-u" ]; then
                new_name=$(basename "$file" | tr 'a-z' 'A-Z')
            elif [ "$to_case" == "-l" ]; then
                new_name=$(basename "$file" | tr 'A-Z' 'a-z')
            fi

            mv "$file" "$dir/$new_name"
        elif [ -d "$file" ]; then
            change_case "$to_case" "$file"
        fi
    done
}

change_case "$to_case" "$dir"

echo "Case change completed."
