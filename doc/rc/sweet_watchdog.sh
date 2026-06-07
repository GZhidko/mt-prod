#!/bin/bash
pid_numbers=16
reacct_counter=$(ps -aef | grep reacct | egrep -v 'grep|watcher|tail ' | wc -l)

if [ "$reacct_counter" -ne "$pid_numbers" ]
then
    mailx -s "Process sweetspot watchdog triger init's shuting down procedure $(hostname) at $(date). Restartng with /etc/rc.d/init.d/reacctd start" linux-adm@rol.ru << EOF
+
EOF
/etc/rc.d/init.d/reacctd stop
sleep 2
/etc/rc.d/init.d/reacctd start
fi
