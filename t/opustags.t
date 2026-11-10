#!/usr/bin/env perl

use strict;
use warnings;
use utf8;

use Test::More tests => 66;
use Test::Deep qw(cmp_deeply re);

use Digest::MD5;
use Encode qw(encode);
use File::Basename;
use File::Copy;
use File::Temp qw(tempfile);
use IPC::Open3;
use List::MoreUtils qw(any);
use Symbol 'gensym';

my $windows = $^O eq 'MSWin32';

my $opustags = $windows ? '../opustags.exe' : '../opustags';
BAIL_OUT("$opustags does not exist or is not executable") if (! -x $opustags);

# opustags.exe always runs in UTF-8 thanks to its manifest.
my $is_utf8 = $windows;
unless ($windows) {
	open(my $ctype, 'locale -k LC_CTYPE |');
	while (<$ctype>) { $is_utf8 = 1 if (/^charmap="UTF-?8"$/i) }
	close($ctype);
}
BAIL_OUT("this test must be run from an UTF-8 environment") unless $is_utf8;

# Perl on Windows translates line endings in pipes by default.
my $text_mode = $windows ? ':raw:utf8' : ':utf8';

# Perl on Windows passes arguments in the ANSI code page, which may not represent 七面鳥.
my $argv_encoding;
if ($windows) {
	require Win32;
	my $acp = Win32::GetACP();
	$argv_encoding = $acp == 65001 ? 'UTF-8' : "cp$acp";
}
my $argv_text = '七面鳥';
$argv_text = 'éàç'
	if $windows && !eval { encode($argv_encoding, $argv_text, Encode::FB_CROAK | Encode::LEAVE_SRC); 1 };
my $footprint = $argv_text eq '七面鳥' ? '66780307a6081523dc9040f3c47b0448'
                                      : 'c18f440161e657fb2db9a95dd9eee76c';

sub opustags {
	my %opt;
	%opt = %{pop @_} if ref $_[-1];
	my ($pid, $pin, $pout, $perr);
	$perr = gensym;
	if ($windows) {
		# The child process inherits the write end of pipes, and would never see their end.
		my ($in, $in_path) = tempfile();
		binmode($in, $opt{mode} // $text_mode);
		print $in $opt{in} if defined $opt{in};
		close($in);
		open($pin, '<', $in_path) or die "$in_path: $!";
		$pid = open3('<&' . fileno($pin), $pout, $perr, $opustags,
		             map { encode($argv_encoding, $_) } @_);
		close($pin);
		unlink($in_path);
	} else {
		$pid = open3($pin, $pout, $perr, $opustags, @_);
		binmode($pin, $opt{mode} // $text_mode);
	}
	binmode($pout, $opt{mode} // $text_mode);
	binmode($perr, $text_mode);
	local $/;
	unless ($windows) {
		print $pin $opt{in} if defined $opt{in};
		close $pin;
	}
	my $out = <$pout>;
	my $err = <$perr>;
	waitpid($pid, 0);
	[$out, $err, $?]
}

####################################################################################################
# Tests related to the overall opustags executable, like the help message.
# No Opus file is manipulated here.

is_deeply(opustags(), ['', <<EOF, 512], 'no options is a failure');
error: No arguments specified. Use -h for help.
EOF

my $help = opustags('--help');
$help->[0] =~ /^([^\n]*+)/;
my $version = $1;
like($version, qr/^opustags version (\d+\.\d+\.\d+)/, 'get the version string');

my $expected_help = qr{opustags version .*\n\nUsage: opustags --help\n};
cmp_deeply(opustags('--help'), [re($expected_help), '', 0], '--help displays the help message');
cmp_deeply(opustags('-h'), [re($expected_help), '', 0], '-h displays the help message too');

is_deeply(opustags('--derp'), ['', <<"EOF", 512], 'unrecognized option shows an error');
error: Unrecognized option '--derp'.
EOF

is_deeply(opustags($opustags), ['', <<"EOF", 256], 'not an Ogg stream');
$opustags: error: Input is not a valid Ogg file.
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
if ($windows) {
	ok(-w 'out.opus', 'new files are writable');
} else {
	is((stat 'out.opus')[2] & 0777, 0644, 'apply umask on new files');
}
umask($previous_umask);

# empty out.opus
{ my $fh; open($fh, '>', 'out.opus') and close($fh) or die }
is_deeply(opustags(qw(gobble.opus -o out.opus)), ['', <<'EOF', 256], 'refuse to override');
gobble.opus: error: 'out.opus' already exists. Use -y to overwrite.
EOF
is(md5('out.opus'), 'd41d8cd98f00b204e9800998ecf8427e', 'the output wasn\'t written');

my $null_device = $windows ? 'NUL' : '/dev/null';
is_deeply(opustags('gobble.opus', '-o', $null_device), ['', '', 0], "write to $null_device");

# The read-only attribute is the only part of the mode on Windows.
chmod($windows ? 0444 : 0604, 'out.opus');
is_deeply(opustags(qw(gobble.opus -o out.opus --overwrite)), ['', '', 0], 'overwrite');
is(md5('out.opus'), '111a483596ac32352fbce4d14d16abd2', 'successfully overwritten');
if ($windows) {
	ok(! -w 'out.opus', 'overwriting preserves the read-only attribute');
} else {
	is((stat 'out.opus')[2] & 0777, 0604, 'overwriting preserves output file\'s mode');
}

chmod($windows ? 0444 : 0700, 'out.opus');
is_deeply(opustags(qw(--in-place out.opus -a A=B --add=A=C --add), "TITLE=Foo Bar",
                   qw(--delete A --add), "TITLE=$argv_text", qw(--set encoder=whatever -s 1=2 -s X=1 -a X=2 -s X=3)),
          ['', '', 0], 'complex tag editing');
is(md5('out.opus'), $footprint, 'check the footprint');
if ($windows) {
	ok(! -w 'out.opus', 'in-place editing preserves the read-only attribute');
	chmod(0644, 'out.opus');
} else {
	is((stat 'out.opus')[2] & 0777, 0700, 'in-place editing preserves file mode');
}

is_deeply(opustags('out.opus'), [<<"EOF", '', 0], 'check the tags written');
A=B
A=C
TITLE=Foo Bar
TITLE=$argv_text
encoder=whatever
1=2
X=1
X=2
X=3
EOF

is_deeply(opustags(qw(out.opus -d A -d foo -s X=4 -a TITLE=gobble -d), "title=$argv_text"), [<<'EOF', '', 0], 'dry editing');
TITLE=Foo Bar
encoder=whatever
1=2
X=4
TITLE=gobble
EOF
is(md5('out.opus'), $footprint, 'the file did not change');

is_deeply(opustags(qw(-i out.opus -a fatal=yes -a FOO -a BAR)), ['', <<'EOF', 512], 'bad tag with --add');
error: Comment does not contain an equal sign: FOO.
EOF
is(md5('out.opus'), $footprint, 'the file did not change');

is_deeply(opustags('out.opus', '-D', '-a', "X=foobar\tquux"), [<<'END_OUT', <<'END_ERR', 0], 'control characters');
X=foobar	quux
END_OUT
warning: Some tags contain control characters.
END_ERR

is_deeply(opustags('out.opus', '-D', '-a', "X=foo\n\nbar"), [<<'END_OUT', '', 0], 'newline characters');
X=foo
	
	bar
END_OUT

is_deeply(opustags(qw(-i out.opus -s fatal=yes -s FOO -s BAR)), ['', <<'EOF', 512], 'bad tag with --set');
error: Comment does not contain an equal sign: FOO.
EOF
is(md5('out.opus'), $footprint, 'the file did not change');

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

if ($windows) {
	open(my $editor, '>', 'editor.pl') or die;
	print $editor 'my $f = $ARGV[-1]; local $/; open(my $in, "<:raw", $f) or die; '
	            . 'my $t = <$in>; close($in); $t =~ tr/aeiou/AEIOU/; '
	            . 'open(my $out, ">:raw", $f) or die; print $out $t;';
	close($editor);
	$ENV{EDITOR} = qq{"$^X" editor.pl};
} else {
	$ENV{EDITOR} = 'sed -i -e y/aeiou/AEIOU/ `sleep 0.1`';
}
is_deeply(opustags('gobble.opus', '-eo', "'screaming !'.opus"), ['', '', 0], 'edit a file with EDITOR');
is(md5("'screaming !'.opus"), '56e85ccaa83a13c15576d75bbd6d835f', 'the tags were modified');

$ENV{EDITOR} = $windows ? 'cmd /c exit 0' : 'true';
is_deeply(opustags('-ie', "'screaming !'.opus"), ['', "Cancelling edition because the tags file was not modified.\n", 256], 'close -e without saving');
is(md5("'screaming !'.opus"), '56e85ccaa83a13c15576d75bbd6d835f', 'the tags were not modified');

$ENV{EDITOR} = $windows ? 'cmd /c exit 1' : 'false';
is_deeply(opustags('-ie', "'screaming !'.opus"), ['', "'screaming !'.opus: error: Child process exited with 1\n", 256], 'editor exiting with an error');
is(md5("'screaming !'.opus"), '56e85ccaa83a13c15576d75bbd6d835f', 'the tags were not modified');

unlink("'screaming !'.opus");
unlink('editor.pl') if $windows;

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

my $locale = 'en_US.iso88591';
my @all_locales = $windows ? () : split(' ', `locale -a`);

SKIP: {
skip "locale $locale is not present", 5 unless (any { $_ eq $locale } @all_locales);

opustags(qw(gobble.opus -a TITLE=七面鳥 -a ARTIST=éàç -o out.opus -y));

local $ENV{LC_ALL} = $locale;
local $ENV{LANGUAGE} = '';

is_deeply(opustags(qw(-S out.opus), {in => <<"END_IN", mode => ':raw'}), [<<"END_OUT", '', 0], 'set all in ISO-8859-1');
T=\xef\xef\xf6
END_IN
T=\xef\xef\xf6
END_OUT

is_deeply(opustags('-i', 'out.opus', "--add=I=\xf9\xce", {mode => ':raw'}), ['', '', 0], 'write tags in ISO-8859-1');

is_deeply(opustags('out.opus', {mode => ':raw'}), [<<"END_OUT", <<"END_ERR", 256], 'read tags in ISO-8859-1 with incompatible characters');
encoder=Lavc58.18.100 libopus
END_OUT
out.opus: error: Invalid or incomplete multibyte or wide character. See --raw.
END_ERR

is_deeply(opustags(qw(out.opus -d TITLE -d ARTIST), {mode => ':raw'}), [<<"END_OUT", '', 0], 'read tags in ISO-8859-1');
encoder=Lavc58.18.100 libopus
I=\xf9\xce
END_OUT

$ENV{LC_ALL} = '';

is_deeply(opustags('out.opus'), [<<"END_OUT", '', 0], 'read tags in UTF-8');
encoder=Lavc58.18.100 libopus
TITLE=七面鳥
ARTIST=éàç
I=ùÎ
END_OUT

unlink('out.opus');
}

####################################################################################################
# Raw edition

if ($windows) {
	# Windows command lines are UTF-16 and cannot carry invalid UTF-8.
	is_deeply(opustags(qw(-S gobble.opus -o out.opus --raw), {in => <<"END_IN", mode => ':raw'}), ['', '', 0], 'raw set-all with binary data');
T=\xFF
U=\xFE
END_IN
} else {
	is_deeply(opustags(qw(-S gobble.opus -o out.opus --raw -a), "U=\xFE", {in => <<"END_IN", mode => ':raw'}), ['', '', 0], 'raw set-all with binary data');
T=\xFF
END_IN
}

is_deeply(opustags(qw(out.opus --raw), { mode => ':raw' }), [<<"END_OUT", '', 0], 'raw read');
T=\xFF
U=\xFE
END_OUT

unlink('out.opus');

####################################################################################################
# Multiple-page tags

my $big_tags = "DATA=x\n" x 15000; # > 90K, which is over the max page size of 64KiB.
is_deeply(opustags(qw(-S gobble.opus -o out.opus), {in => $big_tags}), ['', '', 0], 'write multi-page header');
is_deeply(opustags('out.opus'), [$big_tags, '', 0], 'read multi-page header');
is_deeply(opustags(qw(out.opus -i -D -a), 'encoder=Lavc58.18.100 libopus'), ['', '', 0], 'shrink the header');
is(md5('out.opus'), '111a483596ac32352fbce4d14d16abd2', 'the result is identical to the original file');
unlink('out.opus');

####################################################################################################
# Cover arts

is_deeply(opustags(qw(-D --set-cover pixel.png gobble.opus -o out.opus)), ['', '', 0], 'set the cover');
is_deeply(opustags(qw(--output-cover out.png out.opus)), [<<'END_OUT', '', 0], 'extract the cover');
METADATA_BLOCK_PICTURE=AAAAAwAAAAlpbWFnZS9wbmcAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEWJUE5HDQoaCgAAAA1JSERSAAAAAQAAAAEIAgAAAJB3U94AAAAMSURBVAjXY/j//z8ABf4C/tzMWecAAAAASUVORK5CYII=
END_OUT
is(md5('out.png'), md5('pixel.png'), 'the extracted cover is identical to the one set');
unlink('out.opus');
unlink('out.png');

is_deeply(opustags(qw(-D --set-cover - gobble.opus), { in => "GIF8 x" }), [<<'END_OUT', '', 0], 'read the cover from stdin');
METADATA_BLOCK_PICTURE=AAAAAwAAAAlpbWFnZS9naWYAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAZHSUY4IHg=
END_OUT

####################################################################################################
# Vendor string

is_deeply(opustags(qw(--vendor gobble.opus)), ["Lavf58.12.100\n", '', 0], 'print the vendor string');

is_deeply(opustags(qw(--set-vendor opustags gobble.opus -o out.opus)), ['', '', 0], 'set the vendor string');
is_deeply(opustags(qw(--vendor out.opus)), ["opustags\n", '', 0], 'the vendor string was updated');
unlink('out.opus');

####################################################################################################
# Multi-line tags

is_deeply(opustags(qw(--set-all gobble.opus -o out.opus), { in => "MULTILINE=one\n\ttwo\nSIMPLE=three\n" }), ['', '', 0], 'parses continuation lines');
is_deeply(opustags(qw(out.opus -z)), ["MULTILINE=one\ntwo\0SIMPLE=three\0", '', 0], 'delimits output with NUL on -z');
unlink('out.opus');

is_deeply(opustags(qw(--set-all gobble.opus -o out.opus -z), { in => "MULTILINE=one\ntwo\0SIMPLE=three\0" }), ['', '', 0], 'delimits input with NUL on -z');
is_deeply(opustags(qw(out.opus)), [<<'END', '', 0], 'indents continuation lines');
MULTILINE=one
	two
SIMPLE=three
END
unlink('out.opus');
