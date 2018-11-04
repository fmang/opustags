use strict;
use warnings;
use utf8;

use Test::More tests => 35;

use Digest::MD5;
use File::Basename;
use IPC::Open3;
use Symbol 'gensym';

my $opustags = './opustags';
BAIL_OUT("$opustags does not exist or is not executable") if (! -x $opustags);

my $t = dirname(__FILE__);
BAIL_OUT("'$t' contains unsupported characters") if $t !~ m|^[\w/]*$|;

####################################################################################################
# Tests related to the overall opustags executable, like the help message.
# No Opus file is manipulated here.

chomp(my $version = `$opustags --help | head -n 1`);
like($version, qr/^opustags version \d+\.\d+\.\d+$/, 'get the version string');

is(`$opustags`, <<"EOF", 'no options show the usage');
$version
Usage: opustags --help
       opustags [OPTIONS] FILE
       opustags OPTIONS FILE -o FILE
EOF
is($?, 0, 'no option is not an error'); # should it be?

my $help = <<"EOF";
$version

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

####################################################################################################
# Test the main features of opustags on an Ogg Opus sample file.

sub md5 {
	my ($file) = @_;
	open(my $fh, '<', $file) or return;
	my $ctx = Digest::MD5->new;
	$ctx->addfile($fh);
	$ctx->hexdigest
}

my $fh;

is(md5("$t/gobble.opus"), '111a483596ac32352fbce4d14d16abd2', 'the sample is the one we expect');
is(`./opustags $t/gobble.opus`, <<'EOF', 'read the initial tags');
encoder=Lavc58.18.100 libopus
EOF

# empty out.opus
open($fh, '>', "$t/out.opus") or die;
close($fh);
is(`./opustags $t/gobble.opus -o $t/out.opus 2>&1 >/dev/null`, <<"EOF", 'refuse to override');
'$t/out.opus' already exists (use -y to overwrite)
EOF
is(md5("$t/out.opus"), 'd41d8cd98f00b204e9800998ecf8427e', 'the output wasn\'t written');
is($?, 256, 'check the error code');

is(`./opustags $t/out.opus -o $t/out.opus 2>&1 >/dev/null`, <<'EOF', 'output and input can\'t be the same');
error: the input and output files are the same
EOF
is($?, 256, 'check the error code');

unlink("$t/out.opus");
is(`./opustags $t/gobble.opus -o $t/out.opus 2>&1`, '', 'copy the file without changes');
is(md5("$t/out.opus"), '111a483596ac32352fbce4d14d16abd2', 'the copy is faithful');
is($?, 0, 'check the error code');

is(`./opustags --in-place $t/out.opus -a A=B --add="A=C" --add "TITLE=Foo Bar" --delete A --add TITLE=七面鳥 --set encoder=whatever -s 1=2 -s X=1 -a X=2 -s X=3`, '', 'complex tag editing');
is($?, 0, 'updating the tags went well');
is(md5("$t/out.opus"), '66780307a6081523dc9040f3c47b0448', 'check the footprint');

$/ = undef;
open($fh, "./opustags $t/out.opus |");
binmode($fh, ':utf8');
is(<$fh>, <<'EOF', 'check the tags written');
A=B
A=C
TITLE=Foo Bar
TITLE=七面鳥
encoder=whatever
1=2
X=1
X=2
X=3
EOF
close($fh);

is(`./opustags $t/out.opus -d A -d foo -s X=4 -a TITLE=gobble -d TITLE`, <<'EOF', 'dry editing');
1=2
encoder=whatever
X=4
TITLE=gobble
EOF
is(md5("$t/out.opus"), '66780307a6081523dc9040f3c47b0448', 'the file did not change');

is(`./opustags -i $t/out.opus -a fatal=yes -a FOO -a BAR 2>&1`, <<'EOF', 'bad tag with --add');
invalid comment: 'FOO'
EOF
is($?, 256, 'exited with a failure code');
is(md5("$t/out.opus"), '66780307a6081523dc9040f3c47b0448', 'the file did not change');

is(`./opustags -i $t/out.opus -s fatal=yes -s FOO -s BAR 2>&1`, <<'EOF', 'bad tag with --set');
invalid comment: 'FOO'
EOF
is($?, 256, 'exited with a failure code');
is(md5("$t/out.opus"), '66780307a6081523dc9040f3c47b0448', 'the file did not change');

is(`./opustags $t/out.opus --delete-all -a OK=yes`, <<'EOF', 'delete all');
OK=yes
EOF

my ($pid, $pin, $pout, $perr);
$perr = gensym;

$pid = open3($pin, $pout, $perr, "./opustags $t/out.opus --set-all -a A=B -s X=Z -d OK");
binmode($pin, ':utf8');
binmode($pout, ':utf8');
print $pin <<'EOF';
OK=yes again
ARTIST=七面鳥
A=A
X=Y
EOF
close($pin);
is(<$pout>, <<'EOF', 'set all');
OK=yes again
ARTIST=七面鳥
A=A
X=Y
A=B
X=Z
EOF
waitpid($pid, 0);

$pid = open3($pin, $pout, $perr, "./opustags $t/out.opus --set-all");
print $pin <<'EOF';
whatever

# thing
!
wrong=yes
EOF
close($pin);
is(<$pout>, <<'EOF', 'bad tags are skipped with --set-all');
wrong=yes
EOF
is(<$perr>, <<'EOF', 'get warnings for malformed tags');
warning: skipping malformed tag
warning: skipping malformed tag
warning: skipping malformed tag
EOF
waitpid($pid, 0);
is($?, 0, 'non fatal');

unlink("$t/out.opus");
