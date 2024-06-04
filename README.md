# aptinfo

This tool allows querying apt index files. It is particulary useful when you want
to figure out dependencies of packages by only looking at an index file and not
having to have anything installed. You dont even need a distribution that uses
apt for this to work. This tool also is completely stand-alone and uses no libapt
library which keeps it small and fast.

First you need an index file, you can get the latest one from Ubuntu jammy main like so:

    curl -s http://archive.ubuntu.com/ubuntu/dists/jammy/main/binary-amd64/Packages.gz | gunzip > Packages

To show all dependencies for wget you run:

    ./aptinfo depends wget Packages

To show what packages depend on wget you run:

    ./aptinfo rdepends wget Packages

To show all dependencies recursively you run:

    ./aptinfo resolve wget Packages

This will give a warning because there is a dependencies that can be either
debconf or debconf-2.0. In order to fix that you need to provide it as well:

    ./aptinfo resolve debconf,wget Packages

The second argument can actually be quite complex, you can set specific versions
as well for instance:

    ./aptinfo resolve "debconf (<< 1.5.10)" Packages

Which will not be resolved satisfactory in in this case since debconf has a much
higher version in the Packages file.
