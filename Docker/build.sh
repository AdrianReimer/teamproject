#!/bin/bash
/etc/init.d/bluetooth stop
/etc/init.d/bluetooth start
dbus-launch
#service bluetooth start
#service dbus start
systemctl enable bluetooth
systemctl restart bluetooth
systemctl status bluetooth
hcitool dev
hciconfig hci0 up

hcitool scan

#echo -e "scan on\nexit" | bluetoothctl

hciconfig hci0 sspmode 0
hciconfig hci0 piscan
sleep 2

./app/bluetooth-connect.exp


coproc bluetoothctl

echo hi

echo -e 'agent on\n' >&${COPROC[1]}
sleep 1
echo -e 'default-agent\n' >&${COPROC[1]}
sleep 1
echo -e 'scan on\n' >&${COPROC[1]}
sleep 10
echo -e 'trust 5C:BA:37:FE:E0:03\n' >&${COPROC[1]}
sleep 1
echo -e 'pair 5C:BA:37:FE:E0:03\n' >&${COPROC[1]}
sleep 1
echo -e 'connect 5C:BA:37:FE:E0:03\n' >&${COPROC[1]}
sleep 1

echo -e 'scan off\n' >&${COPROC[1]}
echo -e 'exit\n' >&${COPROC[1]}

output=$(cat <&${COPROC[0]})
echo $output


python main.py





#bluetoothctl
#scan on
#sleep 10s
#remove 5C:BA:37:FE:E0:03
#trust 5C:BA:37:FE:E0:03
#pair 5C:BA:37:FE:E0:03
#connect 5C:BA:37:FE:E0:03
