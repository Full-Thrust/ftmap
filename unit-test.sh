#! /bin/bash
./ftmap -a -l -g -r ftmap.ini -i ex_img -f example1.gif < example.ft
./ftmap -a -b -l -g -r ftmap.ini -i ex_img -f example2.gif < example.ft
./ftmap -a -g -r ftmap.ini -i ex_img -f example3.gif < example.ft
./ftmap -a -i ex_img -f example4.gif < example.ft
./ftmap -i ex_img -f example5.gif < example.ft
./ftmap -a -l -g -r ftmap.ini -i ex_img -f example6.gif < example2.ft
./ftmap -a -r omit.ini -i ex_img -f example7.gif < example2.ft
# tests debug mode - generates test sprites
# ./ftmap - -r ftmap.ini -i ex_img -f example8.gif < example2.ft