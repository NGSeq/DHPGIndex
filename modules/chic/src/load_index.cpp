/*
	 Copyright 2017, Daniel Valenzuela <dvalenzu@cs.helsinki.fi>

	 This file is part of CHIC aligner.

	 CHIC aligner is free software: you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation, either version 3 of the License, or
	 (at your option) any later version.

	 CHIC aligner is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.

	 You should have received a copy of the GNU General Public License
	 along with CHIC aligner.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <getopt.h>
#include <sdsl/util.hpp>
#include <sdsl/vectors.hpp>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
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
  cerr << "For help, type " << argv[0] << " --help" << endl;
}

void print_help();
void print_help() {
  cerr << "Ussage: load_index [OPTIONS] <index_basename> <patterns_filename>" << endl;  // NOLINT
  cerr << "Loads an index previously built with build_index" << endl;
  cerr << endl;
  cerr << "Options:" << endl;
  cerr << "--validation_test" << endl;
  cerr << "-o --output=OUTPUT_PREFIX" << endl;
  cerr << "-i --input=[FQ|PLAIN]" << endl;
  cerr << "-s --secondary_report=[ALL|LZ|NONE] Default=NONE" << endl;
  cerr << "-t --threads=(number of threads)" << endl;
  cerr << "-p --interleaved-reads " << endl;
  cerr << "-K --kernel-options " << endl;
  cerr << "-v --verbose=LEVEL " << endl;
  cerr << "--help " << endl;
}

typedef struct {
  // Required:
  char * index_basename;
  char * patterns_filename;
  // Options:
  char * mates_filename;
  bool interleaved_mates;
  InputType input_type;
  char * output_filename;
  SecondaryReportType secondary_report;
  int n_threads;
  int verbose;
  vector<string> kernel_options;
  int validation_test;
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

void timing_test(Parameters * parameters,
                 HybridLZIndex* my_index);

void validation_test(Parameters * parameters,
                     HybridLZIndex* my_index);

void test_pattern(string query,
                  HybridLZIndex * index,
                  csa_wt<wt_huff<rrr_vector<127> >, 512, 1024> * FMI);

vector<string> LoadPatterns(char * filename, uint max_query_len);

int main(int argc, char *argv[]) {
  Parameters * parameters  = new Parameters();
  // default values:
  parameters->validation_test = false;
  parameters->input_type = InputType::PLAIN;
  parameters->output_filename = NULL;
  parameters->verbose = 1;
  parameters->secondary_report = SecondaryReportType::NONE;
  parameters->n_threads = 1;
  parameters->mates_filename = NULL;
  parameters->interleaved_mates = false;

  while (1) {
    static struct option long_options[] = {
      /* These options set a flag. */
      {"validation_test", no_argument,  &(parameters->validation_test), 1},  // NO NEED TO DO ANYTHING ELSE :)
      /* These options don’t set a flag.
         We distinguish them by their indices. */
      {"output",    required_argument, 0, 'o'},
      {"input",    required_argument, 0, 'i'},
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

    int c = getopt_long(argc, argv, "o:i:s:t:v:p:K:h", long_options, &option_index);

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

      case 'o':
        parameters->output_filename = optarg;
        break;
      case 'i':
        if (strcmp(optarg, "PLAIN") == 0) {
          parameters->input_type = InputType::PLAIN;
        } else if (strcmp(optarg, "FQ") == 0) {
          parameters->input_type = InputType::FQ;;
        } else {
          print_help();
          delete (parameters);
          exit(0);
        }
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
          delete (parameters);
          exit(0);
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
  if (rest != 2 && rest != 3) {
    cerr << "Incorrect number of arguments." << endl;
    suggest_help(argv);
    exit(-1);
  }
  parameters->index_basename = argv[optind];
  parameters->patterns_filename= argv[optind + 1];
  if (rest == 3) {
    parameters->mates_filename = argv[optind + 2];
  }
  //////////////////////////////////////
  // TODO: Sanitize parameters: e,g, -output=file.out
  // fails... it should be --output=file.out
  // or -o file.out
  ///////////////////////////////////////
  HybridLZIndex * my_index = new HybridLZIndex();
  my_index->Load(parameters->index_basename,
                 parameters->n_threads,
                 parameters->verbose);

  cerr << "LZ Index succesfully load" << endl;
  ///////////////////////////////////////////////////
  // FROM HERE ON START ACTING ACCORDING TO PARAMS:
  ///////////////////////////////////////////////////
  if (parameters->validation_test) {
    validation_test(parameters, my_index);
  } else if (parameters->input_type == InputType::FQ) {
    if (parameters->output_filename == NULL) {
      my_index->FindFQ(parameters->patterns_filename,
                       parameters->mates_filename,
                       parameters->interleaved_mates,
                       parameters->secondary_report,
                       parameters->kernel_options,
                       cout);
    } else {
      std::ofstream my_out;
      my_out.open(parameters->output_filename);
      ASSERT(my_out.is_open());
      ASSERT(my_out.good());
      my_index->FindFQ(parameters->patterns_filename,
                       parameters->mates_filename,
                       parameters->interleaved_mates,
                       parameters->secondary_report,
                       parameters->kernel_options,
                       my_out);
      my_out.close();
    }
  } else {
    timing_test(parameters, my_index);
  }

  delete(my_index);
  delete(parameters);
  return 0;
}

void timing_test(Parameters * parameters,
                 HybridLZIndex* my_index) {
  vector<string> patterns;
  patterns = LoadPatterns(parameters->patterns_filename, my_index->GetMaxQueryLen());

  long double t1, t2;
  t1 = Utils::wclock();
  size_t count = 0;
  for (size_t i = 0; i < patterns.size(); i++) {
    vector<uint64_t> locations_lzi;
    my_index->Find(&locations_lzi, patterns[i]);
    count += locations_lzi.size();
  }
  t2 = Utils::wclock();
  cerr << count << " occurrences reported." << endl;
  cerr << "All queries in: "<< (t2-t1) << " seconds. " << endl;
  cerr << "Avg query in: " << (t2-t1)/patterns.size() << "Seconds" << endl;
  Success();
}

void validation_test(Parameters * parameters,
                     HybridLZIndex* my_index) {
  cerr << "Testing correctnes..." << endl;
  csa_wt<wt_huff<rrr_vector<127> >, 512, 1024> FMI;
  if (Utils::GetLength(parameters->index_basename) == 0) {
    cerr << "Basename file has length zero. Cannot run validation_test." << endl;
    cerr << "validation_test might be modified so it gets a file as a parameter?" << endl;
    FailExit();
  }
  // 1 => file is interpreted as a byte sequence,
  construct(FMI, parameters->index_basename, 1);
  cerr << "FMI Succesfully built" << endl;
  size_t fm_size_in_bytes = sdsl::size_in_bytes(FMI);
  size_t text_len = FMI.size() - 1;
  cerr << (float)fm_size_in_bytes/(float)text_len << "|T|" << endl;

  vector<string> patterns;
  patterns = LoadPatterns(parameters->patterns_filename, my_index->GetMaxQueryLen());

  for (size_t i = 0; i < patterns.size(); i++) {
    test_pattern(patterns[i], my_index, &FMI);
  }
  cerr << global_correct << " occurrences reported consistently" << endl;
  cerr << global_correct << " Baseline method: SDSL's FM-Index" << endl;
  Success();
}

void test_pattern(string query,
                  HybridLZIndex * index,
                  csa_wt<wt_huff<rrr_vector<127> >, 512, 1024> * FMI) {
  uint m = query.size();

  auto locations_fmi = locate(*FMI, query.begin(), query.begin()+m);
  vector<uint64_t> locations_lzi;
  index->Find(&locations_lzi, query);

  if (locations_fmi.size() != locations_lzi.size()) {
    cerr << "Diff num of occs:" << endl;
    cerr << "FMI   : " << locations_fmi.size() << endl;
    cerr << "CHICO : " << locations_lzi.size() << endl;

    std::sort(locations_fmi.begin(), locations_fmi.end());
    std::sort(locations_lzi.begin(), locations_lzi.end());
    cerr << "Pattern: " << query << endl;
    cerr << "FMI OCCS:" << endl;
    for (size_t i = 0; i < locations_fmi.size(); i++) {
      cerr << locations_fmi[i] << endl;
    }
    cerr << "LZI OCCS:" << endl;
    for (size_t i = 0; i < locations_lzi.size(); i++) {
      cerr << locations_lzi[i] << endl;
    }

    FailExit();
  }
  std::sort(locations_fmi.begin(), locations_fmi.end());
  std::sort(locations_lzi.begin(), locations_lzi.end());
  for (size_t i = 0; i < locations_fmi.size(); i++) {
    if (locations_fmi[i] != locations_lzi[i]) {
      cerr << "After sorting occs differ at least at position: " << i << endl;
      cerr << locations_fmi[i] << " != " <<locations_lzi[i] << endl;
      FailExit();
    } else {
      global_correct++;
    }
  }
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
  cerr << data.size() << " patterns succesfully loaded from "<< filename << endl;
  return data;
}

