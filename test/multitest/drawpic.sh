#!/bin/bash
grep f8 problem2-peer.txt > f8.dat
grep f9 problem2-peer.txt > f9.dat
grep f10 problem2-peer.txt > f10.dat
grep f11 problem2-peer.txt > f11.dat


echo "set terminal png;set output \"window8.png\";plot \"f8.dat\" using 2:3 title 'flow1' with lines" | gnuplot

echo "set terminal png;set output \"window9\";plot \"f9.dat\" using 2:3 title 'flow1' with lines" | gnuplot

echo "set terminal png;set output \"window10\";plot \"f10.dat\" using 2:3 title 'flow1' with lines" | gnuplot

echo "set terminal png;set output \"window11\";plot \"f11.dat\" using 2:3 title 'flow1' with lines" | gnuplot

