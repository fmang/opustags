# Tests related to the overall opustags executable, like the help message.
# No Opus file is manipulated here.

use strict;
use warnings;

use Test::More tests => 6;

my $opustags = './opustags';
chomp(my $version = `git describe --tags --abbrev=0`);

BAIL_OUT("$opustags does not exist or is not executable") if (! -x $opustags);

is(`$opustags`, <<"EOF", 'no options show the usage');
opustags version $version
Usage: opustags --help
       opustags [OPTIONS] FILE
       opustags OPTIONS FILE -o FILE
EOF

my $help = <<"EOF";
opustags version $version

Usage: opustags --help
       opustags [OPTIONS] FILE
       opustags OPTIONS FILE -o FILE

Options:
  -h, --help              print this help
  -o, --output            write the modified tags to a file
  -i, --in-place [SUFFIX] use a temporary file then replace the original file
  -y, --overwrite         overwrite the output file if it already exists
  -d, --delete FIELD      delete all the fields of a specified type
  -a, --add FIELD=VALUE   add a field
  -s, --set FIELD=VALUE   delete then add a field
  -D, --delete-all        delete all the fields!
  -S, --set-all           read the fields from stdin

See the man page for extensive documentation.
EOF

is(`$opustags --help`, $help, '--help displays the help message');
is($?, 0, '--help returns 0');

is(`$opustags --h`, $help, '-h displays the help message too');

is(`$opustags --derp 2>&1`, <<'EOF', 'unrecognized option shows an error');
./opustags: unrecognized option '--derp'
EOF
is($?, 256, 'unrecognized option causes return code 256');
