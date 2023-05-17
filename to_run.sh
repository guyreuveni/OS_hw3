make
sudo insmod message_slot.ko
sudo mknod /dev/slot0 c 235 0
sudo chmod o+rw /dev/slot0 
gcc message_sender.c -o sender
gcc message_reader.c -o reader
echo
echo ======= Testing Simple write and read ========
./sender /dev/slot0 4 hello

echo Should print hello:
./reader /dev/slot0 4
echo 
./sender /dev/slot0 5 world
echo Should print world:
./reader /dev/slot0 5
echo
echo Should print helloworld:
./reader /dev/slot0 4
./reader /dev/slot0 5
echo
echo
sleep 1
echo ======= Testing 5 processes writing to different channels and then reading 5 channels ========
sudo mknod /dev/slot1 c 235 1
sudo chmod o+rw /dev/slot1 
./sender /dev/slot1 1 first
./sender /dev/slot1 2 second
./sender /dev/slot1 3 third 
./sender /dev/slot1 4 forth 
./sender /dev/slot1 5 fifth


echo Should print first second third forth fifth:
./reader /dev/slot1 1
echo
./reader /dev/slot1 2
echo
./reader /dev/slot1 3
echo
./reader /dev/slot1 4
echo
./reader /dev/slot1 5
echo
sleep 1

echo
echo writing over all five channels and adding a new one:
./sender /dev/slot1 1 newval-
./sender /dev/slot1 2 newval-
./sender /dev/slot1 3 newval-
./sender /dev/slot1 4 newval-
./sender /dev/slot1 5 newval-
./sender /dev/slot1 6 newval-
echo Should print:
echo newval-newval-newval-newval-newval-newval-
echo result:
./reader /dev/slot1 1
./reader /dev/slot1 2
./reader /dev/slot1 3
./reader /dev/slot1 4
./reader /dev/slot1 5
./reader /dev/slot1 6
echo
echo
sleep 1
echo ======= Testing 128 bytes given and handeled ========
echo Should print 128*'a':
./sender /dev/slot1 1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
./reader /dev/slot1 1
echo
echo
sleep 1

echo ======= Testing a bit more complex scenario with 3 open minors, 3 different channels and messages are short->long->short ========
sudo mknod /dev/slot2 c 235 2
sudo chmod o+rw /dev/slot2
./sender /dev/slot0 1 h
./sender /dev/slot0 2 h
./sender /dev/slot0 3 h 
./sender /dev/slot1 1 g
./sender /dev/slot1 2 g
./sender /dev/slot1 3 g
./sender /dev/slot2 1 w
./sender /dev/slot2 2 w
./sender /dev/slot2 3 w

echo Should be hhh ggg www:
./reader /dev/slot0 1 
./reader /dev/slot0 2 
./reader /dev/slot0 3 
echo
./reader /dev/slot1 1 
./reader /dev/slot1 2 
./reader /dev/slot1 3 
echo
./reader /dev/slot2 1 
./reader /dev/slot2 2 
./reader /dev/slot2 3 
echo
sleep 1

./sender /dev/slot0 1 hello_on_minor_0_on_channel_1
./sender /dev/slot0 2 hello_on_minor_0_on_channel_2
./sender /dev/slot0 3 hello_on_minor_0_on_channel_3
./sender /dev/slot1 1 goodbye_on_minor_1_on_channel_1
./sender /dev/slot1 2 goodbye_on_minor_1_on_channel_2
./sender /dev/slot1 3 goodbye_on_minor_1_on_channel_3
./sender /dev/slot2 1 world_on_minor_2_on_channel_1
./sender /dev/slot2 2 world_on_minor_2_on_channel_2
./sender /dev/slot2 3 world_on_minor_2_on_channel_3
sleep 1

echo
echo Should be hello_on_minor_0_on_channel_i for i in 1,2,3:
./reader /dev/slot0 1 
echo
./reader /dev/slot0 2 
echo
./reader /dev/slot0 3 
echo
sleep 1

echo
echo Should be goodbye_on_minor_1_on_channel_i for i in 1,2,3:
./reader /dev/slot1 1 
echo
./reader /dev/slot1 2 
echo
./reader /dev/slot1 3 
echo
sleep 1

echo
echo Should be world_on_minor_2_on_channel_i for i in 1,2,3:
./reader /dev/slot2 1 
echo
./reader /dev/slot2 2 
echo
./reader /dev/slot2 3 
echo
sleep 1

./sender /dev/slot0 1 h1
./sender /dev/slot0 2 h2
./sender /dev/slot0 3 h3
./sender /dev/slot1 1 g1
./sender /dev/slot1 2 g2
./sender /dev/slot1 3 g3
./sender /dev/slot2 1 w1
./sender /dev/slot2 2 w2
./sender /dev/slot2 3 w3

sleep 1
echo
echo Should be hi for i in 1,2,3:
./reader /dev/slot0 1 
echo
./reader /dev/slot0 2 
echo
./reader /dev/slot0 3 
echo
echo
echo Should be gi for i in 1,2,3:
./reader /dev/slot1 1 
echo
./reader /dev/slot1 2 
echo
./reader /dev/slot1 3 
echo
echo
echo Should be wi for i in 1,2,3:
./reader /dev/slot2 1 
echo
./reader /dev/slot2 2 
echo
./reader /dev/slot2 3 
echo

echo
echo Removing all used files
sudo rm /dev/slot0
sudo rm /dev/slot1
sudo rm /dev/slot2
sudo rmmod message_slot
echo ===============================================================
echo Done! Check all results are correct 
