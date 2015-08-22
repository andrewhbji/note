# Pandoc ‘¥¥a∞≤—b≤ΩÛE

## Introduction

If you want to live dangerously, you can install the still-in-progress pandoc from github.  You'll need git and the [Haskell Platform](http://www.haskell.org/platform/).

## Getting code and building pandoc

First, you'll need to install the new pandoc-types from the github repository. If `git` and `cabal` are in your path, these commands will work on both Unix and Windows:

    git clone git://github.com/jgm/pandoc-types
    cd pandoc-types
    cabal update
    cabal install --force
    cd ..

Now install pandoc:

    git clone git://github.com/jgm/pandoc
    cd pandoc
    git submodule update --init
    cabal install --force --enable-tests
    cabal test
    cd ..

And finally pandoc-citeproc:

    git clone git://github.com/jgm/pandoc-citeproc
    cd pandoc-citeproc
    cabal install --enable-tests
    cabal test
    cd ..

## Running pandoc

On Linux, the built pandoc can now be run via:

    ~/.cabal/bin/pandoc

On Windows, the built pandoc can now be run via:

    pandoc\dist\build\pandoc\pandoc.exe

## Extra info for Haskell/Ubuntu novices

Here is one route that successfully sets up an environment to develop pandoc on.

For all these commands, after several attempts, I ended up putting them into a script and saving the output, as several earlier attempts, with different combinations of version, gave errors.

1. Set up an Ubuntu 14.04 environment
1. Run the following commands:

        sudo apt-get update
        sudo apt-get install git
        sudo apt-get install haskell-platform

    The update command is needed. Without this, there are errors about incorrect versions. (I didn't record the messages)

1. Run the commands in the main section of this page.