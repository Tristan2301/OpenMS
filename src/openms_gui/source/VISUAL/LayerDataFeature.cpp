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
// $Maintainer: Chris Bielow $
// $Authors: Chris Bielow $
// --------------------------------------------------------------------------

#include "OpenMS/VISUAL/LayerDataPeak.h"

#include <OpenMS/ANALYSIS/ID/IDMapper.h>
#include <OpenMS/KERNEL/DimMapper.h>

#include <OpenMS/VISUAL/LayerDataFeature.h>
#include <OpenMS/VISUAL/Painter2DBase.h>
#include <OpenMS/VISUAL/VISITORS/LayerStatistics.h>                                                                                       
#include <OpenMS/VISUAL/VISITORS/LayerStoreData.h>

using namespace std;

namespace OpenMS
{
  /// Default constructor
  LayerDataFeature::LayerDataFeature() : LayerDataBase(LayerDataBase::DT_FEATURE)
  {
    flags.set(LayerDataBase::F_HULL);
  }

  std::unique_ptr<Painter2DBase> LayerDataFeature::getPainter2D() const
  {
    return make_unique<Painter2DFeature>(this);
  }

  std::unique_ptr<LayerStoreData> LayerDataFeature::storeVisibleData(const RangeAllType& visible_range, const DataFilters& layer_filters) const
  {
    auto ret = make_unique<LayerStoreDataFeatureMapVisible>();
    ret->storeVisibleFM(*features_.get(), visible_range, layer_filters);
    return ret;
  }

  std::unique_ptr<LayerStoreData> LayerDataFeature::storeFullData() const
  {
    auto ret = make_unique<LayerStoreDataFeatureMapAll>();
    ret->storeFullFM(*features_.get());
    return ret;
  }

  PeakIndex LayerDataFeature::findHighestDataPoint(const RangeAllType& area) const
  {
    using IntType = MSExperiment::ConstAreaIterator::PeakType::IntensityType;
    auto max_int = numeric_limits<IntType>::lowest();
    PeakIndex max_pi;
    for (FeatureMapType::ConstIterator i = getFeatureMap()->begin(); i != getFeatureMap()->end(); ++i)
    {
      if (area.containsRT(i->getRT()) && area.containsMZ(i->getMZ()) && filters.passes(*i))
      {
        if (i->getIntensity() > max_int)
        {
          max_int = i->getIntensity();
          max_pi = PeakIndex(i - getFeatureMap()->begin());
        }
      }
    }
    return max_pi;
  }

  PointXYType LayerDataFeature::peakIndexToXY(const PeakIndex& peak, const DimMapper<2>& mapper) const
  {
    return mapper.map(peak.getFeature(*getFeatureMap()));
  }

  std::unique_ptr<LayerStatistics> LayerDataFeature::getStats() const
  {
    return make_unique<LayerStatisticsFeatureMap>(*getFeatureMap());
  }

  bool LayerDataFeature::annotate(const vector<PeptideIdentification>& identifications, const vector<ProteinIdentification>& protein_identifications)
  {
    IDMapper mapper;
    mapper.annotate(*getFeatureMap(), identifications, protein_identifications);

    return true;
  }
}// namespace OpenMS
