# Test the main features of opustags on an Ogg Opus sample file.

use Test::More tests => 14;

use Digest::MD5;

sub md5 {
	my ($file) = @_;
	open(my $fh, '<', $file) or return;
	my $ctx = Digest::MD5->new;
	$ctx->addfile($fh);
	$ctx->hexdigest
}

is(md5('t/gobble.opus'), '111a483596ac32352fbce4d14d16abd2', 'the sample is the one we expect');
is(`./opustags t/gobble.opus`, <<'EOF', 'read the initial tags');
encoder=Lavc58.18.100 libopus
EOF

unlink('t/out.opus');
is(`./opustags t/gobble.opus -o t/out.opus 2>&1`, '', 'copy the file without changes');
is(md5('t/out.opus'), '111a483596ac32352fbce4d14d16abd2', 'the copy is faithful');
is($?, 0, 'check the error code');

# empty out.opus
open(my $fh, '> t/out.opus') or die;
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

is(`./opustags t/gobble.opus -a A=B`, <<'EOF', 'dry add');
encoder=Lavc58.18.100 libopus
A=B
EOF

is(`./opustags t/gobble.opus --add A=B --output t/out.opus -y 2>&1`, '', 'add a tag');
is(md5('t/out.opus'), '7a0bb7f46edf7d5bb735c32635f145fd', 'check the footprint of the result');

is(`./opustags t/out.opus`, <<'EOF', 'the added tag is read');
encoder=Lavc58.18.100 libopus
A=B
EOF
