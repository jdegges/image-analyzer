#! /usr/bin/perl
#
# generates filters/filters.h and filters/filters.c
#

# generate filters.h

my $header =
'/******************************************************************************
 * Copyright (c) 2008 Joey Degges
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *****************************************************************************/
';

my $body1 =
'
#ifndef _H_FILTERS
#define _H_FILTERS

#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "../common.h"
#include "../image_analyzer.h"
#include "../ia_sequence.h"

';

my $body2 =
'
static inline int offset( int w, int x, int y, int p )
{
    return w*3*y + x*3+p;
}

#define O( y,x ) (s->param->i_width*3*y + x*3)

';

my $body3 =
'
/* Set up the filter function pointers */
typedef void (*init_funcs)(ia_seq_t*, ia_filter_param_t*);
typedef void (*exec_funcs)(ia_seq_t*, ia_filter_param_t*, ia_image_t**, ia_image_t*);
typedef void (*clos_funcs)(ia_filter_param_t*);

typedef struct ia_filters_t
{
    init_funcs      init[20];
    exec_funcs      exec[20];
    clos_funcs      clos[20];
} ia_filters_t;

ia_filters_t filters;

void init_filters( void );

#endif
';

open( FILTERS_DOT_H, ">filters/filters.h" );

print FILTERS_DOT_H $header;
print FILTERS_DOT_H $body1;

my @filters = `ls filters/ | grep -i \.h | grep -v filters | sort`;
chop(@filters);

my $fno = 1;
print FILTERS_DOT_H "/* Filters Indexes */\n";
foreach $filter ( @filters )
{
    $name = "\U$filter\E";
    $name =~ s/\.H//;
    printf( FILTERS_DOT_H "#define %-16s$fno\n", $name);
    $fno++;
}

print FILTERS_DOT_H "\nstatic const char FILTERS[][30] = {\n";
foreach $filter ( @filters )
{
    $name = "\U$filter\E";
    $name =~ s/\.H//;
    print FILTERS_DOT_H "    {\"$name\"},\n";
}

print FILTERS_DOT_H "    {-1}\n};\n";
print FILTERS_DOT_H $body2;

#my $fno = 1;
#print FILTERS_DOT_H "/* Import filters */\n";
#foreach $filter ( @filters )
#{
#    printf( FILTERS_DOT_H "#include \"%s\"\n", $filter, $fno);
#    $fno++;
#}

print FILTERS_DOT_H $body3;

close( FILTERS_DOT_H );

# generate filters.c

$body1 = 
'
#include "filters.h"

';

$body2 =
'
void init_filters( void )
{
';

$body3 = '}
';

open( FILTERS_DOT_C, ">filters/filters.c" );

print FILTERS_DOT_C $header;
print FILTERS_DOT_C $body1;

my $fno = 1;
print FILTERS_DOT_C "/* Import filters */\n";
foreach $filter ( @filters )
{
    printf( FILTERS_DOT_C "#include \"%s\"\n", $filter, $fno);
    $fno++;
}

print FILTERS_DOT_C $body2;

foreach $filter ( @filters )
{
    my $exec_func = "NULL";
    my $init_func = "NULL";
    my $clos_func = "NULL";

    $name = "\U$filter\E";
    $name =~ s/\.H//;

    open( FLTR, "filters/$filter" );
    @lines = <FLTR>;
    close( FLTR );

    foreach $line ( @lines )
    {
        if( $line =~ /\s(\w+_exec)[\(\s].*\;/ ){
            $exec_func = "&$1";
        }elsif( $line =~ /\s(\w+_init)[\(\s].*\;/ ) {
            $init_func = "&$1";
        } elsif( $line =~ /\s(\w+_close)[\(\s].*\;/ ) {
            $clos_func = "&$1";
        }
    }

    printf( FILTERS_DOT_C "    %-35s= %s;\n", "filters.init[$name]", $init_func );
    printf( FILTERS_DOT_C "    %-35s= %s;\n", "filters.exec[$name]", $exec_func );
    printf( FILTERS_DOT_C "    %-35s= %s;\n", "filters.clos[$name]", $clos_func );
    print( FILTERS_DOT_C "\n" );
}

print FILTERS_DOT_C $body3;
