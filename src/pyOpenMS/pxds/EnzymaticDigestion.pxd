from Types cimport *
from DigestionEnzyme cimport *
from StringView cimport *
from libcpp cimport bool

cdef extern from "<OpenMS/CHEMISTRY/EnzymaticDigestion.h>" namespace "OpenMS":

    cdef cppclass EnzymaticDigestion "OpenMS::EnzymaticDigestion":
        # wrap-doc:
        #    Class for the enzymatic digestion of proteins
        #    
        #    Digestion can be performed using simple regular expressions, e.g. [KR] | [^P] for trypsin.
        #    Also missed cleavages can be modeled, i.e. adjacent peptides are not cleaved
        #    due to enzyme malfunction/access restrictions. If n missed cleavages are allowed, all possible resulting
        #    peptides (cleaved and uncleaved) with up to n missed cleavages are returned.
        #    Thus no random selection of just n specific missed cleavage sites is performed.

        EnzymaticDigestion() nogil except +

        EnzymaticDigestion(EnzymaticDigestion &) nogil except + # compiler

        Size getMissedCleavages() nogil except + # wrap-doc:Returns the max. number of allowed missed cleavages for the digestion

        void setMissedCleavages(Size missed_cleavages) nogil except + # wrap-doc:Sets the max. number of allowed missed cleavages for the digestion (default is 0). This setting is ignored when log model is used

        Size countInternalCleavageSites(String sequence) nogil except + # wrap-doc:Returns the number of internal cleavage sites for this sequence.

        String getEnzymeName() nogil except + # wrap-doc:Returns the enzyme for the digestion

        void setEnzyme(DigestionEnzyme* enzyme) nogil except + # wrap-doc:Sets the enzyme for the digestion

        Specificity getSpecificity() nogil except + # wrap-doc:Returns the specificity for the digestion

        void setSpecificity(Specificity spec) nogil except + # wrap-doc:Sets the specificity for the digestion (default is SPEC_FULL)

        Specificity getSpecificityByName(String name) nogil except + # wrap-doc:Returns the specificity by name. Returns SPEC_UNKNOWN if name is not valid

        Size digestUnmodified(StringView sequence,
                              libcpp_vector[ StringView ]& output,
                              Size min_length,
                              Size max_length) nogil except +
        # wrap-doc:
            #  Performs the enzymatic digestion of an unmodified sequence\n
            #  By returning only references into the original string this is very fast
            #  
            #  
            #  :param sequence: Sequence to digest
            #  :param output: Digestion products
            #  :param min_length: Minimal length of reported products
            #  :param max_length: Maximal length of reported products (0 = no restriction)
            #  :return: Number of discarded digestion products (which are not matching length restrictions)

        bool isValidProduct(String sequence,
                            int pos, int length,
                            bool ignore_missed_cleavages) nogil except + 
        # wrap-doc:
            #  Boolean operator returns true if the peptide fragment starting at position `pos` with length `length` within the sequence `sequence` generated by the current enzyme\n
            #  Checks if peptide is a valid digestion product of the enzyme, taking into account specificity and the MC flag provided here
            #  
            #  
            #  :param protein: Protein sequence
            #  :param pep_pos: Starting index of potential peptide
            #  :param pep_length: Length of potential peptide
            #  :param ignore_missed_cleavages: Do not compare MC's of potential peptide to the maximum allowed MC's
            #  :return: True if peptide has correct n/c terminals (according to enzyme, specificity and missed cleavages)

cdef extern from "<OpenMS/CHEMISTRY/EnzymaticDigestion.h>" namespace "OpenMS::EnzymaticDigestion":
    cdef enum Specificity "OpenMS::EnzymaticDigestion::Specificity":
        # wrap-attach:
        #   EnzymaticDigestion
        SPEC_NONE = 0, # wrap-doc:No requirements on start / end
        SPEC_SEMI = 1, # wrap-doc:Semi specific, i.e., one of the two cleavage sites must fulfill requirements
        SPEC_FULL = 2, # warp-doc:Fully enzyme specific, e.g., tryptic (ends with KR, AA-before is KR), or peptide is at protein terminal ends
        SPEC_UNKNOWN = 3
        SPEC_NOCTERM = 8, # wrap-doc:No requirements on CTerm (currently not supported in the class)
        SPEC_NONTERM = 9, # wrap-doc:No requirements on NTerm (currently not supported in the class)
        SIZE_OF_SPECIFICITY = 10
