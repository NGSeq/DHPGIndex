// Copyright Daniel Valenzuela
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <iostream>
#include <string>
#include "./HybridLZIndex.h"

void suggest_help();
void suggest_help(char ** argv) {
  cerr << "For help, type " << argv[0] << " --help" << endl;
}

void print_help();
void print_help() {
  cerr << "Compressed Hybrid Index v0.1 beta" << endl;
  cerr << "chic_index builds the indexing data structure for INPUT_FILE" << endl;
  cerr << "so that it can allign reads up to MAX_QUERY_LEN" << endl;
  cerr << endl;
  cerr << "Ussage: chic_index [OPTIONS] INPUT_FILE MAX_QUERY_LEN" << endl;
  cerr << endl;
  cerr << "Options:" << endl;
  cerr << "--kernel=[FMI,BWA,BOWTIE2] default is FMI" << endl;
  cerr << "--lz-parsing-method=[IM,EM,RLZ,RELZ] default is IM" << endl;
  cerr << "--lz-input-plain-file=PARSE.LZ In case you have the lz parsing of the input (as pairs of 64 bits integers)" << endl;
  cerr << "--lz-input-vbyte-file=PARSE.LZ In case you have the lz parsing of the input (vbyte encoded)" << endl;
  cerr << "--max-edit-distance (default = 0)" << endl;
  cerr << "-o --output=INDEX_BASENAME Default: INPUT_FILE" << endl;
  cerr << "-v --verbose=LEVEL " << endl;
  cerr << "-m --mem=(MAX MEM IN MB)" << endl;
  cerr << "-t --threads=(number of threads)" << endl;
  cerr << "-r --rlz-ref-size=(Prefix size for RLZ method)" << endl;
  cerr << "--help " << endl;
}


void construct_in_memory(BuildParameters * parameters);
void construct_external_memory(BuildParameters * parameters);

int main(int argc, char **argv) {
  BuildParameters * parameters  = new BuildParameters();
  // default values:
  parameters->input_lz_filename = NULL;
  parameters->max_edit_distance = 0;  // TODO: Perhaps in bio setting we want a default > 0
  parameters->mem_limit_MB = 30;
  parameters->n_threads = 1;
  parameters->verbose = 1;
  parameters->rlz_ref_len_MB = 0;
  parameters->kernel_type = KernelType::BWA;
  // TODO: std for options is a hyphen and no an underscore.
  while (1) {
    static struct option long_options[] = {
      /* These options don’t set a flag.
         We distinguish them by their indices. */
      {"kernel",    required_argument, 0, 'K'},
      {"indexing",    required_argument, 0, 'I'},
      {"lz-parsing-method",    required_argument, 0, 'M'},
      {"lz-input-plain-file",    required_argument, 0, 'F'},
      {"lz-input-vbyte-file",    required_argument, 0, 'G'},
      {"max-edit-distance",    required_argument, 0, 'k'},
      {"output",    required_argument, 0, 'o'},
      {"verbose",    required_argument, 0, 'v'},
      {"mem",    required_argument, 0, 'm'},
      {"rlz-ref-size",    required_argument, 0, 'r'},
      {"threads",    required_argument, 0, 't'},
      {"help",    no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    int c = getopt_long(argc, argv, "K:I:M:F:G:k:o:v:m:r:t:h", long_options, &option_index);

    // TODO: Sanitize args, I'm doing a blind atoi.
    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        if (optarg)
          printf(" with arg %s", optarg);
        printf("\n");
        break;

      case 'K':
        if (strcmp(optarg, "BWA") == 0) {
          parameters->kernel_type = KernelType::BWA;;
        } else if (strcmp(optarg, "BOWTIE2") == 0) {
          parameters->kernel_type = KernelType::BOWTIE2;
        } else if (strcmp(optarg, "BLAST") == 0) {
          parameters->kernel_type = KernelType::BLAST;
        } else if (strcmp(optarg, "FMI") == 0) {
          cerr << "CHIC cannot use FMI index" << endl;
          print_help();
          delete (parameters);
          exit(0);
        } else {
          print_help();
          delete (parameters);
          exit(0);
        }
        break;

      case 'I':
            parameters->indexingonly = 1;
            break;

      case 'M':
        ASSERT(parameters->input_lz_filename == NULL);
        if (strcmp(optarg, "IM") == 0) {
          parameters->lz_method = LZMethod::IN_MEMORY;
        } else if (strcmp(optarg, "EM") == 0) {
          parameters->lz_method = LZMethod::EXTERNAL_MEMORY;
        } else if (strcmp(optarg, "RLZ") == 0) {
          parameters->lz_method = LZMethod::RLZ;
        } else if (strcmp(optarg, "RELZ") == 0) {
          parameters->lz_method = LZMethod::RELZ;
        } else {
          print_help();
          delete (parameters);
          exit(0);
        }
        break;

      case 'F':
        parameters->input_lz_filename = optarg;
        parameters->lz_method = LZMethod::INPUT_PLAIN;
        break;

      case 'G':
        parameters->input_lz_filename = optarg;
        parameters->lz_method = LZMethod::INPUT_VBYTE;
        break;

      case 'h':
        print_help();
        delete (parameters);
        exit(0);
        break;

      case 'o':
        parameters->output_filename = optarg;
        break;

      case 'v':
        parameters->verbose = atoi(optarg);
        break;

      case 'm':
        parameters->mem_limit_MB = atoi(optarg);
        break;

      case 't':
        parameters->n_threads= atoi(optarg);
        break;

      case 'r':
        parameters->rlz_ref_len_MB = atoi(optarg);
        break;

      case 'k':
        parameters->max_edit_distance= atoi(optarg);
        break;

      case '?':
        /* getopt_long already printed an error message. */
        suggest_help(argv);
        exit(-1);
        break;

      default:
        suggest_help(argv);
        exit(-1);
    }
  }

  if ((argc - optind) != 2) {
    cerr << "Incorrect number of arguments." << endl;
    suggest_help(argv);
    exit(-1);
  }
  parameters->input_filename = argv[optind];
  parameters->max_query_len= atoi(argv[optind + 1]);

  if (parameters->output_filename == NULL) {
    parameters->output_filename = parameters->input_filename;
  }

  ///////////////////////////////////////////////////////////////

  cerr << "Input filename: " << parameters->input_filename << endl;
  cerr << "maximum pattern length: " << parameters->max_query_len << endl;

  ///////////////////////////////////////////////////////////////
  long double t1, t2;

  t1 = Utils::wclock();
  HybridLZIndex * index = new HybridLZIndex(parameters);
  index->Save();
  t2 = Utils::wclock();

  cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
  cerr << "Index succesfully built in: "<< (t2-t1) << " seconds. " << endl;
  if (t2-t1 > 60) {
    cerr << "Index succesfully built in: "<< (t2-t1)/60 << " minutes. " << endl;
  }
  if (t2-t1 > 3600) {
    cerr << "Index succesfully built in: "<< (t2-t1)/3600 << " hours. " << endl;
  }
  cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
  delete(index);
  delete(parameters);
  exit(0);
}
