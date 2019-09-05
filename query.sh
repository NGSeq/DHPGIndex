#!/usr/bin/env bash

#./index/build_index --threads=${THREADS} --kernel=${KERNEL} --verbose=2 --lz-input-file=${ORIG}/merged.lz ${INDEX_OUT}  ${MATCH_LEN} 2>&1 | tee ${OUTPUT_FOLDER}/index.log
#./index/load_index --input=PLAIN --secondary_report=ALL -t 10 -K FMI -v 3 out/pangen.fa test/human_exon_crispr_v1_target_sequences.txt --search_test
#Tets querying CRISPR-CAS9 target sequences
./index/load_index --input=PLAIN --secondary_report=ALL -t 10 -K FMI -v 3 out/pangen.fa test/human_exon_crispr_v1_target_sequences.txt
