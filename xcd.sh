# Put this in ~/.bashrc (Linux) or ~/.zshrc (macOS)

xcd()
{
    case "$1" in
        -h|-c|-l|-p)
            xcd-core "$@"
            ;;
        *)
            local target
            target=$(xcd-core "$@")
            local status=$?
            if [ $status -ne 0 ]
            then
                return $status
            fi
            if [ -z "$target" ]
            then
                return 0
            fi

            # Map canonical "physical home" back to logical $HOME
            # so that PWD stays under /home/blake and your prompt shows ~.
            local home="$HOME"
            local physical_home
            physical_home=$(cd "$HOME" 2>/dev/null && pwd -P)

            if [ -n "$physical_home" ] && [[ "$target" == "$physical_home"* ]]
            then
                # Strip the physical home prefix
                local remainder="${target#$physical_home}"

                if [ -z "$remainder" ] || [ "$remainder" = "/" ]
                then
                    target="$home"
                else
                    target="$home$remainder"
                fi
            fi

            cd "$target"
            ;;
    esac
}

