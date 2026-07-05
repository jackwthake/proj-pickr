# proj-pickr

Simple folder picker dialogue meant to run on terminal startup.

## Usage

```bash
proj-pickr <directory>
```

Lists the subfolders of `<directory>` and lets you pick one:

| Key | Action |
|-----|--------|
| `↑` / `↓` | Move the selection |
| `Enter` | Select the highlighted folder — prints its full path to stdout |
| `q` | Quit without selecting anything |

Run with no argument (or more than one), and it prints a usage message and
exits with status 1. If `<directory>` has no subfolders, it prints
`No subfolders.` and exits with status 1.

`proj-pickr` only ever prints the final selected path to stdout — the
interactive menu itself is drawn on stderr — so it composes with command
substitution (see the fish wrapper below).

## Building

``` bash
./build.sh
install -Dm755 "proj-pickr" "$HOME/.local/bin/proj-pickr"
```

for it to switch folders correctly it needs to be linked up with the terminal. Here is an example for the fish terminal:

``` bash
function pp
    set dir (proj-pickr $argv)
    if test -n "$dir"
        cd $dir
    end
end
```