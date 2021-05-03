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

#include "./SamReader.h"
#include <string>
#include <cstring>
#include <vector>
#include "./utils.h"

SamReader::SamReader() {
}

SamReader::SamReader(string _filename, size_t _chunk_size, int _n_threads, int _verbose) {
  this->chunk_size = _chunk_size;
  this->n_threads = _n_threads;
  this->verbose = _verbose;
  this->filename = _filename;
	is_ready = false;
  my_fstream.open(filename);
}

SamReader::~SamReader() {
  my_fstream.close();
  Utils::DeleteTmpFile(filename);
}
