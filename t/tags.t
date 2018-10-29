# Test the main features of opustags on an Ogg Opus sample file.

use strict;
use warnings;
use utf8;

use Test::More tests => 18;

use Digest::MD5;

sub md5 {
	my ($file) = @_;
	open(my $fh, '<', $file) or return;
	my $ctx = Digest::MD5->new;
	$ctx->addfile($fh);
	$ctx->hexdigest
}

my $fh;

is(md5('t/gobble.opus'), '111a483596ac32352fbce4d14d16abd2', 'the sample is the one we expect');
is(`./opustags t/gobble.opus`, <<'EOF', 'read the initial tags');
encoder=Lavc58.18.100 libopus
EOF

# empty out.opus
open($fh, '> t/out.opus') or die;
close($fh);
is(`./opustags t/gobble.opus -o t/out.opus 2>&1 >/dev/null`, <<'EOF', 'refuse to override');
't/out.opus' already exists (use -y to overwrite)
EOF
is(md5('t/out.opus'), 'd41d8cd98f00b204e9800998ecf8427e', 'the output wasn\'t written');
is($?, 256, 'check the error code');

is(`./opustags t/out.opus -o t/out.opus 2>&1 >/dev/null`, <<'EOF', 'output and input can\'t be the same');
error: the input and output files are the same
EOF
is($?, 256, 'check the error code');

unlink('t/out.opus');
is(`./opustags t/gobble.opus -o t/out.opus 2>&1`, '', 'copy the file without changes');
is(md5('t/out.opus'), '111a483596ac32352fbce4d14d16abd2', 'the copy is faithful');
is($?, 0, 'check the error code');

is(`./opustags --in-place t/out.opus -a A=B --add="A=C" --add "TITLE=Foo Bar" --delete A --add TITLE=七面鳥 --set encoder=whatever -s 1=2 -s X=1 -a X=2 -s X=3`, '', 'complex tag editing');
is($?, 0, 'updating the tags went well');
is(md5('t/out.opus'), '66780307a6081523dc9040f3c47b0448', 'check the footprint');

$/ = undef;
open($fh, './opustags t/out.opus |');
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

is(`./opustags t/out.opus -d A -d foo -s X=4 -a TITLE=gobble -d TITLE`, <<'EOF', 'dry editing');
1=2
encoder=whatever
X=4
TITLE=gobble
EOF
is(md5('t/out.opus'), '66780307a6081523dc9040f3c47b0448', 'the file did not change');

is(`./opustags t/out.opus --delete-all -a OK=yes`, <<'EOF', 'delete all');
OK=yes
EOF

is(`echo OK='yes again' | ./opustags t/out.opus --set-all`, <<'EOF', 'set all');
OK=yes again
EOF
