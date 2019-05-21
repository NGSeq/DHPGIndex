#ifndef DICTIONARYFASTA_H
#define DICTIONARYFASTA_H

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <stdint.h>
#include <iostream>

#include "common.h"
#include "fastametadata.hpp"

#include <divsufsort.h>
#include <divsufsort64.h>


using std::cin;
using std::cout;
using std::endl;
using FMD::FastaMetaData;

template <typename sa_t>
struct DictionaryFa {
    DictionaryFa(char *path, size_t reference_len, size_t s) {
      sample_size = s;
      in_file_n = reference_len;
      assert(path);
      d = load_reference_fasta(path, reference_len, &n);
      cout << "In file n: " << in_file_n << endl;
      cout << "Seq     n: " << n << endl;
    }

    void BuildSA() {
      cout << "Building DictionaryFa..." << endl;
      long double start_time= wclock();
      fflush(stderr);
      sa = new sa_t[n];
      if (sizeof(sa_t) <= 4) {
        printf("Using 32 bits SA\n");
        assert(n < INT32_MAX/2);
        divsufsort(d, (int32_t*)sa, (int32_t)n);
      } else {
        printf("Using 64 bits SA\n");
        assert(sizeof(sa_t) == 8);
        divsufsort64(d, (int64_t*)sa, (int64_t)n);
      }
      long double end_time= wclock();
      cout << "DictionaryFa (ie SA) build in:  " << end_time - start_time << " (s)" << endl;
      cout << endl;
    }

    ~DictionaryFa() {
        if (sa) delete[] sa;
        delete[] d;
    }
    
    Factor at(uint8_t *t, size_t tn, size_t *njumps, FastaMetaData * metadata, size_t abs_pos) const {
        // cout << "Starting parse at char: " << t[0] << " = SEQ[" << abs_pos << "]" << endl;
        assert(t);
        assert(sa);
        size_t lhs = 0, rhs = n - 1, match_len = 0, intext_len = 0;
        *njumps = 0;
        while (1) {
            //cout << intext_len << ", " << match_len << " || " << tn << endl;
            if (intext_len >= tn) break;
            if (match_len >= sample_size) { match_len = sample_size; break; }
            if (is_forbidden_in_fasta(t[intext_len])) {
                // cout << "blank skipped " << endl;
                intext_len++;
                continue;
            }
            if (metadata->is_within_header(abs_pos + intext_len)) {
                cout << "[RLZ] on fasta. A header poss kipped: " << abs_pos + intext_len << endl;
                intext_len++;
                continue;
            }
    
            uint8_t next_char = t[intext_len];
            // cout << "Next char: " << next_char << endl;
            size_t sp = lhs, ep = rhs, m = (sp + ep) / 2;

            while (sp < ep) {
                if (sa[m] + match_len < n && d[sa[m] + match_len] >= next_char) ep = m;
                else sp = m + 1;
                m = (sp + ep) / 2;
            }

            if (sa[sp] + match_len >= n || d[sa[sp] + match_len] != next_char) break;

            lhs = sp;
            ep = rhs;
            m = (sp + ep + 1) / 2;

            while (sp < ep) {
                if (sa[m] + match_len < n && d[sa[m] + match_len] <= next_char) sp = m;
                else ep = m - 1;
                m = (sp + ep + 1) / 2;
            }

            rhs = ep;
            
            // cout << "P[" << intext_len<< " ] = " << next_char << endl;
            
            match_len++;
            intext_len++;
        }

        Factor factor = {sa[lhs], match_len};

        if (match_len == 0) factor.pos = t[0];
        //cout << "factor len:" << match_len << endl;
        assert(intext_len >= match_len);
        *njumps = (intext_len - match_len);
        return factor;
    }
    
    size_t size_in_bytes() {
      return n*(1+sizeof(sa_t));
    }

    size_t sample_size;
    uint8_t *d;
    sa_t *sa;
    size_t n;
    size_t in_file_n;
};

#endif /* DICTIONARY_H */
