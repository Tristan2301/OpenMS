// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2022.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Mathias Walzer $
// $Authors: $
// --------------------------------------------------------------------------
//
#pragma once

#include <OpenMS/COMPARISON/SPECTRA/BinnedSpectrumCompareFunctor.h>

namespace OpenMS
{

  /**
    @brief Compare functor scoring the spectral contrast angle for similarity measurement

    The details of the score can be found in:
    K. Wan, I. Vidavsky, and M. Gross. Comparing similar spectra: from
    similarity index to spectral contrast angle. Journal of the American Society
    for Mass Spectrometry, 13(1):85-88, January 2002.

    @htmlinclude OpenMS_BinnedSpectralContrastAngle.parameters

    @see BinnedSpectrumCompareFunctor
    @see BinnedSpectrum

    @ingroup SpectraComparison
  */
  class OPENMS_DLLAPI BinnedSpectralContrastAngle :
    public BinnedSpectrumCompareFunctor
  {

public:

    /// default constructor
    BinnedSpectralContrastAngle();

    /// copy constructor
    BinnedSpectralContrastAngle(const BinnedSpectralContrastAngle& source);

    /// destructor
    ~BinnedSpectralContrastAngle() override;

    /// assignment operator
    BinnedSpectralContrastAngle& operator=(const BinnedSpectralContrastAngle& source);

    /** function call operator, calculates the similarity of the given arguments

      @param spec1 First spectrum given in a binned representation
      @param spec2 Second spectrum given in a binned representation
      @throw IncompatibleBinning is thrown if the bins of the spectra are not the same
    */
    double operator()(const BinnedSpectrum& spec1, const BinnedSpectrum& spec2) const override;

    /// function call operator, calculates self similarity
    double operator()(const BinnedSpectrum& spec) const override;

    ///
    static BinnedSpectrumCompareFunctor* create() { return new BinnedSpectralContrastAngle(); }

    /// get the identifier for this DefaultParamHandler
    static const String getProductName()
    {
      return "BinnedSpectralContrastAngle";
    }

protected:
    void updateMembers_() override;
    double precursor_mass_tolerance_;
  };

}
