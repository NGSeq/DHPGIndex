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

#include "./HybridLZIndex.h"
#include <omp.h>
#include <sdsl/util.hpp>
#include <sdsl/vectors.hpp>
#include <string>
#include <utility>
#include <algorithm>
#include <vector>

#include "./utils.h"
#include "./SamReader.h"
#include "./LempelZivParser.h"
#include "../ext/LZ/LZscan/algorithm/lzscan.h"

// INDEX CONSTRUCTION:
void HybridLZIndex::ValidateParams(BuildParameters * parameters) {
  if (Utils::IsBioKernel(parameters->kernel_type)) {
    if (!(parameters->lz_method == LZMethod::IN_MEMORY || 
          parameters->lz_method == LZMethod::RLZ || 
          parameters->lz_method == LZMethod::RELZ ||
          parameters->lz_method == LZMethod::INPUT_VBYTE ||
          parameters->lz_method == LZMethod::INPUT_PLAIN)) {
      cerr << "Currently, only IM, RLZ, and INPUT  are supported for fasta files" << endl;
      cerr << "Abortning" << endl;
      exit(EXIT_FAILURE);
    }
  }
}

HybridLZIndex::HybridLZIndex() {
  this->index_size_in_bytes = 0;
}

HybridLZIndex::HybridLZIndex(BuildParameters * parameters) {
  this->verbose = parameters->verbose;
  //ValidateParams(parameters);
  this->lz_method = parameters->lz_method;
  /*if (lz_method == LZMethod::IN_MEMORY) {
    // Only case when we can assume everything will fit in memory...
    uchar * _seq;
    size_t _len;
    this->book_keeper = new BookKeeper(parameters->input_filename,
                                       parameters->kernel_type,
                                       &_seq,
                                       &_len,
                                       verbose);
    this->tmp_seq = _seq;
    this->text_len = _len;
    this->text_filename = parameters->input_filename;
  } else {*/
    this->book_keeper = new BookKeeper(parameters->input_filename,
                                       parameters->kernel_type,
                                       verbose);
    this->text_filename = parameters->input_filename;
    this->text_len = book_keeper->GetTotalLength();


    this->tmp_seq = NULL;
    // ASSERT(parameters->max_memory_MB > 0);
  //}
  this->input_lz_filename = parameters->input_lz_filename;
    cout << "parameters->input_lz_filename   : " << parameters->input_lz_filename << endl;
    cout << "Indexing only   : " << parameters->indexingonly << endl;
  index_size_in_bytes = 0;

  this->kernel_type = parameters->kernel_type;
  this->max_memory_MB = parameters->mem_limit_MB;  // TODO: uniform name
  this->sparse_sample_ratio = SPARSE_DENS;
  this->max_query_len = parameters->max_query_len;
  this->max_insertions = parameters->max_edit_distance;  // TODO: uniform name
  this->context_len = max_query_len + max_insertions;
  this->sigma = 256;
  this->rlz_ref_len_MB = parameters->rlz_ref_len_MB;
  this->n_threads = parameters->n_threads;

  this->index_prefix = parameters->output_filename;
  SetFileNames();

  if(parameters->kernelizeonly==1){
      if(parameters->indexingonly==1)
          Build();
      else
          InitKernelizeonly();

  }else{
      if(parameters->indexingonly==1)
          Indexing();
      else{
          Build();
      }

  }
}

void HybridLZIndex::Build() {


    if (kernel_type == KernelType::BWA || kernel_type == KernelType::BOWTIE2 || kernel_type == KernelType::BLAST ) {
    special_separator = (uchar)'N';
  } else {
    if (lz_method == LZMethod::IN_MEMORY) {
      ChooseSpecialSeparator(tmp_seq);
    } else {
      ChooseSpecialSeparator(text_filename);
    }
  }

  vector<pair<uint64_t, uint64_t> > lz_phrases;
  LempelZivParser::GetLZPhrases(&lz_phrases, this);
  n_phrases = lz_phrases.size();
  tsrr =  new RangeReporting(&lz_phrases, context_len, verbose);
  tsrr->SetFileNames(index_prefix);
  if (verbose >= 2)
    cerr << "Previous merge: " << n_phrases << " phrases." << endl;
  n_phrases = lz_phrases.size();
  if (verbose >= 2)
    cerr << "After merge: " << n_phrases << " phrases." << endl;
  index_size_in_bytes += tsrr->GetSizeBytes();
  Kernelize();
  ComputeSize();
  if (verbose >= 2) {
    DetailedSpaceUssage();
  }

}

void HybridLZIndex::Indexing() {
    if (kernel_type == KernelType::BWA || kernel_type == KernelType::BOWTIE2 || kernel_type == KernelType::BLAST) {
        special_separator = (uchar)'N';
    } else {
        if (lz_method == LZMethod::IN_MEMORY) {
            ChooseSpecialSeparator(tmp_seq);
        } else {
            ChooseSpecialSeparator(text_filename);
        }
    }
    cout << "Indexing only. Reading LZ parse.." << endl;
    vector<pair<uint64_t, uint64_t> > lz_phrases;
    //LempelZivParser::GetLZPhrases(&lz_phrases, this);
    FILE * lz_infile  = Utils::OpenReadOrDie(this->input_lz_filename);
    LempelZivParser::LoadLZParsePlain(lz_infile, &lz_phrases);
    n_phrases = lz_phrases.size();
    cout << "Indexing only. LZ Range reporting.." << endl;
    tsrr =  new RangeReporting(&lz_phrases, context_len, verbose);
    tsrr->SetFileNames(index_prefix);
    if (verbose >= 2)
        cout << "Previous merge: " << n_phrases << " phrases." << endl;
    n_phrases = lz_phrases.size();
    if (verbose >= 2)
        cout << "After merge: " << n_phrases << " phrases." << endl;
    index_size_in_bytes += tsrr->GetSizeBytes();
    IndexKernel();
    ComputeSize();
    if (verbose >= 2) {
        DetailedSpaceUssage();
    }
}

void HybridLZIndex::InitKernelizeonly(){
    if (kernel_type == KernelType::BWA || kernel_type == KernelType::BOWTIE2|| kernel_type == KernelType::BLAST) {
        special_separator = (uchar)'N';
    } else {
        if (lz_method == LZMethod::IN_MEMORY) {
            ChooseSpecialSeparator(tmp_seq);
        } else {
            ChooseSpecialSeparator(text_filename);
        }
    }
    vector<pair<uint64_t, uint64_t> > lz_phrases;
    LempelZivParser::GetLZPhrases(&lz_phrases, this);
    n_phrases = lz_phrases.size();
    tsrr =  new RangeReporting(&lz_phrases, context_len, verbose);
    tsrr->SetFileNames(index_prefix);
    if (verbose >= 2)
        cout << "Previous merge: " << n_phrases << " phrases." << endl;
    n_phrases = lz_phrases.size();
    if (verbose >= 2)
        cout << "After merge: " << n_phrases << " phrases." << endl;
    index_size_in_bytes += tsrr->GetSizeBytes();
    Kernelizeonly();
    ComputeSize();
    if (verbose >= 2) {
        DetailedSpaceUssage();
    }
}

  void HybridLZIndex::Kernelizeonly() {
        ComputeKernelTextLen();
        if (verbose) {
            cout << "+++++++++++++++++Kernelizeonly++++++++++++++++++++++++++++" << endl;
            cout << "Original length n    : " << text_len << endl;
            cout << "Kernel text length n : " << kernel_text_len << endl;
            cout << "+++++++++++++++++++++++++++++++++++++++++++++" << endl;
        }

        uint64_t *tmp_limits_kernel;
        uchar *kernel_text;

        long t1 = Utils::wclock();
        MyBuffer * my_buffer;

        if (lz_method == LZMethod::IN_MEMORY) {
          my_buffer = new MyBufferMemSeq(tmp_seq, text_len);
        } else {
          if (Utils::IsBioKernel(kernel_type)) {
              my_buffer = new MyBufferFastaFile(text_filename);
          } else {
              my_buffer = new MyBufferPlainFile(text_filename);
          }
        }
        MakeKernelString(my_buffer, &kernel_text, &tmp_limits_kernel);
        long t2 = Utils::wclock();
        cout << "MakeKernelString from HDFS: "<< (t2-t1) << " seconds. " << endl;

        delete(my_buffer);

        long k1 = Utils::wclock();
        if (kernel_type == KernelType::FMI) {
            this->WriteKernelTextFile(kernel_text, kernel_text_len);
        } else if (kernel_type == KernelType::BWA) {
            this->WriteKernelTextFile(kernel_text, kernel_text_len);
        } else if (kernel_type == KernelType::BOWTIE2) {
            this->WriteKernelTextFile(kernel_text, kernel_text_len);
        } else if (kernel_type == KernelType::BLAST) {
            this->WriteKernelTextFile(kernel_text, kernel_text_len);
        } else {
            cerr << "Unknown kernel type given" << endl;
            exit(EXIT_FAILURE);
        }
        long k2 = Utils::wclock();
        cout << "Indexing: "<< (t2-t1) << " seconds. " << endl;

        //delete [] kernel_text;

        EncodeKernelLimitsAndSuccessor(tmp_limits_kernel);
        store_to_file(limits_kernel, limits_kernel_filename);
        store_to_file(sparse_sample_limits_kernel, sparse_sample_limits_kernel_filename);

        delete [] tmp_limits_kernel;

        //index_size_in_bytes += kernel_manager->GetSizeBytes();
        // ASSERT(kernel_text_len == kernel_manager->GetLength());
    }

    void HybridLZIndex::WriteKernelTextFile(uchar * _kernel_text, size_t _kernel_text_len) {
        FILE * fp = Utils::OpenWriteOrDie(kernel_manager_prefix);
        if (_kernel_text_len != fwrite(_kernel_text, 1, _kernel_text_len, fp)) {
            cout << "Error writing the kernel to a file" << endl;
            exit(1);
        }
        fclose(fp);
    }

void HybridLZIndex::IndexKernel() {
    ComputeKernelTextLen();
    if (verbose) {
        cout << "+++++++++++++++++++INDEXING ONLY++++++++++++++++++++++++++" << endl;
        cout << "Original length n    : " << text_len << endl;
        cout << "Kernel text length n : " << kernel_text_len << endl;
        cout << "+++++++++++++++++++++++++++++++++++++++++++++" << endl;
    }


    long t1 = Utils::wclock();



    long k1 = Utils::wclock();
    if (kernel_type == KernelType::FMI) {
        kernel_manager = new KernelManagerFMI(kernel_manager_prefix,
                                              verbose);
    } else if (kernel_type == KernelType::BWA) {
        kernel_manager = new KernelManagerBWA(kernel_manager_prefix,
                                              verbose);
    } else if (kernel_type == KernelType::BOWTIE2) {
        kernel_manager = new KernelManagerBowTie2(kernel_manager_prefix,
                                                  n_threads,
                                                  verbose);
    } else if (kernel_type == KernelType::BLAST) {
        kernel_manager = new KernelManagerBLAST(kernel_manager_prefix,
                                                n_threads,
                                                verbose);
    } else {
        cerr << "Unknown kernel type given" << endl;
        exit(EXIT_FAILURE);
    }
    long t2 = Utils::wclock();
    cout << "Indexing: "<< (t2-t1) << " seconds. " << endl;

    // delete [] kernel_text;
    //EncodeKernelLimitsAndSuccessor(tmp_limits_kernel);
    //load_from_file(limits_kernel, limits_kernel_filename);
    //load_from_file(sparse_sample_limits_kernel, sparse_sample_limits_kernel_filename);

    //delete [] tmp_limits_kernel;

    index_size_in_bytes += kernel_manager->GetSizeBytes();
    // ASSERT(kernel_text_len == kernel_manager->GetLength());
}

void HybridLZIndex::Kernelize() {
        ComputeKernelTextLen();
        if (verbose) {
            cerr << "+++++++++++++++++++++++++++++++++++++++++++++" << endl;
            cerr << "Original length n    : " << text_len << endl;
            cerr << "Kernel text length n : " << kernel_text_len << endl;
            cerr << "+++++++++++++++++++++++++++++++++++++++++++++" << endl;
        }

        uint64_t *tmp_limits_kernel;
        uchar *kernel_text;

        MyBuffer * my_buffer;
        if (lz_method == LZMethod::IN_MEMORY) {
            my_buffer = new MyBufferMemSeq(tmp_seq, text_len);
        } else {
            if (Utils::IsBioKernel(kernel_type)) {
                my_buffer = new MyBufferFastaFile(text_filename);
            } else {
                my_buffer = new MyBufferPlainFile(text_filename);
            }
        }

        MakeKernelString(my_buffer, &kernel_text, &tmp_limits_kernel);
        delete(my_buffer);

        // kenrel_text is deleted by KernelManager
        if (kernel_type == KernelType::FMI) {
            kernel_manager = new KernelManagerFMI(kernel_text,
                                                  kernel_text_len,
                                                  kernel_manager_prefix,
                                                  verbose);
        } else if (kernel_type == KernelType::BWA) {
            kernel_manager = new KernelManagerBWA(kernel_text,
                                                  kernel_text_len,
                                                  kernel_manager_prefix,
                                                  verbose);
        } else if (kernel_type == KernelType::BOWTIE2) {
            kernel_manager = new KernelManagerBowTie2(kernel_text,
                                                      kernel_text_len,
                                                      kernel_manager_prefix,
                                                      n_threads,
                                                      max_memory_MB,
                                                      verbose);
        } else if (kernel_type == KernelType::BLAST) {
            kernel_manager = new KernelManagerBLAST(kernel_text,
                                                    kernel_text_len,
                                                    kernel_manager_prefix,
                                                    n_threads,
                                                    max_memory_MB,
                                                    verbose);
        } else {
            cerr << "Unknown kernel type given" << endl;
            exit(EXIT_FAILURE);
        }

        EncodeKernelLimitsAndSuccessor(tmp_limits_kernel);
        delete [] tmp_limits_kernel;

        index_size_in_bytes += kernel_manager->GetSizeBytes();
        // ASSERT(kernel_text_len == kernel_manager->GetLength());
    }

// Assumes that the resulting kernel string fits in main memory.
// That is OK, as we will need to index it, so this is a hard limit for now.
void HybridLZIndex::MakeKernelString(MyBuffer * is,
                                     uchar ** kernel_ans,
                                     uint64_t ** tmp_limits_kernel_ans) {
  // Relies on GetLimit to navigate through the phrases.
  uchar *kernel_text;   // Filtered text
  kernel_text = new uchar[kernel_text_len];
  uint64_t *tmp_limits_kernel = new uint64_t[n_phrases+1];
  uint64_t left, right, posFil;
  left = posFil = 0;
  for (size_t i = 0; i < n_phrases; i++) {
    tmp_limits_kernel[i] = posFil;
    if (verbose >= 3) {
      cerr << "limit kernel: " << posFil << endl;
    }
    if (i+1 < n_phrases) {
      right = GetLimit(i+1);
      if (right-left < 2*context_len+2 || tsrr->IsLiteral(i)) {
        is->SetPos(left);
        for (size_t j =left; j < right; j++, posFil++) {
          kernel_text[posFil] = is->GetChar();
        }
      } else {
        // copy M symbol + '$' + M symbols...
        is->SetPos(left);
        for (size_t j = left; j < left + context_len; j++, posFil++) {
          kernel_text[posFil] = is->GetChar();;
        }
        for (size_t ss = 0; ss < max_insertions + 1; ss++) {
          kernel_text[posFil] = special_separator;
          posFil++;
        }
        is->SetPos(right - context_len);
        for (size_t j = right - context_len; j < right; j++, posFil++) {
          kernel_text[posFil] = is->GetChar();
        }
      }
    } else {
      right = text_len;
      if (right-left <= context_len || tsrr->IsLiteral(i)) {
        is->SetPos(left);
        for (size_t j =left; j < right; j++, posFil++) {
          kernel_text[posFil] = is->GetChar();
        }
      } else {
        // to copy current symbol + M + '$'= M+2
        is->SetPos(left);
        for (size_t j =left; j <=left+context_len; j++, posFil++) {
          kernel_text[posFil] = is->GetChar();
        }
        // TODO: Do I need those special characters at the end ?
        // Maybe just once?
        // Why in the previous case I use none ? it seems no-consistent...
        for (size_t ss = 0; ss < max_insertions + 1; ss++) {
          kernel_text[posFil] = special_separator;
          posFil++;
        }
      }
    }
    left =right;
  }
  tmp_limits_kernel[n_phrases] = posFil;
  if (posFil != kernel_text_len) {
    cerr << "Error: " << endl;
    cerr << "kernel_text_len = " << kernel_text_len << endl;
    cerr << "posFil = " << posFil << endl;
    exit(33);
  }
  *tmp_limits_kernel_ans = tmp_limits_kernel;
  *kernel_ans = kernel_text;
  if (verbose >= 3 && kernel_text_len <= 1000) {
    string tmp_kernel((const char *)kernel_text, kernel_text_len);
    cerr << "*******";
    cerr << tmp_kernel;
    cerr << "*******";

    cerr << "*******" << endl;
    for (size_t i = 0; i < kernel_text_len; i++) {
      if (kernel_text[i] != special_separator) {
        printf("L");
      } else {
        printf(" S ");
      }
    }
    cerr << "*******" << endl;
  }
}

void HybridLZIndex::EncodeKernelLimitsAndSuccessor(uint64_t * tmp_limits_kernel) {
  size_t sparse_sample_limits_kernel_len = (n_phrases+1)/sparse_sample_ratio;
  if ((n_phrases+1)%sparse_sample_ratio)
    sparse_sample_limits_kernel_len++;
  sparse_sample_limits_kernel = vector<uint64_t>(sparse_sample_limits_kernel_len);
  uint64_t posFil, countSMSucc;

  int_vector<> tmp_limits_int_vector(n_phrases+1);
  posFil = countSMSucc = 0;
  for (size_t i = 0; i <= n_phrases; i++) {
    posFil = tmp_limits_kernel[i];
    if (i%sparse_sample_ratio == 0) {
      sparse_sample_limits_kernel[countSMSucc] = posFil;
      countSMSucc++;
    }
    tmp_limits_int_vector[i] = tmp_limits_kernel[i];
  }
  limits_kernel = enc_vector<elias_delta, REGULAR_DENS>(tmp_limits_int_vector);

  index_size_in_bytes += sparse_sample_limits_kernel_len*sizeof(uint);
  index_size_in_bytes += sdsl::size_in_bytes(limits_kernel);
}

void HybridLZIndex::ComputeKernelTextLen() {
  uint64_t l, r;
  kernel_text_len = l = 0;
  for (size_t i = 0; i< n_phrases; i++) {
    if (i+1 < n_phrases) {
      r = GetLimit(i+1);
      if (r-l < 2*context_len+2 || tsrr->IsLiteral(i))
        kernel_text_len += r-l;
      else
        kernel_text_len += 2*context_len + 1 + max_insertions;  // to copy  M + '$' +k*'$' + M symbols = 2M+1
    } else {
      r = text_len;
      if (r-l <= context_len || tsrr->IsLiteral(i))
        kernel_text_len += r-l;
      else
        kernel_text_len += context_len + 2 + max_insertions;   // to copy current symbol + M + '$'= M+2
    }
    l =r;
  }
}

void HybridLZIndex::SetFileNames() {
  sprintf(sparse_sample_limits_kernel_filename, "%s.sparse_sample_limits_kernel", index_prefix);
  sprintf(limits_kernel_filename, "%s.limits_kernel", index_prefix);
  sprintf(variables_filename, "%s.variables", index_prefix);
  /*
  sprintf(kernel_manager_prefix,
          "%s.P%d_GC%d_kernel_text",
          index_prefix,
          sparse_sample_ratio,
          REGULAR_DENS);
          */
  sprintf(kernel_manager_prefix, "%s.kernel_text", index_prefix);
}

void HybridLZIndex::ChooseSpecialSeparator(char * filename) {
  FILE * fp = Utils::OpenReadOrDie(filename);

  uint64_t * alpha_test_tmp = new uint64_t[sigma];
  for (size_t i = 0; i < sigma; i++) alpha_test_tmp[i] = text_len;

  ////////// EM

  // few bytes, for testing:
  // uint64_t buffer_size = ((uint64_t)max_memory_MB/2);
  uint64_t buffer_size = ((uint64_t)max_memory_MB/2) << 20;
  uchar *buffer = new uchar[buffer_size];
  uint64_t curr_pos = 0;

  for (size_t i = 0;;) {
    ASSERT((uint64_t)ftell(fp) == curr_pos);
    uint64_t remaining = text_len - curr_pos;
    if (remaining == 0)
      break;
    uint64_t chunk_size = std::min(buffer_size, remaining);
    uint64_t read = fread(buffer, sizeof(uchar), chunk_size, fp);
    if (read != chunk_size) {
      cerr << "Error choosing unique char in EM" << endl;
      exit(EXIT_FAILURE);
    }

    for (size_t j = 0; j < chunk_size; j++) {
      i = curr_pos + j;
      uchar char_i = buffer[j];
      if (alpha_test_tmp[char_i] > i)
        alpha_test_tmp[char_i] = i;
    }
    curr_pos += chunk_size;
    if (verbose >= 2) {
      cerr << "Current pos in whole buffer: " << i << endl;
    }
  }
  delete[] buffer;
  fclose(fp);
  ////////// End EM
  SetSpecialSeparator(alpha_test_tmp);

  delete [] alpha_test_tmp;
}

// TODO merge those two using MyBuffer.
void HybridLZIndex::ChooseSpecialSeparator(uchar *seq) {
  // stores the position in T[1..n] of the first occurrence for each character.
  uint64_t * alpha_test_tmp = new uint64_t[sigma];

  for (size_t i = 0; i < sigma; i++) alpha_test_tmp[i] = text_len;

  for (size_t i = 0; i < text_len; i++) {
    if (alpha_test_tmp[seq[i]] > i)
      alpha_test_tmp[seq[i]] = i;
  }

  if (alpha_test_tmp[0] < text_len) {
    cerr << "Input has the Zero characer, sdsl will not build the fmi " << endl;
    exit(33);
  }
  SetSpecialSeparator(alpha_test_tmp);

  delete [] alpha_test_tmp;
}

void HybridLZIndex::SetSpecialSeparator(uint64_t * alpha_test_tmp) {
  /*
  if (verbose >= 3) {
    cerr << "--------------------" << endl;
    cerr << "Alphabet: " << endl;
    for (size_t i = 0; i < sigma; i++)
      cerr << alpha_test_tmp[i] << " ";
    cerr << "--------------------" << endl;
    cerr << endl;
  }
*/
  bool found;
  if (alpha_test_tmp['$'] == text_len) {
    // first preference is symbol '$'
    special_separator = '$';
    found = true;
  } else {
    // second preference is symbol '#'
    if (alpha_test_tmp['#'] == text_len) {
      special_separator = '#';
      found = true;
    } else {
      // third preference are symbols from 32 ASCII code to 126 one
      found = false;
      for (size_t i = 32; (i <= 126) && (found ==false); i++) {
        if (alpha_test_tmp[i] == text_len) {
          found = true;
          special_separator = (uchar)i;
        }
      }

      for (size_t i = 128; (i <= 254) && (found ==false); i++) {
        if (alpha_test_tmp[i] == text_len) {
          found = true;
          special_separator = (uchar)i;
        }
      }

      for (size_t i = 31; (i > 0) && (found ==false); i--) {
        if (alpha_test_tmp[i] == text_len) {
          found = true;
          special_separator = (uchar)i;
        }
      }
    }
  }

  if (found) {
    if (verbose) {
      cerr << "Separator symbol: code = " << (uint)special_separator;
      cerr << "                symbol = " << special_separator << endl;
    }
  } else {
    cerr << "Couldnt find a separator symbol, aborting!" << endl;
    exit(EXIT_FAILURE);
  }
}

void HybridLZIndex::Save() const {
  FILE * fp = Utils::OpenWriteOrDie(variables_filename);
  if (1 != fwrite(&text_len, sizeof(text_len), 1, fp)) {
    cerr << "Error writing the variables" << endl;
    exit(1);
  }
  if (1 != fwrite(&special_separator, sizeof(special_separator), 1, fp)) {
    cerr << "Error writing the variables" << endl;
    exit(1);
  }
  if (1 != fwrite(&max_query_len, sizeof(max_query_len), 1, fp)) {
    cerr << "Error writing the variables" << endl;
    exit(1);
  }
  if (1 != fwrite(&max_insertions, sizeof(max_insertions), 1, fp)) {
    cerr << "Error writing the variables" << endl;
    exit(1);
  }
  if (1 != fwrite(&sparse_sample_ratio, sizeof(sparse_sample_ratio), 1, fp)) {
    cerr << "Error writing the variables" << endl;
    exit(1);
  }
  if (1 != fwrite(&kernel_type, sizeof(kernel_type), 1, fp)) {
    cerr << "Error writing the variables" << endl;
    exit(1);
  }
  if (1 != fwrite(&kernel_text_len, sizeof(kernel_text_len), 1, fp)) {
    cerr << "Error writing the variables" << endl;
    exit(1);
  }
  fclose(fp);

  store_to_file(limits_kernel, limits_kernel_filename);
  store_to_file(sparse_sample_limits_kernel, sparse_sample_limits_kernel_filename);

  tsrr->Save();
  kernel_manager->Save();
  book_keeper->SetFileNames(index_prefix);
  book_keeper->Save();

  if (verbose) {
    cerr << "Saving is ready" << endl;
  }
}

void HybridLZIndex::Load(char * _prefix, int _n_threads, int _verbose) {
  this->book_keeper = NULL;
  this->sparse_sample_ratio = SPARSE_DENS;
  this->verbose = _verbose;
  this->n_threads = _n_threads;
  this->index_prefix = _prefix;
  this->tmp_seq = NULL;
  SetFileNames();

  FILE * fp = Utils::OpenReadOrDie(variables_filename);
  if (1 != fread(&text_len, sizeof(text_len), 1, fp)) {
    cerr << stderr << "Error reading var from file" << endl;
    exit(1);
  }
  if (1 != fread(&special_separator, sizeof(special_separator), 1, fp)) {
    cerr << stderr << "Error reading var from file" << endl;
    exit(1);
  }
  if (1 != fread(&max_query_len, sizeof(max_query_len), 1, fp)) {
    cerr << stderr << "Error reading var from file" << endl;
    exit(1);
  }
  if (1 != fread(&max_insertions, sizeof(max_insertions), 1, fp)) {
    cerr << stderr << "Error reading var from file" << endl;
    exit(1);
  }
  context_len = max_query_len + max_insertions;

  if (1 != fread(&sparse_sample_ratio, sizeof(sparse_sample_ratio), 1, fp)) {
    cerr << stderr << "Error reading var from file" << endl;
    exit(1);
  }
  if (1 != fread(&kernel_type, sizeof(kernel_type), 1, fp)) {
    cerr << stderr << "Error reading var from file" << endl;
    exit(1);
  }
    if(kernel_type==KernelType::BLAST) cout << "Kernel type is BLAST!  " << endl;
    if(kernel_type==KernelType::BOWTIE2) cout << "Kernel type is BOWTIE!  " << endl;
    if(kernel_type==KernelType::BWA) cout << "Kernel type is BWA!  " << endl;
  if (1 != fread(&kernel_text_len, sizeof(kernel_text_len), 1, fp)) {
    cerr << stderr << "Error reading var from file" << endl;
    exit(1);
  }
  fclose(fp);

  load_from_file(limits_kernel, limits_kernel_filename);
  load_from_file(sparse_sample_limits_kernel, sparse_sample_limits_kernel_filename);

  tsrr = new RangeReporting();
  tsrr->Load(_prefix, _verbose);

  if (kernel_type == KernelType::BWA) {
    kernel_manager = new KernelManagerBWA();
  } else if (kernel_type == KernelType::FMI) {
    kernel_manager = new KernelManagerFMI();
  } else if (kernel_type == KernelType::BOWTIE2) {
    kernel_manager = new KernelManagerBowTie2();
  }else if (kernel_type == KernelType::BLAST) {
      kernel_manager = new KernelManagerBLAST();
  }
  kernel_manager->Load(kernel_manager_prefix, n_threads, _verbose);
  // ASSERT(kernel_text_len == kernel_manager->GetLength());

  book_keeper = new BookKeeper();
  book_keeper->Load(_prefix, _verbose);


  n_phrases = limits_kernel.size() - 1;
  index_size_in_bytes = 0;
  ComputeSize();
  if (verbose) {
    cerr << "Index succesully loaded. Details:" << endl;
    DetailedSpaceUssage();
    ASSERT(InspectIndex());
  }
}

bool HybridLZIndex::InspectIndex() {
  return true;
  cerr << "Give a name to save Limits In Kernek" << endl;
  string output_name;
  std::cin >> output_name;
  std::ofstream tmp_out;
  tmp_out.open(output_name);
  for (size_t i = 0; i < n_phrases; i++) {
    tmp_out << GetLimitKernel(i) << endl;
  }
  tmp_out.close();
  cerr << "Succesfully saved to " << output_name << endl;

  cerr << "Give a name to save Limits In Text" << endl;
  std::cin >> output_name;
  tmp_out.open(output_name);
  for (size_t i = 0; i < n_phrases; i++) {
    tmp_out << GetLimit(i) << endl;
  }
  tmp_out.close();
  cerr << "Succesfully saved to " << output_name << endl;

  cerr << "Give a name to save IsLiteral" << endl;
  std::cin >> output_name;
  tmp_out.open(output_name);
  for (size_t i = 0; i < n_phrases; i++) {
    tmp_out << tsrr->IsLiteral(i) << endl;
  }
  tmp_out.close();
  cerr << "Succesfully saved to " << output_name << endl;
  return true;
}

void HybridLZIndex::ComputeSize() {
  uint count = 0;
  count += sparse_sample_limits_kernel.size()*sizeof(uint);
  count += sdsl::size_in_bytes(limits_kernel);
  count += kernel_manager->GetSizeBytes();
  count += tsrr->GetSizeBytes();
  ASSERT(index_size_in_bytes == 0 || index_size_in_bytes == count);
  index_size_in_bytes = count;
}

void HybridLZIndex::DetailedSpaceUssage() const {
  cerr << "--------------------------------------------------" << endl;
  cerr << "Sparse Successor Table   :" << sparse_sample_limits_kernel.size()*sizeof(uint) << endl;
  cerr << "KernelLimits                      :" << sdsl::size_in_bytes(limits_kernel) << endl;
  cerr << "Kernel Index             :" << kernel_manager->GetSizeBytes() << endl;
  cerr << "Range Reporting:" << endl;
  tsrr->DetailedSpaceUssage();
  cerr << "--------------------------------------------------" << endl;
  cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
  cerr << "TOTAL: " << index_size_in_bytes << "bytes, =" << endl;
  cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
  cerr << (float)index_size_in_bytes/(float)text_len << "|T|" << endl;
  cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
}

////////////////////////////////
// Queries functions:
////////////////////////////////
// This FQ implies a SAM output. Perhaps I should look for a better function name
void HybridLZIndex::FindFQ(char * query_filename,
                           char * mates_filename,
                           bool single_file_paired,
                           SecondaryReportType secondary_report,
                           vector<string> kernel_options,
                           ostream& my_out) const {
  long double t1, t2;
  size_t n_partitions = (size_t)n_threads;
  long double * times_per_thread = new long double[n_partitions + 1];
  for (size_t j = 0; j < n_partitions + 1 ; j++)
    times_per_thread[j] = 0;

  // This is the BAM header, probably KernelManager should take care of this?
  vector<string> header = book_keeper->SamHeader();
  if(kernel_type!=KernelType::BLAST)
  for (size_t i = 0; i < header.size(); i++) {
    my_out << header[i] << endl;
  }
  my_out << "@PG\tID:CHIC\tVN:0.1" << endl;

  bool retrieve_all = (secondary_report == SecondaryReportType::ALL);
  t1 = Utils::wclock();
  string tmp_sam_filename = kernel_manager->LocateOccsFQ(query_filename,
                                                     mates_filename,
                                                     retrieve_all,
                                                     single_file_paired,
                                                     kernel_options);
  t2 = Utils::wclock();
  cerr << "LocateOccsFQ took: "<< (t2-t1) << " seconds. " << endl;

  size_t sam_chunk_size = 1000;
	SamReader sam_reader(tmp_sam_filename, sam_chunk_size, n_partitions, verbose);
  Occurrence * primary_buffer = new Occurrence[n_partitions * sam_chunk_size];
  Occurrence * primary_buffer_ALT = new Occurrence[n_partitions * sam_chunk_size];
  size_t primary_buffer_len;
  size_t primary_buffer_ALT_len = 0;

  vector<vector<Occurrence>> data_1((uint)n_threads);
  vector<vector<Occurrence>> data_2((uint)n_threads);
  vector<vector<Occurrence>> * secondary_per_thread = &(data_1);
  vector<vector<Occurrence>> * secondary_per_thread_ALT = &(data_2);


  sam_reader.ReadOccurrences(primary_buffer, n_partitions * sam_chunk_size, &primary_buffer_len);
  while (primary_buffer_len != 0) {
#ifdef _OPENMP
  omp_set_num_threads(n_threads);
#endif
#pragma omp parallel for
    for (size_t t = 0; t < n_partitions + 1; t++) {
    t1 = Utils::wclock();
      if (t == n_partitions) {
        for (size_t i = 0; i < primary_buffer_ALT_len; i++) {
          my_out << primary_buffer_ALT[i].GetSamRecord() << "\n";
        }
        for (size_t tt = 0; tt < secondary_per_thread_ALT->size(); tt++) {
          for (size_t i = 0; i < (secondary_per_thread_ALT->at(tt)).size(); i++) {
            my_out << (secondary_per_thread_ALT->at(tt))[i].GetSamRecord() << "\n";
          }
          secondary_per_thread_ALT->at(tt).clear();
        }
        // INPUT:
        if (!sam_reader.IsReady()) {
          sam_reader.ReadOccurrences(primary_buffer_ALT, n_partitions * sam_chunk_size, &primary_buffer_ALT_len);
        } else {
          primary_buffer_ALT_len = 0;
        }
      } else {
        size_t thread_starting_pos = t*((primary_buffer_len)/n_partitions);
        size_t thread_length = (t != n_partitions - 1) ? (primary_buffer_len/n_partitions) : primary_buffer_len - thread_starting_pos;
        size_t thread_ending_pos = thread_starting_pos + thread_length;

        for (size_t kk = thread_starting_pos; kk < thread_ending_pos; kk++) {
            if(kernel_type!=KernelType::BLAST) primary_buffer[kk].Init();
            else primary_buffer[kk].InitBlast();
        }
        //ASSERT(secondary_per_thread->at(t).size() == 0);
        KernelOccsToPrimaryOccsFQ(&(primary_buffer[thread_starting_pos]),
                                  thread_length,
                                  secondary_report);

        if (secondary_report == SecondaryReportType::ALL ||
            secondary_report == SecondaryReportType::LZ) {
          searchSecondaryOcc(&(primary_buffer[thread_starting_pos]),
                             thread_length,
                             &(secondary_per_thread->at(t)));
        }
        for (size_t kk = thread_starting_pos; kk < thread_ending_pos; kk++) {
          book_keeper->NormalizeOutput(primary_buffer[kk], kernel_type);
        }
        for (size_t i = 0; i < (secondary_per_thread->at(t)).size(); i++) {
          book_keeper->NormalizeOutput(secondary_per_thread->at(t)[i], kernel_type);
        }
      }
  	  t2 = Utils::wclock();
		  times_per_thread[t] += (t2-t1);
    }

    Occurrence * tmp_buff = primary_buffer;
    primary_buffer = primary_buffer_ALT;
    primary_buffer_ALT = tmp_buff;

    size_t tmp_len = primary_buffer_len;
    primary_buffer_len  = primary_buffer_ALT_len;
    primary_buffer_ALT_len = tmp_len;

    vector<vector<Occurrence>> * tmp_sec = secondary_per_thread;
    secondary_per_thread = secondary_per_thread_ALT;
    secondary_per_thread_ALT = tmp_sec;
  }
  for (size_t i = 0; i < primary_buffer_ALT_len; i++) {
    my_out << primary_buffer_ALT[i].GetSamRecord() << "\n";
  }
  for (size_t t = 0; t < secondary_per_thread_ALT->size(); t++) {
    for (size_t i = 0; i < (secondary_per_thread_ALT->at(t)).size(); i++) {
      my_out << (secondary_per_thread_ALT->at(t))[i].GetSamRecord() << "\n";
    }
  }

  delete [] primary_buffer;
  delete [] primary_buffer_ALT;

  for (size_t j = 0; j < n_partitions + 1; j++) {
    cerr << "thread " << j << " took: " << times_per_thread[j] << endl;
  }
  delete [] times_per_thread;
  /*
  cerr << "Pre , sequential took: "<< (t_pre) << " seconds. " << endl;
 	cerr << "Parallel section took: "<< (t_parallel) << " seconds. " << endl;
  cerr << "Post, sequential took: "<< (t_post) << " seconds. " << endl;
  */
}

void HybridLZIndex::Find(vector<uint64_t> * ans, string query) const {
  ASSERT(ans->size() == 0);
  // ASSERT(query.size() <= context_len);  // THAT WAS AN ERROR!
  ASSERT(query.size() <= max_query_len);
  if (verbose >= 3) {
    cerr << "Query string:" << endl;
    cerr << query << endl;
  }
  vector<Occurrence> my_occs;

  FindPrimaryOccs(&my_occs, query);
  searchSecondaryOcc(&my_occs);
  for (size_t i = 0; i < my_occs.size(); i++) {
    ans->push_back(my_occs[i].GetPos());
  }
}

// TODO: move somewhere
bool ListContainsName(vector<Occurrence> * list, string name);
bool ListContainsName(vector<Occurrence> * list, string name) {
  for (size_t i = 0; i < list->size(); i++) {
    if (name == list->at(i).GetReadName()) {
      return true;
    }
  }
  return false;
}

void HybridLZIndex::KernelOccsToPrimaryOccsFQ(Occurrence * kernel_occs,
                                              size_t kernel_occs_len,
                                              SecondaryReportType secondary_report) const {
  // This was used when we were checking for lost reads. this is not done anymore.
  // bool retrieve_all = (secondary_report == SecondaryReportType::ALL);

  if (verbose >= 2) {
    cerr << "Occurrences mapped to kernel: " << kernel_occs_len << endl;
  }
  vector<Occurrence> lost_occs;
  for (size_t i = 0; i < kernel_occs_len; i++) {
    if ((kernel_occs[i]).IsUnmapped()) {
      continue;
    }
    uint64_t next_limit_pos = 0;
    uint prev_limit;
    uint64_t pos_in_text = MapKernelPosToTextPos(kernel_occs[i].GetPos(),
                                                 &next_limit_pos,
                                                 & prev_limit);
    if (next_limit_pos <= kernel_occs[i].GetPos() + kernel_occs[i].GetLength() - 1 ||
        tsrr->IsLiteral(prev_limit) ||
        secondary_report != SecondaryReportType::ALL) {
        if(kernel_type==KernelType::BLAST)
            kernel_occs[i].UpdatePosBlast(pos_in_text);
        else kernel_occs[i].UpdatePos(pos_in_text);
    } else {
      lost_occs.push_back(kernel_occs[i]);
      if (verbose >= 0) {
        cerr << "Warning:" << endl;
        cerr << "This is an important difference between the theory of Hybrid Index and usind a read aligner for the kernel. "<< endl;
        cerr << "Results should be checked..." << endl;
        cerr << "*********" << endl;
        cerr << "We just lost read aligned to Kernel Pos:" << kernel_occs[i].GetPos();
        cerr << "*********" << endl;
      }
    }
  }

  // CreateSamRecordsForTrulyLostAlignments(&lost_occs, kernel_occs, retrieve_all);
}

// This would work if all reads were processed at once.
// It should be rewritten, or deleted; current implementation could "lose" reads,
// in the sense that they are not reported as unaligned, but simply do not appear in the output.
/*
void HybridLZIndex::CreateSamRecordsForTrulyLostAlignments(vector<Occurrence> * lost_occs,
                                                           Occurrence * kernel_occs,
                                                           size_t kernel_occs_len,
                                                           bool retrieve_all) const {
  std::ofstream ofs;
  ofs.open ("MisAlignedAndLost.fq");
  int64_t truly_lost = 0;
  long double t1, t2;
  t1 = Utils::wclock();
  for (size_t i = 0; i < lost_occs->size(); i++) {
    string read_name = lost_occs->at(i).GetReadName();
    // if retrieve_all == false
    // we can assume that ListContainsName => false
    if (!retrieve_all || !ListContainsName(ans, read_name)) {
      truly_lost++;
      ofs << lost_occs->at(i).GetSamRecord() << endl;
      ans->push_back(Utils::SamRecordForUnmapped(lost_occs->at(i).GetReadName()));
    }
  }
  t2 = Utils::wclock();
  if (verbose >= 2) {
    cerr << "Check for truly lost reads:: "<< (t2-t1) << " seconds. " << endl;
    cerr << "Truly lost reads (aligned over a separator): " << truly_lost << endl;
    cerr << "(CHIC created a SAM unmapped-record for them)" << endl;
  }
  ofs.close();

}
*/

void HybridLZIndex::FindPrimaryOccs(vector<Occurrence> * ans, string query) const {
  vector<Occurrence> locations = kernel_manager->LocateOccs(query);
  // TODO: may be more efficient if we change the signature of this method, and we return locations.
  // we will need to do in-place filter instead of the push_backs that follows.
  size_t all_occs_in_kernel = locations.size();
  if (all_occs_in_kernel > 0) {  // TODO: this if seems redundant too
    for (size_t i = 0; i < all_occs_in_kernel; i++) {
      uint64_t next_limit_pos = 0;
      uint prev_limit;
      uint64_t pos_in_text = MapKernelPosToTextPos(locations[i].GetPos(),
                                                   &next_limit_pos,
                                                   & prev_limit);
      if (next_limit_pos <= locations[i].GetPos() + query.size() - 1 ||
          tsrr->IsLiteral(prev_limit)) {
          if(kernel_type==KernelType::BLAST)
              locations[i].UpdatePosBlast(pos_in_text);
          else locations[i].UpdatePos(pos_in_text);
        ans->push_back(locations[i]);
      }
    }
  }
}

void HybridLZIndex::searchSecondaryOcc(vector<Occurrence> * ans,
                                       uint *n_secondary_occs_ret) const {
  if (tsrr->GetNPhrasesGrid() == 0) {
    if (n_secondary_occs_ret != NULL) {
      *n_secondary_occs_ret = 0;
    }
    return;
  }
  size_t n_sec = 0;
  for (size_t i = 0; i < ans->size(); i++) {
    if (ans->at(i).IsUnmapped())
      continue;
    size_t curr_pos = ans->at(i).GetPos();
    size_t curr_m = ans->at(i).GetLength();
    vector<uint64_t> tmp_ans;
    tsrr->queryRR(curr_pos, curr_pos + curr_m - 1, &tmp_ans);

    for (size_t j = 0; j < tmp_ans.size(); j++) {
      size_t pos = tmp_ans[j];
      size_t posLim = tsrr->GetPtr(pos);
      size_t real_pos = GetLimit(posLim) + curr_pos - tsrr->GetX(pos);
      ans->push_back(ans->at(i));
      if(kernel_type==KernelType::BLAST)
          ans->back().UpdatePosBlast(real_pos, "", (int)256);
      else ans->back().UpdatePos(real_pos, "", (int)256);
    }
    n_sec += tmp_ans.size();
  }
  if (n_secondary_occs_ret != NULL) {
    *n_secondary_occs_ret = n_sec;
  }
}

void HybridLZIndex::searchSecondaryOcc(Occurrence * kernel_occs,
                                       size_t kernel_occs_len,
                                       vector<Occurrence> * second) const {
  if (tsrr->GetNPhrasesGrid() == 0) {
    return;
  }
  for (size_t i = 0; i < kernel_occs_len; i++) {
    if (kernel_occs[i].IsUnmapped())
      continue;
    size_t curr_pos = kernel_occs[i].GetPos();
    size_t curr_m = kernel_occs[i].GetLength();
    vector<uint64_t> phrases_that_cover;
    tsrr->queryRR(curr_pos, curr_pos + curr_m - 1, &phrases_that_cover);

    for (size_t j = 0; j < phrases_that_cover.size(); j++) {
      size_t pos = phrases_that_cover[j];
      size_t posLim = tsrr->GetPtr(pos);
      size_t real_pos = GetLimit(posLim) + curr_pos - tsrr->GetX(pos);
      second->push_back(kernel_occs[i]);  // this makes a coppy, invoking the copy constructor.
        if(kernel_type==KernelType::BLAST)
            second->back().UpdatePosBlast(real_pos, "", (int)256);
        else second->back().UpdatePos(real_pos, "", (int)256);
    }
  }
  for (size_t i = 0; i < second->size(); i++) {
    if (second->at(i).IsUnmapped())
      continue;
    size_t curr_pos = second->at(i).GetPos();
    size_t curr_m = second->at(i).GetLength();
    vector<uint64_t> phrases_that_cover;
    tsrr->queryRR(curr_pos, curr_pos + curr_m - 1, &phrases_that_cover);

    for (size_t j = 0; j < phrases_that_cover.size(); j++) {
      size_t pos = phrases_that_cover[j];
      size_t posLim = tsrr->GetPtr(pos);
      size_t real_pos = GetLimit(posLim) + curr_pos - tsrr->GetX(pos);
      second->push_back(second->at(i));  // this makes a coppy, invoking the copy constructor.
        if(kernel_type==KernelType::BLAST)
            second->back().UpdatePosBlast(real_pos, "", (int)256);
        else second->back().UpdatePos(real_pos, "", (int)256);
    }
  }
}

uint64_t HybridLZIndex::MapKernelPosToTextPos(uint64_t pos,
                                              uint64_t * _next_limit_pos,
                                              uint * pred) const {
  uint succ = SuccessorInKernelLimits(pos, _next_limit_pos);
  ASSERT(succ > 0);
  *pred = succ-1;
  uint64_t offset = (*_next_limit_pos) - pos;

  return GetLimit(succ) - offset;
}

uint HybridLZIndex::SuccessorInKernelLimits(uint64_t pos,
                                            uint64_t * _next_limit_pos) const {
  uint i;

  i = SampleBinarySearch(pos);

  uint successor;
  if (i < sparse_sample_limits_kernel.size()) {
    successor = SuccessorBinarySearch(pos,
                                      (i-1)*sparse_sample_ratio+1,
                                      i*sparse_sample_ratio,
                                      _next_limit_pos);
  } else {
    successor = SuccessorBinarySearch(pos,
                                      (i-1)*sparse_sample_ratio+1,
                                      n_phrases,
                                      _next_limit_pos);  // THIS IS THE REAL CHANGE!
  }
  return successor;
}

uint HybridLZIndex::SampleBinarySearch(uint64_t x) const {
  uint l = 0;
  uint r = sparse_sample_limits_kernel.size();
  uint m = (l+r)>>1;
  uint64_t Xm = sparse_sample_limits_kernel[m];
  while (l < r && (r-l) > 30 && (Xm <= x )) {
    if (Xm > x)
      r = m-1;
    else
      l = m+1;
    m = (l+r)>>1;
    Xm = sparse_sample_limits_kernel[m];
  }

  for (; l < r && sparse_sample_limits_kernel[l] <= x; l++) {}
  return l;
}

// return the index of the x successor, it is in LimitsKenel[l, r]
uint HybridLZIndex::SuccessorBinarySearch(uint64_t x,
                                          uint l,
                                          uint r,
                                          uint64_t *_next_limit_pos) const {
  if (r > l) {
    uint m = (l+r)>>1;
    uint64_t Xm = GetLimitKernel(m);

    while (l < r && (Xm <= x || GetLimitKernel(m-1) > x)) {
      if (Xm > x)
        r = m-1;
      else
        l = m+1;
      m = (l+r)>>1;
      Xm = GetLimitKernel(m);
    }
    *_next_limit_pos = Xm;
    return m;
  }

  *_next_limit_pos = GetLimitKernel(l);
  return l;
}


// ACCESSORS:
uint HybridLZIndex::GetSizeBytes() const {
  return index_size_in_bytes;
}

uint64_t HybridLZIndex::GetLimitKernel(uint pos) const {
  return limits_kernel[pos];
}

uint64_t HybridLZIndex::GetLimit(uint pos) const {
  return tsrr->GetLimit(pos);
}

HybridLZIndex::~HybridLZIndex() {
  delete(tsrr);
  if (book_keeper != NULL) {
    delete(book_keeper);
  }
  if (tmp_seq != NULL) {
    delete [] (tmp_seq);  // TODO: should be done earlier, probably.
  }
  delete(kernel_manager);
}
