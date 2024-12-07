# GoL Backwards Minimizer Solver

A `lifesrc` backwards solver for Conway's "Game of Life" that minimizes live cells.

## Compile

1. Make sure `git` is installed
2. Clone the repository: `$ git clone https://github.com/lcsmuller/gol-sat`
3. Change into the newly created directory: `$ cd gol-sat`
4. Build: `$ make`

## Usage

```
Usage: ./gol-sat [OPTIONS]... PATTERN_FILE
Options:
    -h, --help             Display this help message
    -B, --borderDisable    Disable border check (default is false)
    -M, --minimizeDisable  Disable minimization of true literals (default is false)
```

Run `$ ./gol-sat pattern.txt` to perform a *backwards computation* consisting of 1 step that finally yields the pattern specified in the file `pattern.txt`. The solver will attempt to minimize the number of live cells, unless disabled with `-M`.

## Pattern Format
The text file format used for patterns starts with two numbers specifying the `width` and `height` of the pattern. Then `width` * `height` cell characters follow, where

- `.` or `0` is a dead cell
- `X` or `1` is an alive cell
- `?` is an unspecified cell

The `patterns` subdirectory contains some samples.

## Example
The following command performs backwards computation minimizing the amount of live cells, then yield the game of life pattern specified in the file `patterns/smily.txt`:

```console
$ ./gol-sat patterns/smily.txt 
-- Reading pattern from file: patterns/smily.txt
-- Searching for mt value: 55   | Timeout: 68 seconds
                -- Found solution for mt value: 34 (took 0 secs)
-- Searching for mt value: 16   | Timeout: 136 seconds
                -- No solution for mt value: 16
-- Searching for mt value: 25   | Timeout: 204 seconds
                -- No solution for mt value: 25
-- Searching for mt value: 29   | Timeout: 272 seconds
                -- No solution for mt value: 29
-- Searching for mt value: 31   | Timeout: 340 seconds
                -- No solution for mt value: 31
-- Searching for mt value: 32   | Timeout: 408 seconds
                -- No solution for mt value: 32
-- Searching for mt value: 33   | Timeout: 476 seconds
                -- Found solution for mt value: 33 (took 0 secs)
-- Minimum mt value for SAT solution: 33
11 10
. . . . . . . . . . .
. . . O O . O . . O .
. . O O . . O O . O .
. O O . . O . O O . .
. O . . . . . . . O .
. . . O . . . . . O .
. O O . O . O O O . .
. . O O . . O O . O .
. . . O . O O . . O .
. . . . . . . . . . .
```
