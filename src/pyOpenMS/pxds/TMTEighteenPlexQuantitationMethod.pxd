from Types cimport *
from IsobaricQuantitationMethod cimport *

cdef extern from "<OpenMS/ANALYSIS/QUANTITATION/TMTEighteenPlexQuantitationMethod.h>" namespace "OpenMS":
    
    cdef cppclass TMTEighteenPlexQuantitationMethod(IsobaricQuantitationMethod) :
        # wrap-inherits:
        #  IsobaricQuantitationMethod
        TMTEighteenPlexQuantitationMethod() nogil except +
        TMTEighteenPlexQuantitationMethod(TMTEighteenPlexQuantitationMethod &) nogil except +
