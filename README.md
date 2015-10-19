# Markflow
Markov chain based nonsense generator written in C

WARNING: Program is unfinished and very poorly tested. Can currently throw segmentation faults and does not
work for context more than 8.

Usage: markflow \<context length\> \<output length\> \<input filename\>

Program accepts a text file as a seed for generating random text. As a result of processing the given text file, the program generates a Markov model, which describes each symbol present in the file at least once and a set of symbols which can follow that symbol, with probabilites of following attached to them. After that, the program generates random text of given length based on generated Markov model
