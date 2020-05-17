#!/bin/bash

if (! test -f mac.txt); then
    echo "please create a file mac.txt with your device's BLE address"
    exit 1
fi

MAC=`cat mac.txt`
MAC0=`echo $MAC |cut -d ":" -f 1`
MAC1=`echo $MAC |cut -d ":" -f 2`
MAC2=`echo $MAC |cut -d ":" -f 3`
MAC3=`echo $MAC |cut -d ":" -f 4`
MAC4=`echo $MAC |cut -d ":" -f 5`
MAC5=`echo $MAC |cut -d ":" -f 6`

IPADDR=FE80::$MAC0$MAC1:${MAC2}FF:FE$MAC3:$MAC4$MAC5
echo $IPADDR

sudo modprobe bluetooth_6lowpan || true
echo "0" | sudo tee /sys/kernel/debug/bluetooth/6lowpan_enable
sleep .5
echo "1" | sudo tee /sys/kernel/debug/bluetooth/6lowpan_enable
echo "1" | sudo tee /proc/sys/net/ipv6/conf/all/forwarding
echo "connect $MAC 2" | sudo tee /sys/kernel/debug/bluetooth/6lowpan_control
while ( ! sudo ifconfig bt0 ); do
    sleep 1
done
ip route add $IPADDR/128 dev bt0
/etc/init.d/ntp restart
