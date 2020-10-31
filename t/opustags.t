#!/usr/bin/env perl

use strict;
use warnings;
use utf8;

use Test::More tests => 47;

use Digest::MD5;
use File::Basename;
use File::Copy;
use IPC::Open3;
use List::MoreUtils qw(any);
use Symbol 'gensym';

my $opustags = '../opustags';
BAIL_OUT("$opustags does not exist or is not executable") if (! -x $opustags);

my $is_utf8;
open(my $ctype, 'locale -k LC_CTYPE |');
while (<$ctype>) { $is_utf8 = 1 if (/^charmap="UTF-?8"$/i) }
close($ctype);
BAIL_OUT("this test must be run from an UTF-8 environment") unless $is_utf8;

sub opustags {
	my %opt;
	%opt = %{pop @_} if ref $_[-1];
	my ($pid, $pin, $pout, $perr);
	$perr = gensym;
	$pid = open3($pin, $pout, $perr, $opustags, @_);
	binmode($pin, $opt{mode} // ':utf8');
	binmode($pout, $opt{mode} // ':utf8');
	binmode($perr, ':utf8');
	local $/;
	print $pin $opt{in} if defined $opt{in};
	close $pin;
	my $out = <$pout>;
	my $err = <$perr>;
	waitpid($pid, 0);
	[$out, $err, $?]
}

####################################################################################################
# Tests related to the overall opustags executable, like the help message.
# No Opus file is manipulated here.

is_deeply(opustags(), ['', <<EOF, 256], 'no options is a failure');
error: No arguments specified. Use -h for help.
EOF

my $help = opustags('--help');
$help->[0] =~ /^([^\n]*+)/;
my $version = $1;
like($version, qr/^opustags version (\d+\.\d+\.\d+)/, 'get the version string');

my $expected_help = <<"EOF";
$version

Usage: opustags --help
       opustags [OPTIONS] FILE
       opustags OPTIONS -i FILE...
       opustags OPTIONS FILE -o FILE

Options:
  -h, --help                    print this help
  -o, --output FILE             specify the output file
  -i, --in-place                overwrite the input files
  -y, --overwrite               overwrite the output file if it already exists
  -a, --add FIELD=VALUE         add a comment
  -d, --delete FIELD[=VALUE]    delete previously existing comments
  -D, --delete-all              delete all the previously existing comments
  -s, --set FIELD=VALUE         replace a comment
  -S, --set-all                 import comments from standard input
  -e, --edit                    edit tags interactively in VISUAL/EDITOR

See the man page for extensive documentation.
EOF

is_deeply(opustags('--help'), [$expected_help, '', 0], '--help displays the help message');
is_deeply(opustags('-h'), [$expected_help, '', 0], '-h displays the help message too');

is_deeply(opustags('--derp'), ['', <<"EOF", 256], 'unrecognized option shows an error');
error: Unrecognized option '--derp'.
EOF

is_deeply(opustags('../opustags'), ['', <<"EOF", 256], 'not an Ogg stream');
../opustags: error: Input is not a valid Ogg file.
EOF

####################################################################################################
# Test the main features of opustags on an Ogg Opus sample file.

sub md5 {
	my ($file) = @_;
	open(my $fh, '<', $file) or return;
	my $ctx = Digest::MD5->new;
	$ctx->addfile($fh);
	$ctx->hexdigest
}

is(md5('gobble.opus'), '111a483596ac32352fbce4d14d16abd2', 'the sample is the one we expect');
is_deeply(opustags('gobble.opus'), [<<'EOF', '', 0], 'read the initial tags');
encoder=Lavc58.18.100 libopus
EOF

unlink('out.opus');
my $previous_umask = umask(0022);
is_deeply(opustags(qw(gobble.opus -o out.opus)), ['', '', 0], 'copy the file without changes');
is(md5('out.opus'), '111a483596ac32352fbce4d14d16abd2', 'the copy is faithful');
is((stat 'out.opus')[2] & 0777, 0644, 'apply umask on new files');
umask($previous_umask);

# empty out.opus
{ my $fh; open($fh, '>', 'out.opus') and close($fh) or die }
is_deeply(opustags(qw(gobble.opus -o out.opus)), ['', <<'EOF', 256], 'refuse to override');
gobble.opus: error: 'out.opus' already exists. Use -y to overwrite.
EOF
is(md5('out.opus'), 'd41d8cd98f00b204e9800998ecf8427e', 'the output wasn\'t written');

is_deeply(opustags(qw(gobble.opus -o /dev/null)), ['', '', 0], 'write to /dev/null');

chmod(0604, 'out.opus');
is_deeply(opustags(qw(gobble.opus -o out.opus --overwrite)), ['', '', 0], 'overwrite');
is(md5('out.opus'), '111a483596ac32352fbce4d14d16abd2', 'successfully overwritten');
is((stat 'out.opus')[2] & 0777, 0604, 'overwriting preserves output file\'s mode');

chmod(0700, 'out.opus');
is_deeply(opustags(qw(--in-place out.opus -a A=B --add=A=C --add), "TITLE=Foo Bar",
                   qw(--delete A --add TITLE=七面鳥 --set encoder=whatever -s 1=2 -s X=1 -a X=2 -s X=3)),
          ['', '', 0], 'complex tag editing');
is(md5('out.opus'), '66780307a6081523dc9040f3c47b0448', 'check the footprint');
is((stat 'out.opus')[2] & 0777, 0700, 'in-place editing preserves file mode');

is_deeply(opustags('out.opus'), [<<'EOF', '', 0], 'check the tags written');
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

is_deeply(opustags(qw(out.opus -d A -d foo -s X=4 -a TITLE=gobble -d title=七面鳥)), [<<'EOF', '', 0], 'dry editing');
TITLE=Foo Bar
encoder=whatever
1=2
X=4
TITLE=gobble
EOF
is(md5('out.opus'), '66780307a6081523dc9040f3c47b0448', 'the file did not change');

is_deeply(opustags(qw(-i out.opus -a fatal=yes -a FOO -a BAR)), ['', <<'EOF', 256], 'bad tag with --add');
error: Comment does not contain an equal sign: FOO.
EOF
is(md5('out.opus'), '66780307a6081523dc9040f3c47b0448', 'the file did not change');

is_deeply(opustags('out.opus', '-D', '-a', "X=foo\nbar\tquux"), [<<'END_OUT', <<'END_ERR', 0], 'control characters');
X=foo
bar	quux
END_OUT
warning: Some tags contain newline characters. These are not supported by --set-all.
warning: Some tags contain control characters.
END_ERR

is_deeply(opustags(qw(-i out.opus -s fatal=yes -s FOO -s BAR)), ['', <<'EOF', 256], 'bad tag with --set');
error: Comment does not contain an equal sign: FOO.
EOF
is(md5('out.opus'), '66780307a6081523dc9040f3c47b0448', 'the file did not change');

is_deeply(opustags(qw(out.opus --delete-all -a OK=yes)), [<<'EOF', '', 0], 'delete all');
OK=yes
EOF

is_deeply(opustags(qw(out.opus --set-all -a A=B -s X=Z -d OK), {in => <<'END_IN'}), [<<'END_OUT', '', 0], 'set all');
OK=yes again
ARTIST=七面鳥

A=A
X=Y
#IGNORE=COMMENTS
END_IN
OK=yes again
ARTIST=七面鳥
A=A
X=Y
A=B
X=Z
END_OUT

is_deeply(opustags(qw(out.opus -S), {in => <<'END_IN'}), [<<'END_OUT', <<'END_ERR', 256], 'set all with bad tags');
whatever
wrong=yes
END_IN
END_OUT
error: Malformed tag: whatever
END_ERR

sub slurp {
	my ($filename) = @_;
	local $/;
	open(my $fh, '<', $filename);
	binmode($fh);
	my $data = <$fh>;
	$data
}

my $data = slurp 'out.opus';
is_deeply(opustags('-', '-o', '-', {in => $data, mode => ':raw'}), [$data, '', 0], 'read opus from stdin and write to stdout');

unlink('out.opus');

# Test --in-place
unlink('out2.opus');
copy('gobble.opus', 'out.opus');
is_deeply(opustags(qw(out.opus --add BAR=baz -o out2.opus)), ['', '', 0], 'process multiple files with --in-place');
is_deeply(opustags(qw(--in-place --add FOO=bar out.opus out2.opus)), ['', '', 0], 'process multiple files with --in-place');
is(md5('out.opus'), '30ba30c4f236c09429473f36f8f861d2', 'the tags were added correctly (out.opus)');
is(md5('out2.opus'), '0a4d20c287b2e46b26cb0eee353c2069', 'the tags were added correctly (out2.opus)');

unlink('out.opus');
unlink('out2.opus');

####################################################################################################
# Interactive edition

$ENV{EDITOR} = 'sed -i -e y/aeiou/AEIOU/ `sleep 0.1`';
is_deeply(opustags(qw(gobble.opus -o screaming.opus -e)), ['', '', 0], 'edit a file with EDITOR');
is(md5('screaming.opus'), '56e85ccaa83a13c15576d75bbd6d835f', 'the tags were modified');

$ENV{EDITOR} = 'true';
is_deeply(opustags(qw(-i screaming.opus -e)), ['', "Cancelling edition because the tags file was not modified.\n", 256], 'close -e without saving');
is(md5('screaming.opus'), '56e85ccaa83a13c15576d75bbd6d835f', 'the tags were not modified');

$ENV{EDITOR} = 'false';
is_deeply(opustags(qw(-i screaming.opus -e)), ['', "screaming.opus: error: Child process exited with 1\n", 256], 'editor exiting with an error');
is(md5('screaming.opus'), '56e85ccaa83a13c15576d75bbd6d835f', 'the tags were not modified');

unlink('screaming.opus');

####################################################################################################
# Test muxed streams

system('ffmpeg -loglevel error -y -i gobble.opus -c copy -map 0:0 -map 0:0 -shortest muxed.ogg') == 0
	or BAIL_OUT('could not create a muxed stream');

is_deeply(opustags('muxed.ogg'), ['', <<'END_ERR', 256], 'muxed streams detection');
muxed.ogg: error: Muxed streams are not supported yet.
END_ERR

unlink('muxed.ogg');

####################################################################################################
# Locale

my $locale = 'fr_FR.iso88591';
my @all_locales = split(' ', `locale -a`);

SKIP: {
skip "locale $locale is not present", 4 unless (any { $_ eq $locale } @all_locales);

opustags(qw(gobble.opus -a TITLE=七面鳥 -a ARTIST=éàç -o out.opus -y));

local $ENV{LC_ALL} = $locale;

is_deeply(opustags(qw(-S out.opus), {in => <<"END_IN", mode => ':raw'}), [<<"END_OUT", '', 0], 'set all in ISO-8859-1');
T=\xef\xef\xf6
END_IN
T=\xef\xef\xf6
END_OUT

is_deeply(opustags('-i', 'out.opus', "--add=I=\xf9\xce", {mode => ':raw'}), ['', '', 0], 'write tags in ISO-8859-1');

is_deeply(opustags('out.opus', {mode => ':raw'}), [<<"END_OUT", <<'END_ERR', 0], 'read tags in ISO-8859-1');
encoder=Lavc58.18.100 libopus
TITLE=???
ARTIST=\xe9\xe0\xe7
I=\xf9\xce
END_OUT
warning: Some tags have been transliterated to your system encoding.
END_ERR

$ENV{LC_ALL} = '';

is_deeply(opustags('out.opus'), [<<"END_OUT", '', 0], 'read tags in UTF-8');
encoder=Lavc58.18.100 libopus
TITLE=七面鳥
ARTIST=éàç
I=ùÎ
END_OUT
}
