#!/bin/bash
#bash script for strong scaling scenario
submitcommand="qsub"
executable="MarDyn"
working_directory="."
processes_per_node=2
hyperthreading=1
time_limit=00:25:00
#should be changed:
number_of_nodes=1
#output_file_name="${i}nodes.out"
#error_file_name="${i}nodes.err"
#input_file_name="${i}nodes.xml"

input_file_name1node="ljfluid1node.xml"
onenodesize=`cat $input_file_name1node | grep "<lx>" | sed "s/>/</g" | cut -d '<' -f 3`

for (( i=1; i<=4096;i=i*2 ))
do
	#number_of_nodes=$i
	input_file_name="${i}nodes.xml"
	output_file_name="${i}nodes.out"
	error_file_name="${i}nodes.err"
	size=`echo "" | awk "END {print $onenodesize / $i ^ (1/3) }"`
	#change size of new input:
	sed -e "s/$onenodesize/$size/g" $input_file_name1node > $input_file_name
	
	sed -e "s/EXARG/$executable/g; s/WDARG/$working_directory/g; s/IFARG/$input_file_name/g; s/EFARG/$error_file_name/g; s/OFARG/$output_file_name/g; s/NNARG/$number_of_nodes/g; s/PPNARG/$processes_per_node/g; s/HYPARG/$hyperthreading/g; s/TLARG/$time_limit/g" genericJob.sh > job$i.sh
	mv $input_file_name runfolder
	mv job$i.sh runfolder
	#$submitcommand job$number_of_nodes.sh
done

