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
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#include <OpenMS/ANALYSIS/ID/BasicProteinInferenceAlgorithm.h>
#include <OpenMS/ANALYSIS/ID/BayesianProteinInferenceAlgorithm.h>
#include <OpenMS/ANALYSIS/ID/FalseDiscoveryRate.h>
#include <OpenMS/ANALYSIS/ID/IDBoostGraph.h>
#include <OpenMS/ANALYSIS/ID/IDConflictResolverAlgorithm.h>
#include <OpenMS/ANALYSIS/ID/IDScoreSwitcherAlgorithm.h>
#include <OpenMS/ANALYSIS/ID/PeptideIndexing.h>
#include <OpenMS/ANALYSIS/MAPMATCHING/ConsensusMapNormalizerAlgorithmMedian.h>
#include <OpenMS/ANALYSIS/ID/ConsensusMapMergerAlgorithm.h>
#include <OpenMS/ANALYSIS/MAPMATCHING/FeatureGroupingAlgorithmQT.h>
#include <OpenMS/ANALYSIS/MAPMATCHING/MapAlignmentAlgorithmIdentification.h>
#include <OpenMS/ANALYSIS/MAPMATCHING/MapAlignmentAlgorithmTreeGuided.h>
#include <OpenMS/ANALYSIS/MAPMATCHING/MapAlignmentTransformer.h>
#include <OpenMS/ANALYSIS/QUANTITATION/PeptideAndProteinQuant.h>
#include <OpenMS/APPLICATIONS/MapAlignerBase.h>
#include <OpenMS/APPLICATIONS/TOPPBase.h>
#include <OpenMS/DATASTRUCTURES/CalibrationData.h>
#include <OpenMS/FILTERING/CALIBRATION/InternalCalibration.h>
#include <OpenMS/FILTERING/CALIBRATION/MZTrafoModel.h>
#include <OpenMS/FILTERING/CALIBRATION/PrecursorCorrection.h>
#include <OpenMS/FILTERING/DATAREDUCTION/FeatureFindingMetabo.h>
#include <OpenMS/FILTERING/DATAREDUCTION/MassTraceDetection.h>
#include <OpenMS/FILTERING/ID/IDFilter.h>
#include <OpenMS/FILTERING/TRANSFORMERS/ThresholdMower.h>
#include <OpenMS/FORMAT/ConsensusXMLFile.h>
#include <OpenMS/FORMAT/ExperimentalDesignFile.h>
#include <OpenMS/FORMAT/FeatureXMLFile.h>
#include <OpenMS/FORMAT/IdXMLFile.h>
#include <OpenMS/FORMAT/MSstatsFile.h>
#include <OpenMS/FORMAT/MzMLFile.h>
#include <OpenMS/FORMAT/MzTabFile.h>
#include <OpenMS/FORMAT/PeakTypeEstimator.h>
#include <OpenMS/FORMAT/TransformationXMLFile.h>
#include <OpenMS/FORMAT/TriqlerFile.h>
#include <OpenMS/KERNEL/ConversionHelper.h>
#include <OpenMS/KERNEL/MSExperiment.h>
#include <OpenMS/KERNEL/MassTrace.h>
#include <OpenMS/MATH/STATISTICS/StatisticFunctions.h>
#include <OpenMS/METADATA/ExperimentalDesign.h>
#include <OpenMS/METADATA/SpectrumMetaDataLookup.h>
#include <OpenMS/SYSTEM/File.h>
#include <OpenMS/TRANSFORMATIONS/FEATUREFINDER/FeatureFinderIdentificationAlgorithm.h>
#include <OpenMS/TRANSFORMATIONS/FEATUREFINDER/FeatureFinderMultiplexAlgorithm.h>
#include <OpenMS/TRANSFORMATIONS/RAW2PEAK/PeakPickerHiRes.h>

using namespace OpenMS;
using namespace std;
using Internal::IDBoostGraph;

//-------------------------------------------------------------
//Doxygen docu
//-------------------------------------------------------------

/**
  @page UTILS_ProteomicsLFQ ProteomicsLFQ

  ProteomicsLFQ performs label-free quantification of peptides and proteins. @n

  Input: @n
         - Spectra in mzML format
         - Identifications in idXML or mzIdentML format with posterior error probabilities
           as score type.
           To generate those we suggest to run:
           1. PeptideIndexer to annotate target and decoy information.
           2. PSMFeatureExtractor to annotate percolator features.
           3. PercolatorAdapter tool (score_type = 'q-value', -post-processing-tdc)
           4. IDFilter (pep:score = 0.01) to filter PSMs at 1% FDR
           
         - An experimental design file: @n
           (see @ref OpenMS::ExperimentalDesign "ExperimentalDesign" for details) @n
         - A protein database in with appended decoy sequences in FASTA format @n
           (e.g., generated by the OpenMS DecoyDatabase tool) @n

  Processing: @n
         ProteomicsLFQ has different methods to extract features: ID-based (targeted only), or both ID-based and untargeted.
         1. The first method uses targeted feature dectection using RT and m/z information derived from identification data to extract features.
            Note: only identifications found in a particular MS run are used to extract features in the same run.
            No transfer of IDs (match between runs) is performed.
         2. The second method adds untargeted feature detection to obtain quantities from unidentified features.
            Transfer of Ids (match between runs) is performed by transfering feature identifications to coeluting, unidentified features with similar mass and RT in other runs.

         Requantification:
         2. Optionally, a requantification step is performed that tries to fill NA values.
            If a peptide has been quantified in more than half of all maps, the peptide is selected for requantification.
            In that case, the mean observed RT (and theoretical m/z) of the peptide is used to perform a second round of targeted extraction.
  Output:
         - mzTab file with analysis results
         - MSstats file with analysis results for statistical downstream analysis in MSstats
         - ConsensusXML file for visualization and further processing in OpenMS

  // experiments TODO:
  // - change percentage of missingness in ID transfer
  // - disable elution peak fit

  Potential scripts to perform the search can be found under src/tests/topp/ProteomicsLFQTestScripts
  
  <B>The command line parameters of this tool are:</B>
  @verbinclude UTILS_ProteomicsLFQ.cli
  <B>INI file documentation of this tool:</B>
  @htmlinclude UTILS_ProteomicsLFQ.html
 **/

// We do not want this class to show up in the docu:
/// @cond TOPPCLASSES

class ProteomicsLFQ :
  public TOPPBase
{
public:
  ProteomicsLFQ() :
    TOPPBase("ProteomicsLFQ", "A standard proteomics LFQ pipeline.", false)
  {
  }

protected:  
  void registerOptionsAndFlags_() override
  {
    registerInputFileList_("in", "<file list>", StringList(), "Input files");
    setValidFormats_("in", ListUtils::create<String>("mzML"));
    registerInputFileList_("ids", "<file list>", StringList(), 
      "Identifications filtered at PSM level (e.g., q-value < 0.01)."
      "And annotated with PEP as main score.\n"
      "We suggest using:\n"
      "1. PSMFeatureExtractor to annotate percolator features.\n"
      "2. PercolatorAdapter tool (score_type = 'q-value', -post-processing-tdc)\n"
      "3. IDFilter (pep:score = 0.05)\n"
      "To obtain well calibrated PEPs and an initial reduction of PSMs\n"
      "ID files must be provided in same order as spectra files.");
    setValidFormats_("ids", ListUtils::create<String>("idXML,mzId"));

    registerInputFile_("design", "<file>", "", "design file", false);
    setValidFormats_("design", ListUtils::create<String>("tsv"));

    registerInputFile_("fasta", "<file>", "", "fasta file", false);
    setValidFormats_("fasta", ListUtils::create<String>("fasta"));

    registerOutputFile_("out", "<file>", "", "output mzTab file");
    setValidFormats_("out", ListUtils::create<String>("mzTab"));

    registerOutputFile_("out_msstats", "<file>", "", "output MSstats input file", false, false);
    setValidFormats_("out_msstats", ListUtils::create<String>("csv"));

    registerOutputFile_("out_triqler", "<file>", "", "output Triqler input file", false, false);
    setValidFormats_("out_triqler", ListUtils::create<String>("tsv"));

    registerOutputFile_("out_cxml", "<file>", "", "output consensusXML file", false, false);
    setValidFormats_("out_cxml", ListUtils::create<String>("consensusXML"));

    registerDoubleOption_("proteinFDR", "<threshold>", 0.05, "Protein FDR threshold (0.05=5%).", false);
    setMinFloat_("proteinFDR", 0.0);
    setMaxFloat_("proteinFDR", 1.0);

    //TODO test rigorously
    registerStringOption_("picked_proteinFDR", "<choice>", "false", "Use a picked protein FDR?", false);
    setValidStrings_("picked_proteinFDR", {"true","false"});

    registerDoubleOption_("psmFDR", "<threshold>", 1.0, "FDR threshold for sub-protein level (e.g. 0.05=5%). Use -FDR_type to choose the level. Cutoff is applied at the highest level."
                          " If Bayesian inference was chosen, it is equivalent with a peptide FDR", false);
    setMinFloat_("psmFDR", 0.0);
    setMaxFloat_("psmFDR", 1.0);

    registerStringOption_("FDR_type", "<threshold>", "PSM", "Sub-protein FDR level. PSM, PSM+peptide (best PSM q-value).", false);
    setValidStrings_("FDR_type", {"PSM", "PSM+peptide"});

    //TODO expose all parameters of the inference algorithms (e.g. aggregation methods etc.)?
    registerStringOption_("protein_inference", "<option>", "aggregation",
      "Infer proteins:\n" 
      "aggregation  = aggregates all peptide scores across a protein (using the best score) \n"
      "bayesian     = computes a posterior probability for every protein based on a Bayesian network.\n"
      "               Note: 'bayesian' only uses and reports the best PSM per peptide.",
      false, true);
    setValidStrings_("protein_inference", ListUtils::create<String>("aggregation,bayesian"));

    registerStringOption_("protein_quantification", "<option>", "unique_peptides",
      "Quantify proteins based on:\n"
      "unique_peptides = use peptides mapping to single proteins or a group of indistinguishable proteins"
      "(according to the set of experimentally identified peptides).\n"
      "strictly_unique_peptides = use peptides mapping to a unique single protein only.\n"
      "shared_peptides = use shared peptides only for its best group (by inference score)", false, true);
    setValidStrings_("protein_quantification", ListUtils::create<String>("unique_peptides,strictly_unique_peptides,shared_peptides"));
    registerStringOption_("quantification_method", "<option>", 
      "feature_intensity", 
      "feature_intensity: MS1 signal.\n"
      "spectral_counting: PSM counts.", false, false);
    setValidStrings_("quantification_method", ListUtils::create<String>("feature_intensity,spectral_counting"));

    registerStringOption_("targeted_only", "<option>", "false", 
      "true: Only ID based quantification.\n"
      "false: include unidentified features so they can be linked to identified ones (=match between runs).", false, false);
    setValidStrings_("targeted_only", ListUtils::create<String>("true,false"));

    // TODO: support transfer with SVM if we figure out a computationally efficient way to do it.
    registerStringOption_("transfer_ids", "<option>", "false", 
      "Requantification using mean of aligned RTs of a peptide feature.\n"
      "Only applies to peptides that were quantified in more than 50% of all runs (of a fraction).", false, false);
    setValidStrings_("transfer_ids", ListUtils::create<String>("false,mean"));

    registerStringOption_("mass_recalibration", "<option>", "false", "Mass recalibration.", false, true);
    setValidStrings_("mass_recalibration", ListUtils::create<String>("true,false"));

    registerStringOption_("alignment_order", "<option>", "star", "If star, aligns all maps to the reference with most IDs,"
                                                                 "if treeguided, calculates a guiding tree first.", false, true);
    setValidStrings_("alignment_order", ListUtils::create<String>("star,treeguided"));

    registerStringOption_("keep_feature_top_psm_only", "<option>", "true", "If false, also keeps lower ranked PSMs that have the top-scoring"
                                                                     " sequence as a candidate per feature in the same file.", false, true);
    setValidStrings_("keep_feature_top_psm_only", ListUtils::create<String>("true,false"));

    registerTOPPSubsection_("Seeding", "Parameters for seeding of untargeted features");
    registerDoubleOption_("Seeding:intThreshold", "<threshold>", 1e4, "Peak intensity threshold applied in seed detection.", false, true);
    registerStringOption_("Seeding:charge", "<minChg:maxChg>", "2:5", "Charge range considered for untargeted feature seeds.", false, true); //TODO infer from IDs?
    registerDoubleOption_("Seeding:traceRTTolerance", "<tolerance(sec)>", 3.0, "Combines all spectra in the tolerance window to stabilize identification of isotope patterns. Controls sensitivity (low value) vs. specificity (high value) of feature seeds.", false, true); //TODO infer from average MS1 cycle time?

    /// TODO: think about export of quality control files (qcML?)

    Param pp_defaults = PeakPickerHiRes().getDefaults();
    for (const auto& s : {"report_FWHM", "report_FWHM_unit", "SignalToNoise:win_len", "SignalToNoise:bin_count", "SignalToNoise:min_required_elements", "SignalToNoise:write_log_messages"} )
    {
      pp_defaults.addTag(s, "advanced");
    }

    Param ffi_defaults = FeatureFinderIdentificationAlgorithm().getDefaults();
    ffi_defaults.setValue("svm:samples", 10000); // restrict number of samples for training
    ffi_defaults.setValue("svm:log2_C", DoubleList({-2.0, 5.0, 15.0})); 
    ffi_defaults.setValue("svm:log2_gamma", DoubleList({-3.0, -1.0, 2.0})); 
    ffi_defaults.setValue("svm:min_prob", 0.9); // keep only feature candidates with > 0.9 probability of correctness

    // hide entries
    for (const auto& s : {"svm:samples", "svm:log2_C", "svm:log2_gamma", "svm:min_prob", "svm:no_selection", "svm:xval_out", "svm:kernel", "svm:xval", "candidates_out", "extract:n_isotopes", "model:type"} )
    {
      ffi_defaults.addTag(s, "advanced");
    }
    ffi_defaults.remove("detect:peak_width"); // set from data

    Param ma_defaults = MapAlignmentAlgorithmTreeGuided().getDefaults();

    ma_defaults.setValue("align_algorithm:max_rt_shift", 0.1);
    ma_defaults.setValue("align_algorithm:use_unassigned_peptides", "false");
    ma_defaults.setValue("align_algorithm:use_feature_rt", "true");

    // hide entries
    for (const auto& s : {"align_algorithm:use_unassigned_peptides", "align_algorithm:use_feature_rt",
                          "align_algorithm:score_cutoff", "align_algorithm:min_score"} )
    {
      ma_defaults.addTag(s, "advanced");
    }

    //Param fl_defaults = FeatureGroupingAlgorithmKD().getDefaults();
    Param fl_defaults = FeatureGroupingAlgorithmQT().getDefaults();
    fl_defaults.setValue("distance_MZ:max_difference", 10.0);
    fl_defaults.setValue("distance_MZ:unit", "ppm");
    fl_defaults.setValue("distance_MZ:weight", 5.0);
    fl_defaults.setValue("distance_intensity:weight", 0.1); 
    fl_defaults.setValue("use_identifications", "true"); 
    fl_defaults.remove("distance_RT:max_difference"); // estimated from data
    for (const auto& s : {"distance_MZ:weight", "distance_intensity:weight", "use_identifications", "ignore_charge", "ignore_adduct"} )
    {
      fl_defaults.addTag(s, "advanced");
    }

    Param pq_defaults = PeptideAndProteinQuant().getDefaults();
    // overwrite algorithm default, so we export everything (important for copying back MSstats results)
    pq_defaults.setValue("top:include_all", "true");
    pq_defaults.addTag("top:include_all", "advanced");

    // combine parameters of the individual algorithms
    Param combined;
    combined.insert("Centroiding:", pp_defaults);
    combined.insert("PeptideQuantification:", ffi_defaults);
    combined.insert("Alignment:", ma_defaults);
    combined.insert("Linking:", fl_defaults);
    combined.insert("ProteinQuantification:", pq_defaults);

    registerFullParam_(combined);
  }

  // Map between mzML file and corresponding id file
  // Warn if the primaryMSRun indicates that files were provided in the wrong order.
  map<String, String> mapMzML2Ids_(StringList & in, StringList & in_ids)
  {
    // Detect the common case that ID files have same names as spectra files
    if (!File::validateMatchingFileNames(in, in_ids, true, true, false)) // only basenames, without extension, only order
    {
      // Spectra and id files have the same set of basenames but appear in different order. -> this is most likely an error
      throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
        "ID and spectra file match but order of file names seem to differ. They need to be provided in the same order.");
    }

    map<String, String> mzfile2idfile;
    for (Size i = 0; i != in.size(); ++i)
    {
      const String& in_abs_path = File::absolutePath(in[i]);
      const String& id_abs_path = File::absolutePath(in_ids[i]);
      mzfile2idfile[in_abs_path] = id_abs_path;      
      writeDebug_("Spectra: " + in[i] + "\t Ids: " + in_ids[i],  1);
    }
    return mzfile2idfile;
  }

  // map back
  map<String, String> mapId2MzMLs_(const map<String, String>& m2i)
  {
    map<String, String> idfile2mzfile;
    for (const auto& m : m2i)
    {
      idfile2mzfile[m.second] = m.first;
    }
    return idfile2mzfile;
  }

  ExitCodes centroidAndCorrectPrecursors_(const String & mz_file, MSExperiment & ms_centroided)
  { 
    Param pp_param = getParam_().copy("Centroiding:", true);
    writeDebug_("Parameters passed to PeakPickerHiRes algorithm", pp_param, 3);

    // create scope for raw data, so it is properly freed (Note: clear() is not sufficient)
    // load raw file
    MzMLFile mzML_file;
    mzML_file.setLogType(log_type_);

    PeakMap ms_raw;
    mzML_file.load(mz_file, ms_raw);
    ms_raw.clearMetaDataArrays();
    ms_raw.updateRanges();

    if (ms_raw.empty())
    {
      OPENMS_LOG_WARN << "The given file does not contain any spectra.";
      return INCOMPATIBLE_INPUT_DATA;
    }

    // remove MS2 peak data and check if spectra are sorted
    //TODO can we load just MS1 or do we need precursor information?
    for (auto & spec : ms_raw)
    {
      if (spec.getMSLevel() == 2)
      {
        spec.clear(false);  // delete MS2 peaks
      }
      if (!spec.isSorted())
      {
        spec.sortByPosition();
        writeLogInfo_("Info: Sorted peaks by m/z.");
      }
    }

    //-------------------------------------------------------------
    // Centroiding of MS1
    //-------------------------------------------------------------
    PeakPickerHiRes pp;
    pp.setLogType(log_type_);
    pp.setParameters(pp_param);
    pp.pickExperiment(ms_raw, ms_centroided, true);
      
    //-------------------------------------------------------------
    // HighRes Precursor Mass Correction
    //-------------------------------------------------------------
    std::vector<double> deltaMZs, mzs, rts;
    std::set<Size> corrected_to_highest_intensity_peak = PrecursorCorrection::correctToHighestIntensityMS1Peak(
      ms_centroided, 
      0.01, // check if we can estimate this from data (here it is given in m/z not ppm)
      false, // is ppm = false
      deltaMZs, 
      mzs, 
      rts
      );      
    writeLogInfo_("Info: Corrected " + String(corrected_to_highest_intensity_peak.size()) + " precursors.");
    if (!deltaMZs.empty())
    {
      vector<double> deltaMZs_ppm, deltaMZs_ppmabs;
      for (Size i = 0; i != deltaMZs.size(); ++i)
      {
        deltaMZs_ppm.push_back(Math::getPPM(mzs[i], mzs[i] + deltaMZs[i]));
        deltaMZs_ppmabs.push_back(Math::getPPMAbs(mzs[i], mzs[i] + deltaMZs[i]));
      }

      double median = Math::median(deltaMZs_ppm.begin(), deltaMZs_ppm.end());
      double MAD =  Math::MAD(deltaMZs_ppm.begin(), deltaMZs_ppm.end(), median);
      double median_abs = Math::median(deltaMZs_ppmabs.begin(), deltaMZs_ppmabs.end());
      double MAD_abs = Math::MAD(deltaMZs_ppmabs.begin(), deltaMZs_ppmabs.end(), median_abs);
      writeLogInfo_("Precursor correction:\n  median        = "
        + String(median) + " ppm  MAD = " + String(MAD)
        + "\n  median (abs.) = " + String(median_abs) 
        + " ppm  MAD = " + String(MAD_abs));
    }
    return EXECUTION_OK;
  }

  void recalibrateMasses_(MSExperiment & ms_centroided, vector<PeptideIdentification>& peptide_ids, const String & id_file_abs_path)
  {
    InternalCalibration ic;
    ic.setLogType(log_type_);
    ic.fillCalibrants(peptide_ids, 25.0); // >25 ppm maximum deviation defines an outlier TODO: check if we need to adapt this
    if (ic.getCalibrationPoints().size() <= 1) return;

    // choose calibration model based on number of calibration points

    // there seem to be some problems with the QUADRATIC model that we first need to investigate
    //MZTrafoModel::MODELTYPE md = (ic.getCalibrationPoints().size() == 2) ? MZTrafoModel::LINEAR : MZTrafoModel::QUADRATIC;
    //bool use_RANSAC = (md == MZTrafoModel::LINEAR || md == MZTrafoModel::QUADRATIC);

    MZTrafoModel::MODELTYPE md = MZTrafoModel::LINEAR;
    bool use_RANSAC = true;

    Size RANSAC_initial_points = (md == MZTrafoModel::LINEAR) ? 2 : 3;
    Math::RANSACParam p(RANSAC_initial_points, 70, 10, 30, true); // TODO: check defaults (taken from tool)
    MZTrafoModel::setRANSACParams(p);
    // these limits are a little loose, but should prevent grossly wrong models without burdening the user with yet another parameter.
    MZTrafoModel::setCoefficientLimits(25.0, 25.0, 0.5); 

    IntList ms_level = {1};
    double rt_chunk = 300.0; // 5 minutes
    String qc_residual_path, qc_residual_png_path;
    if (debug_level_ >= 1)
    {
      const String & id_basename = File::basename(id_file_abs_path);
      qc_residual_path = id_basename + "qc_residuals.tsv";
      qc_residual_png_path = id_basename + "qc_residuals.png";
    } 

    if (!ic.calibrate(ms_centroided, ms_level, md, rt_chunk, use_RANSAC, 
                  10.0,
                  5.0, 
                  "",                      
                  "",
                  qc_residual_path,
                  qc_residual_png_path,
                  "Rscript"))
    {
      OPENMS_LOG_WARN << "\nCalibration failed. See error message above!" << std::endl;
    }
  }

  double estimateMedianChromatographicFWHM_(MSExperiment & ms_centroided)
  {
    MassTraceDetection mt_ext;
    Param mtd_param = mt_ext.getParameters();
    writeDebug_("Parameters passed to MassTraceDetection", mtd_param, 3);

    std::vector<MassTrace> m_traces;
    mt_ext.run(ms_centroided, m_traces, 1000);

    std::vector<double> fwhm_1000;
    for (auto &m : m_traces)
    {
      if (m.getSize() == 0) continue;
      m.updateMeanMZ();
      m.updateWeightedMZsd();
      double fwhm = m.estimateFWHM(false);
      fwhm_1000.push_back(fwhm);
    }

    double median_fwhm = Math::median(fwhm_1000.begin(), fwhm_1000.end());

    OPENMS_LOG_INFO << "Median chromatographic FWHM: " << median_fwhm << std::endl;

    return median_fwhm;
  }

  void calculateSeeds_(const MSExperiment & ms_centroided, FeatureMap & seeds, double median_fwhm)
  {
    //TODO: Actually FFM provides a parameter for minimum intensity. Also it copies the full experiment again once or twice.
    MSExperiment e;
    for (const auto& s : ms_centroided)
    { 
      if (s.getMSLevel() == 1) 
      {              
        e.addSpectrum(s);
      }
    }

    ThresholdMower threshold_mower_filter;
    Param tm = threshold_mower_filter.getParameters();
    tm.setValue("threshold", getDoubleOption_("Seeding:intThreshold"));  // TODO: derive from data
    threshold_mower_filter.setParameters(tm);
    threshold_mower_filter.filterPeakMap(e);

    FeatureFinderMultiplexAlgorithm algorithm;
    Param p = algorithm.getParameters();
    p.setValue("algorithm:labels", ""); // unlabeled only
    p.setValue("algorithm:charge", getStringOption_("Seeding:charge")); //TODO infer from IDs?
    p.setValue("algorithm:rt_typical", median_fwhm * 3.0);
    p.setValue("algorithm:rt_band", getDoubleOption_("Seeding:traceRTTolerance")); // max 3 seconds shifts between isotopic traces
    p.setValue("algorithm:rt_min", median_fwhm * 0.5);
    p.setValue("algorithm:spectrum_type", "centroid");
    algorithm.setParameters(p);
    //FIXME progress of FFM is not printed at all
    const bool progress(true);
    algorithm.run(e, progress);
    seeds = algorithm.getFeatureMap(); 
    OPENMS_LOG_INFO << "Using " << seeds.size() << " seeds from untargeted feature extraction." << endl;
  }


  // aligns the feature maps 
  double align_(
    vector<FeatureMap> & feature_maps, 
    vector<TransformationDescription>& transformations
  )
  {
    if (feature_maps.size() > 1) // do we have several maps to align / link?
    {
      Param mat_param = getParam_().copy("Alignment:", true);
      writeDebug_("Parameters passed to MapAlignmentAlgorithms", mat_param, 3);

      Param model_params = MapAlignerBase::getModelDefaults("b_spline");
      String model_type = model_params.getValue("type").toString();
      model_params = model_params.copy(model_type + ":", true);

      try
      {
        if (getStringOption_("alignment_order") == "star")
        {
          // Determine reference from data, otherwise a change in order of input files
          // leads to slightly different results
          const int reference_index(-1); // set no reference (determine from data)
          Param ma_param = mat_param.copy("align_algorithm:", true);
          writeDebug_("Parameters passed to MapAlignerIdentification", ma_param, 3);
          MapAlignmentAlgorithmIdentification aligner;
          aligner.setLogType(log_type_);
          aligner.setParameters(ma_param);
          aligner.align(feature_maps, transformations, reference_index);
        }
        else //tree-guided
        {
          MapAlignmentAlgorithmTreeGuided aligner;
          aligner.setLogType(log_type_);
          aligner.setParameters(mat_param);
          aligner.align(feature_maps, transformations);
        }
      }
      catch (Exception::MissingInformation& err)
      {
        if (getFlag_("force"))
        {
          OPENMS_LOG_ERROR
            << "Error: alignment failed. Details:\n" << err.what()
            << "\nProcessing will continue using 'identity' transformations."
            << endl;
          model_type = "identity";
          transformations.resize(feature_maps.size());
        }
        else throw;
      }

      // find model parameters (if model_type == "identity" the fit is a NOP):
      vector<TransformationDescription::TransformationStatistics> alignment_stats;
      for (TransformationDescription & t : transformations)
      {
        writeDebug_("Using " + String() + " points in fit.", 1); 
        if (t.getDataPoints().size() > 10)
        {
          t.fitModel(model_type, model_params);
        }
        t.printSummary(OpenMS_Log_debug);
        alignment_stats.emplace_back(t.getStatistics());
      }

      // determine maximum RT shift after transformation that includes all high confidence IDs 
      using TrafoStat = TransformationDescription::TransformationStatistics;
      for (auto & s : alignment_stats)
      {
        OPENMS_LOG_INFO << "Alignment differences (second) for percentiles (before & after): " << endl;
        OPENMS_LOG_INFO << ListUtils::concatenate(s.percents,"%\t") << "%" << endl;
        OPENMS_LOG_INFO << "before alignment:" << endl;
        for (const auto& p : s.percents)
        {
          OPENMS_LOG_INFO << (int)s.percentiles_before[p] << "\t";
        }
        OPENMS_LOG_INFO << endl;

        OPENMS_LOG_INFO << "after alignment:" << endl;
        for (const auto& p : s.percents)
        {
          OPENMS_LOG_INFO << (int)s.percentiles_after[p] << "\t";
        }
        OPENMS_LOG_INFO << endl;
      }

      double max_alignment_diff = std::max_element(alignment_stats.begin(), alignment_stats.end(),
              [](TrafoStat a, TrafoStat b) 
              { return a.percentiles_after[100] < b.percentiles_after[100]; })->percentiles_after[100];
      // sometimes, very good alignments might lead to bad overall performance. Choose 2 minutes as minimum.
      OPENMS_LOG_INFO << "Max alignment difference (seconds): " << max_alignment_diff << endl;
      max_alignment_diff = std::max(max_alignment_diff, 120.0); // minimum 2 minutes
      max_alignment_diff = std::min(max_alignment_diff, 600.0); // maximum 10 minutes
      return max_alignment_diff;
    }
    return 0;
  }

  void transform_(
    vector<FeatureMap>& feature_maps, 
    vector<TransformationDescription>& transformations
  )
  {
    if (feature_maps.size() > 1 && !transformations.empty())
    {
      // Apply transformations
      for (Size i = 0; i < feature_maps.size(); ++i)
      {
        try 
        {
          MapAlignmentTransformer::transformRetentionTimes(feature_maps[i],
            transformations[i]);
        } catch (Exception::IllegalArgument& e)
        {
          OPENMS_LOG_WARN << e.what() << endl;
        }
          
        if (debug_level_ > 666)
        {
          // plot with e.g.:
          // Rscript ../share/OpenMS/SCRIPTS/plot_trafo.R debug_trafo_1.trafoXML debug_trafo_1.pdf
          TransformationXMLFile().store("debug_trafo_" + String(i) + ".trafoXML", transformations[i]);
        }
      }
    }
  }

  //-------------------------------------------------------------
  // Link all features of this fraction
  //-------------------------------------------------------------
  /// this method will only be used during requantification.
  void link_(
    vector<FeatureMap> & feature_maps, 
    double median_fwhm,
    double max_alignment_diff,
    ConsensusMap & consensus_fraction
  )
  {
    //since requantification only happens with 2+ maps, we do not need to check/skip,
    //in case of a singleton fraction. Would throw an exception in linker.group

    Param fl_param = getParam_().copy("Linking:", true);
    writeDebug_("Parameters passed to feature grouping algorithm", fl_param, 3);

    writeDebug_("Linking: " + String(feature_maps.size()) + " features.", 1);

    // grouping tolerance = max alignment error + median FWHM
    FeatureGroupingAlgorithmQT linker;
    fl_param.setValue("distance_RT:max_difference", 2.0 * max_alignment_diff + 2.0 * median_fwhm);
    linker.setParameters(fl_param);      
/*
    FeatureGroupingAlgorithmKD linker;
    fl_param.setValue("warp:rt_tol", 2.0 * max_alignment_diff + 2.0 * median_fwhm);
    fl_param.setValue("link:rt_tol", 2.0 * max_alignment_diff + 2.0 * median_fwhm);
    fl_param.setValue("link:mz_tol", 10.0);
    fl_param.setValue("mz_unit", "ppm");
    linker.setParameters(fl_param);      
*/
    linker.group(feature_maps, consensus_fraction);
    OPENMS_LOG_INFO << "Size of consensus fraction: " << consensus_fraction.size() << endl;
    assert(!consensus_fraction.empty());
  }

  /// Align and link.
  /// @return maximum alignment difference observed (to guide linking)
  double alignAndLink_(
    vector<FeatureMap> & feature_maps, 
    ConsensusMap & consensus_fraction,
    vector<TransformationDescription>& transformations,
    const double median_fwhm)
  {
    double max_alignment_diff(0.0);

    if (feature_maps.size() > 1)
    {
      max_alignment_diff = align_(feature_maps, transformations);

      transform_(feature_maps, transformations);

      link_(feature_maps,
        median_fwhm, 
        max_alignment_diff, 
        consensus_fraction);
    }
    else // only one feature map
    {
      MapConversion::convert(0, feature_maps.back(), consensus_fraction);                           
    }

    return max_alignment_diff;
  }

  /// determine co-occurrence of peptide in different runs
  /// returns map sequence+charge -> map index in consensus map
  map<pair<String, UInt>, vector<int> > getPeptideOccurrence_(const ConsensusMap &cons)
  {
    map<Size, UInt> num_consfeat_of_size;
    map<Size, UInt> num_consfeat_of_size_with_id;

    map<pair<String, UInt>, vector<int> > seq_charge2map_occurence;
    for (const ConsensusFeature& cfeature : cons)
    {
      ++num_consfeat_of_size[cfeature.size()];
      const auto& pids = cfeature.getPeptideIdentifications();
      if (!pids.empty())
      {
        ++num_consfeat_of_size_with_id[cfeature.size()];

        // count how often a peptide/charge pair has been observed in the different maps
        const vector<PeptideHit>& phits = pids[0].getHits();
        if (!phits.empty())
        {
          const String s = phits[0].getSequence().toString();
          const int z = phits[0].getCharge();

          if (seq_charge2map_occurence[make_pair(s,z)].empty())
          {
            seq_charge2map_occurence[make_pair(s,z)] = vector<int>(cons.getColumnHeaders().size(), 0);
          }

          // assign id to all dimensions in the consensus feature
          for (auto const & f : cfeature.getFeatures())
          {
            Size map_index = f.getMapIndex();
            seq_charge2map_occurence[make_pair(s,z)][map_index] += 1;
          }
        }
      }
    }
    return seq_charge2map_occurence;
  }

  /// simple transfer between runs
  /// if a peptide has not been quantified in more than min_occurrence runs, then take all consensus features that have it identified at least once
  /// and transfer the ID with RT of the consensus feature (the average if we have multiple consensus elements)
  multimap<Size, PeptideIdentification> transferIDsBetweenSameFraction_(const ConsensusMap& consensus_fraction, Size min_occurrence = 3)
  {
    // determine occurrence of ids
    map<pair<String, UInt>, vector<int> > occurrence = getPeptideOccurrence_(consensus_fraction);

    // build map of missing ids
    map<pair<String, UInt>, set<int> > missing; // set of maps missing the id
    for (auto & o : occurrence)
    {
      // more than min_occurrence elements in consensus map that are non-zero?
      const Size count_non_zero = (Size) std::count_if(o.second.begin(), o.second.end(), [](int i){return i > 0;});

      if (count_non_zero >= min_occurrence
       && count_non_zero < o.second.size())
      {
        for (Size i = 0; i != o.second.size(); ++i)
        {
          // missing ID for this consensus element
          if (o.second[i] == 0) { missing[o.first].insert(i); }
        }
      }
    }

    Size n_transferred_ids(0);
    // create representative id to transfer to missing
    multimap<Size, PeptideIdentification> transfer_ids;
    for (auto & c : consensus_fraction)
    {
      const auto& pids = c.getPeptideIdentifications();
      if (pids.empty()) continue; // skip consensus feature without IDs 

      const vector<PeptideHit>& phits = pids[0].getHits();
      if (phits.empty()) continue; // skip no PSM annotated

      const String s = phits[0].getSequence().toString();
      const int z = phits[0].getCharge();
      pair<String, UInt> seq_z = make_pair(s, z);
      map<pair<String, UInt>, set<int> >::const_iterator it = missing.find(seq_z);

      if (it == missing.end()) continue; // skip sequence and charge not marked as missing in one of the other maps

      for (int idx : it->second)
      {
        // use consensus feature ID and retention time to transfer between runs
        pair<Size, PeptideIdentification> p = make_pair(idx, pids[0]);
        p.second.setRT(c.getRT());
        transfer_ids.insert(p);
        ++n_transferred_ids;
      }
    }
    OPENMS_LOG_INFO << "Transferred IDs: " << n_transferred_ids << endl;
    return transfer_ids;
  }

  ExitCodes checkSingleRunPerID_(const vector<ProteinIdentification>& protein_ids, const String& id_file_abs_path)
  {
    if (protein_ids.size() != 1)
    {
      OPENMS_LOG_FATAL_ERROR << "Exactly one protein identification run must be annotated in " << id_file_abs_path << endl;
      return ExitCodes::INCOMPATIBLE_INPUT_DATA;
    }

    StringList run_paths;
    protein_ids[0].getPrimaryMSRunPath(run_paths);
    if (run_paths.size() > 1)
    {
      OPENMS_LOG_FATAL_ERROR << "ProteomicsLFQ does not support merged ID runs. ID file: " << id_file_abs_path << endl;
      return ExitCodes::INCOMPATIBLE_INPUT_DATA;
    }
    if (run_paths.empty())
    {
      OPENMS_LOG_WARN << "Warning: No mzML origin annotated in ID file. This can lead to errors or unexpected behaviour later: " << id_file_abs_path << endl;
    }
    
    return EXECUTION_OK;
  }

  ExitCodes switchScoreType_(vector<PeptideIdentification>& peptide_ids, const String& id_file_abs_path)
  {
    // Check if score types are valid. TODO
    try
    {
      IDScoreSwitcherAlgorithm switcher;
      Size c = 0;
      switcher.switchToGeneralScoreType(peptide_ids, IDScoreSwitcherAlgorithm::ScoreType::PEP, c);
    }
    catch(Exception::MissingInformation&)
    {
      OPENMS_LOG_FATAL_ERROR << "ProteomicsLFQ expects a Posterior Error Probability score in all Peptide IDs. ID file: " << id_file_abs_path << endl;
      return ExitCodes::INCOMPATIBLE_INPUT_DATA;
    }
    return EXECUTION_OK;
  }

  ExitCodes loadAndCleanupIDFile_(
    const String& id_file_abs_path,
    const String& mz_file,
    const String& in_db,
    const Size& fraction_group,
    const Size& fraction,
    vector<ProteinIdentification>& protein_ids, 
    vector<PeptideIdentification>& peptide_ids,
    set<String>& fixed_modifications,  // adds to
    set<String>& variable_modifications) // adds to
  {

    const String& mz_file_abs_path = File::absolutePath(mz_file);
    IdXMLFile().load(id_file_abs_path, protein_ids, peptide_ids);

    ExitCodes e = checkSingleRunPerID_(protein_ids, id_file_abs_path);
    if (e != EXECUTION_OK) return e;

    // Re-index
    if (!in_db.empty())
    {
      PeptideIndexing indexer;
      Param param_pi = indexer.getParameters();
      param_pi.setValue("missing_decoy_action", "silent");
      param_pi.setValue("write_protein_sequence", "true");
      param_pi.setValue("write_protein_description", "true");
      indexer.setParameters(param_pi);

      // stream data in fasta file
      FASTAContainer<TFI_File> fasta_db(in_db);
      PeptideIndexing::ExitCodes indexer_exit = indexer.run(fasta_db, protein_ids, peptide_ids);

      picked_decoy_string_ = indexer.getDecoyString();
      picked_decoy_prefix_ = indexer.isPrefix();
      if ((indexer_exit != PeptideIndexing::EXECUTION_OK) &&
          (indexer_exit != PeptideIndexing::PEPTIDE_IDS_EMPTY))
      {
        if (indexer_exit == PeptideIndexing::DATABASE_EMPTY)
        {
          return INPUT_FILE_EMPTY;
        }
        else if (indexer_exit == PeptideIndexing::UNEXPECTED_RESULT)
        {
          return UNEXPECTED_RESULT;
        }
        else
        {
          return UNKNOWN_ERROR;
        }
      }
    }

    e = switchScoreType_(peptide_ids, id_file_abs_path);
    if (e != EXECUTION_OK) return e;
  
    // TODO we could think about removing this limitation but it gets complicated quickly
    IDFilter::keepBestPeptideHits(peptide_ids, false); // strict = false
 
    // add to the (global) set of fixed and variable modifications
    const vector<String>& var_mods = protein_ids[0].getSearchParameters().variable_modifications;
    const vector<String>& fixed_mods = protein_ids[0].getSearchParameters().fixed_modifications;
    std::copy(var_mods.begin(), var_mods.end(), std::inserter(variable_modifications, variable_modifications.begin())); 
    std::copy(fixed_mods.begin(), fixed_mods.end(), std::inserter(fixed_modifications, fixed_modifications.end())); 

    // delete meta info to free some space
    for (PeptideIdentification & pid : peptide_ids)
    {
      // we currently can't clear the PeptideIdentification meta data
      // because the spectrum_reference is stored in the meta value (which it probably shouldn't)
      // TODO: pid.clearMetaInfo(); if we move it to the PeptideIdentification structure
      for (PeptideHit & ph : pid.getHits())
      {
        // TODO: we only have super inefficient meta value removal
        vector<String> keys;
        ph.getKeys(keys);
        for (const auto& k : keys)
        {
          if (!(k.hasSubstring("_score") 
            || k.hasSubstring("q-value") 
            || k.hasPrefix("Luciphor_global_flr")
            || k == "target_decoy") // keep target_decoy information for QC
            )            
          {
            ph.removeMetaValue(k);
          }
        }
        // we only clear selected metavalues
        //ph.clearMetaInfo();
      }
    }

    ///////////////////////////////////////////////////////
    // annotate experimental design
    // check and reannotate mzML file in ID
    StringList id_msfile_ref;
    protein_ids[0].getPrimaryMSRunPath(id_msfile_ref);

    // fix other problems like missing MS run path annotations
    if (id_msfile_ref.empty())
    {
      OPENMS_LOG_WARN  << "MS run path not set in ID file: " << id_file_abs_path << endl
                       << "Resetting reference to MS file provided at same input position." << endl;
    }
    else if (id_msfile_ref.size() == 1)
    {
      // Check if the annotated primary MS run filename matches the mzML filename (comparison by base name)
      const String& in_bn = FileHandler::stripExtension(File::basename(mz_file_abs_path));
      const String& id_primaryMSRun_bn = FileHandler::stripExtension(File::basename(id_msfile_ref[0]));

      if (in_bn != id_primaryMSRun_bn)  // mismatch between annotation in ID file and provided mzML file
      {
        OPENMS_LOG_WARN << "MS run path referenced from ID file does not match MS file at same input position: " << id_file_abs_path << endl
                        << "Resetting reference to MS file provided at same input position." << endl;
      }
    }
    else
    {
      OPENMS_LOG_WARN << "Multiple MS files referenced from ID file: " << id_file_abs_path << endl
                      << "Resetting reference to MS file provided at same input position." << endl;
    }
    id_msfile_ref = StringList{mz_file};
    protein_ids[0].setPrimaryMSRunPath(id_msfile_ref);
    protein_ids[0].setMetaValue("fraction_group", fraction_group);
    protein_ids[0].setMetaValue("fraction", fraction);

    // update identifiers to make them unique
    // fixes some bugs related to users splitting the original mzML and id files before running the analysis
    // in that case these files might have the same identifier
    const String old_identifier = protein_ids[0].getIdentifier();
    const String new_identifier = old_identifier + "_" + String(fraction_group) + "F" + String(fraction);
    protein_ids[0].setIdentifier(new_identifier);
    for (PeptideIdentification & p : peptide_ids)
    {
      if (p.getIdentifier() == old_identifier)
      {
        p.setIdentifier(new_identifier);
      }
      else
      {
        OPENMS_LOG_WARN << "Peptide ID identifier found not present in the protein ID" << endl;
      }
    }

    bool missing_spec_ref(false);
    for (const PeptideIdentification & pid : peptide_ids)
    {
      if (!pid.metaValueExists("spectrum_reference") 
        || pid.getMetaValue("spectrum_reference").toString().empty()) 
      {          
        missing_spec_ref = true;
        break;
      }
    }
    // reannotate spectrum references if missing
    if (missing_spec_ref)
    {
      OPENMS_LOG_WARN << "Warning: Identification file " << id_file_abs_path << " contains IDs without meta value for the spectrum native id.\n"
                         "OpenMS will try to reannotate them by matching retention times between ID and spectra." << endl;

      SpectrumMetaDataLookup::addMissingSpectrumReferences(
        peptide_ids, 
        mz_file_abs_path,
        true);
    }

    return EXECUTION_OK;
  }
 
  ExitCodes quantifyFraction_(
    const pair<unsigned int, std::vector<String> > & ms_files, 
    const map<String, String>& mzfile2idfile,
    const String& in_db,
    double median_fwhm,
    const multimap<Size, PeptideIdentification> & transfered_ids,
    ConsensusMap & consensus_fraction,
    vector<TransformationDescription> & transformations,
    double& max_alignment_diff,
    set<String>& fixed_modifications,
    set<String>& variable_modifications)
  {
    vector<FeatureMap> feature_maps;
    const Size fraction = ms_files.first;

    // debug output
    writeDebug_("Processing fraction number: " + String(fraction) + "\nFiles: ",  1);
    for (String const & mz_file : ms_files.second) { writeDebug_(mz_file,  1); }

    // for sanity checks we collect the primary MS run basenames as well as the ones stored in the ID files (below)
    StringList id_MS_run_ref;
    StringList in_MS_run = ms_files.second;

    // in theory a single file is "per-definition" aligned as well but has no transformations
    /// if an alignment algorithm was already run (false for singleton fractions, although they are inherently aligned)
    const bool is_already_aligned = !transformations.empty();

    // for each MS file of current fraction (e.g., all MS files that measured the n-th fraction) 
    Size fraction_group{1};
    for (String const & mz_file : ms_files.second)
    {
      writeDebug_("Processing file: " + mz_file,  1);
      // centroid spectra (if in profile mode) and correct precursor masses
      MSExperiment ms_centroided;    

      {
        ExitCodes e = centroidAndCorrectPrecursors_(mz_file, ms_centroided);
        if (e != EXECUTION_OK) { return e; }
      }

      // load and clean identification data associated with MS run
      vector<ProteinIdentification> protein_ids;
      vector<PeptideIdentification> peptide_ids;
      const String& mz_file_abs_path = File::absolutePath(mz_file);
      const String& id_file_abs_path = File::absolutePath(mzfile2idfile.at(mz_file_abs_path));

      {
        ExitCodes e = loadAndCleanupIDFile_(id_file_abs_path, mz_file, in_db, fraction_group, fraction, protein_ids, peptide_ids, fixed_modifications, variable_modifications);
        if (e != EXECUTION_OK) return e;
      }

      StringList id_msfile_ref;
      protein_ids[0].getPrimaryMSRunPath(id_msfile_ref);
      id_MS_run_ref.push_back(id_msfile_ref[0]);
     
      //-------------------------------------------------------------
      // Internal Calibration of spectra peaks and precursor peaks with high-confidence IDs
      //-------------------------------------------------------------
      if (getStringOption_("mass_recalibration") == "true")
      {
        recalibrateMasses_(ms_centroided, peptide_ids, id_file_abs_path);
      }

      vector<ProteinIdentification> ext_protein_ids;
      vector<PeptideIdentification> ext_peptide_ids;

      //////////////////////////////////////////////////////
      // Transfer aligned IDs
      //////////////////////////////////////////////////////
      if (!transfered_ids.empty()) // only non-empty if the fraction has more than one sample
      {
        OPENMS_PRECONDITION(is_already_aligned, "Data has not been aligned.")

        // transform observed IDs and spectra
        MapAlignmentTransformer::transformRetentionTimes(peptide_ids, transformations[fraction_group - 1]);
        MapAlignmentTransformer::transformRetentionTimes(ms_centroided, transformations[fraction_group - 1]);

        // copy the (already) aligned, consensus feature derived ids that are to be transferred to this map to peptide_ids
        auto range = transfered_ids.equal_range(fraction_group - 1);
        for (auto& it = range.first; it != range.second; ++it)
        {
           PeptideIdentification trans = it->second;
           trans.setIdentifier(protein_ids[0].getIdentifier());
           peptide_ids.push_back(trans);
        }
      }

      //////////////////////////////////////////
      // Chromatographic parameter estimation
      //////////////////////////////////////////
      median_fwhm = estimateMedianChromatographicFWHM_(ms_centroided);

      //-------------------------------------------------------------
      // Feature detection
      //-------------------------------------------------------------

      // Run MTD before FFM

      // create empty feature map and annotate MS file
      FeatureMap seeds;
      seeds.setPrimaryMSRunPath({mz_file});

      if (getStringOption_("targeted_only") == "false")
      {
        calculateSeeds_(ms_centroided, seeds, median_fwhm);
        if (debug_level_ > 666)
        {
          FeatureXMLFile().store("debug_seeds_fraction_" + String(ms_files.first) + "_" + String(fraction_group) + ".featureXML", seeds);
        }
      }

      /////////////////////////////////////////////////
      // Run FeatureFinderIdentification

      FeatureMap fm;

      FeatureFinderIdentificationAlgorithm ffi;
      ffi.getMSData().swap(ms_centroided);
      ffi.getProgressLogger().setLogType(log_type_);

      Param ffi_param = getParam_().copy("PeptideQuantification:", true);
      ffi_param.setValue("detect:peak_width", 5.0 * median_fwhm);
      ffi_param.setValue("EMGScoring:init_mom", "true");   // overwrite default settings
      ffi_param.setValue("EMGScoring:max_iteration", 100); // overwrite default settings
      ffi_param.setValue("debug", debug_level_); // pass down debug level

      ffi.setParameters(ffi_param);
      writeDebug_("Parameters passed to FeatureFinderIdentification algorithm", ffi_param, 3);

      FeatureMap tmp = fm;

      ffi.run(peptide_ids, 
        protein_ids, 
        ext_peptide_ids, 
        ext_protein_ids, 
        tmp,
        seeds,
        mz_file);

      // TODO: consider moving this to FFid
      // free parts of feature map not needed for further processing (e.g., subfeatures...)
      for (auto & f : tmp)
      {
        //TODO keep FWHM meta value for QC
        f.clearMetaInfo();
        f.setSubordinates({});
        f.setConvexHulls({});
      }

      IDConflictResolverAlgorithm::resolve(tmp,
          getStringOption_("keep_feature_top_psm_only") == "false"); // keep only best peptide per feature per file

      feature_maps.push_back(tmp);
      
      if (debug_level_ > 666)
      {
        FeatureXMLFile().store("debug_fraction_" + String(ms_files.first) + "_" + String(fraction_group) + ".featureXML", feature_maps.back());
      }

      if (debug_level_ > 670)
      {
        MzMLFile().store("debug_fraction_" + String(ms_files.first) + "_" + String(fraction_group) + "_chroms.mzML", ffi.getChromatograms());
      }

      ++fraction_group;
    }

    // Check for common mistake that order of input files have been switched.
    // This is the case if basenames are identical but the order does not match.
    if (!File::validateMatchingFileNames(in_MS_run, id_MS_run_ref, true, true, false)) // only basenames, without extension, only order
    {
      throw Exception::IllegalArgument(__FILE__, __LINE__,
        OPENMS_PRETTY_FUNCTION, "MS run path reference in ID files and spectra filenames match but order differs.");
    }

    //-------------------------------------------------------------
    // Align all features of this fraction (if not already aligned)
    //-------------------------------------------------------------
    if (!is_already_aligned)
    {
      max_alignment_diff = alignAndLink_(
        feature_maps, 
        consensus_fraction, 
        transformations,
        median_fwhm);
    }
    else // Data already aligned. Link with previously determined alignment difference
    {
      link_(feature_maps,
        median_fwhm,
        max_alignment_diff,
        consensus_fraction);
    }

    // add dataprocessing
    if (feature_maps.size() > 1)
    {
      addDataProcessing_(consensus_fraction,
        getProcessingInfo_(DataProcessing::ALIGNMENT));
      addDataProcessing_(consensus_fraction,
        getProcessingInfo_(DataProcessing::FEATURE_GROUPING));
    }

    ////////////////////////////////////////////////////////////
    // Annotate experimental design in consensus map
    ////////////////////////////////////////////////////////////
    Size j(0);
    // for each MS file (as provided in the experimental design)
    const auto& path_label_to_sampleidx = design_.getPathLabelToSampleMapping(true);
    for (String const & mz_file : ms_files.second) 
    {
      const Size curr_fraction_group = j + 1;
      consensus_fraction.getColumnHeaders()[j].label = "label-free";
      consensus_fraction.getColumnHeaders()[j].filename = mz_file;
      consensus_fraction.getColumnHeaders()[j].unique_id = feature_maps[j].getUniqueId();
      consensus_fraction.getColumnHeaders()[j].setMetaValue("fraction", fraction);
      consensus_fraction.getColumnHeaders()[j].setMetaValue("fraction_group", curr_fraction_group);
      const auto& sample_index = path_label_to_sampleidx.at({File::basename(mz_file), 1});
      const auto& sample_name = design_.getSampleSection().getSampleName(sample_index);
      consensus_fraction.getColumnHeaders()[j].setMetaValue("sample_name", sample_name);
      ++j;
    }

    // assign unique ids
    consensus_fraction.applyMemberFunction(&UniqueIdInterface::setUniqueId);

    // sort list of peptide identifications in each consensus feature by map index
    consensus_fraction.sortPeptideIdentificationsByMapIndex();

    if (debug_level_ >= 666)
    {
      ConsensusXMLFile().store("debug_fraction_" + String(ms_files.first) +  ".consensusXML", consensus_fraction);
      writeDebug_("to produce a consensus map with: " + String(consensus_fraction.getColumnHeaders().size()) + " columns.", 1);
    }

    //-------------------------------------------------------------
    // ID conflict resolution
    //-------------------------------------------------------------
    IDConflictResolverAlgorithm::resolve(consensus_fraction, true);

    //-------------------------------------------------------------
    // ConsensusMap normalization (basic)
    //-------------------------------------------------------------
    if (getStringOption_("out_msstats").empty() 
    && getStringOption_("out_triqler").empty())  // only normalize if no MSstats/Triqler output is generated
    {
      ConsensusMapNormalizerAlgorithmMedian::normalizeMaps(
        consensus_fraction, 
        ConsensusMapNormalizerAlgorithmMedian::NM_SCALE, 
        "", 
        "");
    }



    // max_alignment_diff returned by reference
    return EXECUTION_OK;
  }


  ExitCodes inferProteinGroups_(ConsensusMap& consensus,
    const set<String>& fixed_modifications)
  {
    // since we don't require an index as input but need to calculate e.g., coverage we reindex here (fast)

    //-------------------------------------------------------------
    // Protein inference
    //-------------------------------------------------------------
    // TODO: This needs to be rewritten to work directly on the quant data.
    //  of course we need to provide options to keep decoys and unassigned PSMs all the way through quantification.
    // TODO: Think about ProteinInference on IDs only merged per condition
    bool groups = getStringOption_("protein_quantification") != "strictly_unique_peptides";
    bool bayesian = getStringOption_("protein_inference") == "bayesian";
    bool greedy_group_resolution = getStringOption_("protein_quantification") == "shared_peptides";

    if (!bayesian) // simple aggregation
    {
      ConsensusMapMergerAlgorithm cmerge;
      // The following will result in a SINGLE protein run for the whole consensusMap,
      // but I think the information about which protein was in which run, is not important
      cmerge.mergeAllIDRuns(consensus);

      BasicProteinInferenceAlgorithm bpia;
      auto bpiaparams = bpia.getParameters();
      bpiaparams.setValue("annotate_indistinguishable_groups", groups ? "true" : "false");
      bpiaparams.setValue("greedy_group_resolution", greedy_group_resolution ? "true" : "false");
      bpia.setParameters(bpiaparams);

      // TODO parameterize if unassigned IDs without feature should contribute?
      bpia.run(consensus, consensus.getProteinIdentifications()[0], true);
    }
    else // if (bayesian)
    {
      BayesianProteinInferenceAlgorithm bayes;
      auto bayesparams = bayes.getParameters();
      // We need all PSMs to collect all possible modifications, to do spectral counting and to do PSM FDR.
      // In theory, if none is needed we can save memory. For quantification,
      // we basically discard peptide+PSM information from inference and use the info from the cMaps.
      bayesparams.setValue("keep_best_PSM_only", "false");
      //bayesian inference automatically annotates groups, therefore remove them later
      bayes.inferPosteriorProbabilities(consensus, greedy_group_resolution);
      if (!groups)
      {
        // should be enough to just clear the groups. Only indistinguishable will be annotated above.
        consensus.getProteinIdentifications()[0].getIndistinguishableProteins().clear();
      }
    }

    // TODO think about order of greedy resolution, FDR calc and filtering

    //-------------------------------------------------------------
    // Protein (and additional peptide?) FDR
    //-------------------------------------------------------------
    const double max_fdr = getDoubleOption_("proteinFDR");
    const bool picked = getStringOption_("picked_proteinFDR") == "true";

    //TODO use new FDR_type parameter
    const double max_psm_fdr = getDoubleOption_("psmFDR");
    FalseDiscoveryRate fdr;
    if (getFlag_("PeptideQuantification:quantify_decoys"))
    {
      Param fdr_param = fdr.getParameters();
      fdr_param.setValue("add_decoy_peptides", "true");
      fdr_param.setValue("add_decoy_proteins", "true");
      fdr.setParameters(fdr_param);
    }

    // ensure that only one final inference result is generated for now
    assert(consensus.getProteinIdentifications().size() == 1);

    auto& overall_proteins = consensus.getProteinIdentifications()[0];
    if (!picked)
    {
      fdr.applyBasic(overall_proteins);
    }
    else
    {
      fdr.applyPickedProteinFDR(overall_proteins, picked_decoy_string_, picked_decoy_prefix_);
    }

    bool pepFDR = getStringOption_("FDR_type") == "PSM+peptide";
    //TODO Think about the implications of mixing PSMs from different files and searches.
    //  Score should be PEPs here. We could extract the original search scores, depending on preprocessing. PEPs allow some normalization but will
    //  disregard the absolute score differences between runs (i.e. if scores in one run are all lower than the ones in another run,
    //  do you want to filter them out preferably or do you say: this was a faulty run, if the decoys are equally bad, I want the
    //  best targets to be treated like the best targets from the other runs, even if the absolute match scores are much lower).
    if (pepFDR)
    {
      fdr.applyBasicPeptideLevel(consensus, true);
    }
    else
    {
      fdr.applyBasic(consensus, true);
    }

    if (!getFlag_("PeptideQuantification:quantify_decoys"))
    { // FDR filtering removed all decoy proteins -> update references and remove all unreferenced (decoy) PSMs
      IDFilter::updateProteinReferences(consensus, true);
      IDFilter::removeUnreferencedProteins(consensus, true); // if we don't filter peptides for now, we don't need this
      IDFilter::updateProteinGroups(overall_proteins.getIndistinguishableProteins(),
                                    overall_proteins.getHits());
      IDFilter::updateProteinGroups(overall_proteins.getProteinGroups(),
                                    overall_proteins.getHits());
    }

    // FDR filtering
    if (max_psm_fdr < 1.) // PSM level
    {
      for (auto& f : consensus)
      {
        IDFilter::filterHitsByScore(f.getPeptideIdentifications(), max_psm_fdr);
      }
      IDFilter::filterHitsByScore(consensus.getUnassignedPeptideIdentifications(), max_psm_fdr);
    }

    if (max_fdr < 1.) // protein level
    {
      IDFilter::filterHitsByScore(overall_proteins, max_fdr);
    }

    if (max_fdr < 1. || !getFlag_("PeptideQuantification:quantify_decoys"))
    {
      IDFilter::updateProteinReferences(consensus, true);
    }

    if (max_psm_fdr < 1.)
    {
      IDFilter::removeUnreferencedProteins(consensus, true);
    }

    if (max_fdr < 1. || max_psm_fdr < 1. || !getFlag_("PeptideQuantification:quantify_decoys"))
    {
      IDFilter::updateProteinGroups(overall_proteins.getIndistinguishableProteins(), overall_proteins.getHits());
      IDFilter::updateProteinGroups(overall_proteins.getProteinGroups(), overall_proteins.getHits());
    }

    if (overall_proteins.getHits().empty())
    {
      throw Exception::MissingInformation(
          __FILE__,
          __LINE__,
          OPENMS_PRETTY_FUNCTION,
          "No proteins left after FDR filtering. Please check the log and adjust your settings.");
    }

    // do we only want to keep strictly unique peptides (e.g., no groups)?
    // This filters for the VERY initially computed theoretical uniqueness calculated by PeptideIndexer
    //  which also means that e.g., target+decoy peptides are not unique
    if (!greedy_group_resolution && !groups)
    {
      for (auto& f : consensus)
      {
        IDFilter::keepUniquePeptidesPerProtein(f.getPeptideIdentifications());
      }
      IDFilter::keepUniquePeptidesPerProtein(consensus.getUnassignedPeptideIdentifications());
    }

    // compute coverage (sequence was annotated during PeptideIndexing)
    // TODO: do you really want to compute coverage from unquantified peptides also?
    overall_proteins.computeCoverage(consensus, true);

    // TODO: this might not be correct if only the best peptidoform is kept
    // determine observed modifications (exclude fixed mods)
    overall_proteins.computeModifications(consensus, StringList(fixed_modifications.begin(), fixed_modifications.end()), true);

    return EXECUTION_OK;
  }



  ExitCodes main_(int, const char **) override
  {
    //-------------------------------------------------------------
    // Parameter handling
    //-------------------------------------------------------------    

    // Read tool parameters
    StringList in = getStringList_("in");
    String out = getStringOption_("out");
    String out_msstats = getStringOption_("out_msstats");
    String out_triqler = getStringOption_("out_triqler");
    StringList in_ids = getStringList_("ids");
    String design_file = getStringOption_("design");
    String in_db = getStringOption_("fasta");

    // Validate parameters
    if (in.size() != in_ids.size())
    {
      throw Exception::FileNotFound(__FILE__, __LINE__, 
        OPENMS_PRETTY_FUNCTION, "Number of spectra file (" + String(in.size()) + ") must match number of ID files (" + String(in_ids.size()) + ").");
    }

    if (getStringOption_("quantification_method") == "spectral_counting")
    {
      if (!out_msstats.empty())
      {
        throw Exception::FileNotFound(__FILE__, __LINE__, 
          OPENMS_PRETTY_FUNCTION, "MSstats export for spectral counting data not supported. Please remove output file.");
      }
      if (!out_triqler.empty())
      {
        throw Exception::FileNotFound(__FILE__, __LINE__, 
          OPENMS_PRETTY_FUNCTION, "Triqler export for spectral counting data not supported. Please remove output file.");
      }
    }

    //-------------------------------------------------------------
    // Experimental design: read or generate default
    //-------------------------------------------------------------
    if (!design_file.empty())
    { // load from file
      design_ = ExperimentalDesignFile::load(design_file, false);
    }
    else
    {
      OPENMS_LOG_INFO << "No experimental design file provided.\n"
                      << "Assuming a label-free experiment without fractionation.\n"
                      << endl;

      TextFile design_table;
      design_table.addLine("Fraction_Group\tFraction\tSpectra_Filepath\tLabel\tSample\tMSstats_Condition\tMSstats_BioReplicate");

      Size count{1};
      for (String & s : in)
      {
        design_table.addLine(String(count) + "\t1\t" + s +"\t1\tSample" + String(count) + "\t" + String(count)+ "\t" + String(count));
        ++count;
      }      
      design_ = ExperimentalDesignFile::load(design_table, "--no design file--", false);
    }

    // some sanity checks
    // extract basenames from experimental design and input files
    const auto& pl2fg = design_.getPathLabelToFractionGroupMapping(true);
    set<String> ed_basenames;
    for (const auto& p : pl2fg)
    {
      const String& filename = p.first.first;
      ed_basenames.insert(filename);
    }

    set<String> in_basenames;
    for (const auto & f : in)
    {
      const String& in_bn = File::basename(f);
      in_basenames.insert(in_bn);
    }

    if (!std::includes(ed_basenames.begin(), ed_basenames.end(), in_basenames.begin(), in_basenames.end()))
    {
      throw Exception::InvalidParameter(__FILE__, __LINE__,
        OPENMS_PRETTY_FUNCTION, "Spectra file basenames provided as input need to match a subset the experimental design file basenames.");
    }

    Size nr_filtered = design_.filterByBasenames(in_basenames);
    if (nr_filtered > 0)
    {
      OPENMS_LOG_WARN << "WARNING: " << nr_filtered << " files from experimental design were not passed as mzMLs. Continuing with subset if the fractions still match." << std::endl;
    }

    if (design_.getNumberOfLabels() != 1)
    {
      throw Exception::InvalidParameter(__FILE__, __LINE__, 
        OPENMS_PRETTY_FUNCTION, "Experimental design is not label-free as it contains multiple labels.");          
    }
    if (!design_.sameNrOfMSFilesPerFraction())
    {
      OPENMS_LOG_WARN << "WARNING: Different number of fractions for different samples provided. Support maybe limited in ProteomicsLFQ." << std::endl;
    }
   
    std::map<unsigned int, std::vector<String> > frac2ms = design_.getFractionToMSFilesMapping();

    // experimental design file could contain URLs etc. that we want to overwrite with the actual input files
    for (auto & fraction_ms_files : frac2ms)
    {
      for (auto & s : fraction_ms_files.second)
      { // for all ms files of current fraction number
        // if basename in experimental design matches to basename in input file
        // overwrite experimental design to point to existing file (and only if they were different)
        if (auto it = std::find_if(in.begin(), in.end(), 
              [&s] (const String& in_filename) { return File::basename(in_filename) == File::basename(s); }); // basename matches?
                 it != in.end() && s != *it) // and differ?
        {
          OPENMS_LOG_INFO << "Path of spectra files differ between experimental design (1) and input (2). Using the path of the input file as "
                          << "we know this file exists on the file system: '" << *it << "' vs. '" << s << endl;
          s = *it; // overwrite filename in design with filename in input files
        }
      }
      
    } 

    for (auto & f : frac2ms)
    {
      writeDebug_("Fraction " + String(f.first) + ":", 10);
      for (const String & s : f.second)
      {
        writeDebug_("MS file: " + s, 10);
      }
    }

    // Map between mzML file and corresponding id file
    // Here we currently assume that these are provided in the exact same order.
    map<String, String> mzfile2idfile = mapMzML2Ids_(in, in_ids);
    map<String, String> idfile2mzfile = mapId2MzMLs_(mzfile2idfile);

    // TODO maybe check if mzMLs in experimental design match to mzMLs passed as in parameter
    //  IF both are present
    

    Param pep_param = getParam_().copy("Posterior Error Probability:", true);
    writeDebug_("Parameters passed to PEP algorithm", pep_param, 3);

    // TODO: inference parameter

    Param pq_param = getParam_().copy("ProteinQuantification:", true);
    writeDebug_("Parameters passed to PeptideAndProteinQuant algorithm", pq_param, 3);


    Param com_param = getParam_().copy("algorithm:common:", true);
    writeDebug_("Common parameters passed to both sub-algorithms (mtd and epd)", com_param, 3);

    set<String> fixed_modifications, variable_modifications;

    //-------------------------------------------------------------
    // Loading input
    //-------------------------------------------------------------
    ConsensusMap consensus;

    //-------------------------------------------------------------
    // feature-based quantifications
    //-------------------------------------------------------------
    if (getStringOption_("quantification_method") == "feature_intensity")
    {
      OPENMS_LOG_INFO << "Performing feature intensity-based quantification." << endl;
      double median_fwhm(0);
      for (auto const & ms_files : frac2ms) // for each fraction->ms file(s)
      {      
        ConsensusMap consensus_fraction; // quantitative result for this fraction identifier
        vector<TransformationDescription> transformations; // filled by RT alignment
        double max_alignment_diff(0.0);

        ExitCodes e = quantifyFraction_(
          ms_files, 
          mzfile2idfile,
          in_db,
          median_fwhm,
          multimap<Size, PeptideIdentification>(),
          consensus_fraction,
          transformations,  // transformations are empty, will be filled by alignment
          max_alignment_diff,  // max_alignment_diff not yet determined, will be filled by alignment
          fixed_modifications,
          variable_modifications);

        if (e != EXECUTION_OK) { return e; }
        
        if (getStringOption_("transfer_ids") != "false" && ms_files.second.size() > 1)
        {  
          OPENMS_LOG_INFO << "Transferring identification data between runs of the same fraction." << endl;
          // TODO parameterize
          // needs to occur in >= 50% of all runs for transfer

          const Size min_occurrance = (ms_files.second.size() + 1) / 2;
          multimap<Size, PeptideIdentification> transfered_ids = transferIDsBetweenSameFraction_(consensus_fraction, min_occurrance);
          consensus_fraction.clear();

          // The transferred IDs were calculated on the aligned data
          // So we make sure we use the aligned IDs and peak maps in the re-quantification step
          e = quantifyFraction_(
            ms_files, 
            mzfile2idfile,
            in_db,
            median_fwhm, 
            transfered_ids, 
            consensus_fraction, 
            transformations,  // transformations as determined by alignment
            max_alignment_diff, // max_alignment_error as determined by alignment
            fixed_modifications,
            variable_modifications);

          OPENMS_POSTCONDITION(!consensus_fraction.empty(), "ConsensusMap of fraction empty after ID transfer.!");
          if (e != EXECUTION_OK) { return e; }
        }
        consensus.appendColumns(consensus_fraction);  // append consensus map calculated for this fraction number
      }  // end of scope of fraction related data

      consensus.sortByPosition();
      consensus.sortPeptideIdentificationsByMapIndex();

      if (debug_level_ >= 666)
      {
        ConsensusXMLFile().store("debug_after_normalization.consensusXML", consensus);
      }
    }
    else if (getStringOption_("quantification_method") == "spectral_counting")
    {
      OPENMS_LOG_INFO << "Performing spectral counting-based quantification." << endl;

      // init consensus map with basic experimental design information
      consensus.setExperimentType("label-free");

      auto& all_protein_ids = consensus.getProteinIdentifications();
      auto& all_peptide_ids = consensus.getUnassignedPeptideIdentifications();

      Size run_index(0);
      for (auto const & ms_files : frac2ms) // for each fraction->ms file(s)
      {
        const Size& fraction = ms_files.first;

        // debug output
        writeDebug_("Processing fraction number: " + String(fraction) + "\nFiles: ",  1);
        for (String const & mz_file : ms_files.second) { writeDebug_(mz_file,  1); }

        // for sanity checks we collect the primary MS run basenames as well as the ones stored in the ID files (below)
        StringList id_MS_run_ref;
        StringList in_MS_run = ms_files.second;

        // for each MS file of current fraction (e.g., all MS files that measured the n-th fraction) 
        Size fraction_group{1};
        for (String const & mz_file : ms_files.second)
        { 
          // load and clean identification data associated with MS run
          vector<ProteinIdentification> protein_ids;
          vector<PeptideIdentification> peptide_ids;
          const String& mz_file_abs_path = File::absolutePath(mz_file);
          const String& id_file_abs_path = File::absolutePath(mzfile2idfile.at(mz_file_abs_path));

          {
            ExitCodes e = loadAndCleanupIDFile_(id_file_abs_path, mz_file, in_db, fraction_group, fraction, protein_ids, peptide_ids, fixed_modifications, variable_modifications);
            if (e != EXECUTION_OK) return e;
          }

          StringList id_msfile_ref;
          protein_ids[0].getPrimaryMSRunPath(id_msfile_ref);
          id_MS_run_ref.push_back(id_msfile_ref[0]);

          // append to consensus map
          all_protein_ids.emplace_back(std::move(protein_ids[0]));
          all_peptide_ids.insert(all_peptide_ids.end(), 
            std::make_move_iterator(peptide_ids.begin()), 
            std::make_move_iterator(peptide_ids.end()));
        }

        ////////////////////////////////////////////////////////////
        // Annotate experimental design in consensus map
        ////////////////////////////////////////////////////////////
        Size j(0);
        // for each MS file (as provided in the experimental design)
        const auto& path_label_to_sampleidx = design_.getPathLabelToSampleMapping(true);
        for (String const & mz_file : ms_files.second) 
        {
          const Size curr_fraction_group = j + 1;
          consensus.getColumnHeaders()[run_index].label = "label-free";
          consensus.getColumnHeaders()[run_index].filename = mz_file;
          consensus.getColumnHeaders()[run_index].unique_id = 1 + run_index;
          consensus.getColumnHeaders()[run_index].setMetaValue("fraction", fraction);
          consensus.getColumnHeaders()[run_index].setMetaValue("fraction_group", curr_fraction_group);
          consensus.getColumnHeaders()[run_index].setMetaValue("sample_name", design_.getSampleSection().getSampleName(path_label_to_sampleidx.at({File::basename(mz_file),1})));
          ++j;
          ++run_index;
        }
      }
    }

    //-------------------------------------------------------------
    // ID related algorithms
    //-------------------------------------------------------------

    ExitCodes e = inferProteinGroups_(consensus, fixed_modifications);
    if (e != EXECUTION_OK) return e;

    // clean up references (assigned and unassigned)
    IDFilter::removeUnreferencedProteins(consensus, true);
    
    // only keep best scoring ID for each consensus feature
    IDConflictResolverAlgorithm::resolve(consensus);

    //-------------------------------------------------------------
    // Peptide quantification
    //-------------------------------------------------------------
    PeptideAndProteinQuant quantifier;

    // TODO Why is there no easy quantifier.run(consensus,[inference_prot_ids]) function??
    if (getStringOption_("quantification_method") == "feature_intensity")
    {
      quantifier.setParameters(pq_param);
      quantifier.readQuantData(consensus, design_);
    }
    else if (getStringOption_("quantification_method") == "spectral_counting")
    {
      pq_param.setValue("top:aggregate", "sum");
      pq_param.setValue("top:N", 0); // all
      pq_param.setValue("consensus:normalize", "false");
      quantifier.setParameters(pq_param);

      quantifier.readQuantData(
       consensus.getProteinIdentifications(),
       consensus.getUnassignedPeptideIdentifications(),
       design_);
    }

    // nothing to filter. everything in consensus should be uptodate with inference.
    // on peptide level it does not annotate anything anyway
    quantifier.quantifyPeptides();

    //-------------------------------------------------------------
    // Protein quantification
    //-------------------------------------------------------------

    // Should always be there by now, even if just singletons
    ProteinIdentification& inferred_proteins = consensus.getProteinIdentifications()[0];
    if (inferred_proteins.getIndistinguishableProteins().empty())
    {
      throw Exception::MissingInformation(
       __FILE__, 
       __LINE__, 
       OPENMS_PRETTY_FUNCTION, 
       "No information on indistinguishable protein groups found.");
    }

    quantifier.quantifyProteins(inferred_proteins);
    auto const & protein_quants = quantifier.getProteinResults();
    if (protein_quants.empty())
    {        
     OPENMS_LOG_WARN << "Warning: No proteins were quantified." << endl;
    }

    //-------------------------------------------------------------
    // Export of MzTab file as final output
    //-------------------------------------------------------------

    // Annotate quants to protein(groups) for easier export in mzTab
    // Note: we keep protein groups that have not been quantified
    quantifier.annotateQuantificationsToProteins(
      protein_quants, inferred_proteins, false);

    // For correctness, we would need to set the run reference in the pepIDs of the consensusXML all to the first run then
    // And probably make sure that peptides that correspond to filtered out proteins are not producing errors
    // e.g. by removing them with a Filter beforehand.

    consensus.resolveUniqueIdConflicts(); // TODO: find out if this is still needed    

    if (!getStringOption_("out_cxml").empty())
    {
      // Note: idXML and consensusXML doesn't support writing quantification at protein groups
      // (they are nevertheless stored and passed to mzTab for proper export)
      ConsensusXMLFile().store(getStringOption_("out_cxml"), consensus);
    }

    // Fill MzTab with meta data and quants annotated in identification data structure
    const bool report_unidentified_features(false);
    const bool report_unmapped(true); //TODO we should make a distinction from unassigned after conflict resolution and unassigned because unmappable
    const bool report_subfeatures(false);
    const bool report_unidentified_spectra(false);
    const bool report_not_only_best_psm_per_spectrum(false); 

    MzTabFile().store(out,
                      consensus,
                      false, // first run is inference but also a properly merged run, so we don't need the hack
                      report_unidentified_features,
                      report_unmapped,
                      report_subfeatures,
                      report_unidentified_spectra,
                      report_not_only_best_psm_per_spectrum);

    if (!out_msstats.empty())
    {
      IDFilter::removeEmptyIdentifications(consensus); // MzTab stream exporter currently doesn't support IDs with empty hits.
      
      MSstatsFile msstats;
      // TODO: add a helper method to quickly check if experimental design file contain the right columns
      //  (and put this at start of tool)

      // shrink protein runs to the one containing the inference data
      consensus.getProteinIdentifications().resize(1);

      msstats.storeLFQ(
        out_msstats, 
        consensus, 
        design_,
        StringList(), 
        false, //lfq
        "MSstats_BioReplicate", 
        "MSstats_Condition", 
        "max");
    }


    if (!out_triqler.empty())
    {
      TriqlerFile tf;

      // shrink protein runs to the one containing the inference data
      consensus.getProteinIdentifications().resize(1);

      IDScoreSwitcherAlgorithm switcher;
      Size c = 0;
      switcher.switchToGeneralScoreType(consensus, IDScoreSwitcherAlgorithm::ScoreType::PEP, c);

      tf.storeLFQ(
        out_triqler, 
        consensus, 
        design_,
        StringList(), 
        "MSstats_Condition" // TODO: choose something more generic like "Condition" for both MSstats and Triqler export
        );
    }    

    return EXECUTION_OK;
  }
  String picked_decoy_string_;
  bool picked_decoy_prefix_ = true;
  ExperimentalDesign design_;
};

int main(int argc, const char ** argv)
{
  ProteomicsLFQ tool;
  return tool.main(argc, argv);
}

/// @endcond
