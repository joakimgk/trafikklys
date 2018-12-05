#!/usr/bin/perl

@program = (
	0b00111111,  # reset
	0b00110000,
	0b00101000,
	0b00100100,
	0b00101000
);

open(my $out, '>:raw', 'snake.bin');
foreach $b (@program) {
	my $byte = pack('C', $b);
	print $out "$byte";
}
close $out;

####

@program = (
	0b00111111,  # reset
	0b00111100,
	0b00100000
);

open(my $out, '>:raw', 'blink.bin');
foreach $b (@program) {
	my $byte = pack('C', $b);
	print $out "$byte";
}
close $out;

####

@program = (
	0b01111111,  # reset
	0b01110000,
	0b01101000,
	0b01100100,
	0b01101000
);


open(my $out, '>:raw', 'ignore.bin');
foreach $b (@program) {
	my $byte = pack('C', $b);
	print $out "$byte";
}
close $out;



print "ok\n";


