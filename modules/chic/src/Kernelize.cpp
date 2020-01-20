// Copyright Daniel Valenzuela
#include "./HybridLZIndex.h"
#include <sdsl/util.hpp>
#include <sdsl/vectors.hpp>
#include <string>
#include <utility>
#include <algorithm>
#include <vector>
#include "./utils.h"
#include "./LempelZivParser.h"
#include "../ext/LZ/LZscan/algorithm/lzscan.h"

// INDEX CONSTRUCTION:
void HybridLZIndex::ValidateParams(BuildParameters * parameters) {
    if (Utils::IsBioKernel(parameters->kernel_type)) {
        if (!(parameters->lz_method == LZMethod::IN_MEMORY || parameters->lz_method == LZMethod::RLZ)){
            if (!(parameters->lz_method == LZMethod::RLZ)){
                cout << "Currently, only In memory and RLZ are supported for fasta files" << endl;
                cout << "Abortning" << endl;
                exit(EXIT_FAILURE);
            }
        }}
}

HybridLZIndex::HybridLZIndex() {
  this->index_size_in_bytes = 0;
}

HybridLZIndex::HybridLZIndex(BuildParameters * parameters) {
  this->verbose = parameters->verbose;
  //ValidateParams(parameters);
  this->lz_method = parameters->lz_method;
  if (lz_method == LZMethod::IN_MEMORY) {
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
    //if (_max_memory_MB == 0) _max_memory_MB = 100;
  } else {
    this->book_keeper = new BookKeeper(parameters->input_filename,
                                       parameters->kernel_type,
                                       verbose);
    //char * new_input_name = book_keeper->GetNewFileName();
    //this->text_filename = new_input_name;
    this->text_filename = parameters->input_filename;
    this->text_len = book_keeper->GetTotalLength();
    
    
    this->tmp_seq = NULL;
    //ASSERT(parameters->max_memory_MB > 0);
  }
  this->input_lz_filename = parameters->input_lz_filename;

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

  Build();
}

void HybridLZIndex::Build() {
  if (kernel_type == KernelType::BWA || kernel_type == KernelType::BOWTIE2) {
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
  Kernelize();
  ComputeSize();
  if (verbose >= 2) {
    DetailedSpaceUssage();
  }
}

void HybridLZIndex::Kernelize() {
  ComputeKernelTextLen();
  if (verbose) {
    cout << "+++++++++++++++++++++++++++++++++++++++++++++" << endl;
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
    //if (kernel_type == KernelType::BWA || kernel_type == KernelType::BOWTIE2) {
    if (Utils::IsBioKernel(kernel_type)) {
      my_buffer = new MyBufferFastaFile(text_filename);
    } else {
      my_buffer = new MyBufferPlainFile(text_filename);
    }
  }


  MakeKernelString(my_buffer, &kernel_text, &tmp_limits_kernel);
  long t2 = Utils::wclock();
  cout << "MakeKernelString: "<< (t2-t1) << " seconds. " << endl;

  delete(my_buffer);

    long k1 = Utils::wclock();
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
                                              verbose);
  } else {
    cerr << "Unknown kernel type given" << endl;
    exit(EXIT_FAILURE);
  }
  long k2 = Utils::wclock();
  cout << "Indexing: "<< (t2-t1) << " seconds. " << endl;

    delete [] kernel_text;

  EncodeKernelLimitsAndSuccessor(tmp_limits_kernel);
  delete [] tmp_limits_kernel;

  index_size_in_bytes += kernel_manager->GetSizeBytes();
  // ASSERT(kernel_text_len == kernel_manager->GetLength());
}

// Assumes that the resulting kernel string fits in main memory.
// That is OK, as we will need to index it, so this is a hard limit for now.
//TODO: Do this in partitions and merge kerneltext in the end
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
      cout << "limit kernel: " << posFil << endl;
    }
    if (i+1 < n_phrases) {
      right = GetLimit(i+1);
      if (right-left < 2*context_len+2 || tsrr->IsLiteral(i)) {
        is->SetPos(left);
        for (size_t j =left; j < right; j++, posFil++) {
          kernel_text[posFil] = is->GetChar();;
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
    cout << "*******";
    cout << tmp_kernel;
    cout << "*******";

    cout << "*******" << endl;
    for (size_t i = 0; i < kernel_text_len; i++) {
      if (kernel_text[i] != special_separator) {
        printf("L");
      } else {
        printf(" S ");
      }
    }
    cout << "*******" << endl;

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
        kernel_text_len += 2*context_len + 1 + max_insertions ;  // to copy  M + '$' +k*'$' + M symbols = 2M+1
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
  sprintf(kernel_manager_prefix,
          "%s.P%d_GC%d_kernel_text",
          index_prefix,
          sparse_sample_ratio,
          REGULAR_DENS);
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
      cout << "Current pos in whole buffer: " << i << endl;
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
    cout << "--------------------" << endl;
    cout << "Alphabet: " << endl;
    for (size_t i = 0; i < sigma; i++)
      cout << alpha_test_tmp[i] << " ";
    cout << "--------------------" << endl;
    cout << endl;
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
      cout << "Separator symbol: code = " << (uint)special_separator;
      cout << "                symbol = " << special_separator << endl;
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
    cout << "Saving is ready" << endl;
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
  }
  kernel_manager->Load(kernel_manager_prefix, n_threads, _verbose);
  // ASSERT(kernel_text_len == kernel_manager->GetLength());

  book_keeper = new BookKeeper();
  book_keeper->Load(_prefix, _verbose);


  n_phrases = limits_kernel.size() - 1;
  index_size_in_bytes = 0;
  ComputeSize();
  if (verbose) {
    cout << "Index succesully loaded. Details:" << endl;
    DetailedSpaceUssage();
    ASSERT(InspectIndex());
  }
}

bool HybridLZIndex::InspectIndex() {
  return true;
  cout << "Give a name to save Limits In Kernek" << endl;
  string output_name;
  std::cin >> output_name;
  std::ofstream tmp_out;
  tmp_out.open(output_name);
  for (size_t i = 0; i < n_phrases; i++) {
    tmp_out << GetLimitKernel(i) << endl;
  }
  tmp_out.close();
  cout << "Succesfully saved to " << output_name << endl;

  cout << "Give a name to save Limits In Text" << endl;
  std::cin >> output_name;
  tmp_out.open(output_name);
  for (size_t i = 0; i < n_phrases; i++) {
    tmp_out << GetLimit(i) << endl;
  }
  tmp_out.close();
  cout << "Succesfully saved to " << output_name << endl;

  cout << "Give a name to save IsLiteral" << endl;
  std::cin >> output_name;
  tmp_out.open(output_name);
  for (size_t i = 0; i < n_phrases; i++) {
    tmp_out << tsrr->IsLiteral(i) << endl;
  }
  tmp_out.close();
  cout << "Succesfully saved to " << output_name << endl;
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
  cout << "--------------------------------------------------" << endl;
  cout << "Sparse Successor Table   :" << sparse_sample_limits_kernel.size()*sizeof(uint) << endl;
  cout << "KernelLimits                      :" << sdsl::size_in_bytes(limits_kernel) << endl;
  cout << "Kernel Index             :" << kernel_manager->GetSizeBytes() << endl;
  cout << "Range Reporting:" << endl;
  tsrr->DetailedSpaceUssage();
  cout << "--------------------------------------------------" << endl;
  cout << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
  cout << "TOTAL: " << index_size_in_bytes << "bytes, =" << endl;
  cout << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
  cout << (float)index_size_in_bytes/(float)text_len << "|T|" << endl;
  cout << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
}




// TODO: move somewhere
bool ListContainsName(vector<Occurrence> * list, string name);
bool ListContainsName(vector<Occurrence> * list, string name) {
  for (size_t i = 0; i < list->size(); i++) {
    if(name == list->at(i).GetReadName()) {
      return true;
    }
  }
  return false;
}

uint64_t HybridLZIndex::MapKernelPosToTextPos(uint64_t pos,
                                              uint64_t * _next_limit_pos,
                                              uint * pred) const {
  uint succ = SuccessorInKernelLimits(pos, _next_limit_pos);
  ASSERT(succ > 0);
  *pred = succ-1;
  uint64_t offset = (*_next_limit_pos) - pos;
    cout << "SUCCessor: "<< succ << endl;
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
