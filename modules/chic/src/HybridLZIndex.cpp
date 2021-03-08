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
#include "./jni/hdfs.h"


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
    fprintf(stdout, " HLZ");
  this->index_size_in_bytes = 0;
}

HybridLZIndex::HybridLZIndex(BuildParameters * parameters) {
    fprintf(stdout, " HLZ 0");
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
    //if (_max_memory_MB == 0) _max_memory_MB = 100;
  } else {*/
    fprintf(stdout, " HLZ 1");
    this->book_keeper = new BookKeeper(parameters->input_filename,
                                       parameters->kernel_type,
                                       verbose);
    fprintf(stdout, " HLZ 2");
    //char * new_input_name = book_keeper->GetNewFileName();
    //this->text_filename = new_input_name;
    this->text_filename = parameters->input_filename;
    this->text_len = book_keeper->GetTotalLength();
    
    
    this->tmp_seq = NULL;
    //ASSERT(parameters->max_memory_MB > 0);
  //}
    fprintf(stdout, "LZ file %s",parameters->input_lz_filename);
    fprintf(stdout, "Indexing only = %d", parameters->indexingonly);

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
    this->hdfs_path = parameters->hdfs_path;
  if(parameters->kernelizeonly==1){
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
    try {

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
    }catch(const char *e){
        cout << "Exception: " << e << " "<< endl;
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

    fprintf(stdout," ndexing only. Reading LZ parse..");
    vector<pair<uint64_t, uint64_t> > lz_phrases;
    //LempelZivParser::GetLZPhrases(&lz_phrases, this);
    FILE * lz_infile  = Utils::OpenReadOrDie(GetInputLZFilename());
    LempelZivParser::LoadLZParse(lz_infile, &lz_phrases);
    n_phrases = lz_phrases.size();
    tsrr =  new RangeReporting(&lz_phrases, context_len, verbose);
    tsrr->SetFileNames(index_prefix);

    if (verbose >= 2)
        fprintf(stdout," PHRASES Previous merge:: %d \\n", n_phrases );
    n_phrases = lz_phrases.size();
    if (verbose >= 2)
        fprintf(stdout," After merge: %d \\n", n_phrases );

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
    //if (kernel_type == KernelType::BWA || kernel_type == KernelType::BOWTIE2) {
    if (Utils::IsBioKernel(kernel_type)) {
            if(this->hdfs_path != NULL)
                my_buffer = new MyBufferHDFS(this->hdfs_path);
            else my_buffer = new MyBufferFastaFile(text_filename);
    } else {
      my_buffer = new MyBufferPlainFile(text_filename);
    }
  }

    MakeKernelString(my_buffer, &kernel_text, &tmp_limits_kernel);
    long t2 = Utils::wclock();
    cout << "MakeKernelString from HDFS: "<< (t2-t1) << " seconds. " << endl;

    delete(my_buffer);

    long k1 = Utils::wclock();

    this->WriteKernelTextFile(kernel_text, kernel_text_len);

    long k2 = Utils::wclock();
    cout << "Kernelized: "<< (t2-t1) << " seconds. " << endl;

    //delete [] kernel_text;

    //EncodeKernelLimitsAndSuccessor(tmp_limits_kernel);
    //store_to_file(limits_kernel, limits_kernel_filename);
    //store_to_file(sparse_sample_limits_kernel, sparse_sample_limits_kernel_filename);

    //delete [] tmp_limits_kernel;

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

    fprintf(stdout,"+++++++++++++++++++++++++++++++++++++++++++++");
    fprintf(stdout, "Original length n    :  %lu", text_len);
    fprintf(stdout, "Kernel text length n : %lu", kernel_text_len);
    cout << "+++++++++++++++++++++++++++++++++++++++++++++" << endl;

    try {


        fprintf(stdout, " HERE 1");
    uint64_t *tmp_limits_kernel;
  uchar *kernel_text;

  long t1 = Utils::wclock();
  MyBuffer * my_buffer;
  if (lz_method == LZMethod::IN_MEMORY) {
    my_buffer = new MyBufferMemSeq(tmp_seq, text_len);
  } else {
    //if (kernel_type == KernelType::BWA || kernel_type == KernelType::BOWTIE2) {
    if (Utils::IsBioKernel(kernel_type)) {
            if(this->hdfs_path != NULL)
                my_buffer = new MyBufferHDFS(this->hdfs_path);
            else my_buffer = new MyBufferFastaFile(text_filename);
    } else {
      my_buffer = new MyBufferPlainFile(text_filename);
    }
  }

        fprintf(stdout, " HERE 2");
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
                                              n_threads,
                                              kernel_manager_prefix,
                                              verbose);
  } else if (kernel_type == KernelType::BLAST) {
      kernel_manager = new KernelManagerBLAST(kernel_text,
                                                kernel_text_len,
                                                n_threads,
                                                kernel_manager_prefix,
                                                verbose);
  } else {
    cerr << "Unknown kernel type given" << endl;
    exit(EXIT_FAILURE);
  }
  long k2 = Utils::wclock();
  cout << "Indexing: "<< (t2-t1) << " seconds. " << endl;

   // delete [] kernel_text;

  EncodeKernelLimitsAndSuccessor(tmp_limits_kernel);
  delete [] tmp_limits_kernel;

  index_size_in_bytes += kernel_manager->GetSizeBytes();
  // ASSERT(kernel_text_len == kernel_manager->GetLength());
    }catch(const char *e){
        cout << "Exception: " << e << " "<< endl;
    }
}

// Assumes that the resulting kernel string fits in main memory.
// That is OK, as we will need to index it, so this is a hard limit for now.
//TODO: Do this in partitions and merge kerneltext in the end
void HybridLZIndex::MakeKernelString(MyBuffer * is,
                                     uchar ** kernel_ans,
                                     uint64_t ** tmp_limits_kernel_ans) {
  // Relies on GetLimit to navigate through the phrases.
  //cout << "Allocate memory for kernel text of len:: " << kernel_text_len << endl;
  uchar *kernel_text;   // Filtered text

  fprintf(stdout, "Allocate memory for kernel text of len ");
    fprintf(stdout, "%lu", kernel_text_len);

  kernel_text = new uchar[kernel_text_len];
  uint64_t *tmp_limits_kernel = new uint64_t[n_phrases+1];
  uint64_t left, right, posFil;
  left = posFil = 0;
  for (size_t i = 0; i < n_phrases; i++) {
    tmp_limits_kernel[i] = posFil;
    //fprintf(stdout, " limit kernel... ");
    if (verbose >= 3) {
      cout << "limit kernel: " << posFil << endl;
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
    try {


   fprintf(stdout,"PHRASES: %u",n_phrases);
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
  fprintf(stdout,"%lu ",kernel_text_len);
    }catch(const char *e){
        fprintf(stdout,"Exception: %s",e);
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


//TODO: do this from HDFS
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

void HybridLZIndex::WriteToHDFS(uchar * text) {


    std::string p("/user/root/index/");
    string s = kernel_manager_prefix;
    std::size_t found = s.find_last_of("/\\");
    p += s.substr(found+1);
    const char *writePath = p.c_str();

    fs = hdfsConnect("node-1", 8020);
    hdfsFile hfile = hdfsOpenFile(fs, writePath, O_WRONLY|O_CREAT, 0, 0, 0);
    cout << writePath << endl;
    if(!hfile) {
        fprintf(stderr, "Failed to open %s for writing!\n", writePath);
    }

    char* buffer = reinterpret_cast<char*>(text);
    tSize num_written_bytes = hdfsWrite(fs, hfile, (void*)buffer, strlen(buffer)+1);

    if (hdfsFlush(fs, hfile)) {
        fprintf(stderr, "Failed to 'flush' %s\n", writePath);
        exit(-1);
    }
    hdfsCloseFile(fs, hfile);
}

/*void HybridLZIndex::SaveToHDFS() const {
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
*/

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
    for (size_t i = 0; i < sparse_sample_limits_kernel.size(); i++) {
        cout << sparse_sample_limits_kernel[i] << endl;
    }

    for (size_t i = 0; i < limits_kernel.size(); i++) {
        cout << limits_kernel[i] << endl;
    }
    cout << "Limits kernel" <<  limits_kernel_filename << endl;
    cout << "KernelLimits                      :" << sdsl::size_in_bytes(limits_kernel) << endl;
    cout << "KernelLimits                      :" << limits_kernel.size() << endl;

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
  //ASSERT(kernel_text_len == kernel_manager->GetLength());

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
  vector<Occurrence> my_occs;
  vector<Occurrence> unmapped_occs;
  FindPrimaryOccsFQ(&my_occs,
                    &unmapped_occs,
                    secondary_report,
                    query_filename,
                    mates_filename,
                    single_file_paired,
                    kernel_options);
  if (secondary_report == SecondaryReportType::ALL ||
      secondary_report == SecondaryReportType::LZ) {
    searchSecondaryOcc(&my_occs);
  }

  // TODO:
  // This is the BAM header, probable KernelBWA should take care of this...
  vector<string> header = book_keeper->SamHeader();
  if(kernel_type!=KernelType::BLAST)
  for (size_t i = 0; i < header.size(); i++) {
      my_out << header[i] << endl;
  }
  my_out << "@PG\tID:CHIC\tVN:0.1" << endl;

  for (size_t i = 0; i < my_occs.size(); i++) {
    book_keeper->NormalizeOutput(my_occs[i], kernel_type);
    my_out << my_occs[i].GetMessage() << endl;
  }
  for (size_t i = 0; i < unmapped_occs.size(); i++) {
    // TODO: verify is those also need some kind of normalization.
    my_out << unmapped_occs[i].GetMessage() << endl;
  }
}

void HybridLZIndex::FindFQ2(char * alignment_filename,
                           bool single_file_paired,
                           SecondaryReportType secondary_report,
                           vector<string> kernel_options,
                           ostream& my_out) const {
  vector<Occurrence> my_occs;
  vector<Occurrence> unmapped_occs;
  FindPrimaryOccsFQ2(&my_occs,
                    &unmapped_occs,
                    secondary_report,
                    alignment_filename,
                    single_file_paired,
                    kernel_options);
  if (secondary_report == SecondaryReportType::ALL ||
      secondary_report == SecondaryReportType::LZ) {
    searchSecondaryOcc(&my_occs);
  }

  // TODO:
  // This is the BAM header, probable KernelBWA should take care of this...
  vector<string> header = book_keeper->SamHeader();
  for (size_t i = 0; i < header.size(); i++) {
    my_out << header[i] << endl;
  }
  my_out << "@PG\tID:CHIC\tVN:0.1" << endl;

  for (size_t i = 0; i < my_occs.size(); i++) {
    book_keeper->NormalizeOutput(my_occs[i], kernel_type);
    my_out << my_occs[i].GetMessage() << endl;
  }
  for (size_t i = 0; i < unmapped_occs.size(); i++) {
    // TODO: verify is those also need some kind of normalization.
    my_out << unmapped_occs[i].GetMessage() << endl;
  }
}

void HybridLZIndex::FindALL(vector<Occurrence> my_occs,
                           char * query_filename,
                           char * mates_filename,
                           bool single_file_paired,
                           SecondaryReportType secondary_report,
                           vector<string> kernel_options,
                           ostream& my_out) const {

    size_t all_occs_in_kernel = my_occs.size();
    if (verbose >= 2) {
        cout << "Occurrences mapped to kernel: " << all_occs_in_kernel << endl;
    }
    vector<Occurrence> lost_occs;
    if (all_occs_in_kernel > 0) { // seems redundant.
        for (size_t i = 0; i < all_occs_in_kernel; i++) {
            if (my_occs[i].IsUnmapped()) {

                    cout << "Discarding unmapped read from alignments to kernel" << endl;

                continue;
            }
            uint64_t next_limit_pos = 0;
            uint prev_limit;
            uint64_t pos_in_text = MapKernelPosToTextPos(my_occs[i].GetPos(),
                                                         &next_limit_pos,
                                                         & prev_limit);
            cout << "Position in real text:" << pos_in_text << endl;
            cout << "Position in kernel:" << my_occs[i].GetPos() << endl;
            //book_keeper->NormalizeOutput(my_occs[i]);
            //cout << "Normalized pos :" << my_occs[i].GetPos() << endl;
            //TODO: store sequence(chr) lengths in file and calculate ancsestor from real text position, put ancestor in occurrence message

            cout << "name:" << my_occs[i].GetReadName() << endl;
            cout << "len:" << my_occs[i].GetLength() << endl;
            cout << "message:" << my_occs[i].GetMessage() << endl;
            vector<string> my_seqs = kernel_manager->ExtractSequences(pos_in_text, my_occs[i].GetLength());
            for (size_t j = 1; j < my_seqs.size(); j++) {
                string s = my_seqs[j];
                cout << "Sequence in position " << pos_in_text << " : "<< s << " :: " << endl;
                cout << "WTF" << endl;
            }

            if (next_limit_pos <= my_occs[i].GetPos() + my_occs[i].GetLength() - 1 ||
                tsrr->IsLiteral(prev_limit)) {
                if(kernel_type==KernelType::BLAST)
                    my_occs[i].UpdatePosBlast(pos_in_text);
                else my_occs[i].UpdatePos(pos_in_text);
            } else {
                lost_occs.push_back(my_occs[i]);
                if (verbose >= 3) {
                    cout << "Warning: if we had a kernel that could handle the special separators" << endl;
                    cout << "as characters that differ from everything else (as opposed to N's) "<< endl;
                    cout << "then this should never happen. Here we are losing a mapped read. "<< endl;
                    cout << "It may be the case that there was another position where it could be mapped, but this artifact alignment";
                    cout << "took presedence over the next one" << endl;
                    cout << "*********" << endl;
                    cout << "We just lost read aligned to Kernel Pos:" << my_occs[i].GetPos();
                    cout << "*********" << endl;
                }
            }
        }
    }
    size_t tot_map_to_kernel = my_occs.size();

        cout << "Total occurrence mapped  to the kernel:" << tot_map_to_kernel << endl;
    cout << "Total not mapped  to the kernel:" << lost_occs.size() << endl;
    // TODO:
    // This is the BAM header, probable KernelBWA should take care of this...

    /*for (size_t i = 0; i < my_occs.size(); i++) {
       //cout << "Occurrence:" << endl;
        cout << my_occs[i].GetMessage() << endl;
    }
    for (size_t i = 0; i < my_occs.size(); i++) {
        cout << "Normalized Occurrences:" << endl;
        book_keeper->NormalizeOutput(my_occs[i]);
        cout << my_occs[i].GetMessage() << endl;
    }*/

    if (secondary_report == SecondaryReportType::ALL ||
           secondary_report == SecondaryReportType::LZ) {
        searchSecondaryOcc(&my_occs);
    }

}

void HybridLZIndex::Find(vector<uint64_t> * ans, string query) const {
  ASSERT(ans->size() == 0);
  //ASSERT(query.size() <= context_len);  // THAT WAS AN ERROR!
  ASSERT(query.size() <= max_query_len);
  if (verbose >= 4) {
    cout << "Query string:" << endl;
    cout << query << endl;
  }
  vector<Occurrence> my_occs;

  FindPrimaryOccs(&my_occs, query);
  searchSecondaryOcc(&my_occs);
  for (size_t i = 0; i < my_occs.size(); i++) {
    ans->push_back(my_occs[i].GetPos());
  }
}

void HybridLZIndex::FindPatterns(vector<Occurrence> * ans, string query) const {
    ASSERT(ans->size() == 0);
    //ASSERT(query.size() <= context_len);  // THAT WAS AN ERROR!
    ASSERT(query.size() <= max_query_len);
    if (verbose >= 4) {
        cout << "Query string:" << endl;
        cout << query << endl;
    }
    vector<Occurrence> my_occs;

    FindPrimaryOccs(&my_occs, query);
    //searchSecondaryOcc(&my_occs);
    for (size_t i = 0; i < my_occs.size(); i++) {
        ans->push_back(my_occs[i]);
    }
}

void HybridLZIndex::Find(vector<string> *ans, vector<uint64_t> position, uint64_t range) const {
    ASSERT(ans->size() == 0);
    //ASSERT(query.size() <= context_len);  // THAT WAS AN ERROR!

    /*if (verbose >= 4) {
        cout << "Pos 0" << endl;
        cout << position[0] << endl;
    }*/


    for (size_t i = 0; i < position.size(); i++) {
        vector<string> my_seqs = kernel_manager->ExtractSequences(position[i], range);
        cout << my_seqs[0] <<  " :: " << endl;
        cout << my_seqs[1] <<  " :: " << endl;
        cout << my_seqs[2] <<  " :: " << endl;
        cout << my_seqs[3] <<  " :: " << endl;
        cout << my_seqs[4] <<  " :: " << endl;
        cout << "BLB" << endl;
        for (size_t j = 1; j < my_seqs.size(); j++) {
            string s = my_seqs[j];
            cout << "Sequence in position " << position[i] << " : "<< s << " :: " << endl;
            cout << "WTF" << endl;
        }

        /*const std::basic_string<char> &seqs = extract(index, position-range, position+range);
        vector<string> ans;
        ans.reserve(seqs.size());
        for (size_t i = 0; i < seqs.size(); i++) {
            ans.push_back(seqs);
        }
        return ans;*/


        ans->push_back(my_seqs[i]);
    }
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

void HybridLZIndex::FindPrimaryOccsFQ(vector<Occurrence> * ans,
                                      vector<Occurrence> * unmapped,
                                      SecondaryReportType secondary_report,
                                      char * query_filename,
                                      char * mates_filename,
                                      bool single_file_paired,
                                      vector<string> kernel_options) const {
  bool retrieve_all = (secondary_report == SecondaryReportType::ALL);
  vector<Occurrence> locations = kernel_manager->LocateOccsFQ(query_filename,
                                                              mates_filename,
                                                              retrieve_all,
                                                              single_file_paired,
                                                              kernel_options);

  size_t all_occs_in_kernel = locations.size();
  if (verbose >= 2) {
    cout << "Occurrences mapped to kernel: " << all_occs_in_kernel << endl;
  }
  vector<Occurrence> lost_occs;
  if (all_occs_in_kernel > 0) { // seems redundant.
    for (size_t i = 0; i < all_occs_in_kernel; i++) {
      if (locations[i].IsUnmapped()) {
        unmapped->push_back(locations[i]);
        if (verbose >= 3) {
          cout << "Discarding unmapped read from alignments to kernel" << endl;
        }
        continue;
      }
      uint64_t next_limit_pos = 0;
      uint prev_limit;
      uint64_t pos_in_text = MapKernelPosToTextPos(locations[i].GetPos(),
                                                   &next_limit_pos,
                                                   & prev_limit);

      if (next_limit_pos <= locations[i].GetPos() + locations[i].GetLength() - 1 ||
          tsrr->IsLiteral(prev_limit)) {
          if(kernel_type==KernelType::BLAST){
              locations[i].UpdatePosBlast(pos_in_text);
          }else
              locations[i].UpdatePos(pos_in_text);
        ans->push_back(locations[i]);
      } else {
        lost_occs.push_back(locations[i]);
        if (verbose >= 3) {
          cout << "Warning: if we had a kernel that could handle the special separators" << endl;
          cout << "as characters that differ from everything else (as opposed to N's) "<< endl;
          cout << "then this should never happen. Here we are losing a mapped read. "<< endl;
          cout << "It may be the case that there was another position where it could be mapped, but this artifact alignment";
          cout << "took presedence over the next one" << endl;
          cout << "*********" << endl;
          cout << "We just lost read aligned to Kernel Pos:" << locations[i].GetPos();
          cout << "*********" << endl;
        }
      }
    }
  }
  size_t tot_map_to_kernel = locations.size();
  size_t unmapped_to_kernel = unmapped->size();
  if (verbose) {
    cout << "Total occurrence mapped  to the kernel:" << tot_map_to_kernel << endl;
    cout << "Total occurrence NOT MAP to the kernel:" << unmapped_to_kernel << endl;
  }
  if (verbose >= 3) {
    CreateSamRecordsForTrulyLostAlignments(&lost_occs, unmapped, ans, retrieve_all);
  }
}

void HybridLZIndex::FindPrimaryOccsFQ2(vector<Occurrence> * ans,
                                      vector<Occurrence> * unmapped,
                                      SecondaryReportType secondary_report,
                                      char * alignment_filename,
                                      bool single_file_paired,
                                      vector<string> kernel_options) const {
  bool retrieve_all = (secondary_report == SecondaryReportType::ALL);

  vector<Occurrence> locations;
  if(kernel_type==KernelType::BLAST)
      locations = KernelManagerBLAST::SamOccurrences(alignment_filename);
  else
      locations = KernelManagerBWA::SamOccurrences(alignment_filename);

      size_t all_occs_in_kernel = locations.size();
  if (verbose >= 2) {
    cout << "Occurrences mapped to kernel: " << all_occs_in_kernel << endl;
  }
  vector<Occurrence> lost_occs;
  if (all_occs_in_kernel > 0) { // seems redundant.
    for (size_t i = 0; i < all_occs_in_kernel; i++) {
      if (locations[i].IsUnmapped()) {
        unmapped->push_back(locations[i]);
        if (verbose >= 3) {
          cout << "Discarding unmapped read from alignments to kernel" << endl;
        }
        continue;
      }
      uint64_t next_limit_pos = 0;
      uint prev_limit;
      uint64_t pos_in_text = MapKernelPosToTextPos(locations[i].GetPos(),
                                                   &next_limit_pos,
                                                   & prev_limit);
      if (next_limit_pos <= locations[i].GetPos() + locations[i].GetLength() - 1 ||
          tsrr->IsLiteral(prev_limit) ||
          secondary_report != SecondaryReportType::ALL) {
          if(kernel_type==KernelType::BLAST)
              locations[i].UpdatePosBlast(pos_in_text);
          else locations[i].UpdatePos(pos_in_text);
        ans->push_back(locations[i]);
      } else {
        lost_occs.push_back(locations[i]);
        if (verbose >= 0) {
          cout << "Warning:" << endl;
          cout << "This is an important difference between the theory of Hybrid Index and usind a read aligner for the kernel. "<< endl;
          cout << "Results should be checked..." << endl;
          cout << "*********" << endl;
          cout << "We just lost read aligned to Kernel Pos:" << locations[i].GetPos();
          cout << "*********" << endl;
        }
      }
    }
  }
  size_t tot_map_to_kernel = locations.size();
  size_t unmapped_to_kernel = unmapped->size();
  if (verbose) {
    cout << "Total occurrence mapped  to the kernel:" << tot_map_to_kernel << endl;
    cout << "Total occurrence NOT MAP to the kernel:" << unmapped_to_kernel << endl;
  }
  CreateSamRecordsForTrulyLostAlignments(&lost_occs, unmapped, ans, retrieve_all);
}

void HybridLZIndex::CreateSamRecordsForTrulyLostAlignments(vector<Occurrence> * lost_occs,
                                                           vector<Occurrence> * unmapped,
                                                           vector<Occurrence> * ans,
                                                           bool retrieve_all) const {
  std::ofstream ofs;
  ofs.open ("MisAlignedAndLost.fq");
  //  TODO: to improve time in case that...retrieve_all = true;
  int64_t truly_lost = 0;
  long double t1, t2;
  t1 = Utils::wclock();
  for (size_t i = 0; i < lost_occs->size(); i++) {
    string read_name = lost_occs->at(i).GetReadName();
    // if retrieve_all == false 
    // we can assume that ListContainsName => false
    if (!retrieve_all || !ListContainsName(ans, read_name)) {
      truly_lost++;
      ofs << lost_occs->at(i).GetMessage() << endl;
      unmapped->push_back(Utils::SamRecordForUnmapped(lost_occs->at(i).GetReadName()));
    } 
  }
  t2 = Utils::wclock();
  if (verbose) {
    cout << "Check for truly lost reads:: "<< (t2-t1) << " seconds. " << endl;
    cout << "Truly lost reads (aligned over a separator): " << truly_lost << endl;
    cout << "(CHIC created a SAM unmapped-record for them)" << endl;
  }
  ofs.close();

}

void HybridLZIndex::FindPrimaryOccs(vector<Occurrence> * ans, string query) const {
  vector<Occurrence> locations = kernel_manager->LocateOccs(query);
  // TODO: may be more efficient if we change the signature of this method, and we return locations.
  // we will need to do in-place filter instead of the push_backs that follows.

  size_t all_occs_in_kernel = locations.size();
    for (size_t i = 0; i < all_occs_in_kernel; i++) {

            ans->push_back(locations[i]);

    }
  /*if (all_occs_in_kernel > 0) {  // TODO: this if seems redundant too
    for (size_t i = 0; i < all_occs_in_kernel; i++) {
      uint64_t next_limit_pos = 0;
      uint prev_limit;
      uint64_t pos_in_text = MapKernelPosToTextPos(locations[i].GetPos(),
                                                   &next_limit_pos,
                                                   & prev_limit);
      if (next_limit_pos <= locations[i].GetPos() + query.size() - 1 ||
          tsrr->IsLiteral(prev_limit)) {
        locations[i].UpdatePos(pos_in_text);
        ans->push_back(locations[i]);
      }
    }
  }*/
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


uint64_t HybridLZIndex::MapKernelPosToTextPos(uint64_t pos,
                                              uint64_t * _next_limit_pos,
                                              uint * pred) const {
  uint succ = SuccessorInKernelLimits(pos, _next_limit_pos);
  ASSERT(succ > 0);
  *pred = succ-1;
  uint64_t offset = (*_next_limit_pos) - pos;
    //cout << "SUCCessor: "<< succ << endl;
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
