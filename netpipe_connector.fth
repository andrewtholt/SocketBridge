showstack

$100 constant /line
/line buffer: line-buffer

-1 value fd-in
-1 value fd-out

: write-pipe
	s" /tmp/pipe_to_net"
;

: read-pipe
	s" /tmp/net_to_pipe"
;

: init-in
	read-pipe r/o open-file throw to fd-in
;

: init-out
	line-buffer /line erase
	write-pipe w/o open-file throw to fd-out
;

: tst-write
	s" PING\n" sprintf fd-out write-file
	abort" Write failed" 
	fd-out flush-file drop
;

: tst-read
	line-buffer /line erase
	fd-in line-buffer /line fd-in read-line nip abort" Read" .s cr
	2drop
	line-buffer $20 dump
;


: init
	init-out
	init-in
	line-buffer /line erase
;

: finish
	fd-in close-file abort" Close In."
	fd-out close-file abort" Close Out."
;

: tst
	init
	tst-write
	.s cr
	tst-read
;

cr
.( netpipe connector test in cforth ) cr cr
.( To run enure that netpipe_forwarder is running, and is talking to Server2-standalone ) cr
.( Then enter forth netpipe_connector.fth - ) cr cr
.( tst runs the code ) cr cr



