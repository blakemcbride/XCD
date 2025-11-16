xcd()
{
    local memfile="$HOME/.xcd_memory"
    touch "$memfile"

    # ---------------------------------------------------------
    # Determine logical and physical home, and current physical dir
    # ---------------------------------------------------------
    local logical_home="$HOME"
    local physical_home
    physical_home=$(cd "$HOME" 2>/dev/null && pwd -P)
    local physical_pwd_before
    physical_pwd_before=$(pwd -P)

    # ---------------------------------------------------------
    # Auto-prune memory file: remove non-existent or duplicate paths
    # ---------------------------------------------------------
    if [ -s "$memfile" ]
    then
        local tmpfile
        tmpfile=$(mktemp "${memfile}.XXXXXX") || return
        local line

        while IFS= read -r line
        do
            [ -z "$line" ] && continue
            [ ! -d "$line" ] && continue

            if ! grep -Fx -- "$line" "$tmpfile" >/dev/null 2>&1
            then
                printf '%s\n' "$line" >> "$tmpfile"
            fi
        done < "$memfile"

        mv "$tmpfile" "$memfile"
    fi

    # ---------------------------------------------------------
    # -h option: show help (no directory change)
    # ---------------------------------------------------------
    if [ "$1" = "-h" ]
    then
        cat <<'EOF'
xcd - enhanced cd with directory memory

Usage:
  xcd [dir]           Change to directory (like cd).
  xcd -               Change to previous directory (cd -).
  xcd                 Change to home directory.

Memory features:
  Every successful xcd stores the absolute directory path in ~/.xcd_memory
  (one per line, unique). Dead or duplicate entries are auto-pruned.

Smart matching (fuzzy):
  If you run: xcd segment
    • and "./segment" does NOT exist,
    • and "segment" matches (as a substring) the LAST PATH COMPONENT of
      remembered dirs, e.g. "Backend" matches "/a/Backend", "/b/myBackend",
      then xcd jumps to the first such remembered directory.
    • Repeating the same xcd segment cycles through the next match, wrapping.

Listing:
  xcd -l              List all remembered directories.
  xcd -l segment      List only paths whose last component contains "segment".

Cycle preview:
  xcd -p segment      Show the match cycle for "segment", with indexes,
                      indicating which (if any) is the current directory
                      and which would be used next by "xcd segment".
                      Does NOT change directory.

Maintenance:
  xcd -c              Clear the memory file (~/.xcd_memory).
  xcd -h              Display this help message.

Note:
  When xcd changes into a directory under your home (whether stored as
  /home/you or as its physical target), it invokes cd using the logical
  home path (/home/you/...), so your prompt can show "~" as usual.
EOF
        return 0
    fi

    # ---------------------------------------------------------
    # -c option: clear memory file (no directory change)
    # ---------------------------------------------------------
    if [ "$1" = "-c" ]
    then
        : > "$memfile"
        return 0
    fi

    # ---------------------------------------------------------
    # -l option: list memory, optionally filtered (no directory change)
    # ---------------------------------------------------------
    if [ "$1" = "-l" ]
    then
        local segment="$2"
        local line

        while IFS= read -r line
        do
            [ -z "$line" ] && continue
            [ ! -d "$line" ] && continue

            if [ -z "$segment" ]
            then
                printf '%s\n' "$line"
            else
                local base="${line##*/}"
                if [[ "$base" == *"$segment"* ]]
                then
                    printf '%s\n' "$line"
                fi
            fi
        done < "$memfile"

        return 0
    fi

    # ---------------------------------------------------------
    # -p option: print the current match cycle for a segment (no cd)
    # ---------------------------------------------------------
    if [ "$1" = "-p" ]
    then
        local segment="$2"
        if [ -z "$segment" ]
        then
            printf 'Usage: xcd -p segment\n' >&2
            return 1
        fi

        local line
        local -a matches=()

        while IFS= read -r line
        do
            [ -z "$line" ] && continue
            [ ! -d "$line" ] && continue

            local base="${line##*/}"
            if [[ "$base" == *"$segment"* ]]
            then
                matches+=("$line")
            fi
        done < "$memfile"

        if [ ${#matches[@]} -eq 0 ]
        then
            printf 'No matches for "%s".\n' "$segment"
            return 1
        fi

        # Compare using physical current directory so /home/blake and
        # /drive1/ROOT/home/blake are treated as the same place.
        local idx=-1
        local i
        for (( i = 0; i < ${#matches[@]}; i++ ))
        do
            if [[ "${matches[i]}" == "$physical_pwd_before" ]]
            then
                idx=$i
                break
            fi
        done

        local next_index
        if [ $idx -ge 0 ]
        then
            next_index=$(( (idx + 1) % ${#matches[@]} ))
        else
            next_index=0
        fi

        printf 'Matches for "%s":\n' "$segment"
        for (( i = 0; i < ${#matches[@]}; i++ ))
        do
            local mark=""
            if [ $i -eq $idx ]
            then
                mark="*"
            fi
            printf '  [%d]%s %s\n' "$i" "$mark" "${matches[i]}"
        done

        if [ $idx -ge 0 ]
        then
            printf 'Current dir is in the list at index [%d].\n' "$idx"
        else
            printf 'Current dir is not one of the matches.\n'
        fi

        printf 'Next target on xcd "%s": [%d] %s\n' "$segment" "$next_index" "${matches[$next_index]}"

        return 0
    fi

    # ---------------------------------------------------------
    # Normal behavior: act like cd, plus memory & smart (fuzzy) lookup
    # ---------------------------------------------------------
    if [ $# -eq 0 ]
    then
        builtin cd || return
    elif [ $# -gt 1 ]
    then
        # Let cd handle things like: cd -P, cd old new, etc.
        builtin cd "$@" || return
    else
        local target="$1"

        # If it's a single *relative* path that doesn't exist,
        # try to resolve it via ~/.xcd_memory using fuzzy basename match.
        if [[ "$target" != /* && ! -d "$target" ]]
        then
            local name="$target"
            local line
            local -a matches=()

            while IFS= read -r line
            do
                [ -z "$line" ] && continue
                [ ! -d "$line" ] && continue

                local base="${line##*/}"
                if [[ "$base" == *"$name"* ]]
                then
                    matches+=("$line")
                fi
            done < "$memfile"

            if [ ${#matches[@]} -gt 0 ]
            then
                local idx=-1
                local i

                # Use physical current dir for comparison
                for (( i = 0; i < ${#matches[@]}; i++ ))
                do
                    if [[ "${matches[i]}" == "$physical_pwd_before" ]]
                    then
                        idx=$i
                        break
                    fi
                done

                local next_index
                if [ $idx -ge 0 ]
                then
                    next_index=$(( (idx + 1) % ${#matches[@]} ))
                else
                    next_index=0
                fi

                target="${matches[$next_index]}"
            fi
        fi

        # Normalize any path under physical home to logical home, so
        # PWD becomes /home/you/... (and your prompt can show "~").
        local remainder=""
        local under_home=0

        if [[ "$target" == "$logical_home"* ]]
        then
            remainder="${target#$logical_home}"
            under_home=1
        elif [ -n "$physical_home" ] && [[ "$target" == "$physical_home"* ]]
        then
            remainder="${target#$physical_home}"
            under_home=1
        fi

        if [ $under_home -eq 1 ]
        then
            if [ -z "$remainder" ] || [ "$remainder" = "/" ]
            then
                target="$logical_home"
            else
                target="$logical_home$remainder"
            fi
        fi

        builtin cd "$target" || return
    fi

    # ---------------------------------------------------------
    # Record directory in memory file (absolute *physical* path)
    # ---------------------------------------------------------
    local pwd_now
    pwd_now=$(pwd -P)

    if ! grep -Fx -- "$pwd_now" "$memfile" >/dev/null 2>&1
    then
        printf '%s\n' "$pwd_now" >> "$memfile"
    fi
}

