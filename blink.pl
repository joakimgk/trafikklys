#!/usr/bin/perl

@program = (# 0b001-----
	0b00111111,  # reset
	0b00110000,
	0b00101000,
	0b00100100,
	0b00101000,
	0b00110000,
	0b00111100,
	0b00100000,
	0b00111100,
	0b00100000,
	0b00111100,
	0b00000011   # EOF / start-instruction

);

open(my $out, '>:raw', 'blink.bin');
foreach $b (@program) {
	my $byte = pack('C', $b);
	print $out "$byte";
}
close $out;

print "ok\n";


