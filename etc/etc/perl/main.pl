#!/usr/bin/perl

use warnings;
use strict;
use File::Spec;
use File::Spec::Functions;
use Cwd 'abs_path';
{
    package DirWalker;

    sub new
    {
        my $class = shift;
        my $self = {
            startdir => shift,
        };
        bless $self, $class;
        return $self;
    }

    sub dwalk
    {
        my $item;
        my $fullpath;
        my $type;
        my ($self, $dir, $func) = @_;
        #printf("opendir(\"%s\") ...\n", $dir);
        opendir(my $handle, $dir) or die("failed to open \"$dir\" for reading: $!");
        while(($item = readdir($handle)))
        {
            next if(($item eq ".") || ($item eq ".."));
            $fullpath = File::Spec->catfile($dir, $item);
            if(-d $fullpath)
            {
                $self->dwalk($fullpath, $func);
                $type = 'd';
                next;
            }
            elsif(-f $fullpath)
            {
                $type = 'f';
            }
            else
            {
                $type = '?';
            }
            $func->($fullpath, $type);
        };
    }

    sub walk
    {
        my $start;
        my ($self, $func) = @_;
        $start = $self->{"startdir"};
        $self->dwalk($start, $func);
    }

    1;
};

sub main
{
=begin
    my $dir;
    my $item;
    $dir = abs_path($ARGV[0] or ".");
    opendir my $dh, $dir or die "Could not open '$dir' for reading: $!\n";
    while(($item = readdir($dh)))
    {
        next if (($item eq ".") || ($item eq ".."));
        my $fullp = (catfile($dir, $item));
        printf("%s\n", $fullp);
    }
=cut
    # deja-vu!
    my $initiald = abs_path($ARGV[0] or ".");
    my $walker = new DirWalker($initiald);
    $walker->walk(sub{
        my $path = shift;
        my $type = shift;
        printf("+ (%s) %s\n", $type, $path);
    });

};

main;