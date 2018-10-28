# Test the main features of opustags on an Ogg Opus sample file.

use Test::More tests => 2;

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
