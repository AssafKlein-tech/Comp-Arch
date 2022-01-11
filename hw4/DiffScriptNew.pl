#! /usr/bin/perl

use strict; 
use warnings; 


system('make > /dev/null');
mkdir("Memlogs");
mkdir("Outputs");
my $curdir = `pwd`;
chdir "tests" or die "No 'tests' dir";
my @Inputs = sort { ($a =~ /(\d+)/)[0] <=> ($b =~ /(\d+)/)[0] } glob( '*' );
chomp $curdir; 
chdir($curdir);
my @Flags = ( "" , "-s" , "-f");


foreach my $file (@Inputs) {
	foreach my $Flag (@Flags) {
		
		my ($DiffFlag, $MemFlag) = (0,0); 
		my $AppendedFile = substr($file, 0, -4).$Flag; 
		my $CmdLine;
		if($file =~ /example[1-4]\./ ) {
			$CmdLine ="./sim_main tests/$file 43 $Flag";
		}
		elsif ($file =~ /example12\./) {
			$CmdLine="./sim_main tests/$file 300 $Flag";
		}
		else {
			$CmdLine="./sim_main tests/$file 80 $Flag";
		}

		system("$CmdLine > Outputs/$AppendedFile.log");
		my $Res = `diff -U 0 Outputs/$AppendedFile.log results/$AppendedFile.log | grep ^@ | wc -l`;
		if ($Res <= 1){
			print "$AppendedFile\t[Diff  = SUCCESS!]\t";
		} else {
			print "$AppendedFile\t[Diff  = Failure..]\t";
			$DiffFlag = 1; 
		}
		system("valgrind --tool=memcheck --leak-check=full --track-origins=yes $CmdLine >./Memlogs/${AppendedFile}_memory.log 2>&1");
		my $num_of_errors=`grep "ERROR SUMMARY:" ./Memlogs/${AppendedFile}_memory.log | cut -f4 -d" "`;
		my $NoLeaksFlag=`grep "no leaks are possible" ./Memlogs/${AppendedFile}_memory.log | wc -l`;

		if ( $num_of_errors==0 && $NoLeaksFlag==1) {
			print "[Memory  = SUCCESS!]\n";
		}
		else {
			print "[Memory  = Failure..]\n";
			$MemFlag =1; 
		} 	
		print "run \"gvimdiff Outputs/$AppendedFile.log results/$AppendedFile.log\"\n" if($DiffFlag); 
		print "Relevant memory log at: /Memlogs/${AppendedFile}_memory.log\n" if($MemFlag); 
	}
}