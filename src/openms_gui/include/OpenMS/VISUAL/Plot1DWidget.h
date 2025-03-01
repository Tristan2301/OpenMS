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
// $Maintainer: Timo Sachsenberg$
// $Authors: Marc Sturm $
// --------------------------------------------------------------------------

#pragma once

// OpenMS_GUI config
#include <OpenMS/VISUAL/OpenMS_GUIConfig.h>

// STL
#include <vector>

// OpenMS
#include <OpenMS/VISUAL/PlotWidget.h>
#include <OpenMS/VISUAL/Plot1DCanvas.h>

class QAction;
class QSpacerItem;

namespace OpenMS
{
  class Plot1DCanvas;

  /**
      @brief Widget for visualization of several spectra

      The widget is composed of a scroll bar, an AxisWidget and a Plot1DCanvas as central widget.

      @image html Plot1DWidget.png

      The example image shows %Plot1DWidget displaying a raw data layer and a peak data layer.

      @ingroup PlotWidgets
  */
  class OPENMS_GUI_DLLAPI Plot1DWidget :
    public PlotWidget
  {
    Q_OBJECT

public:    
    /// Default constructor
    Plot1DWidget(const Param& preferences, const DIM gravity_axis = DIM::Y, QWidget* parent = nullptr);
    /// Destructor
    ~Plot1DWidget() override;

    // docu in base class
    Plot1DCanvas* canvas() const override
    {
      return static_cast<Plot1DCanvas*>(canvas_);
    }

    // Docu in base class
    void setMapper(const DimMapper<2>& mapper) override
    {
      canvas_->setMapper(mapper);
    }

    // Docu in base class
    void hideAxes() override;

    // Docu in base class
    void showLegend(bool show) override;

    /// Switches to mirror view, displays another y-axis for the second spectrum
    void toggleMirrorView(bool mirror);
    
    /// Performs an alignment of the layers with @p layer_index_1 and @p layer_index_2
    void performAlignment(Size layer_index_1, Size layer_index_2, const Param & param);

    /// Resets the alignment
    void resetAlignment();

    // Docu in base class
    void saveAsImage() override;

    // Docu in base class
    virtual void renderForImage(QPainter& painter);

signals:
    /// Requests to display the whole spectrum in 2D view
    void showCurrentPeaksAs2D();

    /// Requests to display the whole spectrum in 3D view
    void showCurrentPeaksAs3D();

    /// Requests to display the whole spectrum in ion mobility view
    void showCurrentPeaksAsIonMobility(const MSSpectrum& spec);

    /// Requests to display a full DIA window
    void showCurrentPeaksAsDIA(const Precursor& pc, const MSExperiment& exp);

public slots:
    // Docu in base class
    void showGoToDialog() override;

protected:
    // Docu in base class
    void recalculateAxes_() override;

    /// The second y-axis for the mirror view
    AxisWidget * flipped_y_axis_;

    /// Spacer between the two y-axes in mirror mode (needed when visualizing an alignment)
    QSpacerItem * spacer_;

  };
} // namespace OpenMS

