#!/usr/bin/env bash

#Tets querying CRISPR-CAS9 target sequences
./index/load_index --input=PLAIN --secondary_report=ALL -t 10 -K FMI -v 3 out/pangen.fa test/human_exon_crispr_v1_target_sequences.txt
