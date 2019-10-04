/*
 * Copyright Elasticsearch B.V. and/or licensed to Elasticsearch B.V. under one
 * or more contributor license agreements. Licensed under the Elastic License;
 * you may not use this file except in compliance with the Elastic License.
 */

#ifndef INCLUDED_ml_maths_CBoostedTreeFactory_h
#define INCLUDED_ml_maths_CBoostedTreeFactory_h

#include <core/CDataFrame.h>

#include <maths/CBoostedTree.h>
#include <maths/CLinearAlgebra.h>
#include <maths/ImportExport.h>

#include <boost/optional.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace ml {
namespace core {
class CPackedBitVector;
}
namespace maths {

class CNode;
class CBoostedTree;
class CBoostedTreeImpl;

//! Factory for CBoostedTree objects.
class MATHS_EXPORT CBoostedTreeFactory final {
public:
    using TBoostedTreeUPtr = std::unique_ptr<CBoostedTree>;
    using TProgressCallback = CBoostedTree::TProgressCallback;
    using TMemoryUsageCallback = CBoostedTree::TMemoryUsageCallback;
    using TTrainingStateCallback = CBoostedTree::TTrainingStateCallback;
    using TLossFunctionUPtr = CBoostedTree::TLossFunctionUPtr;

public:
    //! Construct a boosted tree object from parameters.
    static CBoostedTreeFactory constructFromParameters(std::size_t numberThreads,
                                                       TLossFunctionUPtr loss);

    //! Construct a boosted tree object from its serialized version.
    //!
    //! \warning Throws runtime error on fail to restore.
    static CBoostedTreeFactory constructFromString(std::istream& jsonStringStream);

    ~CBoostedTreeFactory();
    CBoostedTreeFactory(CBoostedTreeFactory&) = delete;
    CBoostedTreeFactory& operator=(CBoostedTreeFactory&) = delete;
    CBoostedTreeFactory(CBoostedTreeFactory&&);
    CBoostedTreeFactory& operator=(CBoostedTreeFactory&&);

    //! Set the minimum fraction with a category value to one-hot encode.
    CBoostedTreeFactory& minimumFrequencyToOneHotEncode(double frequency);
    //! Set the number of folds to use for estimating the generalisation error.
    CBoostedTreeFactory& numberFolds(std::size_t numberFolds);
    //! Set the sum of leaf depth penalties multiplier.
    CBoostedTreeFactory& depthPenaltyMultiplier(double depthPenaltyMultiplier);
    //! Set the tree size penalty multiplier.
    CBoostedTreeFactory& treeSizePenaltyMultiplier(double treeSizePenaltyMultiplier);
    //! Set the sum of weights squared multiplier.
    CBoostedTreeFactory& leafWeightPenaltyMultiplier(double leafWeightPenaltyMultiplier);
    //! Set the soft tree depth limit.
    CBoostedTreeFactory& softTreeDepthLimit(double softTreeDepthLimit);
    //! Set the soft tree depth tolerance. This controls how hard we'll try to
    //! respect the soft tree depth limit.
    CBoostedTreeFactory& softTreeDepthTolerance(double softTreeDepthTolerance);
    //! Set the fractional relative tolerance in the target maximum tree depth.
    CBoostedTreeFactory& maxTreeDepthTolerance(double maxTreeDepthTolerance);
    //! Set the amount we'll shrink the weights on each each iteration.
    CBoostedTreeFactory& eta(double eta);
    //! Set the maximum number of trees in the ensemble.
    CBoostedTreeFactory& maximumNumberTrees(std::size_t maximumNumberTrees);
    //! Set the fraction of features we'll use in the bag to build a tree.
    CBoostedTreeFactory& featureBagFraction(double featureBagFraction);
    //! Set the maximum number of optimisation rounds we'll use for hyperparameter
    //! optimisation per parameter.
    CBoostedTreeFactory& maximumOptimisationRoundsPerHyperparameter(std::size_t rounds);
    //! Set the number of restarts to use in global probing for Bayesian Optimisation.
    CBoostedTreeFactory& bayesianOptimisationRestarts(std::size_t restarts);
    //! Set the number of training examples we need per feature we'll include.
    CBoostedTreeFactory& rowsPerFeature(std::size_t rowsPerFeature);
    //! Set the callback function for progress monitoring.
    CBoostedTreeFactory& progressCallback(TProgressCallback callback);
    //! Set the callback function for memory monitoring.
    CBoostedTreeFactory& memoryUsageCallback(TMemoryUsageCallback callback);
    //! Set the callback function for training state recording.
    CBoostedTreeFactory& trainingStateCallback(TTrainingStateCallback callback);

    //! Estimate the maximum booking memory that training the boosted tree on a data
    //! frame with \p numberRows row and \p numberColumns columns will use.
    std::size_t estimateMemoryUsage(std::size_t numberRows, std::size_t numberColumns) const;
    //! Get the number of columns training the model will add to the data frame.
    std::size_t numberExtraColumnsForTrain() const;
    //! Build a boosted tree object for a given data frame.
    TBoostedTreeUPtr buildFor(core::CDataFrame& frame, std::size_t dependentVariable);

private:
    using TDoubleDoublePr = std::pair<double, double>;
    using TOptionalDouble = boost::optional<double>;
    using TOptionalSize = boost::optional<std::size_t>;
    using TVector = CVectorNx1<double, 3>;
    using TOptionalVector = boost::optional<TVector>;
    using TPackedBitVectorVec = std::vector<core::CPackedBitVector>;
    using TBoostedTreeImplUPtr = std::unique_ptr<CBoostedTreeImpl>;
    using TApplyRegularizerStep =
        std::function<void(CBoostedTreeImpl&, double, std::size_t)>;

private:
    static const double MINIMUM_ETA;
    static const std::size_t MAXIMUM_NUMBER_TREES;

private:
    CBoostedTreeFactory(bool restored, std::size_t numberThreads, TLossFunctionUPtr loss);

    //! Compute the row masks for the missing values for each feature.
    void initializeMissingFeatureMasks(const core::CDataFrame& frame) const;

    //! Get the (train, test) row masks for performing cross validation.
    std::pair<TPackedBitVectorVec, TPackedBitVectorVec> crossValidationRowMasks() const;

    //! Encode categorical fields and at the same time select the features to use
    //! as regressors.
    void selectFeaturesAndEncodeCategories(const core::CDataFrame& frame) const;

    //! Determine the encoded feature types.
    void determineFeatureDataTypes(const core::CDataFrame& frame) const;

    //! Initialize the regressors sample distribution.
    bool initializeFeatureSampleDistribution() const;

    //! Set the initial values for the various hyperparameters.
    void initializeHyperparameters(core::CDataFrame& frame);

    //! Estimate a good central value for the regularisation hyperparameters
    //! search bounding box.
    void initializeUnsetRegularizationHyperparameters(core::CDataFrame& frame);

    //! Estimate the reduction in gain from a split and the total curvature of
    //! the loss function at a split.
    TDoubleDoublePr estimateTreeGainAndCurvature(core::CDataFrame& frame,
                                                 const core::CPackedBitVector& trainingRowMask) const;

    //! Perform a line search for the test loss w.r.t. a single regularization
    //! hyperparameter and apply Newton's method to find the minimum. The plan
    //! is to find a value near where the model starts to overfit.
    //!
    //! \return The interval to search during the main hyperparameter optimisation
    //! loop or null if this couldn't be found.
    TOptionalVector testLossLineSearch(core::CDataFrame& frame,
                                       core::CPackedBitVector trainingRowMask,
                                       const TApplyRegularizerStep& applyRegularizerStep,
                                       double returnedIntervalLeftEndOffset,
                                       double returnedIntervalRightEndOffset) const;

    //! Initialize the state for hyperparameter optimisation.
    void initializeHyperparameterOptimisation() const;

    //! Get the number of hyperparameter tuning rounds to use.
    std::size_t numberHyperparameterTuningRounds() const;

    //! Setup monitoring for training progress.
    void initializeTrainingProgressMonitoring();

    //! Refresh progress monitoring after restoring from saved training state.
    void resumeRestoredTrainingProgressMonitoring();

    static void noopRecordProgress(double);
    static void noopRecordMemoryUsage(std::int64_t);
    static void noopRecordTrainingState(CDataFrameRegressionModel::TPersistFunc);

private:
    TOptionalDouble m_MinimumFrequencyToOneHotEncode;
    TOptionalSize m_BayesianOptimisationRestarts;
    bool m_Restored = false;
    TBoostedTreeImplUPtr m_TreeImpl;
    TVector m_LogDepthPenaltyMultiplierSearchInterval;
    TVector m_LogTreeSizePenaltyMultiplierSearchInterval;
    TVector m_LogLeafWeightPenaltyMultiplierSearchInterval;
    TProgressCallback m_RecordProgress = noopRecordProgress;
    TMemoryUsageCallback m_RecordMemoryUsage = noopRecordMemoryUsage;
    TTrainingStateCallback m_RecordTrainingState = noopRecordTrainingState;
};
}
}

#endif // INCLUDED_ml_maths_CBoostedTreeFactory_h