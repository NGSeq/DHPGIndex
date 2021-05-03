CHIC aligner - A read aligner for repetitive references
=========


CHIC aliner is a read aligner that offers similar functionality
as standard read aligners such as BWA or BowTie2, being specially designed to handle
very large and repetitive references.


When to use CHIC ?
-----------

When your reference is very large, even Multi-Terabyte size of repetitive genomes, 
it may be a good idea to use CHIC aligner. Because we use of Lemepel-Ziv factorization
CHIC index are highly compressed. An example of interest is the case of pan-genomix indexing,
where there are many different variants of a genome.



Install and compile
-----------

The latest version can be downloaded from https://gitlab.com/dvalenzu/CHIC

Then you can compile the source by entering the command 
make
in the root folder.
CHIC relies on external tools, such as sdsl which in turn requires cmake to compile.
If the compilation fails due to some missing requirement run 
make clean; make;


Ussage
-----------

To index a file:

./chic_index [OPTIONS] INPUT_FILE MAX_QUERY_LEN

To query an index:

./chic_align[OPTIONS] index_basename patterns_filename



Legacy
-----------
CHIC aligner is based on CHICO: A compressed hybrid index for repetitive collections,
which is also included in the source code. 
