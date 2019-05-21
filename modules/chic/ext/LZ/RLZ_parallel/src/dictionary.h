#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <stdint.h>
#include <iostream>

#include "common.h"

#include <divsufsort.h>
#include <divsufsort64.h>


using std::cin;
using std::cout;
using std::endl;

template <typename sa_t>
struct Dictionary {
    Dictionary(char *path, size_t reference_len, size_t s) {
      sample_size = s;
      n = reference_len;
      assert(path);
      d = load_reference(path, n);
    }

    void BuildSA() {
      cout << "Building Dictionary..." << endl;
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
      cout << "Dictionary (ie SA) build in:  " << end_time - start_time << " (s)" << endl;
      cout << endl;
    }

    ~Dictionary() {
        if (sa) delete[] sa;
        delete[] d;
    }
    
    // char array
    Factor at(uint8_t *t, size_t tn) const {
        assert(t);
        assert(sa);
        size_t lhs = 0, rhs = n - 1, len = 0;

        while (1) {
            if (len >= tn) break;
            if (len >= sample_size) { len = sample_size; break; }

            size_t sp = lhs, ep = rhs, m = (sp + ep) / 2;

            while (sp < ep) {
                if (sa[m] + len < n && d[sa[m] + len] >= t[len]) ep = m;
                else sp = m + 1;
                m = (sp + ep) / 2;
            }

            if (sa[sp] + len >= n || d[sa[sp] + len] != t[len]) break;

            lhs = sp;
            ep = rhs;
            m = (sp + ep + 1) / 2;

            while (sp < ep) {
                if (sa[m] + len < n && d[sa[m] + len] <= t[len]) sp = m;
                else ep = m - 1;
                m = (sp + ep + 1) / 2;
            }

            rhs = ep;

            len++;
        }

        Factor factor = {sa[lhs], len};

        if (len == 0) factor.pos = t[0];

        return factor;
    }
    
    // stream
    Factor at(std::ifstream &fs, size_t tn) const {
        assert(fs.good());
        assert(sa);

        size_t lhs = 0, rhs = n - 1, len = 0;
        
        if(!fs.good()) {
          fprintf(stderr, "A PROBLEM OCCURRED WITH FS!\n");
          exit(EXIT_FAILURE);
        } 
        uint8_t curr_char =  fs.get();
        uint8_t first_curr_char = curr_char;
        while (1) {
          if (len >= tn) break;
          if (len >= sample_size) { len = sample_size; break; }

          size_t sp = lhs, ep = rhs, m = (sp + ep) / 2;


          while (sp < ep) {
            if (sa[m] + len < n && d[sa[m] + len] >= curr_char) ep = m;
                else sp = m + 1;
                m = (sp + ep) / 2;
            }

            if (sa[sp] + len >= n || d[sa[sp] + len] != curr_char) break;

            lhs = sp;
            ep = rhs;
            m = (sp + ep + 1) / 2;

            while (sp < ep) {
                if (sa[m] + len < n && d[sa[m] + len] <= curr_char) sp = m;
                else ep = m - 1;
                m = (sp + ep + 1) / 2;
            }

            rhs = ep;

            len++;
            curr_char =  fs.get();
        }

        Factor factor = {sa[lhs], len};

        if (len == 0) factor.pos = first_curr_char;
        
        if (!len == 0) fs.putback(curr_char);
        return factor;
    }

    size_t size_in_bytes() {
      return n*(1+sizeof(sa_t));
    }

    size_t sample_size;
    uint8_t *d;
    sa_t *sa;
    size_t n;
};

#endif /* DICTIONARY_H */
