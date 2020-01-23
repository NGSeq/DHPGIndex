// Copyright Daniel Valenzuela
/**
 *This is chic alignment mapper for already kernel BWA aligned reads
 *The code is modified version of chic_align and discards BWA alignment phase
 * in KernelManagerBWA.cpp method KernelManagerBWA::LocateOccsFQ
 *
 * See also: HybridLZIndex void HybridLZIndex::FindPrimaryOccsFQ vs ..FQ2
 * Example pipeline:
 * 1. Align reads to kernel sequence with BWA MEM
 * 2. map SAM formatted alignments to original pan-genomic sequences with chic_map
 * 3. Use mapped SAMs for finding heaviest path for adhoc genome.
 * Usage: chic_map <lzindex> <alignmentfile>
 **/
#include <getopt.h>
#include <sdsl/util.hpp>
#include <sdsl/vectors.hpp>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include "./HybridLZIndex.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

using std::ifstream;

void suggest_help();
void suggest_help(char ** argv) {
  cout << "For help, type " << argv[0] << " --help" << endl;
}

void print_help();
void print_help() {
  cout << "Compressed Hybrid Index v0.1 beta" << endl;
  cout << "chic_align aligns reads in fq format" << endl;
  cout << endl;
  cout << "Ussage: chic_align [OPTIONS] <index_basename> <reads1.fq> (reads2.fq)" << endl;
  cout << endl;
  cout << "Options:" << endl;
  cout << "-o --output=OUTPUT_FILENAME Default: Standard output" << endl;
  cout << "-s --secondary_report=[ALL|LZ|NONE] Default=NONE" << endl;
  cout << "-t --threads=(number of threads)" << endl;
  cout << "-p --interleaved-reads " << endl;
  cout << "-K --kernel-options " << endl;
  cout << "-v --verbose=LEVEL " << endl;
  cout << "--help " << endl;
}

typedef struct {
  // Required:
  char * index_basename;
  char * alignment_filename;
  // Options:
  bool interleaved_mates;
  InputType input_type;
  char * output_filename;
  SecondaryReportType secondary_report;
  int n_threads;
  int verbose;
  vector<string> kernel_options;
} Parameters;


int global_correct = 0;
void FailExit();
void Success();

void FailExit() {
  printf(ANSI_COLOR_RED "\tTest Failed\n\n" ANSI_COLOR_RESET);
  exit(1);
}
void Success() {
  printf(ANSI_COLOR_BLUE "\t\tSuccess\n" ANSI_COLOR_RESET);
}

vector<string> LoadPatterns(char * filename, uint max_query_len);

int main(int argc, char *argv[]) {
  Parameters * parameters  = new Parameters();
  // default values:
  parameters->input_type = InputType::FQ;
  parameters->output_filename = NULL;
  parameters->verbose = 1;
  parameters->secondary_report = SecondaryReportType::NONE;
  parameters->n_threads = 1;
  parameters->interleaved_mates = false;

  while (1) {
    static struct option long_options[] = {
      /* These options don’t set a flag.
         We distinguish them by their indices. */
      {"output",    required_argument, 0, 'o'},
      {"secondary_report", required_argument, 0, 's'},
      {"threads",    required_argument, 0, 't'},
      {"verbose",    required_argument, 0, 'v'},
      {"interleaved-reads",    no_argument, 0, 'p'},
      {"kernel-options", required_argument, 0, 'K'},
      {"help",    no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    int c = getopt_long(argc, argv, "o:s:t:v:p:K:h", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

      switch (c) {
      case 0:

            /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        printf("option %s", long_options[option_index].name);
        if (optarg)
          printf(" with arg %s", optarg);
        printf("\n");
        break;

      case 'h':
        print_help();
        delete (parameters);
        exit(0);
        break;

      case 'p':
        parameters->interleaved_mates = true;
        break;

      case 'o':
        parameters->output_filename = optarg;
        break;

      case 's':
        if (strcmp(optarg, "ALL") == 0) {
          parameters->secondary_report = SecondaryReportType::ALL;
        } else if (strcmp(optarg, "LZ") == 0) {
          parameters->secondary_report = SecondaryReportType::LZ;;
        } else if (strcmp(optarg, "NONE") == 0) {
          parameters->secondary_report = SecondaryReportType::NONE;;
        } else {
          print_help();
          suggest_help(argv);
          delete (parameters);
          exit(-1);
        }
        break;
      
      case 'K':
        parameters->kernel_options.push_back(std::string(optarg));
        break;

      case 't':
        parameters->n_threads = atoi(optarg);
        break;

      case 'v':
        parameters->verbose = atoi(optarg);
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

    int rest = argc - optind;
    if (rest != 1 && rest != 2) {
      cout << "Incorrect number of arguments." << endl;
      suggest_help(argv);
      exit(-1);
    }
  parameters->index_basename = argv[optind];
  parameters->alignment_filename= argv[optind + 1];

  //////////////////////////////////////
  // TODO: Sanitize parameters: e,g, -output=file.out
  // fails... it should be --output=file.out
  // or -o file.out
  ///////////////////////////////////////
  long double t1, t2;

  t1 = Utils::wclock();

  HybridLZIndex * my_index = new HybridLZIndex();
  my_index->Load(parameters->index_basename,
                 parameters->n_threads,
                 parameters->verbose);

  //cout << "LZ Index succesfully load" << endl;
  ///////////////////////////////////////////////////
  // FROM HERE ON START ACTING ACCORDING TO PARAMS:
  ///////////////////////////////////////////////////
  if (parameters->output_filename == NULL) {
    my_index->FindFQ2(parameters->alignment_filename,
                     parameters->interleaved_mates,
                     parameters->secondary_report,
                     parameters->kernel_options,
                     cout);
  } else {
    std::ofstream my_out;
    my_out.open(parameters->output_filename);
    ASSERT(my_out.is_open());
    ASSERT(my_out.good());
    // TODO: pass parameters ?
    my_index->FindFQ2(parameters->alignment_filename,
                     parameters->interleaved_mates,
                     parameters->secondary_report,
                     parameters->kernel_options,
                     my_out);
    my_out.close();
  }
  
  t2 = Utils::wclock();
  /*cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
  cout << "Reads aligned in: "<< (t2-t1) << " seconds. " << endl;
  if (t2-t1 > 60) {
    cout << "Reads aligned in: "<< (t2-t1)/60 << " minutes. " << endl;
  }
  if (t2-t1 > 3600) {
    cout << "Reads aligned in: "<< (t2-t1)/3600 << " hours. " << endl;
  }
  cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
*/
  delete(my_index);
  delete(parameters);
  return 0;
}

void ValidatePatterns(vector<string> patterns, size_t max_len) {
  // TODO: this code should be compiled conditionally, perhaps using DNDEBUG
  for (size_t i = 0; i < patterns.size(); i++) {
    if (patterns[i].size() > max_len) {
      cerr << "Patterns are larger than the index limit:" << endl;
      cerr << "Limit: " << max_len << endl;
      cerr << "Offending pattern:" << patterns[i] << endl;

      exit(EXIT_FAILURE);
    }
  }
}

vector<string> LoadPatterns(char * filename, uint max_query_len) {
  ifstream ifile;
  ifile.open(filename);
  if (!ifile.good() || !ifile.is_open()) {
    cerr << "Error loading patterns from '" << filename << "'" << endl;
    exit(EXIT_FAILURE);
  }
  string line;
  vector<string> data;
  while (getline(ifile, line)) {
    data.push_back(line);
  }
  ValidatePatterns(data, max_query_len);
  //cout << data.size() << " patterns succesfully loaded from "<< filename << endl;
  return data;
}
