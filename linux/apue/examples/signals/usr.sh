#/bin/bash
i=0;
for((i=0;i<20;i++));
	do kill -10 `pidof a.out`;done
