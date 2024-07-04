# aptinfo

## About

This tool allows querying apt index files. It is particulary useful when you want
to figure out dependencies of packages by only looking at an index file and not
having to have anything installed. You dont even need a distribution that uses
apt for this to work. This tool is also completely stand-alone and uses no libapt
library which keeps it small and fast.

## Howto

First you need an index file, you can get one for Ubuntu jammy main like this:

    $ curl -s http://archive.ubuntu.com/ubuntu/dists/jammy/main/binary-amd64/Packages.gz | gunzip > Packages

The acquired Packages file is what is refered to as an index file and is a
typical data source for aptinfo.

Show all dependencies for wget:

    $ aptinfo depends wget Packages

Show what packages depend on wget:

    $ aptinfo rdepends wget Packages

Show all packages needed in order to install wget:

    $ aptinfo resolve wget Packages

This last example will actually not work because one of the dependencies uses a
pipe character to tell you that you either need debconf or debconf-2.0 but none
of them has been included previously automatically.

In order to fix this, you need to manually provide any package that in some way
fulfills this requirement.

Actually show all packages needed to install wget:

    $ aptinfo resolve debconf,wget Packages

Alternatively using cdebconf could have worked as well since it provides the
name debconf-2.0 too:

    $ aptinfo resolve cdebconf,wget Packages

The third argument to aptinfo can actually be quite complex, you can list
multiple packages like in the example above using comma as a seperator but you
can also specify if a package should be of a certain version. In this example
debconf has to have a version less than 1.5.10.

    $ aptinfo resolve "debconf (<< 1.5.10), wget" Packages

This will of course give an error because there is no debconf package that
fulfills this criteria. But you get the picture.

The different comparison operators are =, <<, <=, >>, =>. What they mean should
be clear without any further explanation.

The versioning apt uses can actually be quite complicated. Therefor there is a
simple way to compare two versions to see how they compare to eachother.

Compare two versions:

    $ aptinfo compare '2:1.02.175-2.1ubuntu4' '>=' '2:1.02.175-1.1ubuntu4~'

Check the tests for more examples.

## Build and install

Not very complicated:

    $ make
    $ sudo make install [PREFIX=/usr/bin]

